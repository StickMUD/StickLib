/**
 * Module:
 *      /basic/player/tell_me.c
 * Description:
 *      "Full version" of tell_me.c for player.c. Includes all
 *      the nifty things players need, without having to check
 *      if this_object() is an NPC or a player(formerly it was
 *      living.c that included tell_me.c).
 *
 **/

#include "/basic/misc/fstring.c"

#ifndef PLAYER_C

#include "/basic/living/living_private.h"
#include <tell_me.h>
#include <living_defs.h>
#include <conditions.h>

public varargs int
tell_me(mixed str, mixed sense_list, mixed tell_flags, mixed me, mixed him, mixed it, int stat);

int coder_level;

static int liv_Flags;
static string name;

varargs string query_name(int a, object b);
varargs string query_short(int a, object b);
varargs string query_long(int a, object b);
varargs string query_real_name();
varargs string query_race();
varargs string query_guild();
varargs mixed query_condition(int c, int qtype);

#endif

// Saveable preference data:
public mapping text_effects;    // Various player-settable effects for the
                                // text.
mapping Env = ([]);             // Environment-variables

// Temporary run-time data:
static int s_columns, s_rows;   // Screen dimensions.
static string term_reset;
static int effects_on;          // Should we add effects?

// Term codes for terminal_colour on long descs
private nosave mapping term_codes;

// Charsets to set on a user when required
private static int *_ctrl_charset =({
  255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
});

#define TERM_HAS_EFFECTS(x)    (x == "vt100" || x == "ansi")

// Called by /bin/pub/_set.c
void set_text_effects(mapping m) { text_effects = m; }
mapping query_text_effects() { return deep_copy(text_effects); }

int set_env(mixed env, mixed val)
{
    if (!Env) Env =([ ]);

    if (!val) {
    	Env = m_delete(Env, env);
    	return 1;
    }

    // Unfortunately, some env variables are special... *sigh*
    //(actually, might be better to use dedicated variables here but...)

    if (!intp(val) && !stringp(val)) return 0;
    if (env == "term") {
        Env[env] = val;
        if (TERM_HAS_EFFECTS(val)) {
            term_codes = ANSI_D->query_terminal_codes();
            effects_on = 1;
        } else {
            term_codes =([0: ""]);
            effects_on = 0;
        }
    } else if (env == "rows" || env == "columns") {
        Env[env] = to_int(val);
        s_columns = 0;
    } else {
        Env[env] = val;
    }
    return 1;
}

mixed query_env(mixed env)
{
    if (!env || !Env) return Env;
    return Env[env];
}

status set_text_effect(int effect, mixed value)
{
    if (!text_effects) text_effects =([ ]);
    if (!value) m_delete(text_effects, effect);
    else text_effects[effect] = value;
    return 0;
}

string query_text_effect(int effect) {
    if (!text_effects || !text_effects[effect]) return 0;
    return text_effects[effect];
}

void binmsg(string str, int flag)
{
    if (!previous_object()) return;
    if (query_env("binmsg") == "SELF")
        if (!sscanf(object_name(previous_object()),
            sprintf("home/%s/%%~s", name)) &&
          this_player() != this_object()) return;
    binary_message(str, flag);
    return;
}

public varargs int tell_me(mixed str, mixed sense_list, mixed tell_flags,
         mixed me, mixed him, mixed it, int stat)
{
    mixed data, str2;
    int i, j, ret, ansi_count, senses, mxpEnabled;
    string s;

    /* Compatibility section: */
    if (sense_list && intp(sense_list)) {
        tell_flags = sense_list;
        sense_list = 0;
    }

    /* Sense list, done by Chopin.
     * If the second argument is a pointer, we need to check if we can
     * "sense" this message(ie. see, hear, feel or smell):
     * Hope it works now //Chopin
     */
    if (pointerp(sense_list)) {
        senses = 15;
        /* = LIV_CAN_SEE | LIV_CAN_HEAR | LIV_CAN_SMELL | LIV_CAN_FEEL
         * smelling and feeling are quite useless though
         * if "me" object is invisible, we can't "see" the message
         * unless we can detect invis
         */
        if (query_condition(C_BLIND) ||
         (me &&(this_object() != me) &&(me->query_invis()>0)
            && !query_condition(C_DETECT_INVIS)) )
            senses &= ~LIV_CAN_SEE;

        if (query_condition(C_DEAF))
            senses &= ~LIV_CAN_HEAR;

        for(i=0;i<sizeof(sense_list);i++) {
            if (senses ==(sense_list[i] | senses))
                break;
        }
        if (i==sizeof(sense_list))
            return TELL_RETURN_NONE_USED;
        // Couldn't sense this message -> let's return!

        if (stringp(str[i]) && str[i][0] == ':') {
            str[i] = make_format_string(str[i][1..]);
        }
        // If it is an SSF, we'll need to make the format string. Note that
        // this updates the actual array given as argument, so we'll only
        // format em once!
        str2 = str[i];
        ret = i;
    } else {
        if (stringp(str)) {
            if (str[0] == ':') str = make_format_string(str[1..]);
            str2 = str;
            ret = 0;
        } else if (pointerp(str))  str2 = str;
        else return TELL_RETURN_NONE_USED;
    }

    /* Have to make sure screen dimensions are set ok... To optimize, let's
     * handle it here, not sooner.
     */
    if (!s_columns || tell_flags == TELL_UPDATE) {
        // We have to keep track of whether there should be special effects...
        effects_on = (Env && TERM_HAS_EFFECTS(Env["term"])) ? 1 : 0;

        if (data = query_env("columns")) {
            if (stringp(data)) {
                Env["columns"] = s_columns = to_int(data);
                // Hack the "rows" into integer too.
                if (stringp(data = Env["rows"]))
                    Env["rows"] = s_rows = to_int(data);
            } else {
                s_columns =(intp(data) && data) ? data : 80;
            }
        }
        if (s_columns < 2) s_columns = 80;
        if (s_rows < 2)    s_rows = 25;

        if (tell_flags == TELL_UPDATE);
        tell_flags = 0;
    }

    if (pointerp(str2) && sizeof(str2) > 1) {
        str2 = interpret_format_string(str2, effects_on, &ansi_count,
          tell_flags, me, him, it);
    }

    if (!stringp(str2)) return TELL_RETURN_NONE_USED;

    if (intp(tell_flags)) j = tell_flags;
    else if (pointerp(tell_flags)) j = tell_flags[0];

    // 23-Dec-12, Germ
    if (stat) CMD_D->tell_stats(name, str2, coder_level);

    if (j == TELL_TYPE_TELL && query_env("gmcp"))
        TELOPT_D->send_comm_channel_text(this_object(), "Tell", 0, str2);
    else if (j == TELL_TYPE_SAY && query_env("gmcp"))
        TELOPT_D->send_comm_channel_text(this_object(), "Say", 0, str2);
    else if (j == TELL_TYPE_WHISPER && query_env("gmcp"))
        TELOPT_D->send_comm_channel_text(this_object(), "Whisper", 0, str2);

    /* Now we have the message, and we need to worry about how to present it.
     * Also, players may want to add some effects(ANSI-codes) to mark certain
     * types of messages
     */
    if (effects_on && mappingp(text_effects)
        &&(s = text_effects[j & TELL_TYPE_MASK])) {
        binary_message(s, 1);
    } else s = 0;

    int wrap = j&TELL_NO_WRAP ? 0 : s_columns-1;
    int type_term = (j&TELL_TYPE_MASK)==TELL_TYPE_TERMINAL_COLOUR ? 1 : 0;

    if (type_term) {
        int *orig_charset = 0;
        if (!term_codes)
            term_codes = effects_on ? ANSI_D->query_terminal_codes() :([0: ""]);
        if (effects_on) {
            orig_charset = get_connection_charset(0);
            set_connection_charset(_ctrl_charset, 1);
        }
        str2 = terminal_colour(str2, term_codes, wrap, 0);
        if (wrap) tell_object(this_object(), str2 + "\n");
        else tell_object(this_object(), str2);
        if (effects_on) set_connection_charset(orig_charset, 0);
    } else {
    	if (!wrap) tell_object(this_object(), str2);
    	else tell_object(this_object(), terminal_colour(str2, 0, wrap, 0));
    }

    /* And if we had some player-specified effect, we'll end it as well: */
    if (s) {
        if (!term_reset) {
            term_reset = sprintf("%c[0m", /* ] Make Emacs happy*/ 0x1B);
            // term_reset = "\e[0m"; // ] _should_ work, does not.
        }
        binary_message(term_reset, 1);
    }

    /* Send line feed for messages that are using text wrap */
    if (!type_term && wrap) tell_object(this_object(), "\n");

    return ret;
}

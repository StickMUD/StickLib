/****************************************************************
*								*
* Module:							*
*	/basic/player/tell_me.c					*
* Version:							*
*	 2.1a 10-Jan-98			       			*
* Last modification:						*
*	 10-Jan-98, Doomdark					*
* Description:							*
*	"Full version" of tell_me.c for player.c. Includes all	*
*	the nifty things players need, without having to check	*
*	if this_object() is an NPC or a player (formerly it was	*
*	living.c that included tell_me.c).			*
*								*
* NEW:								*
*								*
* 29-dec-97 "Sensing messages" should work now. // Chopin	*
* 10-Jan-98, Doomdark: Added support for client applets/-cations*
* 9-Feb-98, Chopin: Moved the part of code that combines format strings
*   to normal strings from tell_me() to a separate function to fstring.c
*								*
****************************************************************/

#include "/basic/misc/fstring.c"

#ifndef PLAYER_C

#include "/basic/living/living_private.h"
#include <tell_me.h>
#include <living_defs.h>
#include <conditions.h>
#include <client_defs.h>
#include <config.h>

varargs int tell_me(mixed a, mixed b, mixed c, object d, object e, object f);

static int liv_Flags;
static string name;
static int _value_plr_client;	// Defined in /basic/player/client.c

varargs string query_name(int a, object b);
varargs string query_short(int a, object b);
varargs string query_long(int a, object b);
varargs string query_real_name();
varargs string query_race();
varargs string query_guild();
varargs mixed query_condition(int c, int qtype);

#endif

// Saveable preference data:
public mapping text_effects;	// Various player-settable effects for the text.
mapping Env;			// Environment-variables

// Temporary run-time data:
static int s_columns, s_rows;	// Screen dimensions.
static string s_wrap;		// String sprintf uses in tell_me.
static string term_reset;
static int effects_on;		// Should we add effects?

#define	TERM_HAS_EFFECTS(x)	(x == "vt100" || x == "ansi")

int
set_env(mixed env, mixed val)
{
	if (!Env)
		Env = ([ ]);
	if (!val)
		Env = m_delete (Env, env);
// Unfortunately, some env variables are special... *sigh*
// (actually, might be better to use dedicated variables here but...)
	else {
		if (!intp(val) && !stringp(val)) return 0;
		if (env == "term") {
			Env[env] = val;
			if (TERM_HAS_EFFECTS(val))
				effects_on = 1;
			else
				effects_on = 0;
		} else if (env == "rows" || env == "columns") {
			Env[env] = to_int(val);
			s_wrap = 0;	// This will make us update
		} else {
			Env[env] = val;
		}
	}
	return 1;
}

mixed
query_env (mixed env)
{
	if (!env || !Env)
		return Env;
	return Env[env];
}


status
set_text_effect(int effect, mixed value)
{
	if (!text_effects) text_effects = ([ ]);
	if (!value)
		m_delete(text_effects, effect);
	else
		text_effects[effect] = value;
}

void
binmsg(string str, int flag)
{
   if(!previous_object()) return;
   if(query_env("binmsg") == "SELF")
#if 1
        if(
#else
      if(!sscanf(object_name(previous_object()),
	sprintf(PATH_USER_FNAME "%s/%%~s", name)) &&
#endif
	this_player() != this_object())
         return;
   binary_message(str, flag);
   return;
}

public varargs int
tell_me(mixed str, mixed sense_list, mixed tell_flags,
mixed me, mixed him, mixed it)
{
mixed data, str2, j;
int i, ret, ansi_count, senses;
string s;

/* Compatibility section: */
// Umh. Causes problems if used this way!
#if 0
	if (sense_list && intp(sense_list)) {
		tell_flags = sense_list;
		sense_list = 0;
	}
#endif

// If the second argument is a pointer, we need to check if we can
// "sense" this message (ie. see, hear, feel or smell):
// Hope it works now //Chopin

	if (pointerp(sense_list)) {
		senses = 15;
		// = LIV_CAN_SEE | LIV_CAN_HEAR | LIV_CAN_SMELL | LIV_CAN_FEEL
		// smelling is quite useless though
			
		// if "me" object is invisible, we can't "see" the message
		// unless we can detect invis

		/* Hmmh. This is a kludgy way; not always what
		 * we want... -+ Doomdark 05-Sep-98 +-
		 */
		
		if(query_condition(C_BLIND) || 
		(me && (this_object() != me) && (me->query_invis()>0)
		&& !query_condition(C_DETECT_INVIS)) )
			senses &= ~LIV_CAN_SEE;
		
		if(query_condition(C_DEAF))
			senses &= ~LIV_CAN_HEAR;

		for(i=0;i<sizeof(sense_list);i++) {
			if(senses == (sense_list[i] | senses))
				break;
		}
		if(i==sizeof(sense_list))
			return TELL_RETURN_NONE_USED;
// Couldn't sense this message -> let's return!

		if (stringp(str[i]) && str[i][0] == ':') {
			str[i] = make_format_string(str[i][1..]);
		}
// If it is an SSF, we'll need to make the format string. Note that this
// updates the actual array given as argument, so we'll only format em once!
		str2 = str[i];
		ret = i;
	} else {
/* Here we SHOULD consider cases where we have just a single
 * sense-specifier.
 */
	  if (stringp(str)) {
		if (str[0] == ':')
			str = make_format_string(str[1..]);
		str2 = str;
		ret = 0;
	  } else if (pointerp(str)) {
		str2 = str;
	  } else return TELL_RETURN_NONE_USED;
	}

/* Have to make sure screen dimensions are set ok... To optimize, let's
 * handle it here, not sooner.
 */
	if (!s_wrap || tell_flags == TELL_UPDATE) {
// We have to keep track of whether there should be special effects...
		if (Env && TERM_HAS_EFFECTS(Env["term"]))
			effects_on = 1;
		else
			effects_on = 0;

		if (data = query_env("columns")) {
		    if (stringp(data)) {
			Env["columns"] = s_columns = to_int(data);
// Hack the "rows" into integer too.
			if (stringp(data = Env["rows"]))
			    Env["rows"] = s_rows = to_int(data);
		    } else {
			s_columns = (intp(data) && data) ? data : 80;
		    }
		}
		if (s_columns < 2)
			s_columns = 80;
		if (s_rows < 2)
			s_rows = 25;
// And, having gotten needed info, we can define the formatting string:
		s_wrap = sprintf("%%-=%ds\n", s_columns - 1);
		if (tell_flags == TELL_UPDATE);
			tell_flags = 0;
	}

	if (pointerp(str2) && sizeof(str2) > 1) {
		str2 = interpret_format_string(str2, effects_on, &ansi_count,
			tell_flags, me, him, it);
	}
	
	if(!stringp(str2)) return TELL_RETURN_NONE_USED;

	if (intp(tell_flags))
		j = tell_flags;
	else if (pointerp(tell_flags))
		j = tell_flags[0];

#if 0
// Why is this here? - Chopin
	if (j & TELL_NO_WRAP) {
	    string pref;
	    if(mappingp(text_effects) && effects_on && (pref = text_effects[j ^ TELL_NO_WRAP])) binary_message(pref);
		if (ansi_count)
			binary_message(str2);
		else
			tell_object(this_object(), str2);
	    if(pref) binary_message(sprintf("%c[0m",27));
		return ret;
	}
#endif

/* Now we have the message, and we need to worry about how to
 * present it.
 */

/* Client(s) may wish to know type of the message.
 * The MM-client indicates this by using begin and end tags:
 * (which have to be explicitly marked by mudlib, as messages can
 * consist of pieces from more than one call to tell_me())
 */
  if (j && (j & TELL_BEGIN)) {
	if (_value_plr_client == CLIENT_MURDER) {
	    ret = TELL_RETURN_CLIENT_IN_USE;	// Ugh.
	    switch (j & TELL_TYPE_MASK) {
	    case TELL_TYPE_INVENTORY:
		binary_message(CLIENT_DO_TAG_BEGIN(CLIENT_TAG_INV));
		break;
	    case TELL_TYPE_INV_ITEM:
		binary_message(CLIENT_DO_TAG_BEGIN(CLIENT_TAG_ITEM));
		break;
	    case TELL_TYPE_ROOM_DESC:
		binary_message(CLIENT_DO_TAG_BEGIN(CLIENT_TAG_RDESC));
		break;
	    case TELL_TYPE_ROOM_ITEM:
		binary_message(CLIENT_DO_TAG_BEGIN(CLIENT_TAG_RITEM));
		break;
	    }
	}
  }

/* Also, players may want to add some effects (ANSI-codes) to mark certain
 * types of messages (doesn't take into account TELL_BEGIN/END markings,
 * although it could well do just that):
 */
  if (effects_on && mappingp(text_effects)
  && (s = text_effects[j & TELL_TYPE_MASK])) {
	  binary_message(s);
  } else s = 0;

/* We can also prevent the linewrapping: */
  if (j & TELL_NO_WRAP) {
	if (ansi_count)
		binary_message(str2);
	else
		tell_object(this_object(), str2);
  } else {
	// There are some special cases where client-side needs more support:
	switch (j & TELL_TYPE_MASK) {
	case TELL_TYPE_ROOM_DESC:
	  // Room desc should not be wrapped, if we are using the client:
		if (_value_plr_client == CLIENT_MURDER) {
		  if (ansi_count)
			binary_message(str2);
		  else
			tell_object(this_object(), str2);
		// But we also need the trailing linefeed
		  binary_message("\n");
		  break;
		}
		// Otherwise (no client) we just fall back to default:
	  default:
		if (ansi_count)
		  binary_message(sprintf(s_wrap, (str2)));
		else
		  tell_object(this_object(),sprintf(s_wrap,str2));
	  }
  }

/* And if we had some player-specified effect, we'll end it as well: */
  if (s) {
	if (!term_reset) {
		term_reset = sprintf("%c[0m", /* ] Make Emacs happy*/ 0x1B);
		// term_reset = "\e[0m"; // ] _should_ work, does not.
	}
	binary_message(term_reset);
  }

  /* And finally we may have to add the end tag too: */
  if (j && (j & TELL_END)) {
	if (_value_plr_client == CLIENT_MURDER) {
	    ret = TELL_RETURN_CLIENT_IN_USE;	// Ugh again.
	    switch (j & TELL_TYPE_MASK) {
	    case TELL_TYPE_INVENTORY:
		binary_message(CLIENT_DO_TAG_END(CLIENT_TAG_INV));
		break;
	    case TELL_TYPE_ROOM_DESC:
		binary_message(CLIENT_DO_TAG_END(CLIENT_TAG_RDESC));
		break;
	    }
	}
  }
  return ret;
}



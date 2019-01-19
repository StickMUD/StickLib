/****************************************************************
*								*
* Module:							*
*	/bin/daemons/channel_d.c	      			*
* Description:							*
*	The channel daemon of StickLib. Handles all the global	*
*	player channels the mud has (if any). As the original	*
*	channels varied a lot, this daemon is a big mess, but	*
*	at least there are no n+1 separate handlers for each	*
*	channel any more.					*
* Version:							*
*	1.0a (for StickLib), Aug-98				*
* Last update:							*
*	04-Sep-98 by Doomdark					*
*								*
* Modifications:						*
*								*
****************************************************************/

#include "/sys/configuration.h"
#include "/sys/interactive_info.h"
#include "/sys/object_info.h"

#include <channels.h>
#include <coder_levels.h>
#include <guild.h>
#include <room_defs.h>
#include <invis_levels.h>
#include <lord.h>
#include <lights.h>
#include <npc_defs.h>
#include <tell_me.h>
#include <mud_name.h>

static mapping channels;
static mapping ch_ppl;
static mapping ch_admin;
static string *forbid_str;
static closure sort_lambda;
static string *dont_show_ld;

// Channel log dir, max log size
#define CLOG_DIR      "/data/d/channel_d/"
#define MAX_CLOG  50000

/* To enable/disable the support for certain stick-specific
 * guilds uncomment/comment these defines:
 */

//#define	HAVE_MAGE_GUILD
#define		HAVE_PRIEST_GUILD
//#define	HAVE_NECROMANCER_GUILD
//#define	HAVE_NINJA_GUILD
#define	        HAVE_THIEF_GUILD
//#define	HAVE_HEALER_GUILD
#define		HAVE_DANCER_GUILD // Shadowchaser specific

nomask void
create()
{
    channels = ([
      "mud":        ({ TELL_TYPE_MUD_CHANNEL, 0 }),
      "water":      ({ TELL_TYPE_GUILD_CHANNEL, 0 }),
      "earth":      ({ TELL_TYPE_GUILD_CHANNEL, 0 }),
      "wind":       ({ TELL_TYPE_GUILD_CHANNEL, 0 }),
      "fire":       ({ TELL_TYPE_GUILD_CHANNEL, 0 }),
#ifdef USE_STICKMUD_CHANNELS
      "ooc":        ({ TELL_TYPE_LORD_CHANNEL, 0 }),
#endif
      "lord":       ({ TELL_TYPE_LORD_CHANNEL, 0 }),
#ifdef USE_STICKMUD_CHANNELS
      "flord":      ({ TELL_TYPE_FLORD_CHANNEL, 0 }),
      "ninja":      ({ TELL_TYPE_GUILD_CHANNEL, "ninja_list" }),
      "ninjasen":   ({ TELL_TYPE_GUILD_CHANNEL, "ninjasen_list" }),
      "mage":       ({ TELL_TYPE_GUILD_CHANNEL, "mage_list" }),
      "necro":      ({ TELL_TYPE_GUILD_CHANNEL, 0 }),
#endif
      "thief":      ({ TELL_TYPE_GUILD_CHANNEL, "thief_list" }),
      "thiefdmn":   ({ TELL_TYPE_GUILD_CHANNEL, "thiefdmn_list" }),
      "thiefgmc":   ({ TELL_TYPE_GUILD_CHANNEL, "thiefgmc_list" }),
      "priest":     ({ TELL_TYPE_GUILD_CHANNEL, "priest_list" }),
#ifdef USE_STICKMUD_CHANNELS
      "healer":     ({ TELL_TYPE_GUILD_CHANNEL, "healer_list" }),
      "fighter":    ({ TELL_TYPE_GUILD_CHANNEL, "fighter_list" }),
      "jyu":        ({ TELL_TYPE_PARTY,         0 }),
#endif
      "wiz":        ({ TELL_TYPE_CODER_CHANNEL, 0 }),
      "gunn":    ({ TELL_TYPE_GUILD_CHANNEL, 0 }),
      "stealer":    ({ TELL_TYPE_GUILD_CHANNEL, 0 }),
      "duren":    ({ TELL_TYPE_GUILD_CHANNEL, 0 }),
      "rider":    ({ TELL_TYPE_GUILD_CHANNEL, 0 }),
#ifdef USE_STICKMUD_CHANNELS
      "fiz":        ({ TELL_TYPE_CODER_CHANNEL, 0 }),
#endif
      "arch":       ({ TELL_TYPE_CODER_CHANNEL, 0 }),
      "admin":       ({ TELL_TYPE_CODER_CHANNEL, 0 }),
    ]);

    ch_ppl = ([
      "mud":       ({}),
      "water":     ({}),
      "earth":     ({}),
      "wind":      ({}),
      "fire":      ({}),
      "gunn":    ({}),
      "stealer":    ({}),
      "duren":    ({}),
      "rider":    ({}),
#ifdef USE_STICKMUD_CHANNELS
      "ooc":       ({}),
#endif
      "lord":      ({}),
#ifdef USE_STICKMUD_CHANNELS
      "flord":     ({}),
      "ninja":     ({}),
      "ninjasen":  ({}),
      "mage":      ({}),
      "necro":     ({}),
#endif
      "thief":     ({}),
      "thiefdmn":  ({}),
      "thiefgmc":  ({}),
      "priest":    ({}),
#ifdef USE_STICKMUD_CHANNELS
      "jyu":       ({}),
#endif
      "wiz":       ({}),
#ifdef USE_STICKMUD_CHANNELS
      "fiz":       ({}),
#endif
      "arch":      ({}),
      "admin":      ({}),
#ifdef USE_STICKMUD_CHANNELS
      "healer":    ({}),
      "fighter":   ({}),
#endif
    ]);

    ch_admin = ([
      "flord":  ({ "garathan", }),
      "lord":   ({ "garathan", }),
    ]);

    // These strings in attempts to fake messages.
    forbid_str = ({
      " says: ",
      " shouts: ",
      " yells: ",
      //    " tells you: ",
      " whispers: ",
      " asks: ",
      " bellows: ",
    });

    // Channels on which we don't want to see the Linkdeads
    dont_show_ld = ({ "flord", "mage" });

    filter(users(), "check_channels");
}

nomask void reset() { }
nomask void init() { }
nomask void heart_beat() { }

// Returns  : 0 if it's OK, otherwise the reason why not.
nomask string
valid_channel_message(string str)
{
    int i;

    for (i = sizeof(forbid_str) - 1; i >= 0; i--)
    {
	if (strstr(str, forbid_str[i], 0) != -1)
	    return "You can't use string \"" + forbid_str[i] + "\" on a channel.";
    }

    return 0;
}

nomask string *
query_channel_names()
{
    return m_indices(channels);
}

nomask object *
query_ch_ppl(string ch)
{
    return ch_ppl[ch];
}

nomask int
query_is_channel_admin(object who, string ch)
{
    string n;

    if (!ch || !who || !pointerp(ch_admin[ch])) return 0;

    n = (string)who->query_real_name();

    return (member(ch_admin[ch], n) != -1);
}

nomask string query_channel_log_file(string ch) { return CLOG_DIR + ch; }

nomask void
tail_channel(string ch, object plr)
{
    // read in the last 20 lines of a channel, and print it out to the
    // user.
    string msg, file;
    int size;

    file = query_channel_log_file(ch);
    size = file_size(file);

    if(size < 1000)
	msg = read_bytes(file, -size, size);
    else
	msg = read_bytes(file, -1000, 1000);

    if(!msg  ||  msg == "")
	plr->tell_me("Empty file or file not found.");
    else
	plr->tell_me(msg, 0, channels[ch][0]);
}

nomask string *
more_channel(string ch)
{
    if (file_size(CLOG_DIR + ch) < 1)
	return ({ "Empty file or file not found." });
    else return explode(read_file(CLOG_DIR + ch), "\n");
}

nomask static void
log_channel(string ch, string msg)
{
    string fn;

    if (ch == "arch" || ch == "admin") return;

    fn = CLOG_DIR + ch;

    if (file_size(fn) >= MAX_CLOG)
    {
	rm(fn + ".old");
	rename(fn, fn + ".old");
    }

    write_file(fn, msg + "\n");
}

nomask int
can_read_channel(object ob, string ch)
{
    if (!member(channels, ch)) return 0;
    return call_other(this_object(), ch + "_check", ob);
}

nomask int
join_channel(string ch, object ob)
{
    if (!ob) return 0;
    if (!pointerp(ch_ppl[ch])) return 0;
    if (!can_read_channel(ob, ch)) return 0;
    if (member(ch_ppl[ch], ob) != -1) return 1;
    ch_ppl[ch] -= ({ 0 }); // Purge lost members
    ch_ppl[ch] += ({ ob });
    return 1;
}

nomask int
remove_from_channel(string ch, object ob)
{
    if (!ob) return 0;
    if (!pointerp(ch_ppl[ch])) return 0;
    if (member(ch_ppl[ch], ob) == -1) return 1;
    ch_ppl[ch] -= ({ ob });
    ch_ppl[ch] -= ({ 0 }); // Purge lost members
    return 1;
}

nomask int
sort_em(object ob1, object ob2)
{
    int x1, x2;

    if (!objectp(ob2)) return 1;
    if (!objectp(ob1)) return -1;

    x1 = (int)ob1->query_coder_level();
    x2 = (int)ob2->query_coder_level();
    if (x1 > x2) return 1;
    if (x1 < x2) return -1;

    x1 = (int)ob1->query_level();
    x2 = (int)ob2->query_level();
    if (x1 > x2) return 1;
    if (x1 < x2) return -1;

    return 0;
}

nomask string *
list_channel(string ch, object who)
{
    object *m;
    string *tmp, tmp2, n;
    int i, j, s, z, idle, me_coder, isolated, is_coder, ilevel, me_level;

    if (!who) return ({});

    if (!pointerp(ch_ppl[ch]))
	return ({ "There is no channel called \"" + ch + "\"." });

    // Separate list function?
    if (channels[ch][1])
	return call_other(this_object(), channels[ch][1], who);

    ch_ppl[ch] -= ({ 0 });

    m = sort_array(ch_ppl[ch], "sort_em", this_object());

    me_level = (int)who->query_level();
    me_coder = (int)who->query_coder_level();
    isolated = (!me_coder && environment(who)->query(ROOM_ISOLATED));

    tmp = ({ });

    z = sizeof(m);
    j = 0;

    for (i = 0; i < z; i++)
    {
	if (!m[i]) continue;

	ilevel = (int)m[i]->query_invis();

	if (m[i] == who ||
	  call_other(this_object(), ch + "_check", m[i]) &&
	  environment(m[i]) &&
	  !environment(m[i])->query(ROOM_ISOLATED) &&
	  (me_coder || !ilevel) ||
	  ((me_level > CAN_SEE_INVIS) && (ilevel < IL_TOTALLY_INVISIBLE)))
	{
	    if (isolated && m[i] != who) continue;

	    s = (int)m[i]->query_channel_status(ch);

	    if ((ch == "flord") && (s & CHANNEL_OFF)) continue;

	    if ((s & CHANNEL_LD)
	      && member(dont_show_ld, ch) != -1) continue;

	    if (interactive(m[i])) idle = interactive_info(m[i], II_IDLE); else idle = 0;

	    if (!stringp(tmp2 = (string)m[i]->query_pretitle()))
		tmp2 = "";

	    n = capitalize((string)m[i]->query_real_name());

	    if (ilevel >= IL_TOTALLY_INVISIBLE) n = "<"+n+">";
	    else if (ilevel > 0) n = "("+n+")";

	    if (interactive_info(m[i], II_EDITING)) n = "[" + n + "]";

	    tmp += ({
	      sprintf("[%|21s]  %-16s %8s",
		tmp2,
		n,
		((s & CHANNEL_BAN) ? " Banned " :
		  (s & CHANNEL_LD)  ? "Linkdead" :
		  (s & CHANNEL_OFF) ? "   Off  " :
		  (idle > 300)      ? "  Idle  " : "   On   "))
	    });

	    if (!(s & (CHANNEL_BAN | CHANNEL_OFF))) j++;
	}
    }

    if (j == 0) tmp = ({ "No one on channel \"" + ch + "\"." });
    else
	tmp = ({ " Total of " + j + " people on channel \"" + ch + "\":" }) + tmp;

    return tmp;
}

nomask int
check_channels(object ob)
{
    string *c;
    int i;

    if (!ob) return 0;

    c = m_indices(channels);

    for (i = sizeof(c) - 1; i >= 0; i--)
    {
	if (call_other(this_object(), c[i] + "_check", ob))
	    join_channel(c[i], ob);
	else
	    remove_from_channel(c[i], ob);
    }

    return 1;
}

nomask int
send_channel_message(mixed ob, string ch, string msg)
{
    string tmp, cn;
    int i, e, indent;

    if (!member(channels, ch)) return 0;
    if (!stringp(msg)) return 0;

    if (objectp(ob)
      && environment(ob)->query(ROOM_ISOLATED)
      && !ob->query_coder_level())
    {
	ob->tell_me("No one heard you.");
	return 1;
    }
    else cn = capitalize(ch);
    if (!pointerp(ch_ppl[ch])) return 0;

    ch_ppl[ch] -= ({ 0 }); // Purge lost members
    e = 0;

    // Emote?
    if (msg[0] == ':')
    {
	indent = sizeof(ch) + 3;
	msg = msg[1..<1];
	if (msg[0] != ' ') msg = " " + msg;
	e = 1;
    }
    // Normal message, add ": "
    else
    {
	if (stringp(ob))
	    indent = sizeof(ob) + 3;
	else
	    indent = sizeof(ob->query_real_name()) + 3;
	if (ch == "lord" || ch == "flord") msg = " " + msg;
	else 
	{
	    msg = ": " + msg;
	    indent += sizeof(ch) + 2;
	}
    }

    //  if ((int)ob->query_frog() && !(IS_CODER_OB(ob)))
    //     msg = " HHHHRRRRIIIBBBIT HHHHRIBBBIT!";

    tmp = call_other(this_object(), ch + "_format", ob, msg, e);

    tmp = b_string(tmp, 72, indent);

    for (i = sizeof(ch_ppl[ch]) - 1; i >= 0; i--)
    {
	if (objectp(ch_ppl[ch][i]) &&
	  ch_ppl[ch][i]->is_on_channel(ch) &&
	  !((int)ch_ppl[ch][i]->query_channel_status(ch) & CHANNEL_OFF))
	    ch_ppl[ch][i]->tell_me(tmp, 0, channels[ch][0], 0, 0, 0);
    }

    log_channel(ch, tmp);

    return 1;
}

//
// Who can be on each channel? Separate function for each channel
// is probably required.
//

//
// This channel is for Out Of Character talk only!
// Must have played 24h and be on level 5
// Disabled for non-coders atm. /Graah
//
nomask status
ooc_check(object ob)
{
    return (ob
      && ob->query_coder_level()
      && ((int)ob->query_age() > (24 * 60 * 60))
      && ((int)ob->query_level() >= 5));
}

nomask string
ooc_format(mixed ob, string msg, int e)
{
    if (objectp(ob))
	return "[OOC] " + capitalize((string)ob->query_real_name()) + msg;
    if (stringp(ob)) return "[OOC] " + ob + msg;
    else return "[OOC]" + msg;
}

nomask status earth_check(object ob)
{
    return (ob && ((int)ob->query_kingdom("earth") || (int)ob->query_coder_level()));
}

nomask string earth_format(mixed ob, string msg, int e)
{
    if (objectp(ob))
	return "[Earth] " + capitalize((string)ob->query_real_name()) + msg;
    if (stringp(ob)) return "[Earth] " + ob + msg;
    else return "[Earth] " + msg;
}

nomask status fire_check(object ob)
{
    return (ob && ((int)ob->query_kingdom("fire") || (int)ob->query_coder_level()));
}

nomask string fire_format(mixed ob, string msg, int e)
{
    if (objectp(ob))
	return "[Fire] " + capitalize((string)ob->query_real_name()) + msg;
    if (stringp(ob)) return "[Fire] " + ob + msg;
    else return "[Fire] " + msg;
}

nomask status wind_check(object ob)
{
    return (ob && ((int)ob->query_kingdom("wind") || (int)ob->query_coder_level()));
}

nomask string wind_format(mixed ob, string msg, int e)
{
    if (objectp(ob))
	return "[Wind] " + capitalize((string)ob->query_real_name()) + msg;
    if (stringp(ob)) return "[Wind] " + ob + msg;
    else return "[Wind] " + msg;
}

nomask status water_check(object ob)
{
    return (ob && ((int)ob->query_kingdom("water") || (int)ob->query_coder_level()));
}

nomask string water_format(mixed ob, string msg, int e)
{
    if (objectp(ob))
	return "[Water] " + capitalize((string)ob->query_real_name()) + msg;
    if (stringp(ob)) return "[Water] " + ob + msg;
    else return "[Water] " + msg;
}

nomask status mud_check(object ob) {
    return 1;
}

nomask string
mud_format(mixed ob, string msg, int e)
{
    if (objectp(ob))
	return "[Mud] " + capitalize((string)ob->query_real_name()) + msg;
    if (stringp(ob)) return "[Mud] " + ob + msg;
    else return "[Mud]" + msg;
}

nomask status
lord_check(object ob)
{
    return (ob
      && (ob->query_coder_level()
	|| (int)ob->query_level() >= 25));
}

nomask string
lord_format(mixed ob, string msg, int e)
{
    if (objectp(ob))
    {
	if (!e) return "<" + capitalize((string)ob->query_real_name()) +
	    ">" + msg;
	else return "<Lord> " + capitalize((string)ob->query_real_name()) +
	    msg;
    }
    else if (stringp(ob))
    {
	if (!e) return "<" + ob + ">" + msg;
	else return "<Lord> " + ob + msg;      
    }
    else return "<Lord> " + msg;
}

nomask status
flord_check(object ob)
{
    return (ob
      && (ob->query_coder_level()
	|| (int)ob->query_level() >= 25));
}

nomask string
flord_format(mixed ob, string msg, int e)
{
    if (objectp(ob))
    {
	if (!e) return "{" + capitalize((string)ob->query_real_name()) +
	    "}" + msg;
	else return "{FLord} " + capitalize((string)ob->query_real_name()) +
	    msg;
    }
    else if (stringp(ob))
    {
	if (!e) return "{" + ob + "}" + msg;
	else return "{FLord} " + ob + msg;      
    }
    else return "{Lord} " + msg;
}

nomask status
wiz_check(object ob)
{
    return (ob && (int)ob->query_coder_level() >= LVL_APPRENTICE);
}

nomask string
wiz_format(mixed ob, string msg, int e)
{
    if (objectp(ob))
	return "[Wiz] " + capitalize((string)ob->query_real_name()) + msg;
    if (stringp(ob)) return "[Wiz] " + ob + msg;
    else return "[Wiz]" + msg;
}

nomask status
fiz_check(object ob)
{
    return (ob && (int)ob->query_coder_level() >= LVL_APPRENTICE);
}

nomask string
fiz_format(mixed ob, string msg, int e)
{
    if (objectp(ob))
	return "[Fiz] " + capitalize((string)ob->query_real_name()) + msg;
    if (stringp(ob)) return "[Fiz] " + ob + msg;
    else return "[Fiz]" + msg;
}

nomask status
arch_check(object ob)
{    
    return (ob && (int)ob->query_coder_level() >= LVL_ELDER);
}

nomask string
arch_format(mixed ob, string msg, int e)   
{
    if (objectp(ob))
	return "[Arch] " + capitalize((string)ob->query_real_name()) + msg;   
    if (stringp(ob)) return "[Arch] " + ob + msg;   
    else return "[Arch]" + msg;   
}

nomask status
admin_check(object ob)
{
    return (ob && (int)ob->query_coder_level() >= LVL_COADMIN);
}

nomask string
admin_format(mixed ob, string msg, int e)
{
    if (objectp(ob))
	return "[Admin] " + capitalize((string)ob->query_real_name()) + msg;
    if (stringp(ob)) return "[Admin] " + ob + msg;
    else return "[Admin]" + msg;
}

#ifdef GN_THIEF
// Thief Guild channels
nomask status
thief_check(object ob)
{
    if (!ob) return 0;

    return (ob
      && ((string)ob->query_guild() == GN_THIEF)
      || ob->query_coder_level());
}

nomask string
thief_format(mixed ob, string msg, int e)
{
    if (msg[0] == ':') msg = msg[1..<1];

    if (objectp(ob))
    {
	if (!e) return "[Thf " + capitalize((string)ob->query_real_name()) +
	    "]" + msg;
	else return "[Thf] " + capitalize((string)ob->query_real_name()) +
	    msg;
    }
    else if (stringp(ob))
    {
	if (!e) return "[Thf " + ob + "]" + msg;
	else return "[Thf] " + ob + msg;      
    }
    else return "[Thf] " + msg;
}

nomask string *
thief_list(object me)
{
    string *who_list, tmp;
    object *who_member, *who, tm, e;
    int i, z, add_list, num, il, me_coder, me_level;
    status isolated;

    who = ch_ppl["thief"];
    who_member = ({});

    if (me && environment(me))
	isolated = (status)environment(me)->query(ROOM_ISOLATED);
    else isolated = 0;

    if (!isolated)
    {
	for(i = 0; i < sizeof(who); i++)
	{
	    if (who[i] && interactive(who[i]))
		who_member += ({ who[i] });
	}
    }
    else who_member = ({ me });

    who_list = ({ });

    me_coder = (int)me->query_coder_level();
    me_level = (int)me->query_level();

    num = 0;

    for (i = 0; i < sizeof(who_member); i++)
    {
	il = (int)who_member[i]->query_invis();
	tm = present("tmark", who_member[i]);

	// Don't show non-thieves on channel to non-coders.
	if (!tm && !me_coder) continue;

	if (me_coder
	  || (!environment(who_member[i])->query(ROOM_ISOLATED)
	    &&
	    (!il
	      || (il < IL_TOTALLY_INVISIBLE && me_level > 21))))
	{
	    tmp = sprintf("%15s %-12s\t %-28s : ", 
	      (tm ? (string)tm->thief_pretitle() : ""),
	      capitalize((string)who_member[i]->query_real_name()),
	      (tm ? "(" +
		(string)"/guilds/thieves/rooms/guild"->query_thief_title((int)who_member[i]->query_level(),
		  who_member[i]->query_gender()) + ")" : ""));

	    z = who_member[i]->query_channel_status("thief");
	    if ((z & CHANNEL_BAN)) tmp += "banned";
	    else if ((z & CHANNEL_OFF)) tmp += "off";
	    else { tmp += "on"; num++; }

	    who_list += ({ tmp });
	}
    }

    who_list += ({ "Total number of people on channel: " + num });

    return who_list;
}

// Thief "guildmaster" channel (demons, enforcers)
nomask status
thiefgmc_check(object ob)
{
    object tm;

    if (!ob) return 0;

    return (ob->query_coder_level()
      || (((string)ob->query_guild() == GN_THIEF)
	&& (tm = present("tmark", ob))
	&& ((int)tm->query_guildmaster() > 0)));
}


nomask string
thiefgmc_format(mixed ob, string msg, int e)
{
    if (msg[0] == ':') msg = msg[1..<1];

    if (objectp(ob))
    {
	if (!e) return "[GMC " + capitalize((string)ob->query_real_name()) +
	    "]" + msg;
	else return "[GMC] " + capitalize((string)ob->query_real_name()) +
	    msg;
    }
    else if (stringp(ob))
    {
	if (!e) return "[GMC " + ob + "]" + msg;
	else return "[GMC] " + ob + msg;      
    }
    else return "[GMC] " + msg;
}

nomask string *
thiefgmc_list(object me)
{
    string *who_list, tmp;
    object *who_member, *who, tm, e;
    int i, z, add_list, num, il, me_coder, me_level;
    status isolated;

    who = ch_ppl["thiefgmc"];
    who_member = ({});

    if (me && environment(me))
	isolated = (status)environment(me)->query(ROOM_ISOLATED);
    else isolated = 0;

    if (!isolated)
    {
	for(i = 0; i < sizeof(who); i++)
	{
	    if (who[i] && interactive(who[i]))
		who_member += ({ who[i] });
	}
    }
    else who_member = ({ me });

    who_list = ({ });

    me_coder = (int)me->query_coder_level();
    me_level = (int)me->query_level();

    num = 0;

    for (i = 0; i < sizeof(who_member); i++)
    {
	il = (int)who_member[i]->query_invis();
	tm = present("tmark", who_member[i]);

	// Don't show non-thieves on channel to non-coders.
	if (!tm && !me_coder) continue;

	if (me_coder
	  || (!environment(who_member[i])->query(ROOM_ISOLATED)
	    &&
	    (!il
	      || (il < IL_TOTALLY_INVISIBLE && me_level > 21))))
	{
	    tmp = sprintf("%15s %-12s\t %-28s : ", 
	      (tm ? (string)tm->thief_pretitle() : ""),
	      capitalize((string)who_member[i]->query_real_name()),
	      (tm ? "(" +
		(string)"/guilds/thieves/rooms/guild"->query_thief_title((int)who_member[i]->query_level(),
		  who_member[i]->query_gender()) + ")" : ""));

	    z = who_member[i]->query_channel_status("thief");
	    if ((z & CHANNEL_BAN)) tmp += "banned";
	    else if ((z & CHANNEL_OFF)) tmp += "off";
	    else { tmp += "on"; num++; }

	    who_list += ({ tmp });
	}
    }

    who_list += ({ "Total number of people on channel: " + num });

    return who_list;
}

// Thief demon channel
nomask status
thiefdmn_check(object ob)
{
    object tm;

    if (!ob) return 0;

    return (ob->query_coder_level()
      || (((string)ob->query_guild() == GN_THIEF)
	&& (tm = present("tmark", ob))
	&& ((int)tm->query_guildmaster() >= 500)));
}


nomask string
thiefdmn_format(mixed ob, string msg, int e)
{
    if (msg[0] == ':') msg = msg[1..<1];

    if (objectp(ob))
    {
	if (!e) return "[Demon " + capitalize((string)ob->query_real_name()) +
	    "]" + msg;
	else return "[Demon] " + capitalize((string)ob->query_real_name()) +
	    msg;
    }
    else if (stringp(ob))
    {
	if (!e) return "[Demon " + ob + "]" + msg;
	else return "[Demon] " + ob + msg;      
    }
    else return "[Demon] " + msg;
}

nomask string *
thiefdmn_list(object me)
{
    string *who_list, tmp;
    object *who_member, *who, tm, e;
    int i, z, add_list, num, il, me_coder, me_level;
    status isolated;

    who = ch_ppl["thiefdmn"];
    who_member = ({});

    if (me && environment(me))
	isolated = (status)environment(me)->query(ROOM_ISOLATED);
    else isolated = 0;

    if (!isolated)
    {
	for(i = 0; i < sizeof(who); i++)
	{
	    if (who[i] && interactive(who[i]))
		who_member += ({ who[i] });
	}
    }
    else who_member = ({ me });

    who_list = ({ });

    me_coder = (int)me->query_coder_level();
    me_level = (int)me->query_level();

    num = 0;

    for (i = 0; i < sizeof(who_member); i++)
    {
	il = (int)who_member[i]->query_invis();
	tm = present("tmark", who_member[i]);

	// Don't show non-thieves on channel to non-coders.
	if (!tm && !me_coder) continue;

	if (me_coder
	  || (!environment(who_member[i])->query(ROOM_ISOLATED)
	    &&
	    (!il
	      || (il < IL_TOTALLY_INVISIBLE && me_level > 21))))
	{
	    tmp = sprintf("%15s %-12s\t %-28s : ", 
	      (tm ? (string)tm->thief_pretitle() : ""),
	      capitalize((string)who_member[i]->query_real_name()),
	      (tm ? "(" +
		(string)"/guilds/thieves/rooms/guild"->query_thief_title((int)who_member[i]->query_level(),
		  who_member[i]->query_gender()) + ")" : ""));

	    z = who_member[i]->query_channel_status("thief");
	    if ((z & CHANNEL_BAN)) tmp += "banned";
	    else if ((z & CHANNEL_OFF)) tmp += "off";
	    else { tmp += "on"; num++; }

	    who_list += ({ tmp });
	}
    }

    who_list += ({ "Total number of people on channel: " + num });

    return who_list;
}
#endif

#ifdef GN_NINJA

// Ninja Guild channel

#define HLINE    "-=0=-=0=-=0=-=0=-=0=-=0=-=0=-=0=-=0=-=0=-=0=-=0=-=0=-=0=-=0=-=0=-=0=-=0=-=0=-"
#define HHEAD  "\nTitle            Name           Belt        Status Activity Location"
#define HHEAD2   "---------------- -------------- ----------- ------ -------- -----------------"

nomask status
ninja_check(object ob)
{
    if (!ob) return 0;

    return (ob
      && ((string)ob->query_guild() == GN_NINJA)
      || ob->query_coder_level());
}

nomask string
ninja_format(mixed ob, string msg, int e)
{
    string pretitle;
    object nmark;

    if (msg[0] == ':') msg = msg[1..<1];

    if (objectp(ob))
    {
	// Added to support pretitles - Excalibur
	if((nmark=present("nmark", ob)))
	    pretitle = (string)nmark->ninja_pretitle();
	else pretitle = "";

	if (!e) return pretitle + capitalize((string)ob->query_real_name()) +
	    " >>" + msg;
	else return "Ninja >> " + pretitle + capitalize((string)ob->query_real_name()) +
	    msg;
    }
    else if (stringp(ob))
    {
	if (!e) return ob + " >>" + msg;
	else return "Ninja >> " + ob + msg;
    }
    else return "Ninja >> " + msg;
}

nomask string *
ninja_list(object me)
{
    string *who_list, tmp, name, beltstr, chanstat, actvstat, loc;
    object *who_member, *who, nmark, e;
    int i, z, add_list, num, il, me_coder, me_level;
    status isolated;

    //  who = ch_ppl["ninja"];
    who = sort_array(ch_ppl["ninja"], "sort_em", this_object());
    who_member = ({});

    if (me && environment(me))
	isolated = (status)environment(me)->query(ROOM_ISOLATED);
    else isolated = 0;

    if (!isolated)
    {
	for(i = 0; i < sizeof(who); i++)
	{
	    //   if (who[i] && interactive(who[i]))
	    if (who[i])
		who_member += ({ who[i] });
	}
    }
    else who_member = ({ me });

    who_list = ({ HLINE });
    who_list += ({ HHEAD });
    who_list += ({ HHEAD2 });

    me_coder = (int)me->query_coder_level();
    me_level = (int)me->query_level();

    num = 0;

    for (i = 0; i < sizeof(who_member); i++)
    {
	il = (int)who_member[i]->query_invis();
	nmark = present("nmark", who_member[i]);

	// Don't show non-ninjas on channel to non-coders.
	if (!nmark && !me_coder) continue;

	if (me_coder
	  || (!environment(who_member[i])->query(ROOM_ISOLATED)
	    &&
	    (!il
	      || (il < IL_TOTALLY_INVISIBLE && me_level > 21))))
	{
	    // Format name
	    name = capitalize((string) who_member[i]->query_real_name());
	    if(me_coder){
		if (il >= IL_TOTALLY_INVISIBLE) name = "<"+name+">";
		else if (il) name = "("+name+")";
	    }
	    else {
		if (who_member[i]->query_coder_level() && il) continue;
		else if (il) name = "("+name+")";
	    }

	    // Strip the word "belt" from ninja belt desc
	    if(nmark)
	    {
		sscanf(nmark->get_title(nmark->query_nlevel()), "%s belt",
		  beltstr);
		num++;
	    }
	    else beltstr = "";

	    // Channel Status
	    z = who_member[i]->query_channel_status("ninja");
	    if ((z & CHANNEL_OFF)) chanstat = "Off";
	    else if ((z & CHANNEL_BAN)) chanstat = "BANNED";
	    else chanstat = "On";

	    // Active Status
	    if ((z & CHANNEL_LD))
		actvstat = "LinkDead";
	    else if (interactive_info(who_member[i], II_IDLE) > 300)
		actvstat = "Idle";
	    else if (who_member[i]->is_fighting())
		actvstat = "Fighting";
	    else if (interactive_info(who_member[i], II_EDITING))
		actvstat = "Editing";
	    else actvstat = "";

	    // Location
	    if (me_coder<0) loc = "(test character)";
	    else
	    if (me_coder || me == who_member[i] ||
	      (int)who_member[i]->query_level() < me_level)
	    { loc = (string) environment(who_member[i])->short();
		if (!loc) loc = "Unknown place"; }
	    else loc = "--";
	    if (sizeof(loc) > 18) loc = loc[0..14] + "...";

	    tmp = sprintf("%-16s %-14s %-11s %-6s %-8s %-17s",
	      (nmark ? (string)nmark->ninja_pretitle() : who_member[i]->query_pretitle() ),
	      name,
	      beltstr,
	      chanstat,
	      actvstat,
	      loc
	    );
	    who_list += ({ tmp });
	}
    }

    who_list += ({ "\n" + num + " ninja"+((num>1) ? "s" : "")+", "+sizeof(users())+" player"+((sizeof(users())>1) ? "s" : "")+"." });
    //  who_list += ({ "\n" + num + " ninjas." });
    who_list += ({ HLINE });

    return who_list;
}

nomask status
ninjasen_check(object ob)
{
    object mark;

    if (!ob) return 0;

    return (ob
      && (((string)ob->query_guild() == GN_NINJA) 
	&& (mark = present("nmark", ob))
	&& (mark->query_sensei() || mark->query_kensai()))
      || ob->query_coder_level());
}

nomask string
ninjasen_format(mixed ob, string msg, int e)
{
    if (msg[0] == ':') msg = msg[1..<1];

    if (objectp(ob))
    {
	if (!e) return capitalize((string)ob->query_real_name()) +
	    " >>>" + msg;
	else return "Ninja >>> " + capitalize((string)ob->query_real_name()) +
	    msg;
    }
    else if (stringp(ob))
    {
	if (!e) return ob + " >>>" + msg;
	else return "Ninja >>> " + ob + msg;
    }
    else return "Ninja >>> " + msg;
}

nomask string *
ninjasen_list(object me)
{
    string *who_list, tmp, name, beltstr, chanstat, actvstat, loc;
    object *who_member, *who, nmark, e;
    int i, z, add_list, num, il, me_coder, me_level;
    status isolated;

    //  who = ch_ppl["ninjasen"];
    who = sort_array(ch_ppl["ninjasen"], "sort_em", this_object());
    who_member = ({});

    if (me && environment(me))
	isolated = (status)environment(me)->query(ROOM_ISOLATED);
    else isolated = 0;

    if (!isolated)
    {
	for(i = 0; i < sizeof(who); i++)
	{
	    //   if (who[i] && interactive(who[i]))
	    if (who[i])
		who_member += ({ who[i] });
	}
    }
    else who_member = ({ me });

    who_list = ({ HLINE });
    who_list += ({ HHEAD });
    who_list += ({ HHEAD2 });

    me_coder = (int)me->query_coder_level();
    me_level = (int)me->query_level();

    num = 0;

    for (i = 0; i < sizeof(who_member); i++)
    {
	il = (int)who_member[i]->query_invis();
	nmark = present("nmark", who_member[i]);

	// Don't show non-ninjas on channel to non-coders.
	if (!nmark && !me_coder) continue;

	if (me_coder
	  || (!environment(who_member[i])->query(ROOM_ISOLATED)
	    &&
	    (!il
	      || (il < IL_TOTALLY_INVISIBLE && me_level > 21))))
	{
	    // Format name
	    name = capitalize((string) who_member[i]->query_real_name());
	    if(me_coder){
		if (il >= IL_TOTALLY_INVISIBLE) name = "<"+name+">";
		else if (il) name = "("+name+")";
	    }
	    else {
		if (who_member[i]->query_coder_level() && il) continue;
		else if (il) name = "("+name+")";
	    }

	    // Strip the word "belt" from ninja belt desc
	    if(nmark)
	    {
		sscanf(nmark->get_title(nmark->query_nlevel()), "%s belt",
		  beltstr);
		num++;
	    }
	    else beltstr = "";

	    // Channel Status
	    z = who_member[i]->query_channel_status("ninjasen");
	    if ((z & CHANNEL_OFF)) chanstat = "Off";
	    else if ((z & CHANNEL_BAN)) chanstat = "BANNED";
	    else chanstat = "On";

	    // Active Status
	    if ((z & CHANNEL_LD))
		actvstat = "LinkDead";
	    else if (interactive_info(who_member[i], II_IDLE) > 300)
		actvstat = "Idle";
	    else if (who_member[i]->is_fighting())
		actvstat = "Fighting";
	    else if (interactive_info(who_member[i], II_EDITING))
		actvstat = "Editing";
	    else actvstat = "";

	    // Location
	    if (me_coder<0) loc = "(test character)";
	    else
	    if (me_coder || me == who_member[i] ||
	      (int)who_member[i]->query_level() < me_level) 
		loc = (string) environment(who_member[i])->short();
	    else loc = "--";
	    if (sizeof(loc) > 18) loc = loc[0..14] + "...";

	    tmp = sprintf("%-16s %-14s %-11s %-6s %-8s %-17s",
	      (nmark ? (string)nmark->ninja_pretitle() : who_member[i]->query_pretitle() ),
	      name,
	      beltstr,
	      chanstat,
	      actvstat,
	      loc
	    );
	    who_list += ({ tmp });
	}
    }

    who_list += ({ "\n" + num + " ninja"+((num>1)? "s" : "")+", "+sizeof(users())+" player"+((sizeof(users())>1) ? "s" : "")+"." });
    //  who_list += ({ "\n" + num + " ninjas." });
    who_list += ({ HLINE });

    return who_list;
}
#endif

#ifdef GN_MAGE

// Mages have joined in! :)
#include "/guilds/mage/include/channel.h"

// If you can't have 'em, try these: 
#if 0
#define	WHITE		400
#define	BLACK		-400
#define	MAGE_LIST_D	"/guilds/mage/daemon/listd"
#define	MAGE_GUILD_TITLE "Guild of Mages"
#define MAGE_CHANNEL_LEFT    "--=*=--"
#define MAGE_CHANNEL_RIGHT   "--=*=--"
#define MAGE_CHANNEL_HEADER \
"-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-"
#define IS_MAGE_GUILD_CODER(x) (member(({ }), x) >= 0 ? 1 : 0)
#endif

nomask status
mage_check(object ob)
{

    if (!ob) return 0;

    return (ob
      && ((string)ob->query_guild() == GN_MAGE)
      || ob->query_coder_level());
}

nomask string
mage_format(mixed ob, string msg, int e)
{
    if (objectp(ob))
	return "@M-" + capitalize((string)ob->query_real_name()) + msg;
    if (stringp(ob)) return "@M-" + ob + msg;
    else return "@M-" + msg;
}

/* Function: sort_by_level (int) is called from mage_list */
int query_sort_by_level(object one, object two) {
    int first, second, x1;

    if (!objectp(one)) return 1;
    if (!objectp(two)) return -1;

    x1 = (int)one->query_coder_level();
    if( x1 && (x1 > (int)two->query_coder_level())) return -1;

    first  = (int) one->query_level();
    second = (int) two->query_level();

    return (( first > second ) ? -1 : 1);
}

/* Function: sort_by_align (object *) is called from mage_list */
object *query_sort_by_align(object *us) {
    object *white, *grey, *black;
    int i, j, align;

    j = sizeof(us);
    white = grey = black = ({ });

    for (i = 0; i < j; i ++)
    {
	align = ((int) us[i]->query_alignment());

	if (align >= WHITE) white += ({ us[i] });
	else if (align <= BLACK) black += ({ us[i] });
	else grey += ({ us[i] });
    }

    return white+grey+black;
}

nomask string *
mage_list(object who)
{
    object *us;
    string name, title, text, text2, text3;
    string place, alig, idle;
    string *council;
    int i, j, his_level, my_level;
    int itotal, ctotal, ptotal, s, cl;

    us = ch_ppl["mage"];
    us -= ({ 0 });
    us = sort_array(us, "query_sort_by_level", this_object());
    us = query_sort_by_align(us);

    text = text2 = "";
    council = MAGE_LIST_D->read_council();
    text3 = sprintf("\n%|25s%|27s%|25s\n%|15s%|47s%|15s%|78s\n",
      MAGE_CHANNEL_LEFT, MAGE_GUILD_TITLE, MAGE_CHANNEL_RIGHT,
      MAGE_CHANNEL_LEFT,
      sprintf("* %s * %s * %s *",
	sprintf("%s",capitalize(council[0])),
	sprintf("%s",capitalize(council[1])),
	sprintf("%s",capitalize(council[2]))),
      MAGE_CHANNEL_RIGHT,
      MAGE_CHANNEL_HEADER);

    my_level = who->query_level();

    for (i = 0; i < sizeof(us); i ++) {
	place = 0;

	s = (int)us[i]->query_channel_status("mage");

	if ((us[i]->query_invis() < IL_TOTALLY_INVISIBLE &&
	    interactive(us[i]) &&
	    (us[i]->query_guild() == GN_MAGE) && // Let's not show non-mage
	    // coders to mortals.
	    !environment(us[i])->query(ROOM_ISOLATED)) ||
	  ((int)who->query_coder_level()&&!(s & CHANNEL_OFF)) ) {

	    if(interactive(us[i])) {
		if (interactive_info(us[i], II_EDITING) >= 300) {
		    idle = "(idle)";
		    itotal ++;
		}
		else idle = "";
	    }
	    j = us[i]->query_alignment();
	    if (j >= WHITE)
		alig = "White";
	    else if (j <= BLACK)
		alig = "Black";
	    else
		alig = "Grey";

	    if (cl = us[i]->query_coder_level()) {
		ctotal ++;
		title = (string) us[i]->query_pretitle();
		if (title == "Apprentice ") title = "Novice Coder ";

		if(title != "Apprentice " && title != "Novice Coder " &&
		  title != "Coder " && title != "Senior Coder " &&
		  title != "Pseudo-Admin " && title != "Co-Admin " &&
		  title != "Admin ")
		    title = "(void) ";
	    } else {
		ptotal++;
		title = sprintf("%s Mage", alig);

		if(MAGE_LIST_D->is_council(us[i]->query_real_name()))
		    title = "Councillor";
	    }

	    if(MAGE_LIST_D->query_outcast(us[i]->query_real_name()) == 1)
		title = "Outcast";
	    name = capitalize((string) us[i]->query_real_name() );
	    his_level = us[i]->query_level();

	    if (us[i]->query_invis()) {
		if (who->query_level() >= CAN_SEE_INVIS || who == us[i]) {
		    if (us[i]->query_invis() >= IL_TOTALLY_INVISIBLE)
			name = sprintf("<%s>", name);
		    else name = sprintf("(%s)", name);
		} else {
		    name = 0;
		    if (cl)
			ctotal --;
		    else ptotal --;
		}
	    }
	    if ((his_level <= my_level &&
		!environment(us[i])->query(ROOM_PREVENT_WHERE)) &&
	      !environment(us[i])->query(ROOM_ISOLATED) ||
	      IS_MAGE_GUILD_CODER(who->query_real_name()) )
		place = environment(us[i])->query_short();

	    if (sizeof(place) > 35) place = place[0..34];

	    {  // Stripataan moniriviset yksirivisiksi.
		j = strstr((string)place, "\n");
		if( j >= 0 )
		    place = place[0..j-1];
	    }

	    if(!interactive(us[i])) {
		if(cl) ctotal --;
		else ptotal --;
		if(interactive_info(us[i], II_IDLE) > 300) {
		    itotal --;
		    if (itotal < 0) itotal = 0;
		}
		name = 0;
	    }

	    if(name) {
		name += " ";
		if (cl) {
		    text2 += sprintf("  %s [%|14s] %-14'.'s %-34s %-6s\n",
		      ((s & CHANNEL_BAN) ? "B" :
			(s & CHANNEL_LD)  ? "L" :
			(s & CHANNEL_OFF) ? "-" : "+"),
		      title, name, (place ? place : ""), idle);
		}
		else {
		    text += sprintf("  %s [%|14s] %-14'.'s %-34s %-6s\n",
		      ((s & CHANNEL_BAN) ? "B" :
			(s & CHANNEL_LD)  ? "L" :
			(s & CHANNEL_OFF) ? "-" : "+"),
		      title, name, (place ? place : " "), idle);
		}
	    }
	}
    }

    text += sprintf("%|78s\n%|78s", MAGE_CHANNEL_HEADER,
      sprintf("Total of %d coder%s and %d mage%s visible to you,\
 %d idle.", ctotal, (ctotal != 1 ? "s" : ""), ptotal,
	(ptotal != 1 ? "s" : ""), itotal)
    );
    if (sizeof(text2) >= 10)
	text2 += sprintf("%|78s\n", MAGE_CHANNEL_HEADER);
    who->tell_me(text3+text2+text);
}

// .. #ifdef GN_MAGE
#endif


#ifdef GN_NECROMANCER
// Necromancers joining in too.

nomask status
necro_check(object ob)
{

    if (!ob) return 0;

    return (ob
      && ((string)ob->query_guild() == GN_NECROMANCER)
      || ob->query_coder_level());
}

nomask string
necro_format(mixed ob, string msg, int e)
{
    if (objectp(ob)) {
	if( msg[0] != ':')
	    return "[Guild] "+capitalize((string)ob->query_real_name()) + msg;
	else
	    return capitalize((string)ob->query_real_name()) + " [Guild]" + msg;
    }
    if (stringp(ob)) return ob + " [Guild] "+ msg;
    else return "[Guild]" + msg;
}
// ... #ifdef GN_NECROMANCER
#endif


#ifdef HAVE_PRIEST_GUILD
// PRIESTS

#include "/guilds/priests/include/channel.h"

nomask status
priest_check(object ob)
{
    if (!ob) return 0;

    return (ob
      && ((string)ob->query_guild() == GN_PRIEST)
      || ob->query_coder_level());
}

nomask string
priest_format(mixed ob, string msg, int e)
{
    if (msg[0] == ':') msg = msg[1..<1];

    if (objectp(ob)) {
	if (!e)
	    return "+" + capitalize((string)ob->query_real_name()) + "+" + msg;
	return "+Priest+ " + capitalize((string)ob->query_real_name()) + msg;
    }
    if(stringp(ob)) {
	if(!e) return "+" + ob + "+" + msg;
	return "+Priest+ " + ob + msg;
    }
    return "+Priest+ " + msg;
}


nomask string *
priest_list(object me)
{
    string *list, tmp, *temp_strs, locat_s, idle_s;
    string order_s;
    object *who_member, *ppl, cur;
    object room, sym;
    int i, s, t, num, il, me_coder, me_level;
    status isolated;

    if(!me || !environment(me))
	return ({});

    isolated = (status)environment(me)->query(ROOM_ISOLATED);

    if(isolated) ppl = ({ me });
    else ppl = sort_array(ch_ppl["priest"], #'sort_em);

    me_coder = (int)me->query_coder_level();
    me_level = (int)me->query_level();

    num = 0;

    list = ({
      sprintf("%|77s", "[The Holy Order of Boccob]"), PRIEST_SL1,
      sprintf("  %-12s %-12s %-6s %-5s %-36s", "Rank", "Name", 
	"Deity", "Idle", "Location"), PRIEST_SL2
    });

    for (i=0; i<sizeof(ppl); i++) {
	if(!(cur = ppl[i])) continue;
	il = cur->query_invis();

	// Players in isolated rooms and "deep invis" coders are not shown
	// to non-coders:
	if((!me_coder) && (environment(cur)->query(ROOM_ISOLATED)
	    || (il >= IL_TOTALLY_INVISIBLE)))
	    continue;

	s = cur->query_channel_status("priest");

	// Don't show non-priests on channel to non-coders, and if they
	// aren't on the channel don't show them at all.
	if(cur->query_guild() != GN_PRIEST && (s & CHANNEL_OFF || !me_coder))
	    continue;

	if(s & CHANNEL_LD)
	    locat_s = "(Linkdead)";
	else if(interactive_info(cur, II_EDITING))
	    locat_s = "(Editing)";
	else if(cur->query_testplayer())
	    locat_s = "(test character)";
	else if(me_coder ||
	  (cur->query_level()<=me_level && !cur->query_coder_level())) {
	    room = environment(cur);
	    if(!(locat_s = room->query_short()))
		locat_s = "";
	    else if(room->query_virtual_room()) {
		temp_strs = explode(locat_s, "\n");
		locat_s = temp_strs[sizeof(temp_strs)-1];
	    }
	    if(sizeof(locat_s)>36) locat_s = locat_s[0..32] + "...";
	}
	else locat_s = "";

	if(interactive(cur)) {
	    t = interactive_info(cur, II_IDLE);
	    if(t<60) idle_s = "";
	    else if(t/3600)
		idle_s = sprintf("%d h", t/3600);
	    else idle_s = sprintf("%d m", t/60);
	}
	else idle_s = "";

	if(interactive(cur) && cur->query_guild() == GN_PRIEST &&
	  (sym = (object) cur->query_guild_object())) {
	    order_s = sym->query_order_string();
	    num++;
	}
	else order_s = "";

	tmp = sprintf("%-1s %-12s %-12s %-6s %-5s %-36s",
	  ((s & CHANNEL_BAN) ? "B" :
	    (s & CHANNEL_OFF) ? "-" : "+"),
	  OFFICER_D->query_title(cur),
	  capitalize(cur->query_real_name()),
	  order_s,
	  idle_s, locat_s);

	list += ({ tmp });
    }

    list += ({ PRIEST_SL2,
      sprintf("Total of %d priests out of %d players.", num, sizeof(users())),
      PRIEST_SL1,
    });

    return list;
}

// ...#ifdef HAVE_PRIEST_GUILD
#endif

// And the good old JYU channel is back. :)

nomask status
jyu_check(object ob)
{
    string tmp;

    if (!ob) return 0;

#define JYU_EXTRA_LIST ({ "eldaron" })

    return (ob
      && (object_info(ob, OI_ONCE_INTERACTIVE) && (tmp = interactive_info(ob, II_IP_NAME)) &&
	(strstr(tmp,"jyu.fi") != -1))
      || ob->query_coder_level()
      || ( member( JYU_EXTRA_LIST, ob->query_real_name() )>= 0 )
    );
}

nomask string
jyu_format(mixed ob, string msg, int e)
{
    if (objectp(ob))
	return "-=JYU=- " + capitalize((string)ob->query_real_name()) + msg;
    if (stringp(ob)) return "-=JYU=- " + ob + msg;
    else return "-=JYU=-" + msg;
}

#ifdef	GN_HEALER

// Healer channel...

nomask status
healer_check(object ob)
{
    if (!ob) return 0;

    return (ob
      && ((string)ob->query_guild() == GN_HEALER)
      || ob->query_coder_level());
}

nomask string
healer_format(mixed ob, string msg, int e)
{
    if (msg[0] == ':') msg = msg[1..<1];

    if (objectp(ob))
    {
	if (!e) return "@H-" + capitalize((string)ob->query_real_name()) + ":" + msg;
	else return "@H-" + capitalize((string)ob->query_real_name()) +
	    msg;
    }
    else if (stringp(ob))
    {
	if (!e) return "@H-" + ob + ":" + msg;
	else return "@H-" + ob + msg;
    }
    else return "@H-" + msg;
}

nomask string *
healer_list(object me)
{


    string *who_list, tmp, pretitle, name, chanstat, loc;
    object *who_member, *who, hmark, e;
    int i, z, add_list, num, il, me_coder, me_level;
    status isolated;

    //  who = ch_ppl["healer"];
    who = sort_array(ch_ppl["healer"], "sort_em", this_object());
    who_member = ({});

    if (me && environment(me))
	isolated = (status)environment(me)->query(ROOM_ISOLATED);
    else isolated = 0;

    if (!isolated)
    {
	for(i = 0; i < sizeof(who); i++)
	{
	    if (who[i])
		who_member += ({ who[i] });
	}
    }
    else who_member = ({ me });

    who_list = ({});

    me_coder = (int)me->query_coder_level();
    me_level = (int)me->query_level();

    num = 0;

    for (i = 0; i < sizeof(who_member); i++)
    {
	il = (int)who_member[i]->query_invis();
	hmark = present("healer_mark", who_member[i]);

	// Don't show non-healers on channel to non-coders.
	if (!hmark && !me_coder) continue;

	if (me_coder || (!environment(who_member[i])->query(ROOM_ISOLATED)
	    && (!il || (il < IL_TOTALLY_INVISIBLE && me_level > 21))))
	{
	    // Guild pretitle
	    if(hmark)
	    {
		pretitle = hmark->query_htitle();
		num++;
	    }
	    else pretitle = "";
	    //         else      pretitle = who_member[i]->query_pretitle();

	    // Format name
	    name = capitalize((string) who_member[i]->query_real_name());
	    if(me_coder){
		if (il >= IL_TOTALLY_INVISIBLE) name = "<"+name+">";
		else if (il) name = "("+name+")";
	    }
	    else {
		if (who_member[i]->query_coder_level() && il) continue;
		else if (il) name = "("+name+")";
	    }

	    // Channel Status
	    z = who_member[i]->query_channel_status("healer");
	    if ((z & CHANNEL_LD)) chanstat = "LinkDead";
	    else if(interactive_info(who_member[i], II_IDLE) > 300) chanstat = "Idle";
	    else if ((z & CHANNEL_OFF)) chanstat = "Off";
	    else if ((z & CHANNEL_BAN)) chanstat = "BANNED";
	    else chanstat = "On";

	    // Location
	    if (me_coder<0) loc = "(test character)";
	    else
	    if (me_coder || me == who_member[i] ||
	      (int)who_member[i]->query_level() < me_level) 
	    { 
		loc = (string) environment(who_member[i])->short();
		if (!loc) loc = "Unknown place";
	    }
	    else loc = " ";
	    if (sizeof(loc) > 26) loc = loc[0..23] + "...";

	    tmp = sprintf("  %18s %-14s %-8s  %s", pretitle, name, chanstat, loc);
	    who_list += ({ tmp });
	}
    }

    who_list += ({ "Total of " + num + " people on channel." });

    return who_list;
}

// ... #ifdef GN_HEALER
#endif


#ifdef GN_FIGHTER

// Fighter's bring up the end and join the channel system!
#define FLINE1 " ___________________________________________________________________________\n/\\__________________________________________________________________________\\\n\\                                                                          \\"
#define FLINE2 "\\/__________________________________________________________________________/"
nomask status
fighter_check(object ob)
{
    if (!ob) return 0;

    return (ob
      && ((string)ob->query_guild() == GN_FIGHTER)
      || ob->query_coder_level());
}

nomask string
fighter_format(mixed ob, string msg, int e)
{
    if (msg[0] == ':') msg = msg[1..<1];

    if (objectp(ob))
    {
	if (!e) return "=" + capitalize((string)ob->query_real_name()) + "=" + msg;
	else return "==" + capitalize((string)ob->query_real_name()) +
	    msg;
    }
    else if (stringp(ob))
    {
	if (!e) return "=" + ob + "=" + msg;
	else return "==" + ob + msg;
    }
    else return "==" + msg;
}

nomask string *
fighter_list(object me)
{

    string *who_list, tmp, pretitle, posttitle, name, chanstat, loc;
    object *who_member, *who, fmark, e;
    int i, z, add_list, num, il, me_coder, me_level;
    status isolated;

    who = sort_array(ch_ppl["fighter"], "sort_em", this_object());
    who_member = ({});

    if (me && environment(me))
	isolated = (status)environment(me)->query(ROOM_ISOLATED);
    else isolated = 0;

    if (!isolated)
    {
	for(i = 0; i < sizeof(who); i++)
	{
	    if (who[i])
		who_member += ({ who[i] });
	}
    }
    else who_member = ({ me });

    who_list =  ({ FLINE1 });
    me_coder = (int)me->query_coder_level();
    me_level = (int)me->query_level();

    num = 0;

    for (i = 0; i < sizeof(who_member); i++)
    {
	il = (int)who_member[i]->query_invis();
	fmark = present("fsymbol", who_member[i]);

	// Don't show non-fighters on channel to non-coders.
	if (!fmark && !me_coder) continue;

	if (me_coder || (!environment(who_member[i])->query(ROOM_ISOLATED)
	    && (!il || (il < IL_TOTALLY_INVISIBLE && me_level > 21))))
	{
	    if(fmark){ 
		pretitle = fmark->query_pretitle();
		num++;
	    }
	    else pretitle = "";

	    // Format name
	    name = capitalize((string) who_member[i]->query_real_name());
	    if(me_coder){
		if (il >= IL_TOTALLY_INVISIBLE) name = "<"+name+">";
		else if (il) name = "("+name+")";
	    }
	    else {
		if (who_member[i]->query_coder_level() && il) continue;
		else if (il) name = "("+name+")";
	    }

	    if(fmark)
		name += " the "+fmark->query_fighter_title();

	    // Channel Status
	    z = who_member[i]->query_channel_status("fighter");
	    if ((z & CHANNEL_LD)) chanstat = "LD";
	    else if(interactive_info(who_member[i], II_IDLE) > 300) chanstat = "Idle";
	    else if ((z & CHANNEL_OFF)) chanstat = "Off";
	    else if ((z & CHANNEL_BAN)) chanstat = "BANNED";
	    else chanstat = "On";

	    if (me_coder<0) loc = "(test character)";
	    else
	    if (me_coder || me == who_member[i] ||
	      (int)who_member[i]->query_level() < me_level) 
		loc = (string) environment(who_member[i])->short();
	    else loc = " ";
	    if (sizeof(loc) > 17) loc = loc[0..13] + "...";

	    tmp = sprintf(" | %14s %-31s %-6s %-17s |", pretitle, name, chanstat, loc);
	    who_list += ({ tmp });
	}
    }
    tmp = "Total of " + num + " Fighter"+((num>1) ? "s": "")+" visible to you.";
    tmp = sprintf(" |%73s|\n_\\ %-72s \\", " ",tmp);

    who_list += ({ tmp });
    who_list += ({ FLINE2 });

    return who_list;
}

// ...#ifdef GN_FIGHTER
#endif

nomask status gunn_check(object ob)
{
    return (ob && ((int)ob->query_clan("gunn") || (int)ob->query_coder_level()));
}

nomask string gunn_format(mixed ob, string msg, int e)
{
    if (objectp(ob))
	return "[Gunn] " + capitalize((string)ob->query_real_name()) + msg;
    if (stringp(ob)) return "[Gunn] " + ob + msg;
    else return "[Gunn] " + msg;
}

nomask status rider_check(object ob)
{
    return (ob && ((int)ob->query_clan("wind riders") || (int)ob->query_coder_level()));
}

nomask string rider_format(mixed ob, string msg, int e)
{
    if (objectp(ob))
	return "[WindRiders] " + capitalize((string)ob->query_real_name()) + msg;
    if (stringp(ob)) return "[WindRiders] " + ob + msg;
    else return "[WindRiders] " + msg;
}

nomask status duren_check(object ob)
{
    return (ob && ((int)ob->query_clan("o'duren") || (int)ob->query_coder_level()));
}

nomask string duren_format(mixed ob, string msg, int e)
{
    if (objectp(ob))
	return "[O'Duren] " + capitalize((string)ob->query_real_name()) + msg;
    if (stringp(ob)) return "[O'Duren] " + ob + msg;
    else return "[O'Duren] " + msg;
}

nomask status stealer_check(object ob)
{
    return (ob && ((int)ob->query_clan("soul stealers") || (int)ob->query_coder_level()));
}

nomask string stealer_format(mixed ob, string msg, int e)
{
    if (objectp(ob))
	return "[Soul Stealers] " + capitalize((string)ob->query_real_name()) + msg;
    if (stringp(ob)) return "[Soul Stealers] " + ob + msg;
    else return "[Soul Stealers] " + msg;
}

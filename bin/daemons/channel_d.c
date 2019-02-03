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
#include <daemons.h>
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
#define CLOG_DIR	"/data/d/channel_d/"
#define MAX_CLOG	50000

/* To enable/disable the support for certain stick-specific
 * guilds uncomment/comment these defines:
 */

#define		HAVE_PRIEST_GUILD
#define	        HAVE_THIEF_GUILD

nomask void
create()
{
    channels = ([
      "broadcast":	({ TELL_TYPE_BROADCAST, 0 }),
      "info":		({ TELL_TYPE_INFO, 0 }),
      "mort":		({ TELL_TYPE_MORT, 0 }),
      "newbie":		({ TELL_TYPE_NEWBIE, 0 }),
      "ooc":		({ TELL_TYPE_OOC, 0 }),
      "lord":		({ TELL_TYPE_LORD, 0 }),
      "water":		({ TELL_TYPE_FACTION, 0 }),
      "earth":		({ TELL_TYPE_FACTION, 0 }),
      "wind":		({ TELL_TYPE_FACTION, 0 }),
      "fire":		({ TELL_TYPE_FACTION, 0 }),
      "wiz":		({ TELL_TYPE_WIZ, 0 }),
      "arch":		({ TELL_TYPE_ARCH, 0 }),
      "admin":		({ TELL_TYPE_ADMIN, 0 }),
      "thief":		({ TELL_TYPE_GUILD, "thief_list" }),
      "thiefdmn":	({ TELL_TYPE_GUILD, "thiefdmn_list" }),
      "thiefgmc":	({ TELL_TYPE_GUILD, "thiefgmc_list" }),
      "priest":		({ TELL_TYPE_GUILD, "priest_list" }),
    ]);

    ch_ppl = ([
      "broadcast":	({ }),
      "info":		({ }),
      "mort":		({ }),
      "newbie":		({ }),
      "ooc":		({ }),
      "lord":		({ }),
      "water":		({ }),
      "earth":		({ }),
      "wind":		({ }),
      "fire":		({ }),
      "wiz":		({ }),
      "arch":		({ }),
      "admin":		({ }),
      "thief":		({ }),
      "thiefdmn":	({ }),
      "thiefgmc":	({ }),
      "priest":		({ }),
    ]);

    ch_admin = ([
      //"lord":   ({ "garathan", }),
    ]);

    // These strings in attempts to fake messages.
    forbid_str = ({
      " says: ",
      " shouts: ",
      " yells: ",
      " whispers: ",
      " asks: ",
      " bellows: ",
    });

    // Channels on which we don't want to see the Linkdeads
    dont_show_ld = ({
      //"mage"
    });

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

    foreach (object player : ch_ppl[ch]) {
	if (player->query(LIV_IS_PLAYER) && player->query_env("gmcp")) {
            TELOPT_D->send_comm_channel_players(player);
	}
    }

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

    foreach (object player : ch_ppl[ch]) {
	if (player->query(LIV_IS_PLAYER) && player->query_env("gmcp")) {
            TELOPT_D->send_comm_channel_players(player);
	}
    }

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
    string tmp, cn, ob_name;
    int i, e, indent;
    object who;

    if (!member(channels, ch)) return 0;
    if (!stringp(msg)) return 0;

    if (objectp(ob)
      && environment(ob)->query(ROOM_ISOLATED)
      && !ob->query_coder_level()) {
	ob->tell_me("No one heard you.");
	return 1;
    } else cn = capitalize(ch);

    if (!pointerp(ch_ppl[ch])) return 0;

    ch_ppl[ch] -= ({ 0 }); // Purge lost members
    e = 0;

    // Emote?
    if (msg[0] == ':') {
	indent = sizeof(ch) + 3;
	msg = msg[1..<1];
	if (msg[0] != ' ') msg = " " + msg;
	e = 1;
    } else { // Normal message, add ": "
	if (stringp(ob))
	    indent = sizeof(ob) + 3;
	else
	    indent = sizeof(ob->query_real_name()) + 3;
	if (ch == "lord") msg = " " + msg;
	else {
	    msg = ": " + msg;
	    indent += sizeof(ch) + 2;
	}
    }

    if (objectp(ob)) {
        ob_name = (string)ob->query_real_name();
    } else if (stringp(ob)) {
	ob_name = ob;
    } else ob_name = "";

    tmp = call_other(this_object(), ch + "_format", ob, msg, e);

    //tmp = b_string(tmp, 72, indent);

    for (i = sizeof(ch_ppl[ch]) - 1; i >= 0; i--) {
	if (objectp(ch_ppl[ch][i]) &&
	  ch_ppl[ch][i]->is_on_channel(ch) &&
	  !((int)ch_ppl[ch][i]->query_channel_status(ch) & CHANNEL_OFF))
	    ch_ppl[ch][i]->tell_me(tmp, 0, channels[ch][0], 0, 0, 0);

        // Send to GMCP Clients
        if (ch_ppl[ch][i]->query(LIV_IS_PLAYER) && ch_ppl[ch][i]->query_env("gmcp"))
            TELOPT_D->send_comm_channel_text(ch_ppl[ch][i], ch, ob_name, tmp);
    }

    log_channel(ch, tmp);

    return 1;
}

//
// Who can be on each channel? Separate function for each channel
// is probably required.
//

nomask status
broadcast_check(object ob)
{
    return 1;
}

nomask string
broadcast_format(mixed ob, string msg, int e)
{
    if (objectp(ob))
	return "[Broadcast] " + capitalize((string)ob->query_real_name()) + msg;
    if (stringp(ob)) return "[Broadcast] " + ob + msg;
    else return "[Broadcast]" + msg;
}

nomask status
info_check(object ob)
{
    return 1;
}

nomask string
info_format(mixed ob, string msg, int e)
{
    if (objectp(ob))
	return "[Info] " + capitalize((string)ob->query_real_name()) + msg;
    if (stringp(ob)) return "[Info] " + ob + msg;
    else return "[Info]" + msg;
}

nomask status
mort_check(object ob)
{
    return 1;
}

nomask string
mort_format(mixed ob, string msg, int e)
{
    if (objectp(ob))
	return "[Mort] " + capitalize((string)ob->query_real_name()) + msg;
    if (stringp(ob)) return "[Mort] " + ob + msg;
    else return "[Mort]" + msg;
}

nomask status
newbie_check(object ob)
{
    return 1;
}

nomask string
newbie_format(mixed ob, string msg, int e)
{
    if (objectp(ob))
	return "[Newbie] " + capitalize((string)ob->query_real_name()) + msg;
    if (stringp(ob)) return "[Newbie] " + ob + msg;
    else return "[Newbie]" + msg;
}

nomask status
ooc_check(object ob)
{
    return (ob && (ob->query_coder_level()
	|| (ob->query_age() > (24 * 60 * 60) && ob->query_level() >= 5)));
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
	return "[Wind] " + capitalize(ob->query_real_name()) + msg;
    if (stringp(ob)) return "[Wind] " + ob + msg;
    else return "[Wind] " + msg;
}

nomask status water_check(object ob)
{
    return (ob && (ob->query_kingdom("water") || (int)ob->query_coder_level()));
}

nomask string water_format(mixed ob, string msg, int e)
{
    if (objectp(ob))
	return "[Water] " + capitalize(ob->query_real_name()) + msg;
    if (stringp(ob)) return "[Water] " + ob + msg;
    else return "[Water] " + msg;
}

nomask status
lord_check(object ob)
{
    return (ob && (ob->query_coder_level() || ob->query_level() >= 25));
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
nomask status
thief_check(object ob)
{
    if (!ob) return 0;

    return (ob && (ob->query_guild() == GN_THIEF || ob->query_coder_level()));
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

#define	WHITE		400
#define	BLACK		-400

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

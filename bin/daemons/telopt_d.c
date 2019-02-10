/*
** /bin/daemons/telopt_d.c
**
** Author: Tamarindo
**   Date: August 2018
**
** Purpose:
**
**	Included inside of the login and player objects is a handler
**	(specifically /basic/player/telopt.c) that receives and sends
**	Generic Mud Communications Protocol or GMCP messages which are
**	formatted as JSON. To reduce GMCP clutter inside of the player
**	object, this Telopt Daemon receives events from across the
**	mudlib and prepares the data intended to be sent to any player
**	object that is interested to send or receive GMCP messages.
**
**	Prior to making a call to the Telopt Daemon, one should test
**	first that the object intended to receive the message is a
**	player object and wishes to receive it. This is an example:
**
**	if (ob->query(LIV_IS_PLAYER) && ob->query_env("gmcp")) {
**	    TELOPT_D->send_comm_channel_text(ob, "Tell", "Tamarindo", "Ni!");
**	}
**
** References:
**
**	Mudlet: https://wiki.mudlet.org/w/Manual:Supported_Protocols
**	Mudlet: https://wiki.mudlet.org/w/Listing_Your_MUD
**	Iron Realms: http://nexus.ironrealms.com/GMCP
**	Aardwolf: http://www.aardwolf.com/wiki/index.php/Clients/GMCP
**	Mudlet: https://github.com/Mudlet/Mudlet/pull/1716 (for Discord)
**	Discord: https://discordapp.com/developers/docs/rich-presence/best-practices#tips
**
** This Telopt Daemon relates in various ways to files like these:
**
**	/basic/player/telopt.c
**	/basic/player/command_driver.c
**	/basic/player/tell_me.c
**	/basic/player/clothing.c
**	/basic/player/player_cmds.c
**	/basic/living/resistance.c
**	/basic/living/conditions.c
**	/basic/living/moving.c
**	/bin/pub/_drop.c
**	/bin/pub/_put.c
**	/bin/pub/_get.c
**	/bin/daemons/compass_d.c
**	/bin/daemons/channel_d.c
**	/include/gmcp_defs.h
**	/obj/container.c
**	/obj/living.c
**	/obj/npc.c
**	/obj/room.c
**	/obj/player.c
**	/secure/login.c
**	/secure/simul_efun/json.c
**
*/
#include "/sys/tls.h"

#include <armour_defs.h>
#include <daemons.h>
#include <conditions.h>
#include <gmcp_defs.h>
#include <invis_levels.h>
#include <player_defs.h>
#include <resistance.h>
#include <room_defs.h>
#include <map_environment_defs.h>
#include <map_area_defs.h>
#include <stats.h>
#include <std_lib>

static mapping conditions_descs = ([
  C_BLIND: ({"blind", "You can't see!"}),
  C_DEAF: ({"deaf", "You can't hear!"}),
  C_STUNNED: ({"stunned", "You can't move!"}),
  C_UNCONSCIOUS: ({"unconscious", "You are unconscious!"}),
  C_CONFUSED: ({"confused", "You are confused!"}),
  C_POISONED: ({"poisoned", "You are poisoned and sick!"}),
  C_DETECT_INVIS: ({"detect invis", "Your eyes are tingling!"}),
  C_SLOWED: ({"slowed", "The environment is racing around you!"}),
  C_HASTED: ({"hasted", "The environment crawls slowly around you!"}),
  C_HALLUCINATING: ({"hallucinating", "You are hallucinating!"}),
  C_PASSED_OUT: ({"passed out", "You are passed out!"}),
]);

static mapping resistances_descs = ([
  RESISTANCE(C_BLIND): ({"blind", "Blindness"}),
  RESISTANCE(C_DEAF): ({"deaf", "Deafness"}),
  RESISTANCE(C_STUNNED): ({"stunned", "Stunning"}),
  RESISTANCE(C_UNCONSCIOUS): ({"unconscious", "Unconsciousness"}),
  RESISTANCE(C_CONFUSED): ({"confused", "Confusion"}),
  RESISTANCE(C_POISONED): ({"poisoned", "Poison"}),
  RESISTANCE(C_DETECT_INVIS): ({"detect invis", "Detect Invis"}),
  RESISTANCE(C_SLOWED): ({"slowed", "Slowing"}),
  RESISTANCE(C_HASTED): ({"hasted", "Hastening"}),
  RESISTANCE(C_HALLUCINATING): ({"hallucinating", "Hallucinating"}),
  RESISTANCE(C_PASSED_OUT): ({"passed out", "Passing Out"}),
  RT_EDGED: ({"edged", "Edged"}),
  RT_POINTED: ({"pointed", "Pointed"}),
  RT_BLUNT: ({"blunt", "Blunt"}),
  RT_IMPACT: ({"impact", "Impact"}),
  RT_BITE: ({"bite", "Bite"}),
  RT_PHYSICAL: ({"physical", "Physical"}),
  RT_POISON: ({"poison", "Poison"}),
  RT_EARTH: ({"earth", "Earth"}),
  RT_WATER: ({"water", "Water"}),
  RT_FIRE: ({"fire", "Fire"}),
  RT_AIR: ({"air", "Air"}),
  RT_COLD: ({"cold", "Cold"}),
  RT_ACID: ({"acid", "Acid"}),
  RT_ELECTRICITY: ({"electricity", "Electricity"}),
  RT_TIME: ({"time", "Time"}),
  RT_NETHER: ({"nether", "Nether"}),
  RT_PLASMA: ({"plasma", "Plasma"}),
  RT_DARK: ({"dark", "Dark"}),
  RT_NEGATIVE: ({"negative", "Negative"}),
  RT_LIGHT: ({"light", "Light"}),
  RT_POSITIVE: ({"positive", "Positive"}),
  RT_MAGICAL: ({"magic", "Magic"}),
]);

// Frequently used helper functions first...

/*
**   Function: tell_me_items_add_inv
**
**    Returns: int
**
** Parameters: object me, player object to send the information
**             object added, the object being added to the player's inventory
**
**    Purpose: Inform player's client that an object was added to their
**             inventory if the client understands GMCP.
*/
nomask int
tell_me_items_add_inv(object me, object added) {
    if (me->query(LIV_IS_PLAYER) && me->query_env("gmcp")) {
	TELOPT_D->send_char_items_add(me, "inv", added);
    }

    return 1;
}

/*
**   Function: tell_me_items_add_container
**
**    Returns: int
**
** Parameters: object me, player object to send the information
**             object container, container object receiving an added item
**             object added, the object being added to the container
**
**    Purpose: Inform player's client that an object was added to a
**             container referenced by a hash if the client understands GMCP.
*/
nomask int
tell_me_items_add_container(object me, object container, object added) {
    if (me->query(LIV_IS_PLAYER) && me->query_env("gmcp")) {
	TELOPT_D->send_char_items_add(me, hash(TLS_HASH_SHA1, object_name(container)), added);
    }

    return 1;
}

/*
**   Function: tell_me_items_list_container
**
**    Returns: int
**
** Parameters: object me, player object to send the information
**             object container, the container object being queried to list its contents
**
**    Purpose: Inform player's client of the contents within a container
**             if the client understands GMCP.
*/
nomask int
tell_me_items_list_container(object me, object container) {
    if (me->query(LIV_IS_PLAYER) && me->query_env("gmcp")) {
	TELOPT_D->send_char_items_list(me, hash(TLS_HASH_SHA1, object_name(container)), container);
    }

    return 1;
}

/*
**   Function: tell_me_items_list_inv
**
**    Returns: int
**
** Parameters: object me, player object to send the information
**
**    Purpose: Inform player's client of the contents of the player's inventory
**             if the client understands GMCP.
*/
nomask int
tell_me_items_list_inv(object me) {
    if (me->query(LIV_IS_PLAYER) && me->query_env("gmcp")) {
	TELOPT_D->send_char_items_list(me, "inv");
    }

    return 1;
}

/*
**   Function: tell_me_items_list_room
**
**    Returns: int
**
** Parameters: object me, player object to send the information
**
**    Purpose: Inform player's client of the contents of the room they are
**             located if the client understands GMCP.
*/
nomask int
tell_me_items_list_room(object me) {
    if (me->query(LIV_IS_PLAYER) && me->query_env("gmcp")) {
	TELOPT_D->send_char_items_list(me, "room");
    }

    return 1;
}

/*
**   Function: tell_me_items_remove_inv
**
**    Returns: int
**
** Parameters: object me, player object to send the information
**             object removed, the object being removed from the player's inventory
**
**    Purpose: Inform player's client that an object was removed from their
**             inventory if the client understands GMCP.
*/
nomask int
tell_me_items_remove_inv(object me, object removed){
    if (me->query(LIV_IS_PLAYER) && me->query_env("gmcp")) {
	TELOPT_D->send_char_items_remove(me, "inv", removed);
    }

    return 1;
}

/*
**   Function: tell_me_items_remove_container
**
**    Returns: int
**
** Parameters: object me, player object to send the information
**             object container, container object losing the removed item
**             object removed, the object being removed from the container
**
**    Purpose: Inform player's client that an object was removed from a container
**             if the client understands GMCP.
*/
nomask int
tell_me_items_remove_container(object me, object container, object removed) {
    if (me->query(LIV_IS_PLAYER) && me->query_env("gmcp")) {
	TELOPT_D->send_char_items_remove(me, hash(TLS_HASH_SHA1, object_name(container)), removed);
    }

    return 1;
}

/*
**   Function: tell_me_items_update_inv
**
**    Returns: int
**
** Parameters: object me, player object to send the information
**             object added, the object being updated in the player's inventory
**
**    Purpose: Inform player's client that an object was updated in their
**             inventory if the client understands GMCP.
*/
nomask int
tell_me_items_update_inv(object me, object updated) {
    if (me->query(LIV_IS_PLAYER) && me->query_env("gmcp")) {
	TELOPT_D->send_char_items_update(me, updated);
    }

    return 1;
}

/*
**   Function: tell_here_items_add
**
**    Returns: int
**
** Parameters: object env, room object to identify players to send the information
**             object added, the object being added to the room's inventory
**
**    Purpose: Inform player client(s) that an object was added to their room's
**             inventory if the client understands GMCP.
*/
nomask int
tell_here_items_add(object env, object added) {
    foreach (object you : filter(all_inventory(env), (: $1->query(LIV_IS_PLAYER) :))) {
	if (you->query_env("gmcp")) {
	    TELOPT_D->send_char_items_add(you, "room", added);
	}
    }

    return 1;
}

/*
**   Function: tell_here_items_list
**
**    Returns: int
**
** Parameters: object env, room object to identify players to send the information
**
**    Purpose: Inform player client(s) of a refresh to their room's
**             inventory if the client understands GMCP.
*/
nomask int
tell_here_items_list(object env) {
    foreach (object you : filter(all_inventory(env), (: $1->query(LIV_IS_PLAYER) :))) {
	if (you->query_env("gmcp")) {
	    TELOPT_D->send_char_items_list(you, "room");
	}
    }

    return 1;
}

/*
**   Function: tell_here_items_remove
**
**    Returns: int
**
** Parameters: object env, room object to identify players to send the information
**             object removed, the object being removed from the room's inventory
**
**    Purpose: Inform player client(s) that an object was removed from their room's
**             inventory if the client understands GMCP.
*/
nomask int
tell_here_items_remove(object env, object removed) {
    foreach (object you : filter(all_inventory(env), (: $1->query(LIV_IS_PLAYER) :))) {
	if (you->query_env("gmcp")) {
	    TELOPT_D->send_char_items_remove(you, "room", removed);
	}
    }

    return 1;
}

// End of helper functions... now on to the real GMCP processing!

/*
**   Function: send_client_gui
**
**    Returns: void
**
** Parameters: object who, player object to send the information
**
**    Purpose: For players that receive GMCP, this function will send
**             a message containing the current version and URL of where
**             to find the current version of the gui (Graphical User
**             Interface) so that if the client does not have it or if
**             they do not have the current version it will be downloaded
**             from the game's website.  Although the intentions were
**             good to place this function here, there is a check inside
**             of the Telopt Daemon that forces the information below
**             to be sent as raw instead of as a string because that's the
**             only way it works in the client.
**
**             When this is working you will see something like the
**             following inside of Mudlet:
**
**             <Server offers downloadable GUI (url='') (package='')>
*/
nomask void
send_client_gui(object who) {
    who->gmcp_message(GMCP_PKG_CLIENT_GUI, sprintf("%s\n%s",
        GMCP_KEY_CLIENT_GUI_VERSION, GMCP_VALUE_CLIENT_GUI_URL));
}

/*
**   Function: send_client_map
**
**    Returns: void
**
** Parameters: object who, player object to send the information
**
**    Purpose: For players that recieve GMCP, this function will tell
**             clients like Mudlet that our game supports an MMP (Mud
**             Mapping Protocol map. In clients like Mudlet, this map
**             is downloadable from settings as well as when the user
**             opens the mapper for the first time.
*/
nomask void
send_client_map(object who) {
    mapping client_map = ([ GMCP_KEY_CLIENT_MAP_URL: GMCP_VALUE_CLIENT_MAP_URL ]);

    who->gmcp_message(GMCP_PKG_CLIENT_MAP, client_map);
}

/*
** Function: send_core_ping
**
** Returns: void
**
** Parameters:
**
**	object who
**		The player object we intended to send the information.
**
** Purpose:
**
**	send_core_ping is sent in response to a ping received from the
**	client which typically includes the average ping time from previous
**	requests. That information is cached within the player object and
**	may be of use to someone in the future?
**
** In the Mudlet client, type lua sendGMCP("Core.Ping 120") which will
** send a value to the MUD and the MUD will send back a Core.Ping with
** no value which you could see by typing lua gmcp.Core.Ping and you should
** see and empty string like this: "". It's likely there are some publicly
** available scripts that make some good use of this on the client side.
**
** If this is the first time your seeing GMCP, the Core.Ping is know as the
** "package" and the "120" is the value. If the player object is set to
** handle GMCP it will receive the sendGMCP() message, parse it, then
** cache it in a static mapping inside of the player object. In this case,
** the player object will also call this function in the Telopt Daemon
** which will instruct the player object to send some very simple JSON
** to the client containing only "Core.Ping" per the GMCP specification.
**
** This is a nice example of sending as close as we could manage to the
** value "null" to the client.
*/
nomask void
send_core_ping(object who) {
    who->gmcp_message(GMCP_PKG_CORE_PING, 0);
}

/*
** Function: send_core_goodbye
**
** Returns: void
**
** Parameters:
**
**	object who
**		The player object we intended to send the information.
**
** Purpose:
**
**	send_core_goodbye is sent immediately before terminating a
**	connection and may explain the reason.
**
** In the Mudlet client, type lua gmcp.Core.Goodbye and if we remembered to
** implement this with the quit, shutdown and failed login functionality
** you may see something like this:
**  "Goodbye, adventurer"
**
** This is a nice example of sending a string object to the client.
*/
nomask void
send_core_goodbye(object who) {
    who->gmcp_message(GMCP_PKG_CORE_GOODBYE, "Goodbye, adventurer");
}

/*
** Function: send_external_discord_info
**
** Returns: void
**
** Parameters:
**
**	object who
**		The player object we intended to send the information.
**
** Purpose:
**
**	See https://wiki.mudlet.org/w/Standards:Discord_GMCP#External.Discord.Info
*/
nomask void
send_external_discord_info(object who) {
    mapping external_discord_info = ([
	GMCP_KEY_EXTERNAL_DISCORD_INFO_INVITEURL: GMCP_VALUE_EXTERNAL_DISCORD_INFO_INVITEURL,
	GMCP_KEY_EXTERNAL_DISCORD_INFO_APPLICATIONID: GMCP_VALUE_EXTERNAL_DISCORD_INFO_APPLICATIONID,
      ]);

    who->gmcp_message(GMCP_PKG_EXTERNAL_DISCORD_INFO, external_discord_info);
}

/*
** Function: send_external_discord_status
**
** Returns: void
**
** Parameters:
**
**	object who
**		The player object we intended to send the information.
**
** Purpose:
**
**	See https://wiki.mudlet.org/w/Standards:Discord_GMCP#External.Discord.Status
*/
nomask void
send_external_discord_status(object who) {
    mapping external_discord_status = ([
	GMCP_KEY_EXTERNAL_DISCORD_STATUS_GAME: GMCP_VALUE_EXTERNAL_DISCORD_STATUS_GAME,
	GMCP_KEY_EXTERNAL_DISCORD_STATUS_STARTTIME: who->query(PLR_LOGIN_TIME),
      ]);

    who->gmcp_message(GMCP_PKG_EXTERNAL_DISCORD_STATUS, external_discord_status);
}

/*
** Function: send_char_name
**
** Returns: void
**
** Parameters:
**
**	object who
**		The player object we not only intended to send the information
**		to but also the player object we will query to determine the
**		name and full name.
**
** Purpose:
**
**	send_char_name is sent when the player object is being initialized
**	upon login to the mud.  It's possible we could get more creative with
**	the fullname to include more than the pretitle and name, like:
**	Admin Tamarindo, dwarven mage, citizen of Ered Mithrin.
**
** In the Mudlet client, type lua gmcp.Char.Name and you will see something like:
** {
**    name = "Tamarindo",
**    fullname = "Admin Tamarindo"
** }
**
** This is a nice example of sending a mapping (key / value pair) to the client
** and a good opportunity to show that we require the keys to always be strings
** so the JSON is appropriate.
*/
nomask void
send_char_name(object who) {
    string name = capitalize(who->query_real_name());
    string fullname = sprintf("%s%s", who->query_pretitle(), capitalize(who->query_real_name()));

    mapping char_name = ([
      GMCP_KEY_CHAR_NAME_NAME: name,
      GMCP_KEY_CHAR_NAME_FULLNAME: fullname,
    ]);

    who->gmcp_message(GMCP_PKG_CHAR_NAME, char_name);
}

/*
** Function: send_char_stats
**
** Returns: void
**
** Parameters:
**
**	object who
**		The player object we not only intended to send the information
**		to but also the player object we will query to determine the
**		current stats.
**
** Purpose:
**
**	send_char_stats is familiar to players of Aardwolf MUD and could be
**	used in combination with the information returned in send_char_maxstats
**	to monitor whether stats are drained or enhanced. The reason there are
**	two functions is 1) to match Aardwolf and client side scripts people may
**	have authored in the past and 2) to possibly reduce the amount of data
**	transferred as send_char_stats may be needed sent more often where the
**	send_char_max_stats function changes less.  It's worthwhile to note that
**	caching is used inside the player object so the mud will only send the
**	change in stats to the client when a change occurs or when the client
**	requests the information be refreshed.
**
** In the Mudlet client, type lua gmcp.Char.Stats and you will see something like:
** {
**    int = 1,
**    str = 1,
**    con = 1,
**    dex = 1
** }
**
** This is a good example to show that yes, we can send numeric values and not only
** string values.
*/
nomask void send_char_stats(object who) {
    mapping stats =  ([
      GMCP_KEY_CHAR_STATS_CON: who->query_stat(ST_CON),
      GMCP_KEY_CHAR_STATS_INT: who->query_stat(ST_INT),
      GMCP_KEY_CHAR_STATS_DEX: who->query_stat(ST_DEX),
      GMCP_KEY_CHAR_STATS_STR: who->query_stat(ST_STR),
      //GMCP_KEY_CHAR_STATS_FAT: who->query_stat(ST_FAT),
    ]);

    who->gmcp_message(GMCP_PKG_CHAR_STATS, stats);
}

/*
** Function: send_char_max_stats
**
** Returns: void
**
** Parameters:
**
**	object who
**		The player object we not only intended to send the information
**		to but also the player object we will query to determine the
**		stat maximums.
**
** Purpose:
**
**	send_char_maxstats is familiar to players of Aardwolf MUD and could be
**	used in combination with the information returned in send_char_stats to
**	monitor whether stats are drained or enhanced. The reason there are
**	two functions is 1) to match Aardwolf and client side scripts people may
**	have authored in the past and 2) to possibly reduce the amount of data
**	transferred as send_char_stats could be needed more often where this
**	function changes less.  It's worthwhile to note that caching is used
**	inside the player object so the mud will only send the change in stats
**	to the client when a change occurs or when the client requests the
**	information be refreshed.
**
** In the Mudlet client, type lua gmcp.Char.MaxStats and you will see something like:
** {
**    maxint = 1,
**    maxstr = 1,
**    maxcon = 1,
**    maxdex = 1
** }
*/
nomask void
send_char_maxstats(object who) {
    mapping maxstats = ([
      GMCP_KEY_CHAR_MAXSTATS_MAXCON: who->query_stat(ST_BASE_CON),
      GMCP_KEY_CHAR_MAXSTATS_MAXINT: who->query_stat(ST_BASE_INT),
      GMCP_KEY_CHAR_MAXSTATS_MAXDEX: who->query_stat(ST_BASE_DEX),
      GMCP_KEY_CHAR_MAXSTATS_MAXSTR: who->query_stat(ST_BASE_STR),
      //GMCP_KEY_CHAR_MAXSTATS_MAXFAT: who->query_stat(ST_BASE_FAT),
    ]);

    who->gmcp_message(GMCP_PKG_CHAR_MAXSTATS, maxstats);
}

/*
** Function: send_char_status
**
** Returns: void
**
** Parameters:
**
**	object who
**		The player object we not only intended to send the information
**		to but also the player object we will query to determine the
**		much of the data from either directly or from their environment.
**
** Purpose:
**
**	send_char_status performs much of the work load of updating many of the
**	most interesting attributes of the MUD and matches up well with the
**	standard information people expect from MUDs these days, such as the
**	popular Iron Realms MUDs where much of the GMCP standards are based.
**	The fields within send_char_status are intended to match up directly
**	with the headers sent from the send_char_statusvars function and are
**	often displayed in a table style interface in the client. It's worthwhile
**	to note that caching is used inside the player object so the mud will
**	only send to the client when values change or when the client requests
**	the information be refreshed.
**
** In the Mudlet client, type lua gmcp.Char.Status and you will see something like:
** {
**   thirst = "Yes", -- This could send categories of thirst future case
**   draincon = "No",
**   shield = "No", -- Am I wearing a shield?
**   guild = "priest", -- GMCP standard, in statusvars we display the header as "Guild"
**   belt = "No",
**   drainint = "No",
**   invis = "No",
**   girdle = "No",
**   level = 1,
**   ring = "No",
**   fullname = "Tamjr",
**   pkable = "No",
**   align = "Neutral",
**   gender = "male",
**   xp = 2,
**   drunk = "No", -- This could be categories of drunkeness future case
**   title = "the new member of the priesthood",
**   enemy_health = "0", -- In battle this is "in good health", "is mortally wounded", etc.
**   drainstr = "No",
**   rest = "No",
**   gloves = "No",
**   brave = "Yes", -- Example when wimpy is "No (30%)"
**   enemy = "None", -- Name of enemy
**   leggings = "No",
**   frog = "No",
**   gold = 2998, -- gold carried, there is another value for "bank" to implement soon
**   boots = "No",
**   cloak = "No",
**   summonable = "No",
**   amulet = "No",
**   draindex = "No",
**   armour = "No",
**   sit = "No",
**   mask = "No",
**   nl = 1012, -- exp to next level
**   helmet = "No",
**   disguise = "No",
**   name = "Tamjr", -- Iron Realms sends name here. May helps some client scripts to have here too.
**   bracers = "No",
**   race = "human",
**   hunger = "Yes",
**   dead = "No",
**   necklace = "No"
** }
*/
nomask void
send_char_status(object who) {
    mixed x;
    string hdesc;

    int *exp = who->query_experience();
    string exp_str = LEVELS_D->query_experience_string(exp);
    int *tnl_exp = LEVELS_D->subtract_experience(LEVELS_D->query_experience(who->query_level()+1), exp);
    string tnl_exp_str = LEVELS_D->query_experience_string(tnl_exp);
    int connect_time = who->query(PLR_CONNECT_TIME);

    mapping char_status = ([ ]);

    // At one time local variables were used before doing the mapping this way, but there is a limit to the
    // local variables one could use in a function.  Who knew? :)
    char_status[GMCP_KEY_CHAR_STATUS_NAME] = capitalize(who->query_real_name());
    char_status[GMCP_KEY_CHAR_STATUS_FULLNAME] = sprintf("%s%s", who->query_pretitle(), capitalize(who->query_real_name()));
    //char_status[GMCP_KEY_CHAR_STATUS_AGE] = secs2sstring(who->query_age() * 2); // This works but sends a new age every heart beat.
    char_status[GMCP_KEY_CHAR_STATUS_SESSION_LOGIN] = ctime(who->query(PLR_LOGIN_TIME));
    //char_status[GMCP_KEY_CHAR_STATUS_SESSION_CONNECTED] = ctime(connect_time);
    //char_status[GMCP_KEY_CHAR_STATUS_SESSION_XP] = "/bin/daemons/telopt/char_session"->send_char_session_exp(who);
    //char_status[GMCP_KEY_CHAR_STATUS_SESSION_KILLS] = "/bin/daemons/telopt/char_session"->send_char_session_kills(who);
    char_status[GMCP_KEY_CHAR_STATUS_SESSION_ROOMS] = who->query_room_count();
    char_status[GMCP_KEY_CHAR_STATUS_RACE] = (who->query_race() ? who->query_race() : "human");
    char_status[GMCP_KEY_CHAR_STATUS_LEVEL] = who->query_level();
    char_status[GMCP_KEY_CHAR_STATUS_XP] = exp_str;
    char_status[GMCP_KEY_CHAR_STATUS_CLASS] = who->query_guild();
    char_status[GMCP_KEY_CHAR_STATUS_GOLD] = who->query_money();
    char_status[GMCP_KEY_CHAR_STATUS_BANK] = FINGER_D->query_bank_gold(who->query_real_name());
    char_status[GMCP_KEY_CHAR_STATUS_QP] = who->query_qp();
    char_status[GMCP_KEY_CHAR_STATUS_ENEMY] = ((x=who->is_fighting()) ? x->query_name() : "None");
    char_status[GMCP_KEY_CHAR_STATUS_ENEMY_HEALTH] = ((x=who->is_fighting()) ? ((hdesc=x->query_health_desc()) ? hdesc : "0" ) : "0");
    char_status[GMCP_KEY_CHAR_STATUS_GENDER] = who->query_gender_string();
    char_status[GMCP_KEY_CHAR_STATUS_TITLE] = who->query_title();
    char_status[GMCP_KEY_CHAR_STATUS_NL] = tnl_exp_str;
    char_status[GMCP_KEY_CHAR_STATUS_DEAD] = who->query_ghost() ? "Yes" : "No";
    char_status[GMCP_KEY_CHAR_STATUS_BRAVE] = who->query_wimpy() ? sprintf("No (%d%%)", who->query_wimpy()) : "Yes";
    char_status[GMCP_KEY_CHAR_STATUS_DRUNK] = who->query_intoxicated() ? "Yes" : "No";
    char_status[GMCP_KEY_CHAR_STATUS_HUNGER] = who->query_stuffed() ? "No" : "Yes";
    char_status[GMCP_KEY_CHAR_STATUS_THIRST] = who->query_soaked() ? "No" : "Yes";
    char_status[GMCP_KEY_CHAR_STATUS_DISGUISE] = who->query_disguised() ? "Yes" : "No";
    char_status[GMCP_KEY_CHAR_STATUS_ALIGN] = capitalize(who->query_al_title());
    //char_status[GMCP_KEY_CHAR_STATUS_STATE] = ;
    char_status[GMCP_KEY_CHAR_STATUS_SIT] = who->query_sit() ? "Yes" : "No";
    char_status[GMCP_KEY_CHAR_STATUS_REST] = who->query_rest() ? "Yes" : "No";
    char_status[GMCP_KEY_CHAR_STATUS_FROG] = who->query_frog() ? "Yes" : "No";
    char_status[GMCP_KEY_CHAR_STATUS_INVIS] = who->query_invis() ? "Yes" : "No";
    char_status[GMCP_KEY_CHAR_STATUS_SUMMONABLE] = who->query(PLR_ALLOW_SUMMON) ? "Yes" : "No";
    char_status[GMCP_KEY_CHAR_STATUS_PKABLE] = who->query(LIV_OK_TO_PK) ? "Yes" : "No";
    char_status[GMCP_KEY_CHAR_STATUS_DRAINCON] = (who->query_stat(ST_CON) < who->query_stat(ST_BASE_CON)) ? "Yes" : "No";
    char_status[GMCP_KEY_CHAR_STATUS_DRAININT] = (who->query_stat(ST_INT) < who->query_stat(ST_BASE_INT)) ? "Yes" : "No";
    char_status[GMCP_KEY_CHAR_STATUS_DRAINDEX] = (who->query_stat(ST_DEX) < who->query_stat(ST_BASE_DEX)) ? "Yes" : "No";
    char_status[GMCP_KEY_CHAR_STATUS_DRAINSTR] = (who->query_stat(ST_STR) < who->query_stat(ST_BASE_STR)) ? "Yes" : "No";

    // We're matching up to what is done in the "eq" command here, but this desperately needs refactored to be
    // less lines of code when we get a minute!
    mapping armours = ([
      ARMOUR_TYPE_AMULET: 0,
      ARMOUR_TYPE_BODY_ARMOUR: 0,
      ARMOUR_TYPE_BELT: 0,
      ARMOUR_TYPE_BOOTS: 0,
      ARMOUR_TYPE_BRACERS: 0,
      ARMOUR_TYPE_CLOAK: 0,
      ARMOUR_TYPE_GLOVES: 0,
      ARMOUR_TYPE_HELMET: 0,
      ARMOUR_TYPE_LEGGINGS: 0,
      ARMOUR_TYPE_MASK: 0,
      ARMOUR_TYPE_RING: 0,
      ARMOUR_TYPE_SHIELD: 0,
      ARMOUR_TYPE_NECKLACE: 0,
    ]);

    foreach (object ob : filter(all_inventory(who), (: $1->is_worn() :))) {
	string type = (string)ob->query_armour_type();
    }

    char_status[GMCP_KEY_CHAR_STATUS_AMULET] =
	armours[ARMOUR_TYPE_AMULET] ? (armours[ARMOUR_TYPE_AMULET]->query_damage() ? "Damaged" : "Yes") : "No";
    char_status[GMCP_KEY_CHAR_STATUS_ARMOUR] =
	armours[ARMOUR_TYPE_BODY_ARMOUR] ? (armours[ARMOUR_TYPE_BODY_ARMOUR]->query_damage() ? "Damaged" : "Yes") : "No";
    char_status[GMCP_KEY_CHAR_STATUS_BELT] =
	armours[ARMOUR_TYPE_BELT] ? (armours[ARMOUR_TYPE_BELT]->query_damage() ? "Damaged" : "Yes") : "No";
    char_status[GMCP_KEY_CHAR_STATUS_BOOTS] =
	armours[ARMOUR_TYPE_BOOTS] ? (armours[ARMOUR_TYPE_BOOTS]->query_damage() ? "Damaged" : "Yes") : "No";
    char_status[GMCP_KEY_CHAR_STATUS_BRACERS] =
	armours[ARMOUR_TYPE_BRACERS] ? (armours[ARMOUR_TYPE_BRACERS]->query_damage() ? "Damaged" : "Yes") : "No";
    char_status[GMCP_KEY_CHAR_STATUS_CLOAK] =
	armours[ARMOUR_TYPE_CLOAK] ? (armours[ARMOUR_TYPE_CLOAK]->query_damage() ? "Damaged" : "Yes") : "No";
    char_status[GMCP_KEY_CHAR_STATUS_GLOVES] =
	armours[ARMOUR_TYPE_GLOVES] ? (armours[ARMOUR_TYPE_GLOVES]->query_damage() ? "Damaged" : "Yes") : "No";
    char_status[GMCP_KEY_CHAR_STATUS_HELMET] =
	armours[ARMOUR_TYPE_HELMET] ? (armours[ARMOUR_TYPE_HELMET]->query_damage() ? "Damaged" : "Yes") : "No";
    char_status[GMCP_KEY_CHAR_STATUS_LEGGINGS] =
	armours[ARMOUR_TYPE_LEGGINGS] ? (armours[ARMOUR_TYPE_LEGGINGS]->query_damage() ? "Damaged" : "Yes") : "No";
    char_status[GMCP_KEY_CHAR_STATUS_MASK] =
	armours[ARMOUR_TYPE_MASK] ? (armours[ARMOUR_TYPE_MASK]->query_damage() ? "Damaged" : "Yes") : "No";
    char_status[GMCP_KEY_CHAR_STATUS_NECKLACE] =
	armours[ARMOUR_TYPE_NECKLACE] ? (armours[ARMOUR_TYPE_NECKLACE]->query_damage() ? "Damaged" : "Yes") : "No";
    char_status[GMCP_KEY_CHAR_STATUS_RING] =
	armours[ARMOUR_TYPE_RING] ? (armours[ARMOUR_TYPE_RING]->query_damage() ? "Damaged" : "Yes") : "No";
    char_status[GMCP_KEY_CHAR_STATUS_SHIELD] =
	armours[ARMOUR_TYPE_SHIELD] ? (armours[ARMOUR_TYPE_SHIELD]->query_damage() ? "Damaged" : "Yes") : "No";

    who->gmcp_message(GMCP_PKG_CHAR_STATUS, char_status);
}

/*
**   Function: send_char_status_vars
**
**    Returns: void
**
** Parameters:
**
**	object who
**		The player object we not only intended to send the information.
**
**   Purpose:
**
**	send_char_status_vars sends the headers that are used to describe the
**	data that is frequently updated through the send_char_status function.
**	The naming conventions used here match up well with the popular Iron
**	Realms MUDs where much of the GMCP standards were based.  These fields are
**	often displayed in a table style interface in the client. It's worthwhile
**	to note that caching is used inside the player object so the mud will
**	only send to the client when values change or when the client requests
**	the information be refreshed. This function will likely only be called
**	upon player initialization after a successful login.
**
** In the Mudlet client, type lua gmcp.Char.StatusVars and you will see something like:
** {
**   thirst = "Thirst",
**   draincon = "CON Drain",
**   shield = "Shield",
**   class = "Guild", -- Aligns with the standard, less updates to public scripts needed
**   belt = "Belt",
**   drainint = "INT Drain",
**   invis = "Invisible",
**   girdle = "Girdle",
**   level = "Level",
**   ring = "Ring",
**   fullname = "Full Name",
**   pkable = "PK Flag",
**   align = "Alignment",
**   gender = "Gender",
**   xp = "Experience",
**   drunk = "Drunk",
**   title = "Title",
**   drainstr = "STR Drain",
**   rest = "Resting",
**   gloves = "Gloves",
**   brave = "Brave",
**   enemy = "Enemy Health",
**   leggings = "Leggings",
**   frog = "Frogged",
**   gold = "Gold", -- A "bank" header should be available too when this code is merged
**   qp = "Quest Points",
**   boots = "Boots",
**   cloak = "Cloak",
**   summonable = "Summonable",
**   amulet = "Amulet",
**   draindex = "DEX Drain",
**   armour = "Armour",
**   mask = "Mask",
**   sit = "Sitting",
**   nl = "Next Level",
**   helmet = "Helmet",
**   necklace = "Necklace",
**   name = "Name",
**   bracers = "Bracers",
**   race = "Race",
**   hunger = "Hunger",
**   dead = "Dead",
**   disguise = "Disguise"
** }
**
*/
nomask void
send_char_status_vars(object who) {
    mapping statusvars = ([
      GMCP_KEY_CHAR_STATUS_NAME: "Name",
      GMCP_KEY_CHAR_STATUS_FULLNAME: "Full Name",
      //GMCP_KEY_CHAR_STATUS_AGE: "Age",
      GMCP_KEY_CHAR_STATUS_SESSION_LOGIN: "Login Time",
      GMCP_KEY_CHAR_STATUS_SESSION_CONNECTED: "Last Connected",
      GMCP_KEY_CHAR_STATUS_SESSION_XP: "Session Experience",
      GMCP_KEY_CHAR_STATUS_SESSION_KILLS: "Session Kills",
      GMCP_KEY_CHAR_STATUS_SESSION_ROOMS: "Entered Rooms",
      GMCP_KEY_CHAR_STATUS_RACE: "Race",
      GMCP_KEY_CHAR_STATUS_LEVEL: "Level",
      GMCP_KEY_CHAR_STATUS_XP: "Experience",
      GMCP_KEY_CHAR_STATUS_CLASS: "Guild",
      GMCP_KEY_CHAR_STATUS_GOLD: "Gold",
      //GMCP_KEY_CHAR_STATUS_BANK: "Bank",
      GMCP_KEY_CHAR_STATUS_QP: "Quest Points",
      GMCP_KEY_CHAR_STATUS_ENEMY: "Enemy",
      GMCP_KEY_CHAR_STATUS_ENEMY: "Enemy Health",
      GMCP_KEY_CHAR_STATUS_GENDER: "Gender",
      GMCP_KEY_CHAR_STATUS_TITLE: "Title",
      GMCP_KEY_CHAR_STATUS_NL: "Next Level",
      GMCP_KEY_CHAR_STATUS_DEAD: "Dead",
      GMCP_KEY_CHAR_STATUS_BRAVE: "Brave",
      GMCP_KEY_CHAR_STATUS_DRUNK: "Drunk",
      GMCP_KEY_CHAR_STATUS_HUNGER: "Hunger",
      GMCP_KEY_CHAR_STATUS_THIRST: "Thirst",
      GMCP_KEY_CHAR_STATUS_DISGUISE: "Disguise",
      GMCP_KEY_CHAR_STATUS_ALIGN: "Alignment",
      //GMCP_KEY_CHAR_STATUS_STATE: "State",
      GMCP_KEY_CHAR_STATUS_SIT: "Sitting",
      GMCP_KEY_CHAR_STATUS_REST: "Resting",
      GMCP_KEY_CHAR_STATUS_FROG: "Frogged",
      GMCP_KEY_CHAR_STATUS_INVIS: "Invisible",
      GMCP_KEY_CHAR_STATUS_SUMMONABLE: "Summonable",
      GMCP_KEY_CHAR_STATUS_PKABLE: "PK Flag",
      GMCP_KEY_CHAR_STATUS_DRAINCON: "CON Drain",
      GMCP_KEY_CHAR_STATUS_DRAININT: "INT Drain",
      GMCP_KEY_CHAR_STATUS_DRAINDEX: "DEX Drain",
      GMCP_KEY_CHAR_STATUS_DRAINSTR: "STR Drain",
      GMCP_KEY_CHAR_STATUS_AMULET: "Amulet",
      GMCP_KEY_CHAR_STATUS_ARMOUR: "Armour",
      GMCP_KEY_CHAR_STATUS_BELT: "Belt",
      GMCP_KEY_CHAR_STATUS_BOOTS: "Boots",
      GMCP_KEY_CHAR_STATUS_BRACERS: "Bracers",
      GMCP_KEY_CHAR_STATUS_CLOAK: "Cloak",
      GMCP_KEY_CHAR_STATUS_GIRDLE: "Girdle",
      GMCP_KEY_CHAR_STATUS_GLOVES: "Gloves",
      GMCP_KEY_CHAR_STATUS_HELMET: "Helmet",
      GMCP_KEY_CHAR_STATUS_LEGGINGS: "Leggings",
      GMCP_KEY_CHAR_STATUS_MASK: "Mask",
      GMCP_KEY_CHAR_STATUS_NECKLACE: "Necklace",
      GMCP_KEY_CHAR_STATUS_RING: "Ring",
      GMCP_KEY_CHAR_STATUS_RING2: "Ring 2",
      GMCP_KEY_CHAR_STATUS_SHIELD: "Shield",
    ]);

    who->gmcp_message(GMCP_PKG_CHAR_STATUSVARS, statusvars);
}

/*
**   Function: send_char_vitals
**
**    Returns: void
**
** Parameters:
**
**	object who
**		The player object we intended to send the information to and also
**		the player object we will gather a report on vitals.
**
**   Purpose:
**
**	send_char_vitals reports on the health, mana and fatigue points of the
**	player as well as changes in experience points. It's worthwhile
**	to note that caching is used inside the player object so the mud will
**	only send to the client when values change or when the client requests
**	the information be refreshed.
**
** In the Mudlet client, type lua gmcp.Char.Vitals and you will see something like:
** {
**   fp = 69,
**   string = "HP:80/80 SP:25/25 FP:69/69 NL:2/1014",
**   maxfp = 69,
**   maxhp = 80,
**   hp = 80,
**   sp = 25,
**   maxsp = 25,
**   nl = 1012
** }
*/
nomask void send_char_vitals(object who) {
    int hp = who->query_hp();
    int maxhp = who->query_max_hp();
    int sp = who->query_sp();
    int maxsp = who->query_max_sp();
    int fp = who->query_fp();
    int maxfp = who->query_max_fp();
    int level = who->query_level();
    int *exp = who->query_experience();
    string exp_str = LEVELS_D->query_experience_string(exp);
    int *tnl_exp = LEVELS_D->subtract_experience(LEVELS_D->query_experience(level+1), exp);
    string tnl_exp_str = LEVELS_D->query_experience_string(tnl_exp);

    mapping vitals = ([
      GMCP_KEY_CHAR_VITALS_HP: hp,
      GMCP_KEY_CHAR_VITALS_MAXHP: maxhp,
      GMCP_KEY_CHAR_VITALS_SP: sp,
      GMCP_KEY_CHAR_VITALS_MAXSP: maxsp,
      GMCP_KEY_CHAR_VITALS_FP: fp,
      GMCP_KEY_CHAR_VITALS_MAXFP: maxfp,
      GMCP_KEY_CHAR_VITALS_NL: tnl_exp_str,
      GMCP_KEY_CHAR_VITALS_STRING: sprintf("HP:%d/%d SP:%d/%d FP:%d/%d",
	hp, maxhp, sp, maxsp, fp, maxfp),
    ]);

    who->gmcp_message(GMCP_PKG_CHAR_VITALS, vitals);
}

/*
**   Function: send_char_worth
**
**    Returns: void
**
** Parameters: who, the player object who will be send this GMCP
**             data as well as the player object who will be queried
**             for this data
**
**    Purpose: This matches up with Aardwolf MUD and reports on the
**             money one has on hand, in the bank and quest points.
**             Since it is a repeat of what is inside of the
**             Char.Status information, this function is not yet
**             tied into the player object yet.  Perhaps since it
**             changes more specifically and less often that some of
**             the other information in Char.Status it should be
**             considered to be used instead.
*/
nomask void
send_char_worth(object who) {
    mapping worth = ([
      GMCP_KEY_CHAR_STATUS_GOLD: who->query_money(),
      //GMCP_KEY_CHAR_STATUS_BANK: FINGER_D->query_bank_gold(who->query_real_name()),
      GMCP_KEY_CHAR_STATUS_QP: who->query_qp(),
    ]);
    who->gmcp_message(GMCP_PKG_CHAR_WORTH, worth);
}

/*
**   Function: send_char_training_list
**
**    Returns: void
**
** Parameters: who, the player object who will be send this GMCP
**             data as well as the player object who will be queried
**             for this data
**
**    Purpose: Updates the training detail for a player in a guild.
*/
nomask void
send_char_training_list(object who) {
    mapping training_list = "/bin/daemons/telopt/char_training"->send_char_training_list(who);

    who->gmcp_message(GMCP_PKG_CHAR_TRAINING_LIST, training_list);
}

/*
**   Function: send_char_session_training
**
**    Returns: void
**
** Parameters: who, the player object who will be send this GMCP
**             data as well as the player object who will be queried
**             for this data
**
**    Purpose: Updates the session training for a player in a guild.
*/
nomask void
send_char_session_training(object who) {
    mapping session_training = "/bin/daemons/telopt/char_session"->send_char_session_training(who);

    who->gmcp_message(GMCP_PKG_CHAR_SESSION_TRAINING, session_training);
}

/*
**   Function: send_char_conditions_list
**
**    Returns: void
**
** Parameters: object who, the player object who the data is queried from as well
**             as GMCP message is sent to.
**
**   Purpose: This function is StickMUD specific although very valuable in that
**            it enables the player to visualize their current conditions such
**            as Stunned or Confused.
*/
nomask void
send_char_conditions_list(object who) {
    mapping conditions;

    foreach (int key : conditions_descs) {
	if (who->query_condition(key)) {
	    if (!conditions)
		conditions = ([ conditions_descs[key][0]: conditions_descs[key][1] ]);
#if 0
	{
#endif
	    else conditions[conditions_descs[key][0]] = conditions_descs[key][1];
	}
    }

    who->gmcp_message(GMCP_PKG_CHAR_CONDITIONS_LIST, conditions);
}

/*
**   Function: send_char_conditions_add
**
**    Returns: void
**
** Parameters: object who, the player object who the data is queried from as well
**             as GMCP message is sent to.
**
**             int condition, the condition being added to the player object
**
**   Purpose: This function is StickMUD specific although very valuable in that
**            it enables the player to visualize their current conditions such
**            as Stunned or Confused.
*/
nomask varargs
void send_char_conditions_add(object who, int condition) {
    who->gmcp_message(GMCP_PKG_CHAR_CONDITIONS_ADD,
      ({ conditions_descs[condition][0], conditions_descs[condition][1] }));
}

/*
**   Function: send_char_conditions_remove
**
**    Returns: void
**
** Parameters: object who, the player object who the data is queried from as well
**             as GMCP message is sent to.
**
**             int condition, the condition being removed from the player object
**
**   Purpose: This function is StickMUD specific although very valuable in that
**            it enables the player to visualize their current conditions such
**            as Stunned or Confused.
*/
nomask void
send_char_conditions_remove(object who, int condition) {
    who->gmcp_message(GMCP_PKG_CHAR_CONDITIONS_REMOVE, conditions_descs[condition][0]);
}

/*
**   Function: send_char_resistancess_list
**
**    Returns: void
**
** Parameters: object who, the player object who the data is queried from as well
**             as GMCP message is sent to.
**
**   Purpose: This function is StickMUD specific although very valuable in that
**            it enables the player to visualize their current resistances such
**            as Can't Be Stunned or Resistant to Blunt Damage.
*/
nomask void
send_char_resistances_list(object who) {
    mapping resistances;

    foreach (int key : resistances_descs) {
	if ((key < 0 && who->query_conditions(key)) || who->query_resistances(key)) {
	    if (!resistances)
		resistances = ([ resistances_descs[key][0]: resistances_descs[key][1] ]);
#if 0
	{
#endif
	    else resistances[resistances_descs[key][0]] = resistances_descs[key][1];
	}
    }

    who->gmcp_message(GMCP_PKG_CHAR_RESISTANCES_LIST, resistances);
}

/*
**   Function: send_char_resistances_add
**
**    Returns: void
**
** Parameters: object who, the player object who the data is queried from as well
**             as GMCP message is sent to.
**
**             int resistance, the resistance being added to the player object
**
**   Purpose: This function is StickMUD specific although very valuable in that
**            it enables the player to visualize their current resistances such
**            as Can't Be Stunned or Resistant to Blunt Damage.
*/
nomask varargs
void send_char_resistances_add(object who, int resistance) {
    who->gmcp_message(GMCP_PKG_CHAR_RESISTANCES_ADD,
      ({ resistances_descs[resistance][0], resistances_descs[resistance][1] }));
}

/*
**   Function: send_char_resistances_remove
**
**    Returns: void
**
** Parameters: object who, the player object who the data is queried from as well
**             as GMCP message is sent to.
**
**             int resistance, the resistance being removed from the player object
**
**   Purpose: This function is StickMUD specific although very valuable in that
**            it enables the player to visualize their current resistances such
**            as Can't Be Stunned or Resistant to Blunt Damage.
*/
nomask void
send_char_resistances_remove(object who, int resistance) {
    who->gmcp_message(GMCP_PKG_CHAR_RESISTANCES_REMOVE, resistances_descs[resistance][0]);
}

/*
**   Function: send_char_afflictions_list
**
**    Returns: void
**
** Parameters: object who, the player object who the data is queries from as well
**             as GMCP message is sent to.
**
**    Purpose: This function is not yet supported in StickMUD, but is very standard
**             for other games.  This could be tied to diseases in StickMUD.
*/
nomask void
send_char_afflictions_list(object who) {
    mapping afflictions;

    who->gmcp_message(GMCP_PKG_CHAR_AFFLICTIONS_LIST, afflictions);
}

/*
**   Function: send_char_afflictions_add
**
**    Returns: void
**
** Parameters: object who, the player object who the data is queries from as well
**             as GMCP message is sent to.
**
**             string affliction, the disease the player has.
**
**             string description, a description of the diseases effects.
**
**             string cure, a hint on one way to cure the disease, like
**             "eat a jojojopo plant".
**
**    Purpose: This function is not yet supported in StickMUD, but is very standard
**             for other games.  This could be tied to diseases in StickMUD.
*/
nomask varargs
void send_char_afflictions_add(object who, string affliction, string description, string cure) {
    who->gmcp_message(GMCP_PKG_CHAR_AFFLICTIONS_REMOVE, ({affliction, description, cure}));
}

/*
**   Function: send_char_afflictions_remove
**
**    Returns: void
**
** Parameters: object who, the player object who the data is queries from as well
**             as GMCP message is sent to.
**
**             string affliction, the disease the player is recovering from.
**
**    Purpose: This function is not yet supported in StickMUD, but is very standard
**             for other games.  This could be tied to diseases in StickMUD.
*/
nomask void
send_char_afflictions_remove(object who, string affliction) {
    who->gmcp_message(GMCP_PKG_CHAR_AFFLICTIONS_REMOVE, affliction);
}

/*
**   Function: send_char_defences_list
**
**    Returns: void
**
** Parameters: object who, the player object who the data is queries from as well
**             as GMCP message is sent to.
**
**    Purpose: This function is not yet supported in StickMUD, but is very standard
**             for other games.  This could be similar to resistances in StickMUD.
*/
nomask void
send_char_defences_list(object who) {
    mapping defences;

    who->gmcp_message(GMCP_PKG_CHAR_DEFENCES_LIST, defences);
}

/*
**   Function: send_char_defences_add
**
**    Returns: void
**
** Parameters: object who, the player object who the data is queries from as well
**             as GMCP message is sent to.
**
**             string defence, the defense the player now has
**
**             string description, more deatils about the defense
**
**    Purpose: This function is not yet supported in StickMUD, but is very standard
**             for other games.  This could be similar to resistances in StickMUD.
*/
nomask varargs
void send_char_defences_add(object who, string defence, string description) {
    who->gmcp_message(GMCP_PKG_CHAR_DEFENCES_REMOVE, ({defence, description}));
}

/*
**   Function: send_char_defences_remove
**
**    Returns: void
**
** Parameters: object who, the player object who the data is queries from as well
**             as GMCP message is sent to.
**
**             string defence, the defense the player is losing
**
**    Purpose: This function is not yet supported in StickMUD, but is very standard
**             for other games.  This could be similar to resistances in StickMUD.
*/
nomask void
send_char_defences_remove(object who, string defence) {
    who->gmcp_message(GMCP_PKG_CHAR_DEFENCES_REMOVE, defence);
}

/*
**   Function: send_char_items_add
**
**    Returns: void
**
** Parameters: object who, the player object who the data is queries from as well
**             as GMCP message is sent to.
**
**             string location, "room", "inventory" or a container that the player
**             has in their possession.
**
**             object ob, the object being added to the location
**
**    Purpose: This will enable clients to display tables of items that are relevant
**             to the players environment and allow them to perform actions upon
**             them and display relevant icons.
*/
nomask void
send_char_items_add(object who, string location, object ob) { // 3rd parameter should be the item "id" per the spec, maybe later?
    mapping item;
    string short_str;
    int see_level, hallucinating;

    if (!who->query_can_move() || who->test_dark()) return;

    if (who->query_coder_level()) see_level = IL_TOTALLY_INVISIBLE;
    else if (who->query_condition(C_DETECT_INVIS)) see_level = IL_INVISIBLE;
    else see_level = IL_VISIBLE;

    see_level = who->test_dark2(environment(who)->query_total_light(),
      environment(who)->query_light_level(),
      environment(who)->query(ROOM_WD));

    if (see_level < 0) see_level = -see_level;

    // If we can see objects
    if (see_level >= 4) {
	if (who->query_condition(C_HALLUCINATING)) hallucinating = 1;

	if (ob != who
	  && (see_level >= 6 && !ob->query(LIV_IS_PLAYER))
	  && (who->query_coder_level()
	    || member(({IL_SNEAKING, IL_HIDING, IL_HIDDEN, IL_TOTALLY_INVISIBLE}), ob->query_invis()) == -1)) {
	    // We may be hallucinating.
	    if (hallucinating && random(100) > 74) {
		short_str = (string) HALLU_D->random_short(1);
	    } else {
		short_str = (string) ob->query_short(see_level, who);
	    }

	    item = ([ GMCP_KEY_CHAR_ITEMS_LOCATION: location,
	      GMCP_KEY_CHAR_ITEMS_ITEM: ([
		GMCP_KEY_CHAR_ITEMS_ITEMS_ID: hash(TLS_HASH_SHA1, object_name(ob)),
		GMCP_KEY_CHAR_ITEMS_ITEMS_NAME: short_str,
		GMCP_KEY_CHAR_ITEMS_ITEMS_ICON: 0,
		GMCP_KEY_CHAR_ITEMS_ITEMS_ATTRIB: ob->query_worn() ? "w"
		: ((ob->query_armour() || ob->query_clothing()) ? "W"
		  : (ob->query_wielded() ? "l"
		    : (ob->can_put_and_get() ? "c"
		      : (ob->get(1) ? "t"
			: (ob->query(LIV_IS_NPC) ? "m"
			  : (ob->query_corpse() ? "d"
			    : "")))))),
	      ])
	    ]);

	    who->gmcp_message(GMCP_PKG_CHAR_ITEMS_ADD, item);
	}
    }
}

/*
**   Function: send_char_items_list
**
**    Returns: void
**
** Parameters: object who, the player object who the data is queries from as well
**             as GMCP message is sent to.
**
**             string location, "room", "inventory" or a container that the player
**             has in their possession.
**
**             object container, the container the list is being generated.
**
**    Purpose: This will enable clients to display tables of items that are relevant
**             to the players environment and allow them to perform actions upon
**             them and display relevant icons.
*/
nomask varargs
void send_char_items_list(object who, string location, object container) { // 3rd parameter should be the item "id" per the spec, maybe later?
    mixed *items;
    mapping items_list;
    object ob, *obs;
    string short_str;
    int see_level, hallucinating;

    if (!who->query_can_move() || who->test_dark()) return;

    if (who->query_coder_level()) see_level = IL_TOTALLY_INVISIBLE;
    else if (who->query_condition(C_DETECT_INVIS)) see_level = IL_INVISIBLE;
    else see_level = IL_VISIBLE;

    switch (location) {
    case GMCP_VALUE_CHAR_ITEMS_LOCATION_INV:
	obs = all_inventory(who);

	foreach (ob : obs) {
	    mapping item = ([
	      GMCP_KEY_CHAR_ITEMS_ITEMS_ID: hash(TLS_HASH_SHA1, object_name(ob)),
	      GMCP_KEY_CHAR_ITEMS_ITEMS_NAME: ob->query_short(see_level, who),
	      GMCP_KEY_CHAR_ITEMS_ITEMS_ICON: 0,
	      GMCP_KEY_CHAR_ITEMS_ITEMS_ATTRIB: ob->query_worn() ? "w"
	      : ((ob->query_armour() || ob->query_clothing()) ? "W"
		: (ob->query_wielded() ? "l"
		  : (ob->can_put_and_get() ? "c"
		    : (ob->get(1) ? "t"
		      : (ob->query(LIV_IS_NPC) ? "m"
			: (ob->query_corpse() ? "d"
			  : ""))))))
	    ]);
#if 0
	{
#endif
	    if (!items || !sizeof(items)) {
		items = ({ item });
	    } else {
		items += ({ item });
	    }
	}

	items_list = ([ GMCP_KEY_CHAR_ITEMS_LOCATION: location, GMCP_KEY_CHAR_ITEMS_ITEMS: items ? items : "" ]);

	break;

    case GMCP_VALUE_CHAR_ITEMS_LOCATION_ROOM:
	// If we are the only object in the room, skip this.
	if (sizeof(obs = all_inventory(environment(who))) > 1) {
	    see_level = who->test_dark2(environment(who)->query_total_light(),
	      environment(who)->query_light_level(),
	      environment(who)->query(ROOM_WD));

	    if (see_level < 0) see_level = -see_level;

	    // If we can see objects
	    if (see_level >= 4) {
		if (who->query_condition(C_HALLUCINATING)) hallucinating = 1;

		foreach (ob : obs) {
		    if (ob != who
		      && (see_level >= 6 && !ob->query(LIV_IS_PLAYER))
		      && (who->query_coder_level() || member(({IL_SNEAKING, IL_HIDING, IL_HIDDEN, IL_TOTALLY_INVISIBLE}), ob->query_invis()) == -1)) {
			mapping item;

			// We may be hallucinating.
			if (hallucinating && random(100) > 74) {
			    short_str = (string) HALLU_D->random_short(1);
			} else {
			    short_str = (string) ob->query_short(see_level, who);
			}

			item = ([
			  GMCP_KEY_CHAR_ITEMS_ITEMS_ID: hash(TLS_HASH_SHA1, object_name(ob)),
			  GMCP_KEY_CHAR_ITEMS_ITEMS_NAME: short_str,
			  GMCP_KEY_CHAR_ITEMS_ITEMS_ICON: 0,
			  GMCP_KEY_CHAR_ITEMS_ITEMS_ATTRIB: ob->query_worn() ? "w"
			  : ((ob->query_armour() || ob->query_clothing()) ? "W"
			    : (ob->query_wielded() ? "l"
			      : (ob->can_put_and_get() ? "c"
				: (ob->get(1) ? "t"
				  : (ob->query(LIV_IS_NPC) ? "m"
				    : (ob->query_corpse() ? "d"
				      : ""))))))
			]);
#if 0
		    {
#endif
			if (!items || !sizeof(items)) {
			    items = ({ item });
			} else {
			    items += ({ item });
			}
		    }
		}
	    }
	}

	items_list = ([ GMCP_KEY_CHAR_ITEMS_LOCATION: location, GMCP_KEY_CHAR_ITEMS_ITEMS: items ? items : "" ]);

	break;

    default:
	if (present(container, who)
	  && sizeof(obs = all_inventory(container)) > 0) {
	    see_level = who->test_dark2(environment(who)->query_total_light(),
	      environment(who)->query_light_level(),
	      environment(who)->query(ROOM_WD));

	    if (see_level < 0) see_level = -see_level;

	    // If we can see objects
	    if (see_level >= 4) {
		if (who->query_condition(C_HALLUCINATING)) hallucinating = 1;

		foreach (ob : obs) {
		    if (ob != who &&
		      (see_level >= 6 && !ob->query(LIV_IS_PLAYER)) &&
		      (who->query_coder_level() ||
			member(({IL_SNEAKING, IL_HIDING, IL_HIDDEN, IL_TOTALLY_INVISIBLE}),
			  ob->query_invis()) == -1)) {
			mapping item;

			// We may be hallucinating.
			if (hallucinating && random(100) > 74) {
			    short_str = (string) HALLU_D->random_short(1);
			} else {
			    short_str = (string) ob->query_short(see_level, who);
			}

			item = ([
			  GMCP_KEY_CHAR_ITEMS_ITEMS_ID: hash(TLS_HASH_SHA1, object_name(ob)),
			  GMCP_KEY_CHAR_ITEMS_ITEMS_NAME: short_str,
			  GMCP_KEY_CHAR_ITEMS_ITEMS_ICON: 0,
			  GMCP_KEY_CHAR_ITEMS_ITEMS_ATTRIB: ob->query_worn() ? "w" // Not possible?
			  : ((ob->query_armour() || ob->query_clothing()) ? "W"
			    : (ob->query_wielded() ? "l" // Not possible?
			      : (ob->can_put_and_get() ? "c"
				: (ob->get(1) ? "t"
				  : (ob->query(LIV_IS_NPC) ? "m" // Not possible?
				    : (ob->query_corpse() ? "d"
				      : ""))))))
			]);

#if 0
		    {
#endif
			if (!items || !sizeof(items)) {
			    items = ({ item });
			} else {
			    items += ({ item });
			}
		    }
		}
	    }
	}

	items_list = ([ GMCP_KEY_CHAR_ITEMS_LOCATION: location, GMCP_KEY_CHAR_ITEMS_ITEMS: items ? items : "" ]);

	break;
    }

    who->gmcp_message(GMCP_PKG_CHAR_ITEMS_LIST, items_list);
}

/*
**   Function: send_char_items_remove
**
**    Returns: void
**
** Parameters: object who, the player object who the data is queries from as well
**             as GMCP message is sent to.
**
**             string location, "room", "inventory" or a container that the player
**             has in their possession.
**
**             object ob, the object being removed from the location..
**
**    Purpose: This will enable clients to display tables of items that are relevant
**             to the players environment and allow them to perform actions upon
**             them and display relevant icons.
*/
nomask void
send_char_items_remove(object who, string location, object ob) {
    mapping item;
    string short_str;
    int see_level, hallucinating;

    if (!who->query_can_move() || who->test_dark()) return;

    if (who->query_coder_level()) see_level = IL_TOTALLY_INVISIBLE;
    else if (who->query_condition(C_DETECT_INVIS)) see_level = IL_INVISIBLE;
    else see_level = IL_VISIBLE;

    see_level = who->test_dark2(environment(who)->query_total_light(),
      environment(who)->query_light_level(),
      environment(who)->query(ROOM_WD));

    if (see_level < 0) see_level = -see_level;

    // If we can see objects
    if (see_level >= 4) {
	if (who->query_condition(C_HALLUCINATING)) hallucinating = 1;

	if (ob != who
	  && (see_level >= 6
	    && (!living(ob) || !ob->query(LIV_IS_PLAYER_OR_NPC) || !ob->query(LIV_UNDEAD)))
	  && (who->query_coder_level()
	    || member(({IL_SNEAKING, IL_HIDING, IL_HIDDEN, IL_TOTALLY_INVISIBLE}),
	      ob->query_invis()) == -1)) {
	    // We may be hallucinating.
	    if (hallucinating && random(100) > 74) {
		short_str = (string) HALLU_D->random_short(1);
	    } else {
		short_str = (string) ob->query_short(see_level, who);
	    }

	    item = ([ GMCP_KEY_CHAR_ITEMS_LOCATION: location,
	      GMCP_KEY_CHAR_ITEMS_ITEM: ([
		GMCP_KEY_CHAR_ITEMS_ITEMS_ID: hash(TLS_HASH_SHA1, object_name(ob)),
		GMCP_KEY_CHAR_ITEMS_ITEMS_NAME: short_str,
		GMCP_KEY_CHAR_ITEMS_ITEMS_ICON: 0,
		GMCP_KEY_CHAR_ITEMS_ITEMS_ATTRIB: ob->query_worn() ? "w"
		: ((ob->query_armour() || ob->query_clothing()) ? "W"
		  : (ob->query_wielded() ? "l"
		    : (ob->can_put_and_get() ? "c"
		      : (ob->get(1) ? "t"
			: (ob->query(LIV_IS_NPC) ? "m"
			  : (ob->query_corpse() ? "d"
			    : "")))))),
	      ])
	    ]);

	    who->gmcp_message(GMCP_PKG_CHAR_ITEMS_REMOVE, item);
	}
    }
}

/*
**   Function: send_char_items_update
**
**    Returns: void
**
** Parameters: object who, the player object who the data is queries from as well
**             as GMCP message is sent to.
**
**             object ob, the object being updated within the player's inventory.
**
**    Purpose: This will enable clients to display tables of items that are relevant
**             to the players environment and allow them to perform actions upon
**             them and display relevant icons.
**
**       Note: This is only sent for items within ones inventory!
*/
nomask void
send_char_items_update(object who, object ob) { // 2rd parameter should be the item "id" per the spec, maybe later?
    mapping item;
    string short_str;
    int see_level, hallucinating;

    if (!who->query_can_move() || who->test_dark()) return;

    if (who->query_coder_level()) see_level = IL_TOTALLY_INVISIBLE;
    else if (who->query_condition(C_DETECT_INVIS)) see_level = IL_INVISIBLE;
    else see_level = IL_VISIBLE;

    see_level = who->test_dark2(environment(who)->query_total_light(),
      environment(who)->query_light_level(),
      environment(who)->query(ROOM_WD));

    if (see_level < 0) see_level = -see_level;

    // If we can see objects
    if (see_level >= 4) {
	if (who->query_condition(C_HALLUCINATING)) hallucinating = 1;

	if (ob != who
	  && (see_level >= 6 && !ob->query(LIV_IS_PLAYER))
	  && (who->query_coder_level()
	    || member(({IL_SNEAKING, IL_HIDING, IL_HIDDEN, IL_TOTALLY_INVISIBLE}),
	      ob->query_invis()) == -1)) {
	    // We may be hallucinating.
	    if (hallucinating && random(100) > 74) {
		short_str = (string) HALLU_D->random_short(1);
	    } else {
		short_str = (string) ob->query_short(see_level, who);
	    }

	    item = ([ GMCP_KEY_CHAR_ITEMS_LOCATION: GMCP_VALUE_CHAR_ITEMS_LOCATION_INV,
	      GMCP_KEY_CHAR_ITEMS_ITEM: ([
		GMCP_KEY_CHAR_ITEMS_ITEMS_ID: hash(TLS_HASH_SHA1, object_name(ob)),
		GMCP_KEY_CHAR_ITEMS_ITEMS_NAME: short_str,
		GMCP_KEY_CHAR_ITEMS_ITEMS_ICON: 0,
		GMCP_KEY_CHAR_ITEMS_ITEMS_ATTRIB: ob->query_worn() ? "w"
		: ((ob->query_armour() || ob->query_clothing()) ? "W"
		  : (ob->query_wielded() ? "l"
		    : (ob->can_put_and_get() ? "c"
		      : (ob->get(1) ? "t"
			: (ob->query(LIV_IS_NPC) ? "m"
			  : (ob->query_corpse() ? "d"
			    : "")))))),
	      ])
	    ]);

	    who->gmcp_message(GMCP_PKG_CHAR_ITEMS_UPDATE, item);
	}
    }
}

/*
**   Function: send_char_skills_groups
**
**    Returns: void
**
** Parameters: object who, the player object who the data is queries from as well
**             as GMCP message is sent to.
**
**    Purpose: Groups of skills available to the character.
**
** Char.Skills.Groups [
**  {"name": "combat.offense", "rank": "Unearthly (1%)"},
**  {"name": "combat.weapon", "rank": "Mythical (15%)"}
** ]
*/
nomask void
send_char_skills_groups(object who) {
    mixed *skills_groups;

    who->gmcp_message(GMCP_PKG_CHAR_SKILLS_GROUPS, skills_groups);
}

/*
**   Function: send_char_skills_list
**
**    Returns: void
**
** Parameters: object who, the player object who the data is queries from as well
**             as GMCP message is sent to.
**
**             string group, a group of related skills
**
**    Purpose: List of skills in a group available to the character.
**
** Char.Skills.List {
**  "group": "combat.weapon",
**  "list": ["1h axe", "1h blunt", "1h edged", "1h pointed"],
**  "descs": ["Gimli likes axes.", "Perrin prefers the hammer.", "This is a sword.", "Maybe a dagger.""]
** }
*/
nomask void
send_char_skills_list(object who, string group) {
    mapping skills_list;

    who->gmcp_message(GMCP_PKG_CHAR_SKILLS_LIST, skills_list);
}

/*
**   Function: send_char_skills_info
**
**    Returns: void
**
** Parameters: object who, the player object who the data is queries from as well
**             as GMCP message is sent to.
**
**             string group, a group of related skills
**
**             string skill, a group of related skills
**
**    Purpose: Information about a single skills, only sent upon request.
**
** Char.Skills.Info {
**  "group": "combat.magic",
**  "skill": "fireball",
**  "info": "SYNTAX: cast fireball [at <target>]\nUsing this ability you could start forest fire."
** }
*/
nomask void
send_char_skills_info(object who, string group, string skill) {
    mapping skills_info;

    who->gmcp_message(GMCP_PKG_CHAR_SKILLS_INFO, skills_info);
}

/*
**   Function: send_comm_channel_list
**
**    Returns: void
**
** Parameters: object who, the player object who the data is queries from as well
**             as GMCP message is sent to.
**
**    Purpose: List of communication channels available to the player, sent on
**             login/negotiation and if changed.
**
** Comm.Channel.List [
**  {"name": "stick", "caption": "Stick", "command": "channel stick send"},
**  {"name": "priest", "caption": "Priest", "command": "channel priest send"},
** ]
*/
nomask void
send_comm_channel_list(object who) {
    mixed *channel_list = CHANNEL_D->query_gmcp_channels(who);

    who->gmcp_message(GMCP_PKG_COMM_CHANNEL_LIST, channel_list);
}

/*
**   Function: send_comm_channel_list
**
**    Returns: void
**
** Parameters: object who, the player object who the data is queries from as well
**             as GMCP message is sent to.
**
**             int refresh, whether to force a refresh of the data
**
**    Purpose: List of players and channels that they share with this player.
**
** Comm.Channel.Players [
**  {"name": "Tamarindo", "channels": ["stick", "bs", "thief"]},
**  {"name": "Sabre", "channels": ["stick", "thief"]},
** ]
*/
nomask varargs void
send_comm_channel_players(object who, int refresh) {
    mixed *channel_players;
    mapping channel_data = CHANNEL_D->query_gmcp_players(who);

    foreach (string key : channel_data) {
	if (!channel_players)
	    channel_players = ({ ([ GMCP_KEY_COMM_CHANNEL_NAME: key, GMCP_KEY_COMM_CHANNEL_CHANNELS: channel_data[key] ]) });
#if 0
	{
#endif
	else channel_players += ({ ([ GMCP_KEY_COMM_CHANNEL_NAME: key, GMCP_KEY_COMM_CHANNEL_CHANNELS: channel_data[key] ]) });
#if 0
	{
#endif
    }

    who->gmcp_message(GMCP_PKG_COMM_CHANNEL_PLAYERS, channel_players, refresh);
}

/*
**   Function: send_comm_channel_text
**
**    Returns: void
**
** Parameters: object who, the player object who the data is queries from as well
**             as GMCP message is sent to.
**
**             string channel, the handle of the channel
**
**             string talker, who is sending a message
**
**             string text, the message being sent
**
**    Purpose: The text of the communication that you heard, who spoke, and which
**             channel it was on.
**
** Comm.Channel.Text {
**  "channel": "tell",
**  "talker": "Tamarindo",
**  "text": "You tell Filth: What a nice statue you have there."
** }
*/
nomask void
send_comm_channel_text(object who, string channel, string talker, string text) {
    mapping channel_text = ([
      GMCP_KEY_COMM_CHANNEL_CHANNEL: channel,
      GMCP_KEY_COMM_CHANNEL_TALKER: talker,
      GMCP_KEY_COMM_CHANNEL_TEXT: text,
    ]);

    who->gmcp_message(GMCP_PKG_COMM_CHANNEL_TEXT, channel_text);
}

/*
**   Function: send_group
**       
**    Returns: void
** 
** Parameters: object who, the player object who the data is queries from as well
**             as GMCP message is sent to.
**      
**    Purpose: Group messages are only sent if the group has changed or the vitals/stats
**             of a group member have changed.  The GMCP group information consists of two main
**             sections, the group header and an array of members.
**
**      Notes: This follows a similar pattern used by Aardwolf.  Reference:
**             http://www.aardwolf.com/wiki/index.php/Clients/GMCP
**
** Group {
**  "groupname": "Knights of the Round Table",
**  "leader": "Arthur",
**  "created": "01 May 12:03",
**  "status": "Private",
**  "count": 2,
**  "kills": 0,
**  "exp": 0,
**  "members": [
**  {
**    "name": "Arthur",
**    "info": {
**      "hp": 500,
**      "maxhp": 500,
**      "sp": 450,
**      "maxsp": 500,
**      "fp": 475,
**      "maxfp": 500,
**      "lvl": 100,
**      "here": "Yes"
**    }
**   },
**  {
**    "name": "Lancelot",
**    "info": {
**      "hp": 550,
**      "maxhp": 600,
**      "sp": 237,
**      "maxsp": 300,
**      "fp": 450,
**      "maxfp": 500,
**      "lvl": 100,
**      "here": "No"
**    }
**   }
**  ]
** }
*/
nomask void
send_group(object who) {
    mixed group;
    int party_id;

    if (party_id = PARTY_D->query_party(who)) {
	mixed *members;
	object *players = PARTY_D->query_party_members(party_id);
	string group_name = PARTY_D->query_party_title(party_id);
	object party_leader = PARTY_D->query_party_leader(party_id);

	group = ([
	  GMCP_KEY_GROUP_GROUPNAME: group_name ? group_name : "",
	  GMCP_KEY_GROUP_LEADER: party_leader ? party_leader->query_real_name() : "",
	  //GMCP_KEY_GROUP_CREATED: "01 May 12:03",
	  //GMCP_KEY_GROUP_STATUS: "Private",
	  GMCP_KEY_GROUP_COUNT: sizeof(players),
	  //GMCP_KEY_GROUP_KILLS: 0
	  GMCP_KEY_GROUP_EXP: PARTY_D->query_members_party_exp(party_id),
	]);

	foreach (object player : players) {
	    if (!members || !pointerp(members) || !sizeof(members)) {
		members = ({ ([
		    GMCP_KEY_GROUP_MEMBERS_NAME: player->query_real_name(),
		    GMCP_KEY_GROUP_MEMBERS_INFO: ([
		      GMCP_KEY_GROUP_MEMBERS_INFO_HP: player->query_hp(),
		      GMCP_KEY_GROUP_MEMBERS_INFO_MAXHP: player->query_max_hp(),
		      GMCP_KEY_GROUP_MEMBERS_INFO_SP: player->query_sp(),
		      GMCP_KEY_GROUP_MEMBERS_INFO_MAXSP: player->query_max_sp(),
		      GMCP_KEY_GROUP_MEMBERS_INFO_FP: player->query_fp(),
		      GMCP_KEY_GROUP_MEMBERS_INFO_MAXFP: player->query_max_fp(),
		      GMCP_KEY_GROUP_MEMBERS_INFO_LEVEL: player->query_level(),
		      GMCP_KEY_GROUP_MEMBERS_INFO_HERE: (present(player, environment(who)) ? "Yes" : "No"),
		    ])
		  ]) });
	    } else {
		members += ({ ([
		    GMCP_KEY_GROUP_MEMBERS_NAME: player->query_real_name(),
		    GMCP_KEY_GROUP_MEMBERS_INFO: ([
		      GMCP_KEY_GROUP_MEMBERS_INFO_HP: player->query_hp(),
		      GMCP_KEY_GROUP_MEMBERS_INFO_MAXHP: player->query_max_hp(),
		      GMCP_KEY_GROUP_MEMBERS_INFO_SP: player->query_sp(),
		      GMCP_KEY_GROUP_MEMBERS_INFO_MAXSP: player->query_max_sp(),
		      GMCP_KEY_GROUP_MEMBERS_INFO_FP: player->query_fp(),
		      GMCP_KEY_GROUP_MEMBERS_INFO_MAXFP: player->query_max_fp(),
		      GMCP_KEY_GROUP_MEMBERS_INFO_LEVEL: player->query_level(),
		      GMCP_KEY_GROUP_MEMBERS_INFO_HERE: (present(player, environment(who)) ? "Yes" : "No"),
		    ])
		  ]) });
	    }
	}

	group[GMCP_KEY_GROUP_MEMBERS] = members;
    } else group = "";

    who->gmcp_message(GMCP_PKG_GROUP, group);
}

/*
**   Function: send_room_add_player
**
**    Returns: void
**
** Parameters: object who, the player object who the data is queries from as well
**             as GMCP message is sent to.
**
**             object player, the player who arrived in the environment of who.
**
**    Purpose: Sends a message when a player arrives.
**
** Room.AddPlayer {
**  "name": "Tamarindo",
**  "fullname": "Admin Tamarindo"
** }
*/
nomask void
send_room_add_player(object who, object player) {
    int see_ilevel, see_level, hallucinating;
    mapping player_data;
    string short_str;

    if (interactive(who) && who->query_coder_level())
	see_ilevel = IL_TOTALLY_INVISIBLE;
    else see_ilevel = IL_VISIBLE;

    see_level = (int) who->test_dark2((int) environment(who)->query_total_light(),
      (int) environment(who)->query_light_level(),
      (int) environment(who)->query(ROOM_WD));

    if (see_level < 0) see_level = -see_level;

    // We can see players
    if (see_level >= 4) {
	if (who->query_condition(C_HALLUCINATING)) hallucinating = 1;

	// We may be hallucinating.
	if (hallucinating && random(100) > 74) {
	    short_str = (string) HALLU_D->random_short(1);
	} else {
	    short_str = (string) player->short(see_ilevel, who);
	}

	player_data = ([ "name": player->query_real_name(), "fullname": short_str ]);
    }

    who->gmcp_message(GMCP_PKG_ROOM_ADDPLAYER, player_data);
}

/*
**   Function: send_room_info
**
**    Returns: void
**
** Parameters: object who, the player object who the data is queries from as well
**             as GMCP message is sent to.
**
**    Purpose: Contains information about the room a player is in that may likely be
**             picked up by the mapper installed in the client.
**
** Room.Info {
**  map = "", -- Could be a URL to show an HTML map of the given area.
**  num = 1, -- The unique room number of Temple Yard
**  name = "Temple yard", -- The brief name of the room
**  environment = "City", -- The environment the room is in
**  details = "", -- bank, pub, post office...
**  coords = "0,0,0", -- x,y,z coordinates on the map
**  exits = {
**    northwest = 2, -- wedding chapel
**    west = 5, -- race hall
**    south = 6, -- north of the well
**    east = 3, -- door to Leo
**    north = 4 -- to the grave stone
**  },
**  area = "Temple of Tristeza" -- area we are in
** }
*/
nomask void
send_room_info(object who) {
    //int see_level;
    mapping map_room, room_info;
    /*
	see_level = (int) who->test_dark2((int) environment(who)->query_total_light(),
	  (int) environment(who)->query_light_level(),
	  (int) environment(who)->query(ROOM_WD));

	if (see_level < 0) see_level = -see_level;

	// We can see
	if (see_level < 5) {
		// This is a placeholder.  If we want to hide the name of the room, exits or other details we could do a test below.
	}
    */
    // TODO: Optimize second call below to MAPPER_D
    if (map_room = MAPPER_D->query_map_room(environment(who))) {
	room_info = ([
	  GMCP_KEY_ROOM_INFO_NUM: map_room["num"] ? map_room["num"] : "",
	  GMCP_KEY_ROOM_INFO_NAME: map_room["name"] ? map_room["name"] : "",
	  GMCP_KEY_ROOM_INFO_AREA: map_room["area"] ? (map_room["area"] > 0 ? MAPPER_D->query_map_area_name(map_room["area"]) : "") : "",
	  GMCP_KEY_ROOM_INFO_ENVIRONMENT: map_room["env"] ? (map_room["env"] > 0 ? MAP_ENVIRONMENTS[map_room["env"]]["name"] : "") : "",
	  GMCP_KEY_ROOM_INFO_COORDS: map_room["coords"] ? sprintf("%d,%d,%d", map_room["coords"][0], map_room["coords"][1], map_room["coords"][2]) : "",
	  GMCP_KEY_ROOM_INFO_MAP: map_room["map"] ? map_room["map"] : "", // Intended to be a URL to MMP, often area specific.  Not yet supported.
	  GMCP_KEY_ROOM_INFO_EXITS: map_room["exits"] ? map_room["exits"] : "",
	  GMCP_KEY_ROOM_INFO_DETAILS: map_room["details"] ? map_room["details"] : "",
	]);

	who->gmcp_message(GMCP_PKG_ROOM_INFO, room_info);
    }
}

/*
**   Function: send_room_players
**
**    Returns: void
**
** Parameters: object who, the player object who the data is queries from as well
**             as GMCP message is sent to.
**
**    Purpose: Typically sent to the client when this player object enters the
**             room to see a list of who is there.
**
** Room.Players [
**  {
**   "name": "Sabre",
**   "fullname": "Co-Admin Sabre"
**  },
**  {
**   "name": "Germ",
**   "fullname": "Germ"
**  }
** ]
*/
nomask void
send_room_players(object who) {
    int see_ilevel, see_level, i, is_living, hallucinating;
    object ob;
    mixed *inv, *room_players;
    string short_str;

    if (interactive(who) && who->query_coder_level())
	see_ilevel = IL_TOTALLY_INVISIBLE;
    else see_ilevel = IL_VISIBLE;

    see_level = (int) who->test_dark2((int) environment(who)->query_total_light(),
      (int) environment(who)->query_light_level(),
      (int) environment(who)->query(ROOM_WD));

    if (see_level < 0) see_level = -see_level;

    // We can see and there is more than us in the room
    if (see_level > 4 && sizeof(inv = all_inventory(environment(who))) > 1) {
	if (who->query_condition(C_HALLUCINATING)) hallucinating = 1;

	for (i = 0; i < sizeof(inv); i++) {
	    // So; with see_level of 7 we see all the objects, with 4-6 only
	    // non-undead NPCs and players, otherwise nothing...
	    if ((ob = inv[i]) == who || (see_level < 7
		&& (!living(ob) || !ob->query(LIV_IS_PLAYER_OR_NPC)
		  || ob->query(LIV_UNDEAD)))) {
		continue;
	    }

	    // We may be hallucinating.
	    if (hallucinating && random(100) > 74) {
		if (living(ob)) short_str = (string) HALLU_D->random_short(1);
		else short_str = (string) HALLU_D->random_short(2);
	    } else short_str = (string) ob->short(see_ilevel, who);

	    // Now filter for players only
	    if (ob->query(LIV_IS_PLAYER)) {
		if (!room_players || !sizeof(room_players)) {
		    room_players = ({ ([ "name": ob->query_real_name(), "fullname": short_str ]) });
		} else {
		    room_players += ({ ([ "name": ob->query_real_name(), "fullname": short_str ]) });
		}
	    }
	}
    }

    who->gmcp_message(GMCP_PKG_ROOM_PLAYERS, room_players ? room_players : "");
}

/*
**   Function: send_room_remove_player
**
**    Returns: void
**
** Parameters: object who, the player object who the data is queries from as well
**             as GMCP message is sent to.
**
**             object player, the player who left in the environment of who.
**
**    Purpose: Sends a message when a player leaves.
**
** Room.RemovePlayer "Tamarindo"
*/
nomask void
send_room_remove_player(object who, object player) {
    int see_ilevel, see_level, hallucinating;
    string player_data;
    string short_str;

    if (interactive(who) && who->query_coder_level())
	see_ilevel = IL_TOTALLY_INVISIBLE;
    else see_ilevel = IL_VISIBLE;

    see_level = (int) who->test_dark2((int) environment(who)->query_total_light(),
      (int) environment(who)->query_light_level(),
      (int) environment(who)->query(ROOM_WD));

    if (see_level < 0) see_level = -see_level;

    // We can see players
    if (see_level >= 4) {
	player_data = player->query_real_name();
    }

    who->gmcp_message(GMCP_PKG_ROOM_REMOVEPLAYER, player_data);
}

/*
/*
**   Function: send_room_wrong_direction
**
**    Returns: void
**
** Parameters: object who, the player object who the data is queries from as well
**             as GMCP message is sent to.
**
**             string direction, the direction we tried to go..
**
**    Purpose: Sends a message when who can't go in that direction.  Perhaps this
**             could be use to stop speedwalking when something changed with the
**             exits, like a door is shut.
**
** Room.WrongDir "northeast"
*/
nomask void
send_room_wrong_direction(object who, string direction) {
    who->gmcp_message(GMCP_PKG_ROOM_WRONGDIR, direction);
}

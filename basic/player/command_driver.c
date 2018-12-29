/****************************************************************
*								*
* Module name: command_driver					*
*								*
* Description: Handles input from players/mobiles; calls the	*
*	appropriate functions if they are available for the	*
*	object.							*
*								*
* Included/inheritted by /obj/player.c. NPCs have a customized	*
* version of this module to allow them use commands in /bin/pub.*
*								*
* Version 1.3 / 28-jan-96					*
*								*
* Last modifications by Doomdark 10-Jan-98			*
*								*
*	NEW:							*
*								*
* - Commands from guild object will be called automagically	*
*   without need for add_action's. Later we'll modify this	*
*   system to take care of preventing normal attacks while	*
*   casting spells or using special skills.			*
* - Commands aren't stored as mappings any more; instead, we'll	*
*   use command paths. This way we are hoping to save memory.	*
* - Commands from lord crown moved to /bin/lord; they are called*
*   like commands in /bin/pub & /bin/usr for players over	*
*   level 24.							*
* - In-built commands now handled through closures; much more	*
*   efficient!							*
* 10-Jan-98, Doomdark: Added support for client-originated	*
*	commands. They look much like HTML-tags; <SET xxx=yyy>	*
*	and so on.						*
*								*
****************************************************************/

// private static status commands_loaded;
private static int cmds_available;
// static mapping cmd;
static string *cmds;	// New! Saves memory.
static object cmdd;	// New. Should make call_others bit faster...
private static mapping InbuiltCmds;
			// Should make things faster as well...

#include <coder_levels.h>
#include <daemons.h>

#define	PLAYER_CMDS	1
#define	CODER_CMDS	2
#define	ADMIN_CMDS	4
#define STD_CMD_PATH "/bin/pub:/bin/lord"
#define	WIZ_CMD_PATH	"/bin/usr"
#define	GET_PATH()	query_env("path")
#define	SET_PATH(P)	set_env("path", (P))
#define	CMDD	"/bin/daemons/cmdd"

#ifndef PLAYER_C
#ifndef LIVING_C

#include <player_defs.h>
#include "/basic/player/player_cmds.c"
#include "/basic/player/cmd_files.c"
#include "/basic/player/coder_cmds.c"
#include "/basic/player/interact_cmds.c"
#include "/basic/player/misc_cmds.c"
#include "/basic/player/lord_handling.c"
#include "/basic/player/rsay.c"
#include "/basic/player/desc.c"

static object guild_object;
static mapping guild_commands;
int coder_level;

int query_level();
status query_can_move();
int query(int x);
varargs void tell_me(string s, status a, status b);
status save_character();
string get_env(string s);
void set_env(string s, mixed x);
mixed do_client_command(string cmd, string arg);

#endif
#endif

nomask void
rehash_commands()
{
string path;

	if (!cmdd) return;	// If no command-daemon, can't proceed.

	if (1) {

// Let's only load commands for players; monsters can only use those found
// from /bin/pub and /bin/skills.
		InbuiltCmds = ([
"kill"		: #'kill_cmd,
"converse"	: #'converse_cmd,
"stop"		: #'stop_cmd,
"quit"		: #'quit,
"email"		: #'email_cmd,
"password"	: #'change_password,
"save"		: #'save_character,
"say"		: #'communicate,
"'"		: #'communicate,
"ignore"	: #'ignore_cmd,
"allow"		: #'allow_cmd,
"whisper"	: #'whisper_cmd,
"lich"		: #'lich_cmd,
"lichon"	: #'lichon_cmd,
"liege"		: #'liege_cmd,
"lichoff"	: #'lichoff_cmd,
"rsay"		: #'rsay_cmd,
"desc"		: #'desc_cmd,
		]);

		path = GET_PATH();
		cmds_available |= PLAYER_CMDS;
		if (coder_level > 0) {
			cmds_available |= CODER_CMDS;
			InbuiltCmds += ([
"banlist"	: #'banlist_wizcmd,
"wizlist"	: #'wizlist_wizcmd,
"localcmd"	: #'localcmd_wizcmd,
"snoop"		: #'snoop_wizcmd,
"earmuffs"	: #'earmuffs_wizcmd,
"ed"		: #'ed_wizcmd,
"cd"		: #'_file_cmd,
"cp"		: #'_file_cmd,
"mv"		: #'_file_cmd,
"rm"		: #'_file_cmd,
"rmdir"		: #'_file_cmd,
"pwd"		: #'_file_cmd,
"mkdir"		: #'_file_cmd,
"tail"		: #'_file_cmd,
"cat"		: #'_file_cmd,
"chown"		: #'_file_cmd,
"chmod"		: #'_file_cmd,
"exprate"       : #'_file_cmd,
"banish"	: #'banish_wizcmd,
"shutdown"	: #'shutdown_wizcmd
			]);
			if (IS_ADMIN(coder_level)) {
				InbuiltCmds += ([
				]);
				cmds_available |= ADMIN_CMDS;
			}
		} else if (!coder_level)
			SET_PATH(path = 0);

// Previous line is to prevent cheating, while still allowing testplayers to
// use non-standard commands.

		if (path) cmds = (string*) CMD_D->query_paths(path, this_object());
		else cmds = 0;
	}
}

nomask status
command_driver(string arg)
{
string verb, tmp;
int i;
mixed x;
closure func;
// Need to do a kludge to prevent forcing us. Let's see if this works. :-/
	if (coder_level && this_interactive() != this_object()) return 0;
	if (!(verb = query_verb())) return 0; // Impossible?

	if ((i = verb[0]) == ':') {
		if (sizeof(verb) > 1)
			if (arg)
				arg = sprintf("%s %s", verb[1..<1], arg);
			else arg = verb[1..<1];
		verb = "emote";
/* NEW! Client may use special html-attribute-like commands; they begin
 * with '<'. Of course, player might manually use it all by him/herself too
 * but it's mainly tailored for client-programs' needs
 */
	} else if (i == '<') do {
	  if (arg) {
	    if (arg[<1] != '>')
		break;
	    arg = arg[0..<2];
	  } else {
	    if (verb[<1] != '>')
		break;
	    verb = verb[0..<2];
	  }
	// Ok, now we're sure to have a client-command!
	  x = do_client_command(verb[1..], arg);
	  if (stringp(x))
		tell_me(x, 0, 0, 0, 0, 0);
	  return 1;
	} while (0);

// New: Guild commands!

	if (guild_commands) {
		if (!guild_object || !mappingp(guild_commands))
			guild_commands = 0;
// Just checking; might speed up things after resigning etc.
		else if (tmp = guild_commands[verb]) {
			if (closurep(tmp))
				x = funcall(tmp, arg);
			else
				x = (int) call_other(guild_object, tmp, arg);
			if (x) return x;
		}
	}

// Is this the first time we're called? (handled this way so that monsters
// load the commands only if they really use them. -Doomdark).
// Of course, this only applies to command in /bin etc...

	if (!cmdd) {
		if (!(cmdd = (object) CMDD->cmdd()) && !(cmdd = find_object(CMDD)))
			return notify_fail("<ERROR> Standard commands missing!\n"), 0;
		rehash_commands();
	}

	switch(verb) {
	case "north": case "east": case "south": case "west":
	case "northeast": case "southeast": case "southwest": case "northwest":
	case "exit": case "out": case "enter": case "up": case "down": 
	notify_fail(query(LIV_IS_FLEEING) ? "" : "You can't go that way.\n");
	   if (!query_can_move()) {
	      tell_me("You are unable to move!");
	      return 1;
	   }
           return 0;
	   break;
	case "take": verb = "get"; break;
	case "describe": verb = "desc"; break;
	case "destruct": verb = "dest"; break;
//	case "destr": case "dest": verb = "destruct"; break;
	case "wizem": verb = "wiz"; break;
	case "i": verb = "inventory"; break;
	case "consider": verb = "cons"; break;
	case "verbose": verb = "brief"; break;
	case "sc": verb = "score"; break;
	case "l": verb = "look"; break;
	case "exa": case "exam": case "exami": case "examin": verb = "examine";
	}

// Neat and simple now:
	if (func = InbuiltCmds[verb]) {
		if (x = funcall(func, arg, verb))
			return x;
	}

// New. We won't have to check if command is ok, let command daemon
// check that out. May be a bit slower, but saves memory (not having to
// use all these mappings etc.).

	if (cmdd)
		return (int) cmdd->do_command(cmds, verb, arg, (coder_level > 0 ? coder_level : 0));

	return 0;
}

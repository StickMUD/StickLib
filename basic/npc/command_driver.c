/****************************************************************
*								*
* Module name: command_driver					*
*	Custom version for NPCs. Doomdark.			*
*								*
* Description: Handles input from players/mobiles; calls the	*
*	appropriate functions if they are available for the	*
*	object.							*
*								*
* Included/inheritted by /obj/living.c; both players and	*
* mobiles can use 'general' commands found in /bin/pub; also,	*
* coders can use commands from /bin/usr.			*
*								*
* Version 1.3b / 11-may-95					*
*								*
* Last modifications by Doomdark 11-may-95			*
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
*   level 20.							*
* - Different command parsers for npcs and players.		*
*								*
****************************************************************/

private static int cmds_available;
static object cmdd;	// New. Should make call_others bit faster...

#include <coder_levels.h>
#include <daemons.h>

#define	CMDD	"/bin/daemons/cmdd"

#ifndef NPC_C
#ifndef LIVING_C

#include <npc_defs.h>

static status is_fleeing;
static object guild_object;
static mapping guild_commands;

int query_coder_level();
int query(int x);
int query_level();
status query_can_move();
varargs void tell_me(string s, status a, status b);
#endif
#endif

nomask status
command_driver(string arg)
{
    string verb, tmp;
    int x;

    if (!(verb = query_verb())) return 0; // Impossible?

    if (verb[0] == ':') {
	if (sizeof(verb) > 1)
	    if (arg) arg = verb[1..<1] + " " + arg;
	    else arg = verb[1..<1];
	verb = "emote";
    }

    // Is this the first time we're called? (handled this way so that monsters
    // load the commands only if they really use them. -Doomdark).
    // Of course, this only applies to command in /bin etc...

    if (!cmdd) {
	if (!(cmdd = (object) CMDD->cmdd()) && !(cmdd = find_object(CMDD)))
	    return notify_fail("<ERROR> Standard commands missing!\n"), 0;
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

    // New. We won't have to check if command is ok, let command daemon
    // check that out. May be a bit slower, but saves memory (not having to
    // use all these mappings etc.).

    if (cmdd)
	return (int) cmdd -> do_command(0, verb, arg);

    return 0;
}

/****************************************************************
*								*
* Module:							*
*	/basic/player/client.c					*
* Description:							*
*	This module handles those commands that are intended	*
*	solely for supporting client-programs. These commands	*
*	look much like HTML-tags; they can be used manually	*
*	by players, but mainly they are used for allowing	*
*	clients to negotiate things with the player object.	*
* Version:							*
*	1.0a 07-jul-97						*
* Last changed:							*
*	10-Jan-98, Doomdark					*
* NEW:								*
*	-							*
****************************************************************/

#if !defined(PLAYER_C) && !defined(LOGIN_C)

#include <player_defs.h>
#include <client_defs.h>
#include <tell_me.h>

//int set_player_value(int v, mixed val);
status set_env(mixed ix, mixed val);
static int _value_plr_client;	// Client player is using, if any.

#endif

int
query_client()
{
    return _value_plr_client;
}

int
set_client(mixed val)
{
    mixed x;
    if (intp(val))
	x = val;
    else if (member(CLIENT_MURDER_ID, val) >= 0)
	x = CLIENT_MURDER;
    else if (!val || member(CLIENT_NONE_ID, val) >= 0)
	x = CLIENT_NONE;
    else return -1;
    return (_value_plr_client = x);
}

mixed
do_client_command(string cmd, string arg)
{
    mixed val;
    mixed x, y;

    if (arg)
	arg = lower_case(arg);

    switch (cmd) {
    case "set":
    case "SET":

	if (sscanf(arg, "%s=%s", arg, val) < 2)
	    return sprintf("ERROR: Client-program sent an invalid set-string: '%s'.",
	      arg);
	switch (arg) {
	case "screen":
	    if (sscanf(val, "%d%t%d", x, y) < 2 &&
	      sscanf(val, "%d,%.0t%d", x, y) < 2)
		return sprintf("ERROR: Client-program tried to set screen size\
 to an invalid value: '%s'.", val);
	    if (x < TELL_MIN_X)
		x = TELL_MIN_X;
	    if (y < TELL_MIN_Y)
		y = TELL_MIN_Y;

	    set_env("columns", x);
	    set_env("rows", y);

	    break;

	case "client":

	    if (set_client(val) < 0)
		return sprintf("ERROR: Trying to set an unknown\
 value ('%s') for attribute 'client'.", val);
	    break;

	default:
	    return sprintf("ERROR: Client program tried to set an unknown value: '%s'."
	      ,arg);
	}
	break;
    default:
	if (arg)
	    return sprintf("ERROR: Client-program sent an unknown command: '%s %s'.",
	      cmd, arg);
	return sprintf("ERROR: Client-program sent an unknown command: '%s'.", cmd);
    }
}

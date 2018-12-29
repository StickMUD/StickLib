/****************************************************************
*								*
* Module:							*
*	/bin/usr/switch.c      					*
* Description:							*
*	The coder command with which a coder to 'change the	*
*	character'; technically speaking to begin controlling	*
*	another interactive object. Usually this is restricted	*
*	so that only certain trusted coders (administrators)	*
*	can do this, and then usually can only switch to	*
*	one of predefined 'alternate' characters; in the case	*
*	of the basic vanilla StickLib, to one of the 'police	*
*	characters', sometimes necessary for punishing cheating	*
*	players or such.					*
* Version:							*
*	1.0a, Aug-98.						*
* Last update:							*
*	23-Aug-98 by Doomdark					*
*								'
* Modifications:						*
*								*
* 23-Aug-98, Doomdark: A new mode; if no argument(s) given	*
*	(ie. plain "switch") will switch to new version of the	*
*	standard player object. This is useful for coders	*
*	after the player object has been updated; there's no	*
*	need to quit + re-login.				*
* 05-Nov-98, Kieve: Fixed up a few bugs in the new mode ;)      *
****************************************************************/

#include <cmd.h>
#include <coder_levels.h>
#include <player_defs.h>
#include <config.h>
#include <city_name.h>

#include "/u/police/police_defs.h"

nomask static void
do_switch(string t, string name)
{
  object p, old;

  old = this_player();
  p = clone_object(PLAYER_FILE);
  if (!exec(p, old))
	return notify_fail("Switch failed!\n"), 0;

  p->initialize(t, "xhohefkmjwfe", 1000, 1000, "police@court.tristeza", 1, 0);
#if 1
  write_file(SWITCHLOG, capitalize(name)+" switched to "+capitalize(t)+" "+
		     ctime(time())+".\n");
#endif
  move_object(p, PATH_FOR_USER("police") + "workroom");
  old->quit();

	if (old)
		destruct(old);
}

nomask int
switch_cmd(string s, object me)
{
string t, name;
object x, y;

	/* We _HAVE_ to be careful with security here... */
	if (!me)
		me = this_player();

	if (!this_interactive() || this_interactive() != me)
		return 0;

  /* NEW! Without an argument, will just switch to a new player object
   * without changing the player character.
   */
	if (!s) {
	  me->save_me();
	  if (s = catch(x = clone_object(PLAYER_FILE))) {
		me->tell_me(sprintf("Couldn't clone " PLAYER_FILE
			", error: %s", s));
		return 1;
	  }
	  if (exec(x, me)) {
	// Args for player.c::initialize():
	//	name, passwd, passtime, lvl, maddr, gen,
	//	interactive_copy, attrs

	/* Actually, _most_ values are really 'dont-care'; only
	 * name is important (for loading the savefile) and
	 * 'interactive_copy' should be set to TRUE so that
	 * the player object won't be moved around.
	 */
		x->initialize(me->query_real_name(),
			0, 0, me->query_level(),
			0, me->query_gender(),
			(environment(me) ? 1 : 0), ([ ]));
		if (environment(me))
		  move_object(x, environment(me));
	  x->tell_me("Ok, switched to use the new " PLAYER_FILE ".");
	/* Hmmh. Some stuff may be autoloaded... So we probably should NOT
	 * just move everything. But for now that has to do:
	 */
                while (y = first_inventory(me)) {
                  if(!y->drop())
		    move_object(y, x);
		  if (environment(y) != x)
			destruct(y);
		}
		destruct(me);
	  } else {
		me->tell_me("Failed to switch to new player object!");
	  }
	  return 1;
	}

	name = (string) me->query_real_name();
	if(!s && member(JUDGES, name) > 0)
		do_switch(JUDGES[name], name);
	if (!s || !sscanf(s, "to %s", t)) return
		notify_fail("Switch to which object?\n"), 0;
	t = lower_case(t);
	if (t != "police" && t != "deputy") {
		this_player()->tell_me(
"Available characters currently: Police, Deputy.");
		return 1;
	}
         if (t == "deputy")
          {
        if ((int) this_player()->query_coder_level() < LVL_SENIOR
	  && member(JUDGES, name) == 0)
        {
          this_player()->tell_me("Only (co)admins and " CITY_NAME
" Secret Police members are allowed to switch to '"+t+"' char!");
                return 1;
         }
	} else
         {
	if ((int) this_player()->query_coder_level() < LVL_COADMIN
	&& member(JUDGES, name) == 0) {
		this_player()->tell_me("Only (co)admins and " CITY_NAME
" Secret Police members are allowed to switch to '"+t+"' char!");
		return 1;
	 }
	}
	if (find_player(t)) {
		this_player()->tell_me(
"Some other coder is already using character '"+t+"'; only one\
 allowed at a time!");
		return 1;
	}
	do_switch(t, name);
}

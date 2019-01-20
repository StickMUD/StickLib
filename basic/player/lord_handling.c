// Moved this to separate files to chop down player.c; preferably
// to under 50k. -+ Doomdark 20-oct-95 +-
// Last Modified: 01-14-98, Kieve

#include "/sys/interactive_info.h"

#ifndef PLAYER_C
#include <mud_name.h>
#include <player_defs.h>
#include <tell_me.h>
#include <room_defs.h>
int coder_level;
string name, cap_name;

varargs int tell_me(string s, status x);
status dump_esc(string s);
int query_lord();
int query(int x);
int set(int x);
string query_real_name();
#endif

#ifndef	MASTER_OB
#define MASTER_OB "/secure/master"
#endif
static string new_liege;
string liege;			// Which Lord are we serving, if any?

// Liege command: player chooses which Lord (Lady) he or she serves.

int
liege_cmd(string arg)
{
    object ob;
    //return notify_fail("Sorry, but there'll be no lieges for now.\n"), 0;
    if (coder_level > 0) {
	notify_fail("You are too powerful to serve others.\n");
	return (liege = 0);
    }
    if (!arg) {
	if (!liege)
	    notify_fail("You are not serving any Lord or Lady.\n");
	else
	    notify_fail("Your liege is " + capitalize(liege) + ".\n");
	return 0;
    }
    new_liege = lower_case(arg);
    if (new_liege == "none") {
	if (!liege)
	    return notify_fail("You didn't have any liege.\n"), 0;
	tell_me("Do you really want to stop serving " + capitalize(liege)
	  + " (y/n)?\n:", 1);
	input_to("liege_none",0);
	return 1;
    }

    if (!(ob = find_player(new_liege)))
	return notify_fail("There is no such Lord or Lady currently\
 in "+MUD_NAME+".\n"), 0;

    if ((int) ob->query_lord() < 4 || (int) ob->query_coder_level())
	return notify_fail("That character cannot be your liege.\n"), 0;

    if ((string) ob->query_liege() == query_real_name()) {
	notify_fail("You cannot be a follower of your own follower!\n");
	return 0;
    }

    if (liege)
	tell_me("\nBut you are already serving " + capitalize(liege)
	  + "!\nDo you really want to change your liege? (y/n)\n:",1);
    else tell_me("\nYou are choosing your Lord or Lady! This action might\
 give you more power, but also it might not be so pleasant. So, are you\
 really sure about this? (And have you tried 'help liege'?) (y/n)");
    tell_me(":",1);
    input_to("liege_cmd2",0); return 1;
    return 1;
}

static int
liege_none(string arg)
{
    if (stringp(arg)) arg = lower_case(arg);
    if (arg != "y" && arg != "yes") {
	tell_me("Ok. Your liege is still " + capitalize(liege) + ".");
	return 1;
    }
    tell_me("You decided to stop serving " + capitalize(liege) + ".");
    MASTER_OB->log_lord(liege, capitalize(name), "BETRAYED YOU");
    /* Log it for lord */
    liege = 0;
    return 1;
}

static int
liege_cmd2(string arg)
{
    object ob, l;
    if (stringp(arg)) arg = lower_case(arg);
    if (arg != "y" && arg != "yes") {
	tell_me("Ok, your liege wasn't changed.");
	return 1;
    }
    if ((ob = find_player(new_liege))) {
	ob -> tell_me("\n\n" + cap_name +" is requesting to become your servant!\n\
If you accept that, type 'accept " + name + "'.\nOtherwise 'reject "
	  + name + "'.\n");
	if (l = present("lord_player_object",ob))
	    l->servant_request(this_object());
    } else {
	tell_me("Oh, that person has just left "+MUD_NAME+" or went link dead -\
 you should try again later.");
	return 1;
    }
    tell_me("\nYou are requesting " + capitalize(new_liege) +
      " to take you into service.\
 That person will consider it for a while. (Keep playing).\n");
    return 1;
}

static object*
liches()
{
    object *u,*ob;
    int i;

    if (environment(this_object())->query(ROOM_ISOLATED)) return ({});

    u = users();
    ob = ({});

    for (i = sizeof(u) - 1; i >= 0; i--)
    {
	if (u[i] != this_object() &&
	  ((liege && (string)u[i]->query_liege() == liege) ||
	    (string)u[i]->query_real_name() == liege ||
	    (string)u[i]->query_liege() == name) &&
	  // On login, before the password was entered, the environemnt
	  // is 0.  Caused a bug to any 'lich' command.
	  // 01-14-98, Kieve
	  environment(u[i]) &&
	  (!environment(u[i])->query(ROOM_ISOLATED) ||
	    this_object()->query_coder_level()))
	    ob += ({ u[i] });
    }

    return ob;
}

// Liege-follower channel
int
lich_cmd(string str)
{
    object *ob;
    int i,dummy;
    string s, t, txt, n;

    if (!liege && query_lord() < 4) {
	tell_me("You must have a liege to use this channel.");
	return 1;
    }

    ob = liches();
    dummy = 0;
    if (!str)
    {
	if (sizeof(ob) < 1)
	{
	    tell_me("There is no one else on the channel now.");
	    return 1;
	}

	for (i = sizeof(ob) - 1; i >= 0; i--) {
	    n = (string) ob[i]->query_real_name();
	    if (!s) s = sprintf("%13s (%s)",
		  capitalize(n), (ob[i]->query(PLR_LICH_ON) ? "on" : "off"));
	    else s += sprintf("\n%13s (%s)",
		  capitalize(n), (ob[i]->query(PLR_LICH_ON) ? "on" : "off"));
	    if (n == liege) s +="\t(Your liege)";
	    if (query_lord() >= 4) {
		if (!interactive_info(ob[i], II_IP_NUMBER)) s += "  (Link dead)";
		else {
		    if (ob[i]->query_ghost()) s += "  (Dead)";
		    else {
			if (t = (string) environment(ob[i])->query_short(0, this_object()))
			    s += sprintf(" (%s)", t);
			else s += " (???)";
		    }
		}
	    }
	}
	if (query(PLR_LICH_OFF))
	    tell_me("(Your channel is off)\n" + s);
	else
	    tell_me(s, 0, TELL_TYPE_HOUSE, 0, 0, 0);
	return 1;
    }

    if (sizeof(ob) < 1) {
	tell_me("No one is on the channel.");
	return 1;
    }

    if (dump_esc(str)) {
	tell_me("Trying to use nasty CTRL-codes! Shame on you...");
	return 1;
    }

    if (!query(PLR_LICH_ON))
	s = "(Your channel is off)";

    txt = sprintf("/@ %s @/: %s", capitalize(name), str);

    for (i = sizeof(ob) - 1; i >= 0; i--) {
	if (ob[i]->query(PLR_LICH_ON)) {
	    ob[i]->tell_me(txt, 0, TELL_TYPE_HOUSE, 0, 0, 0);
	    dummy = 1;
	}
    }

    if (!dummy) tell_me("No one heard you.");
    else tell_me(txt, 0, TELL_TYPE_HOUSE, 0, 0, 0);

    return 1;
}

int
lichon_cmd()
{
    // if (!liege) return notify_fail("You don't have a liege.\n"), 0;
    tell_me("Your liege channel is now on.");
    set(PLR_LICH_ON);
    return 1;
}

int
lichoff_cmd()
{
    // if (!liege) return notify_fail("You don't have a liege.\n"), 0;
    tell_me("Your liege channel is now off.");
    set(PLR_LICH_OFF);
    return 1;
}

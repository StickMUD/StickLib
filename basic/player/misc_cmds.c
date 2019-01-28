/* Miscellaneous commands */

#pragma strict_types

#include <conditions.h>
#include <coder_levels.h>
//#include <macros.h>
#include <std_lib>
#include <player_defs.h>

//#define NAME query_real_name()
#define LEVEL query_level()
#define CODERLVL TOBJ->query_coder_level()

// Session variables...
static string *session_kills;
static int *session_knum, session_exp;

// Ignorings, not saved. There's no need.
static string *ignores;
static string *not_ignoring;
static status ignore_all;

#if 1
// New session _function_, only one. -+ Doomdark +-

varargs mixed
session(string type, string attr, mixed arg, mixed arg2)
{
    int x;
    switch (type) {
    case "query":
	switch (attr) {
	case "exp": return session_exp;
	case "kill": return session_kills;
	case "knum": return session_knum;
	}
	break;
    case "set":
	switch (attr) {
	case "exp":
	    session_exp = arg;
	    break;
	case "kill":
	    session_kills = arg;
	    session_knum = arg2;
	    break;
	}
	break;
    case "add":
	switch (attr) {
	case "exp":
	    session_exp += arg;
	    break;
	case "kill":
	    if (!arg) return;
	    if (!session_kills) {
		session_kills = ({ arg });
		session_knum = ({ 1 });
	    } else if ((x = member(session_kills, arg)) >= 0) {
		session_knum[x]++;
	    } else {
		session_kills += ({ arg });
		session_knum += ({ 1 });
	    }
	    break;
	}
	break;
    }
}

#else
// Old session funcs. *puke*
string *query_session_kills() { return session_kills; }
int *query_session_knums() { return session_knum; }
int query_session_exp() { return session_exp; }
void set_session_exp(int x) { session_exp = x; }
void add_session_exp(int x) { session_exp += x; }
// Don't call this unless you know what you are doing...
void
set_session_kills(string *arg1, int *arg2)
{
    session_kills = arg1;
    session_knum  = arg2;
}
// This is called when we have killed someone.
// Perhaps it's best to use short desc as the "name" here?
// This can be called by others...coders can do funny jokes
// (but don't do such things...)
void
add_session_kill(string name)
{
    int x;
    if (!name) return;
    if (!session_kills) { session_kills = ({}); session_knum = ({}); }
    if ((x = member(session_kills, name)) != -1)
	session_knum[x]++;
    else {
	session_kills += ({ name });
	session_knum += ({ 1 });
    }
}
#endif

status
allow_cmd(string arg)
{
    int i;

    if (!this_object()->query_coder_level())
	return 0;

    if (!arg)
    {
	if (!not_ignoring || (i = sizeof(not_ignoring)) < 1)
	{
	    notify_fail("No allowes through ignore all.\n");
	    return 0;
	}

	for (i--; i >= 0; i--)
	{
	    this_object()->tell_me("Allowing: " + not_ignoring[i]);
	}
	return 1;
    }

    if (!not_ignoring) not_ignoring = ({ });

    if (arg == "none" || arg == "no one" || arg == "nobody")
    {
	not_ignoring = ({ });
	this_object()->tell_me("Allowings cleared.");
	return 1;
    }

    arg = lower_case(arg);

    if (member(not_ignoring, arg) != -1)
    {
	this_object()->tell_me("Already allowing " + arg + ".");
	return 1;
    }

    this_object()->tell_me(
      "Ok. Allowing " + arg + "'s tells etc. to get through.");
    not_ignoring += ({ arg });
    return 1;
}

int
ignore_cmd(string arg)
{
    int i, x;
    object ob;

    if (!ignores) ignores = ({});

    if (!arg) {
	write("You are ignoring:\n");
	if (ignore_all) {
	    write("Everybody! Check 'allow' command.\n");
	    return 1;
	}
	x = 0;
	for (i = 0; i < sizeof(ignores); i++) {
	    if (!ignores[i]) continue;
	    x++;
	    ob = find_player(ignores[i]);
	    if (!ob || (ob->query_invis() &&
		!this_object()->query_coder_level() &&
		(int)this_object()->query_level() < 21)) {
		write("\t" + capitalize(ignores[i]) +
		  " (Doesn't seem to be in the game now)\n");
	    } else write("\t" + capitalize(ignores[i]) + "\n");
	}
	if (!x) write("no one.\n");
	return 1;
    }

    if (arg == "all")
    {
	if (!this_object()->query_coder_level())
	{
	    notify_fail("You can't ignore everyone.");
	    return 0;
	}

	if (!ignore_all)
	{
	    this_object()->tell_me("Ok. Ignoring everyone!");
	    ignore_all = 1;
	}
	else
	{
	    this_object()->tell_me(
	      "Ok. You are NOT ignoring everyone any more.");
	    ignore_all = 0;
	}

	return 1;
    }

    if (ignores) ignores -= ({ 0 });

    if (sizeof(ignores) > 9) {
	write("Remove some of the old ignorations first.\n");
	return 1;
    }

    ignore_all = 0;
    arg = lower_case(arg);

    if ((i = member(ignores, arg)) != -1) {
	write("Ok, not ignoring " + capitalize(arg) + " any more.\n");
	ignores -= ({ arg });
	return 1;
    }

    ob = find_player(arg);

    /* Fix: can ignore non-lord higher level ones. //Graah */
    if (!ob ||
      ((int)ob->query_coder_level() > 0) )
    {
	write("You can't ignore " + capitalize(arg) + ".\n");
	return 1;
    }

    write("Ok. Ignoring " + arg + ".\n");
    ignores += ({ arg });
    return 1;
}

int
query_is_ignoring(string arg)
{
    if (find_player(arg) && find_player(arg)->query_coder_level())
	return 0;

    if (ignore_all &&
      (!not_ignoring ||
	member(not_ignoring, lower_case(arg)) == -1))
	return 1;

    if (ignores && arg &&
      (member(ignores, lower_case(arg)) != -1)) return 1;

    return 0;
}

string *
query_ignores()
{
    if (ignores) return ignores; else return ({});
}

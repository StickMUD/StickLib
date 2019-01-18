// lords_guild.h 
// This is the place where lords can advance their stats and lvls,
// as well as check their quest status, etc.
// 
// Quest-handling recoded 22-Mar-96 by Germ
//
// New features:
//
//	- Specific quests are no longer required per level.
//	  Instead, raising lvl is based only on total quest points.
//	  This allows players to complete quests in any order.
//
//	- The quest daemon now handles all the quest stuff.
//	  The quest room (lords_quest_room) is no longer needed.
//
//	- Quest objects are not used anymore.
//	  All quest info is kept by the quest daemon.
//
// 26-Sep-96 / Graah: Level 25 is the new 1st Lord level!

#include <nroom.h>
#include <areas.h>
#include <quests.h>
#include <daemons.h>
#include <generic_items.h>

// #include "tune.h" // Not needed anymore - Germ
#include <coder_levels.h>
#include <stats.h>

#define LL (int)this_player()->query_lord()
#define MAX_AB 30
#define MAX_THEORETICAL_AB 500

#define L_CROWN_ID      "lord_player_object"

static mapping statList;
static mapping statList2;

void
create_room()
{
    set_short("The Lord's Guild Hall");
    set_long("In this huge hall the lords can check their Lord Quests and\
 advance their abilities to godlike levels.\nCommands: advance [level/str/dex\
/int/con], list [number], cost, form.\nIf you have lost your crown, type\
 'crown'.");
    set(ROOM_WD, WD_INDOORS);
    set_light_level(LT_LIGHTS);
    set(NO_PK);
    set(IN_CITY);
    set_exits(([ "north" : AREA_TRISTEZA+"shops/lords_pub" ]));

    set_commands(([
	"list"	: "list_quests",
	"advance" : "advance",
	"cost"	: "cost",
	"crown"	: "crown",
	"form"	: "form"
      ]));
    statList = ([
      "str"	: ST_BASE_STR,
      "con"	: ST_BASE_CON,
      "dex"	: ST_BASE_DEX,
      "int"	: ST_BASE_INT,
    ]);
    statList2 = ([
      "str"	: ST_STR,
      "dex"	: ST_DEX,
      "con"	: ST_CON,
      "int"	: ST_INT,
      "fat"	: ST_FAT,
    ]);
}


status
crown(string s)
{
    if (s) return notify_fail("Crown what?\n"), 0;
    if (!this_player() -> query_lord()) {
	this_player() -> tell_me("You have to be a lord or a lady to\
 get the crown.");
	return 1;
    }
    if (present(L_CROWN_ID, this_player())) {
	this_player() -> tell_me("You already have a crown.");
	return 1;
    }
    move_object(clone_object(GENERIC_LORD_CROWN), this_player());
    this_player()->tell_me("Ok, new crown given.");
    return 1;
}

status
form(string s)
{
    object form;
    if (s) return notify_fail("Form what?\n"), 0;
    if (LL != 5 || this_player() -> query_coder_level()) {
	this_player() -> tell_me("Since you are not on level 29, you\
 don't need the form.");
	return 1;
    }
    this_player() -> tell_me("Ok. Now give it to a lord/lady of level 35\
 or higher, and ask to sign it.");
    form = clone_object(GENERIC_LORD_BLESS);
    form -> set_applyer((string) this_player() -> query_real_name());
    move_object(form, this_player());
    return 1;
}

// Hmm maybe this should also be done in the quest daemon - Germ
status
list_quests(string num)
{
    int i,qn;
    string *quests, *m, tmp;

    if ( this_player()->query_level() < 25 )
    {
	this_player()->tell_me("Try the Lord's Quests when you are a Lord.");
	return 1;
    }

    qn = to_int(num);
    quests = QUESTS_LORD;

    if (qn) 
	for ( i=0; i<sizeof(quests); i++ )
	{
	    if ( !this_player()->query_quests(quests[i])) qn--;
	    if (!qn)
	    {
		this_player()->tell_me(QUEST_D->query_short(quests[i]) + "\n" +
		  QUEST_D->query_hint(quests[i]));
		return 1;
	    }
	}
    else
    {
	for (i = 0; i<sizeof(quests); i++)
	    if ( !this_player()->query_quests(quests[i])) qn++;
	if (!qn)
	    m = ({ "You have solved all of the Lord Quests!" });
	else
	    m = ({ "You have "+qn+" Lord Quests unsolved." });

	for (i = qn = 0; i < sizeof(quests); i++)
	    if (!this_player()->query_quests(quests[i]))
	    {
		qn++;
		tmp = QUEST_D->query_short(quests[i]);
		if (sizeof(tmp) > 70) tmp = tmp[0..69] + "...";
		m += ({ sprintf("%2d. %s", qn, tmp) });
	    }

	m += ({ "Type list <num> to see the whole hint text." });

	this_player()->more(m);
	return 1;
    }
}



int
get_next_exp(int lvl)
{
    return (int) LEVELS_D->query_exp_for_level(lvl);
}

string
get_new_title()
{
    return (string) this_player() -> query_pretitle();
}

status
check_quest_points(int level)
{
    int qp_diff;

    // Lets recalculate how many QP the player has, based on his completed
    // quests. This allows us to change the value of various quests without
    // any problems.  -=Germ=-
    // Coders can now experiment. /Graah
    if (!this_player()->query_coder_level())
    {
	qp_diff = QUEST_D->recalc_qp(this_player());
	this_player()->set_qp(qp_diff);
    }

    // Ok now figure out how many QP they need to advance from this level.
    qp_diff = QUEST_D->qp_for_level(level) - this_player()->query_qp();
#ifdef HAVE_QUESTS
    if ( qp_diff > 0 )
    {
	this_player()->tell_me("You need "+qp_diff
	  +" more quest points before you can advance.");
	return 0;
    }
#endif

    return 1;
}

status
cost()
{
    int exp, expt, need_expt, lvl, cost, next_exp, exp_to_use;
    int st, in, dx, co;
    string r;

    if ((lvl = (int) this_player()->query_level()) < 20) {
	this_player() -> tell_me("You cannot advance anything here, go \
to Adventurers' Guild.");
	return 1;
    }

    expt = (int)this_player()->query_exp_times();
    need_expt = (int)LEVELS_D->query_exp_times_for_level(lvl) - expt;
    exp = (int) this_player()->query_ep();
    next_exp = (int) LEVELS_D->query_exp_for_level(lvl);
    exp_to_use = exp - (int) LEVELS_D->query_exp_for_level(lvl - 1);

    r = (string) this_player()->query_race();

    if ((next_exp <= exp && need_expt < 1) ||
      (need_expt < 0))
	this_player() -> tell_me(
	  "*** You have collected enough experience to advance a level! ***");
    else
    {
	// Not super-level
	if (!need_expt)
	    this_player() -> tell_me(sprintf("You still need %d experience for \
next level (%d).", (next_exp - exp), lvl + 1));
	else
	    this_player()->tell_me(sprintf("You still need %d experience for \
next level (%d).", (need_expt * 2100000000 + next_exp) - exp, lvl + 1));
    }

    check_quest_points(lvl);
    st = (int) LEVELS_D->query_stat_cost(ST_STR,
      this_player()->query_stat(ST_BASE_STR) + 1, r, lvl);
    in = (int) LEVELS_D->query_stat_cost(ST_INT,
      this_player()->query_stat(ST_BASE_INT) + 1, r, lvl);
    dx = (int) LEVELS_D->query_stat_cost(ST_DEX,
      this_player()->query_stat(ST_BASE_DEX)+ 1, r, lvl);
    co = (int) LEVELS_D->query_stat_cost(ST_CON,
      this_player()->query_stat(ST_BASE_CON)+ 1, r, lvl);

    this_player() -> tell_me(sprintf(
	"You may use %d experience for your abilities.\n\
Str : %s\n\
Dex : %s\n\
Int : %s\n\
Con : %s",
	(need_expt < 0 ? exp_to_use + 2100000000 : exp_to_use),
	(st < 1 ? (st ? "- can't advance, racial max. -" :
	    "- can't advance before advancing level -") :
	  sprintf("%d exp. points.", st)),
	(dx < 1 ? (dx ? "- can't advance, racial max. -" :
	    "- can't advance before advancing level -") :
	  sprintf("%d exp. points.", dx)),
	(in < 1 ? (in ? "- can't advance, racial max. -" :
	    "- can't advance before advancing level -") :
	  sprintf("%d exp. points.", in)),
	(co < 1 ? (co ? "- can't advance, racial max. -" :
	    "- can't advance before advancing level -") :
	  sprintf("%d exp. points.", co))
      ));
    return 1;
}


status
advance(string str)
{
    int stat, level, exp, cost, need_expt;
    string race, title;

    if (!str)
	return notify_fail("Advance what (level, str, con, dex, int)?\n"), 0;

    str = lower_case(str);
    level = (int) this_player()->query_level();

    if (level < 25) {
	this_player()->tell_me("Non-lords can only advance at the Adventurers' Guild.");
	return 1;
    }

    exp = (int) this_player()->query_ep();
    need_expt = ((int)LEVELS_D->query_exp_times_for_level(level) -
      (int)this_player()->query_exp_times());

    if (stat = statList[str])
    {
	exp -= (int) LEVELS_D->query_exp_for_level(level - 1);

	if (need_expt < 0) exp += 2100000000;

	if (!(race = (string) this_player()->query_race()))
	    race = "human";

	cost = (int)LEVELS_D->query_stat_cost(statList2[str],
	  (int)this_player()->query_stat(stat) + 1, race, level);
	if (cost == -1)
	{
	    this_player()->tell_me(sprintf("You can't advance that stat over racial \
maximum; it is already as high as it can be for any %s.", race));
	    return 1;
	}
	else
	if (!cost)
	{
	    this_player()->tell_me(
	      "You can't advance this stat any higher before \
advancing your level.");
	    return 1;
	}

	if (exp < cost)
	{
	    this_player()->tell_me("You don't have enough useable experience!");
	    return 1;
	}

	this_player()->add_exp(-cost);
	this_player()->add_stat(stat, 1);
	this_player()->tell_me("Ok.");
	return 1;
    }

    if (str != "level")
	return notify_fail("Advance what (level, str, con, dex, int)?\n"), 0;

    cost = (int) LEVELS_D->query_exp_for_level(level);

    if ((need_expt > 0) ||
      (cost > exp && need_expt >= 0))
    {
	this_player() -> tell_me("You don't have enough \
experience to advance.");
	return 1;
    }

    if (!check_quest_points(level)) return 1;
    this_player()->tell_me("Ok.");
    if (level == 29) {
	object form;
	form = present("lord_bless", this_player());
	if (!form) {
	    this_player() -> tell_me("You can't advance straight away to level 30; first you \
have to get a form with which you can prove you are worthy of advancing.\n\
The form itself you can get by typing 'form'.");
	    return 1;
	}
	if (!form -> query_signed()) {
	    this_player() -> tell_me("You have to get the form signed by a \
level 35 (or higher) lord or lady.");
	    return 1;
	}
	this_player()->tell_me("Your lord application form disappears!\n\
You hear a voice saying: Well done, mortal!\n\
Perhaps getting it filled wasn't in vain after all..."); 
	destruct(form);
    }

    level++;
    tell_here(sprintf("%s is now level %d!",
	(string) this_player() -> query_name(), level), this_player());
    this_player() -> set_level(level);
    title = get_new_title();
    this_player() -> tell_me(sprintf("You are now %s %s (level %d)",
	title, capitalize((string) this_player() -> query_real_name()),
	level));
    return 1;
}

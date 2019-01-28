/****************************************************************
*								*
* Module:							*
*	/lib/guild.c						*
* Description:							*
*	The standard Guild Room which should be used by (Main)	*
*	guild rooms of individual guilds. In any case, used	*
*	by the Adventurers' Guild's main guild room.		*
* Version:							*
*	1.0a (for StickLib), Aug-98				*
* Last update:							*
*	15-Sep-98 by Doomdark					*
*								'
* Modifications:						*
*                                                               *
* 15-Sep-98, Kieve: Took out the listings of quests and moved   *
*      them into quest daemon                                   *
*								*
****************************************************************/
/*
 * /lib/guild.c - Standard Guild Room
 *
 * Inherit this, then define create_guild() etc.
 *
 */

// V1.1 25-feb-95 by Doomdark
// V1.2 04-oct-95 - "" -
// V2.0 01-mar-96 - "" -
// V2.1 23-mar-96 by -=Germ=-
// V2.2  3-Jul-96 by Graah (was doing something, but it was already done :)
//
// NEW:
// - Quests are now handled using the quest daemon - no more quest objects
//   or quest_room. Players QP is recalculated each time they try to advance
//   their lvl.
// - Quests are no longer divided into major and minor categories.
// - No more preventing of pk (or setting in city) as this is supposed to
//   be _general_ guild room.
// - Now handles stats with new system.
// - As stats were rewritten, so was this, 01-mar-96.
//
// 26-Sep-96 / Graah: Lords are level 25, not 20!

#include <nroom.h>
#include <daemons.h>
//#include <macros.h>
#include <npc_defs.h>
#include <stats.h>
#include <quests.h>
#include <config.h>
#include <generic_items.h>

#include "tune.h"

#define L_CROWN_ID          "lord_crown"
#define L_APPLICATION_ID    "lord_application"
#undef HAVE_QUESTS

static closure guildInit;

void
create_room()
{
    set(ROOM_WD, WD_INDOORS);
    set_light_level(LT_LIGHTS);
    set(NO_CLEANUP);
    set_short("Standard Guild");
    set_long(
      "This is a standard guild room. The coder who made this has been very \
lazy or ignorant, and has not even set a decent long description!");

    set_commands(([
	"cost"	: "cost_cmd",
	"advance" : "advance_cmd",
	"quests"	: "list_quests",
      ]));

    guildInit = symbol_function("init_guild", this_object());

    this_object()->create_guild();

    add_item("sign",
      " --------------------------------------------------------------------\n\
 Type:\n\n\
  cost					-  show costs for advancing\n\
  advance [level/str/dex/int/con]	-  do the advance\n\
  list <number>				-  list quests\n\n\
 --------------------------------------------------------------------"
    );
}

void
init_room()
{
    funcall(guildInit);
}

string
gnd_prn()
{
    switch ((int)this_player()->query_gender()) {
    case 0: return "mighty Worm";
    case 2: return "Madam";
    default: return "Sir";
    }
}


string
get_new_title(int lvl)
{
    int g;
    string r;

    g = r = 0;

    g = (int)this_player()->query_gender();
    r = (string)this_player()->query_race();

    return (string)LEVELS_D->query_title_for_level(lvl, g, r);
}


int
required_qp()
{
    // TODO: Possibly: Healers don't need ANY qps. It is because
    // they can't kill big monsters! /Graah
    return ((int)QUEST_D->qp_for_level(this_player()->query_level())
      -this_player()->query_qp());
}

// This routine is called by monster, to calculate how much they are worth.
// This value should not depend on the tuning.

int
query_cost(int level)
{
    if (level < 0) return 0;
    if (level >= 20) return 1000000 + (level - 20) * 50000;
}

// Special function for other guilds to call. Arguments are current level
// and experience points.
int
query_cost_for_level(int l, int e)
{
    int cost;

    cost = (int) LEVELS_D->query_cost_for_level(l);
    if ((cost -= e) < 0) return 0;
    return cost;
}


/* Return 0 if the player has an unsolved minor task. Setting tasks:

   this_player()->set_quest("M<level>");

   <level> is the level which the player cannot reach before
   solving this task. Look at /room/maze/maze5 for an example.

   Guilds _must_ check these too!!!!!
*/

status
check_quest_points(int silently)
{
    int qp_diff;

    // Lets recalculate how many QP the player has, based on his completed
    // quests. This allows us to change the QP value of various quests
    // without too much problems.  -=Germ=-
    this_player()->set_qp(QUEST_D->recalc_qp(this_player()));

    // Ok now figure out how many QP they need to advance from this level.
    qp_diff = required_qp();
#ifdef HAVE_QUESTS
    if ( qp_diff > 0 )
    {
	if ( !silently )
	    this_player()->tell_me(
	      "You need "+qp_diff+" more quest points before you can advance.\
 Try 'quests' to see how.");
	return 0;
    }
#endif /* HAVE_QUESTS */

    return 1;
}


status
cost_cmd()
{
    int exp, can_use, str, con, dex, Int, next_exp, level, cost;
    string race;

    level = (int)this_player()->query_level();
    if ( level > 24 )
    {
	this_player()->tell_me("You can't advance anything here.\
 Go to the Lord's Guild to do that.");
	return 1;
    }

    exp = (int)this_player()->query_ep();
    can_use = exp - (int) LEVELS_D->query_exp_for_level(level - 1);
    next_exp = (int) LEVELS_D->query_exp_for_level(level);
    if (!(race = (string) this_player()->query_race()))
	race = "human";

    if (can_use > 0) {
	if (level > 1) {
	    this_player()->tell_me(
	      "You can use "+can_use+" experience points for advancing your abilities.\n\They cost as follows:");
	    str =(int)LEVELS_D->query_stat_cost(ST_STR,(int)this_player()->query_stat(ST_BASE_STR)+ 1,race,level);
	    dex =(int)LEVELS_D->query_stat_cost(ST_DEX,(int)this_player()->query_stat(ST_BASE_DEX)+ 1,race,level);
	    con =(int)LEVELS_D->query_stat_cost(ST_CON,(int)this_player()->query_stat(ST_BASE_CON)+ 1,race,level);
	    Int =(int)LEVELS_D->query_stat_cost(ST_INT,(int)this_player()->query_stat(ST_BASE_INT)+ 1,race,level);
	    this_player()->tell_me(sprintf(
		"\nStrength:	%s\n\
Constitution:	%s\n\
Dexterity:	%s\n\
Intelligence:	%s\n\n",
		(str < 1) ? "- can't advance (racial or level max.) -" : sprintf("%d experience points", str),
		(con < 1) ? "- can't advance (racial or level max.) -" : sprintf("%d experience points", con),
		(dex < 1) ? "- can't advance (racial or level max.) -" : sprintf("%d experience points", dex),
		(Int < 1) ? "- can't advance (racial or level max.) -" : sprintf("%d experience points", Int)
	      ));
	} else this_player()->tell_me("You cannot raise stats before you \
have advanced to level 2.");
    } else this_player()->tell_me("You don't have enough experience points \
to advance abilities!");

    if (next_exp <= exp) {
	if (level >= 25) {
	    this_player() -> tell_me(
	      "You can't advance your level here. Go to Lords' Guild to do that.");
	    return 1;
	} else this_player()->tell_me(
	      "You have collected enough experience to advance a level!");
    } else this_player()->tell_me(sprintf("You still need %d experience points\
 for next level.", next_exp - exp));

    check_quest_points(0);

    return 1;
}

status
raise_attrib(string too_high, int stat)
{
    int lvl, st_lvl, st_value;
    int exp_to_use, cost;
    string race;

    lvl = (int) this_player()->query_level();
    if (lvl < 2) {
	this_player()->tell_me("You cannot raise stats before you have advanced to level 2.");
	return 1;
    }
    // We should really get base stat here, since players might have boosts
    // Kind of a cludge until someone makes it better :) //Sumppen
    st_lvl = (int) this_player()->query_stat(stat+10);
    if (!(race = (string) this_player()->query_race()))
	race = "human";
    cost = (int) LEVELS_D->query_stat_cost(stat,st_lvl + 1,race,lvl);
    exp_to_use = (int)this_player()->query_ep() - LEVELS_D->query_exp_for_level(lvl - 1);

    if (cost == -1) {
	this_player()->tell_me(sprintf("You are already as %s as\
 you (or anyone with your race) may ever become!", too_high));
	return 1;
    } else if (!cost) {
	this_player()->tell_me("You can't advance that stat any more\
 before you raise your player level.");
	return 1;
    }

    if (exp_to_use < cost) {
	this_player()->tell_me(
	  "You don't have enough experience points to advance that ability!");
	return 1;
    }

    this_player()->add_exp(-cost);
    this_player()->add_stat(stat+10,1);
    this_player()->tell_me("Ok.");
    return 1;
}

status
advance_cmd(string arg)
{
    object form, crown;
    int cost, level;
    int exp, next_exp;
    string title;

    if ( this_player()->query_level() > 24 )
    {
	this_player()->tell_me("You can't advance anything here.\
 Go to the Lord's Guild to do that.");
	return 1;
    }
    if (!arg) arg = "";
    switch (arg) {
    case "con":
	return raise_attrib("tough and endurable", ST_CON);
    case "dex":
	return raise_attrib("skilled and vigorous", ST_DEX);
    case "int":
	return raise_attrib("knowledgeable and wise", ST_INT);
    case "str":
	return raise_attrib("strong and powerful", ST_STR);
    case "level":
	break;
    default:
	return notify_fail("Advance what (con, dex, int, str, level)?\n"), 0;
    }

    if (!check_quest_points(0))
	return 1;

    level = (int)this_player()->query_level();

    if (level < 0)
	level = 1;

    exp = (int)this_player()->query_ep();
    next_exp = (int) LEVELS_D->query_exp_for_level(level);

    if (next_exp < 0) {
	this_player()->tell_me(
	  "You have already advanced to the highest mortal level!");
	return 1;
    }

    if (level > 24) {
	this_player()->tell_me(sprintf(
	    "To advance your level, you have to use the Lords' Guild. You are still %s.",
	    (string)this_player()->query_title()));
	return 1;
    }

    if (next_exp > exp) {
	this_player()->tell_me("You do not have enough experience points to advance \
your level.");
	return 1;
    }

    if (level < 24)
    {
	level++;
	title = (string) LEVELS_D->query_title_for_level(
	  level, (int)this_player()->query_gender(),
	  (string)this_player()->query_race());
	tell_here(this_player()->query_name()+" is now level "+level+".", this_player());
	this_player()->set_level(level);
	this_player()->set_title(title);
	this_player()->tell_me(
	  "You hear a loud fanfare! (Intermezzo from the Karelia Suite by Jean Sibelius)\n\n\
Finally, all your hard work is going to be credited - you will advance\
 into a new level of power! Greater challenges and even more mightier\
 adventures await you...");
	if (level < 12)
	{
	    this_player()->tell_me("You are now "+capitalize(this_player()->query_real_name())
	      +" "+title+" (level "+level+")."); 
	}
	else
	if (level < 19)
	{
	    this_player()->tell_me("Well done, "+capitalize(this_player()->query_real_name())
	      +" "+title+" (level "+level+").");
	}
	else
	{
	    this_player()->tell_me("Welcome to your new class, mighty one.\n\
You are now "+capitalize(this_player()->query_real_name())+" "+title+" (level "+level+").");
	}
	return 1;
    }

    // Now they have to have a permission from a level 30 (or higher)
    // lord/lady before reaching lord/ladyhood (25) /Frobozz
    if (!(crown = present(L_CROWN_ID, this_player()))
      && !(form = present(L_APPLICATION_ID, this_player()))) {
	move_object(form = clone_object(GENERIC_LORD_APPLICATION),this_player());
	form->set_applyer((string)this_player()->query_real_name());
	this_player()->tell_me(
	  "You can't advance to the lordhood straight away; instead, you now get an\
 application form, which you have to get signed by a lord or lady of at least\
 level 30 or higher, before you can advance to lord/ladyhood\
 (look at the form for more information).\n\
You suddenly notice a strange form in your hands.");
	return 1;
    }

    if (form && !form->query_signed()) {
	this_player()->tell_me(
	  "You have to get the application form signed by a level 30 (or higher)\
 lord or lady before you can advance to the lordhood.");
	return 1;
    }

    if (form) destruct(form);

    if (crown) {
	this_player()->tell_me(
	  "You already are on level 25. Here's the advancement message again:\n\n\
You have completed the normal game. You may remain as a 'Lord player' if\
 you don't want to become a coder. Lord players have their own extra\
 powers and also some special extremely tough quests.\
 Mail/talk to a (co-)admin if you want to become a coder\
 . Coders have to give something for the mud: new ideas, new\
 areas, new patches for mudlib or something.\n\
Note also that nowadays the level limit for coderhood is 25, but it's\
 suggested you advance to level 30 before applying.");
	return 1;
    }

    if ((int)this_player()->query_gender() == G_FEMALE)
	this_player()->set_title("the lady");
    else 
	this_player()->set_title("the lord");

    this_player()->set_level(25);

    move_object(clone_object(GENERIC_LORD_CROWN), this_player());

    // If the lord hasn't got a directory, make it.
    // What a waste of disk space... :-/
    if (file_size("/lords/" + (string)this_player()->query_real_name()) != -2)
    {
	/* Doesn't work //Frobozz
	   mkdir("/lords/"+this_player()->query_real_name());
	    log_lord creates /lords/player_name -directory if it doesn't exist
	 */
	MASTER_OB->log_lord((string)this_player()->query_real_name(),
	  capitalize((string)this_player()->query_real_name()),
	  " advanced to lordhood.");
    }

    return 1;
}

/* For compatibility */
void raise_con() { raise_attrib("tough and endurable", ST_BASE_CON); }
void raise_dex() { raise_attrib("skilled and vigorous", ST_BASE_DEX); }
void raise_int() { raise_attrib("knowledgeable and wise", ST_BASE_INT); }
void raise_str() { raise_attrib("strong and powerful", ST_BASE_STR); }

// Compute cost for raising a stat one level. 'base' is the level that
// you have now, but never less than 1.

// DO NOT USE THIS; NOT UP-TO-DATE! -+ Doomdark 29-feb-96 +-

int
query_stat_cost(int base, int stat)
{
    int cost;

    if (base >= 20)
	return 0;

    return (int) LEVELS_D->query_stat_cost(ST_STR, base + 1,
      (string) this_player()->query_race(), (int) this_player()->query_level());
}


// Hmm maybe this should also be done in the quest daemon - Germ
status list_quests(string num)
{
    int qn;

    qn = to_int(num);
    if(qn > 0)
    {
	if(!QUEST_D->list_single_quest(qn))
	    return notify_fail("What quest is that??\n"), 0;
	return 1;
    }
    else
    {
	return QUEST_D->list_quests();
    }
}

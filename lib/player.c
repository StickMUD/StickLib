/****************************************************************
*								*
* Module:							*
*	Player.c						*
* Description:							*
*	The player object for Sticklib distribution, based on	*
*	the actual player object used by StickMud.		*
* Version:							*
*	1.0a (for StickLib), Aug-98				*
* Last update:							*
*	16-Sep-98 by Doomdark					*
*								'
* Modifications:						*
*								*
* 16-Sep-98, Doomdark: Removed definitions of ADMIN_LIST etc;	*
*	now placed in /secure/coders.h				*
****************************************************************/


#include "/sys/interactive_info.h"

#define SKILLS_IN_ATTACK

#include <player_defs.h>

inherit LIVING_FILE;

#include <tune.h>
#include <config.h>

#include <logging.h>
#include <room_defs.h>
#include <conditions.h>
#include <coder_levels.h>
#include <daemons.h>
#include <weatherdesc.h>
#include <invis_levels.h>
#include <party.h>
#include <tell_me.h>
#include <race.h>
#include <stats.h>
#include <client_defs.h>
#include <areas.h>
#include <generic_rooms.h>
#include <guild.h>
#include <mud_name.h>

#define START_ROOM	GENERIC_ROOM_CHURCH
#define START2_ROOM	GENERIC_ROOM_VOID
#define POST_ROOM	GENERIC_ROOM_POST_OFFICE

#define	PLAYER_C

inherit "/basic/player/relogin";
inherit "/basic/player/misc_cmds";
inherit "/basic/player/criminal";
inherit "/basic/player/messages";
#ifndef SKILLS_IN_ATTACK
inherit "/basic/player/skills";
#endif

inherit "/basic/player/clothing";
inherit "/basic/player/skill_mod";

#define CURRENT_PATH query_env("cwd")
#define TP this_player()
#define MAX_SCAR	18
#define	NATURE	"/complex/nature"
#define	NAME_OF_GHOST	"some mist"

#define	PRINT_INV	call_other("/bin/pub/_inventory", "inventory_cmd", 0, this_object());

// Protos (include file?)

int query(int x);
varargs int set(int x, int y);
nomask void command_queue();
nomask int command_driver(string s);
string get_short(object ob);
int query_lord();
object check_fight(int silently);
int set_allow_shadow(int x);
string compute_auto_str();
void make_scar();
void inform_us(string what);
int more_help(string x);
int invis();
int vis();
void load_auto_obj(string str);
nomask void add_standard_commands();
static void move_player_to_start3(mixed where);
static void move_player_to_start2(string where);
int drop_one_item(mixed ob);
varargs string show_scar(status x);
// static private int check_fullness();
varargs int save_me(int flag);
int illegal_patch(string what);
static void move_player_to_start(mixed where);
string query_link_death_roomfile();
object query_link_death_room();
void add_exp(int x);
int dump_esc(string s);
status query_can_quit();
int listen_shout(int x);
int save_character();
mixed do_client_command(string a, string b);

// Banish server
#define LOGIN_OBJECT  "secure/login"

#define TOO_RELAXED "Nah, you feel too relaxed to do that."
#define Aa(x,y) add_action(x,y)
#define Aa2(x,y,z) add_action(x,y,z)

#define	INVIS_LEVEL	invis_level
#define DEATH_MARK	AREA_TRISTEZA "death/death_mark"
#define	NEWS_FILE	"/data/login/NEWS"
#define	CODERNEWS_FILE	"/data/login/CODERNEWS"
#define ALLOW_SHADOW_FILE "bin/usr/_allowshadow"
#define	LEVEL_D		"/lib/levels"
#define GRAVE_STONE	AREA_TRISTEZA "temple/grave_yard"

#define SHOUT_OLD(x) shout(x)
#define SHOUT(x) shout(x)
#define Ilg(x) log_file("STRANGE_STATS", sprintf("%s: %O", name, x))

#define	ACTIONS_NON_COMBAT	2
// So we can do 2 actions / round when not fighting, vs. 1 when fighting.

// Global variables:

string password;		// This player's crypted password.
int    passwd_time;		// Time of last password change.
int scar;			// Bitfield of scars
int quest_points;		// Quest points (qp)
int bank_money;                // Amount of money in the bank.
string called_from_ip_name;     // IP name, if any
string called_from_ip;		// IP number was used last time
int last_saved;			// When the player was here last
string quests;			// A list of all quests
string mailaddr;		// Email address of player
int tot_value;			// Saved values of this player.
int coder_level;		// The coders level. 0=mortal, -1=testplayer
string login_room;		// set/query_start_location()
mapping addictions;		// Are we addicted to something

// Non-saveable global variables:
static string link_death_room;  // The room, where link death occurred
static object myself;		// Ourselves.
static int temp;		// save money for a moment
static int forcing;
static string saved_where;	// Temp...
static int last_hp,last_sp,last_fp; // Used for reporter
static int disguise_level;	// Are we disguised?
static string disguise_mess;  // Do we have a special disguise mess?
static object other_copy;
static status player_initialized; // initialized?
static int ld_delay;		// No instant ld for players that are fighting
static int listen_to_shouts_from_level;
// static string it;		// Last thing referenced.
static int hp_delay, sp_delay, fp_delay;
// Static count-down variables.
static closure _busy;		// Closure set by guild objects.
static int allow_shadow;       // Do we allow shadowing?
static int _value_plr_client;	// Client we are using, if any

// NEW! Global arrays; note that AmyGD does _NOT_ allocate separate array
// for all instances but uses same one, and only gives pointer to it... neat!
// (note that this way to do it more efficient; main reason)

// YOOORRGHH!! These titles can change any time! I'll fix this
// to ask them from levels_d... /Graah
static string *LordTitles;
#if 0
static string *LordTitles =
({
  "Servant", "Squire", "Knight", "Lord", "Warlord",
  "Supreme Lord", "Overlord", "Baron", "Viscount",
  "Earl", "Count", "Marquis", "Duke", "Archduke",
  "Prince", "Crown Prince", "King", "Imperial Prince",
  "Emperor", "Sovereign", "Legend", "Temporal",
  "High Temporal", "Celestial", "High Celestial",
  "Empyreal", "High Empyreal", "Eternal", "High Eternal",
  "Hierarch", "Old One", "Great Old One",
  "Outer God", "Crazy", "Mad", "rm My Savefile",

  "Servant", "Squiress", "Knight", "Lady",
  "Warlady", "Supreme Lady", "Overlady", "Baroness",
  "Viscountess", "Lesser Countess",
  "Greater Countess", "Marquise", "Duchess",
  "Archduchess", "Princess", "Crown Princess",
  "Queen", "Imperial Princess",
  "Empress", "Sovereigness", "Legend",
  "Temporal", "High Temporal","Celestial", "High Celestial",
  "Empyreal", "High Empyreal","Eternal", "High Eternal",
  "Hierarch", "Old One", "Great Old One",
  "Outer Goddess", "Crazy", "Mad", "rm My Savefile"
});
#endif

// Let's filter out garbage CTRL-codes from all text...
private static int *_charset = ({
  0, 6, 0, 0, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0
});

// Included code modules. If there are errors caused by them, try
// inheritting them to get better error descs! -+ Doomdark +-
// Otherwise let's use includes; faster & more efficient.

#include "/basic/player/tell_me.c"
#include "/basic/player/set_n_query.c"
#include "/basic/player/spells.c"
#include "/basic/player/guild.c"
#include "/basic/player/interact_cmds.c"
#include "/basic/player/keep.c"
#include "/basic/player/party_handling.c"
#include "/basic/player/more.c"
#include "/basic/player/rsay.c"
#include "/basic/player/cmd_files.c"
#include "/basic/player/modify_command.c"
#include "/basic/player/lord_handling.c"
#include "/basic/player/kingdoms.c"
#include "/basic/player/clans.c"
#include "/basic/player/extra_info.c"
#include "/basic/player/desc.c"
#include "/basic/player/player_cmds.c"
#include "/basic/player/coder_cmds.c"
#include "/basic/player/command_driver.c"
#ifndef SKILLS_IN_ATTACK
#include "/basic/player/attack.c"
#else
#include "/basic/player/attack_new.c"
#endif
#include "/basic/player/client.c"

/****************************************************************
*								*
*			Query-functions.			*
*								*
****************************************************************/

nomask int
query_coder_level()
{
    if (coder_level < 0) return 0;
    // We don't want to mix testplayers (coder_level==-1) with real coders.
    return coder_level;
}

nomask int query_qp() { return quest_points; }

nomask int query_testplayer() { return (coder_level < 0); }
nomask int query_is_testplayer() { return (coder_level < 0); }
nomask status query_linkdead() { if (!interactive()) return 1; else return 0; }

nomask int query_allow_shadow() {
    return (coder_level <= 0 ? 1 : allow_shadow);
}

int query_disguised() { return disguise_level; }

nomask status query_prevent_shadow() {
    return (coder_level <= 0 ? 0 : !allow_shadow);
}


object
query_link_death_room()
{
    return link_death_room ? find_object(link_death_room) : 0;
}

// We now use this instead of the former function.
// It's left here for the historical reasons. ++Tron

string query_link_death_roomfile() { return link_death_room; }
int query_lord() { return (level > 24) ? (level - 24) : 0; }
int query_lord_rank() { return query_lord(); }

// Let's only use this one; later we may need to remove the daemon too
// to get this more efficient?
public string
query_pretitle()
{
    if (coder_level)
    {
        if(coder_level < 0)
            return "Testchar ";
        if(coder_level < LVL_APPRENTICE)
            return "Divided ";
        if(coder_level < LVL_NOVICE)
            return "Apprentice Wizard ";
        if(coder_level < LVL_CODER)
            return "Novice Wizard ";
        if(coder_level < LVL_SENIOR)
            return "Wizard ";
        if(coder_level < LVL_ELDER)
            return "Senior Wizard ";
        if(coder_level < LVL_ARCH)
            return "Elder Wizard ";
        if(coder_level < LVL_COADMIN)
            return "Arch Wizard ";
        if(coder_level < LVL_ADMIN)
            return "Co-Admin ";
        return "Admin ";
    }
    if (level > 121) return "Cheater";
    else if (level > 24)
	return LordTitles[(gender > 1 ? 1 : 0) * sizeof(LordTitles)/2 + level - 25] + " ";
    return "";	// For non-lords //Frobozz
}

// Liege: which Lord are we serving.
string query_liege() { return liege; }

string
query_mailaddr()
{
    if (this_interactive() != this_object() &&
      (int) this_interactive()->query_coder_level() < LVL_COADMIN)
	return "Only (co)admins can see";
    return mailaddr;
}

nomask status
query_can_quit()
{
  // Not during a fight or when mortally wounded
  if (is_fighting() || query_hp() < 0) return 0;

  // Also not when poisoned by those nasty thieves! /Graah
#if 0
  if (call_other("/guilds/thieves/poison_d",
		 "is_poisoned", this_object())) return 0;
#endif
  return 1;
}

mixed
query_quests(string str)
{
    string tmp, rest, rest_tmp;
    int i;
    if (!str) return quests;
    rest = quests;
    while(rest) {
	if(str == rest)
	    return 1;
	if (!(i = sscanf(rest, "%s#%s", tmp, rest_tmp))) return 0;
	if(tmp == str) return 1;
	if(i == 1) return 0;
	rest = rest_tmp;
    }
    return 0;
}

varargs string
query_name(int mode, object x, status non_cap)
{
    // Now we can decide what to return depending on who's asking!
    if (ghost) return NAME_OF_GHOST;
    if (INVIS_LEVEL && intp(mode) && mode < INVIS_LEVEL)
        return non_cap ? "someone" : "Someone";
    // Also, invisibility levels are taken into account as well.
    if (disguise_level && (!intp(mode) || mode < IL_INVISIBLE)) {
        if(disguise_mess) return non_cap ? disguise_mess :
            capitalize(disguise_mess);
        // We have to capitalize disguise_mess so that same string can
        // be used as id too! // Chopin 21-May-98
        if(!race) return non_cap ? "human" : "Human";
        else return non_cap ? race : capitalize(race);
    }
    return cap_name;
}

// Bit more beautiful short description //Starbyt
varargs string
query_short(int mode, object who)
{
#define WHO_PRETITLES
    object ob;
    string tmp,tmp2,tmp_race;
    if (ghost) return "Ghost of " + cap_name;
    if (frog) return cap_name + " the frog";

    if (mode && query_verb() == "who") {
	// We could do this at the who_cmd(), too, though :-7

#ifdef WHO_PRETITLES
	if (title) tmp = sprintf("%s%s %s", this_object()->query_pretitle(), cap_name, title);
	else tmp = sprintf("%s%s", this_object()->query_pretitle(), cap_name);
#else
	if(query_lord())
	    if (!IS_CODER(coder_level))
		if (title) tmp = sprintf("%s %s (%s)", cap_name, title, this_object()->query_pretitle());
		else tmp = sprintf("%s (%s)", cap_name, this_object()->query_pretitle());
#endif

	if (liege) tmp = sprintf("%s (follower of %s)", tmp, capitalize(liege));

    } else {
	if (INVIS_LEVEL && (!intp(mode) || mode < INVIS_LEVEL)) return 0;
	if (!disguise_level) tmp = (string)this_object()->query_pretitle();
	else tmp = "";
	if(race && race != "human" && !disguise_level)
	    tmp_race=" the "+race;
	else tmp_race="";
	if (hit_point < 0) {
	    // Changed from 'cap_name' to 'query_name()', because now we can disguise
	    // ourselves 31.1.1993 //Frobozz
	    tmp = sprintf("%s %s%s lies here, mortally wounded",
	      tmp, query_name(mode,who), tmp_race);
	} else {
	    if (query_condition(C_UNCONSCIOUS))
		tmp = sprintf("%s%s%s lies here unconscious",
		  tmp, query_name(mode, who), tmp_race);
	    else if (query(LIV_RESTING))
		tmp = sprintf("%s%s%s is resting here", tmp, query_name(mode,who), tmp_race);
	    else if(query(LIV_SITTING))
		tmp = sprintf("%s%s%s is sitting here", tmp, query_name(mode,who), tmp_race);
	    else tmp = sprintf("%s%s%s is here", tmp, query_name(mode,who), tmp_race);
	}
	if (INVIS_LEVEL) {
	    if (INVIS_LEVEL >= IL_INVISIBLE) {
		if (INVIS_LEVEL >= IL_TOTALLY_INVISIBLE)
		    tmp += " <totally invisible>";
		else return tmp += " <invisible>";
	    } else if (INVIS_LEVEL >= IL_HIDING) {
		if (INVIS_LEVEL >= IL_HIDDEN)
		    tmp += " <hidden>";
		else tmp += " <hiding>";
	    } else tmp += " <sneaking>";
	}
    }
    return tmp;
}

varargs string
query_long(string what, object who)
{
    object looker;
    string cap_pronoun, txt;

    cap_pronoun = capitalize(query_pronoun());
    if (!(txt = query_short(1, who)))
	txt = "Someone.";

    if (!disguise_level)
	txt = sprintf("%s\n%s is a %s%s %s", txt, disguise_level ? cap_name : cap_pronoun,
	  disguise_level ? "disguised " : "", query_gender_string(),
	  frog ? "frog" : (race ? race : "player"));

    if (ghost || frog) return txt;

    txt = sprintf("%s\n%s\n", txt, show_scar(1));

    // Player's own description, if any.
    if (desc) txt = sprintf("%s%s\n", txt, desc);

    if (hit_point <= 0) txt = sprintf("%s%s is mortally wounded.", txt, cap_pronoun);
    else if (hit_point < 4) txt = sprintf("%s%s is nearly dead.", txt, cap_pronoun);
    else if (hit_point < max_hp / 10) txt = sprintf("%s%s is in a very bad shape.", txt, cap_pronoun);
    else if (hit_point < max_hp / 5) txt = sprintf("%s%s is in a bad shape.", txt, cap_pronoun);
    else if (hit_point < max_hp / 2) txt = sprintf("%s%s is not in a good shape.", txt, cap_pronoun);
    else if (hit_point < max_hp - 20) txt = sprintf("%s%s is slightly hurt.", txt, cap_pronoun);
    else txt = sprintf("%s%s is in good shape.", txt, cap_pronoun);

    return txt;
}

nomask object
query_start_location_obj()
{
    object ob, def;

    if (!(def = find_object(START_ROOM))) {
	catch(call_other(START_ROOM, "query"));
	def = find_object(START_ROOM);
    }

    if (dead && !query(LIV_DEATH_MARK))
	ob = def;
    else {
	if (login_room && !(ob = find_object(login_room))) {
	    if (catch(call_other(login_room, "query"))) {
		tell_me("Buggy or non-existing starting room!");
		login_room = 0;
	    } else ob = find_object(login_room);
	}

	if (!coder_level && ob && !ob->query_is_start_location()) {
	    tell_me("Removing start location.");
	    ob = login_room = 0;
	}
    }

    // No default (church), try 2nd choice.
    if (!ob && !(ob = def) && !(ob = find_object(START2_ROOM)) &&
      !catch(call_other(START2_ROOM, "query")))
	ob = find_object(START2_ROOM);

    return ob;
}

int query_bank_money()
{
    return bank_money;
}

/****************************************************************
*								*
*			Set-functions.				*
*								*
****************************************************************/

int set_liege(string str) { liege = str; }
void set_intoxination(int i) { intoxicated = i; }

void
set_disguised(status dl, string dm)
{
    disguise_level = dl; 
    disguise_mess=dm;
}

// The command "starthere" should do the checking that the location is
// acceptable. Coders can set any room as their starting location.
// --Val (940729)

nomask string
set_start_location(string file)
{
    if (!file || (stringp(file) && !sscanf(file, "%s#", file) &&
	find_object(file))) login_room = file;
    return login_room;
}

nomask void
set_ld_room(string s)
{
    if(!s) return;

    link_death_room = s;
}

nomask void
set_qp(int q)
{
    if (q < 0) return;

    if ((q < quest_points) && !coder_level &&
      TP && living(TP) && previous_object()) {
	log_file("QUESTS", sprintf(
	    "%s: from %s %s reduced QP from %d to %s",
	    name, object_name(previous_object()), (string) TP->query_real_name(),
	    quest_points, q));
    }

    quest_points = q;
}

nomask int
set_allow_shadow(int x) {
    if(object_name(previous_object()) != ALLOW_SHADOW_FILE) return 0;
    allow_shadow = x;
}

nomask int
set_quest(string q)
{
    int c;
    if (!stringp(q)) return 0;
    if (query_quests(q)) return 0;
#ifdef LOG_SET_QUEST
    if(previous_object()) {
	log_file("QUESTS", sprintf("%s: %s from %s.\n",
	    name, q, object_name(previous_object())));
	if(TP && TP != this_object() &&
	  interactive_info(TP, II_IP_NUMBER))
	    c = TP->query_coder_level();
	log_file("QUESTS", sprintf("Done by %s (%s%d)\n",
	    TP->query_real_name(),
	    (c ? "C" : ""), (c ? c : TP->query_level())));
    }
#endif
    if (!quests) quests = q;
    else quests = sprintf("%s#%s", quests, q);
    set_qp(QUEST_D->recalc_qp(this_object()));
    return 1;
}

// Allow coders to set their quest string
nomask void
set_quest_string(string q)
{
    if (!coder_level) return;
    quests = q;
}

void set_bank_money(int amt)
{
    if(object_name(previous_object()) != (GENERIC_ROOM_BANK)[1..<1])
        return 0;
    if(amt < 0)
        amt = 0;
    bank_money = amt;
}

/****************************************************************
*								*
*			Add-functions.				*
*								*
****************************************************************/

nomask void
add_exp(int e)
{
    int c;

// Level/xp limit is lifted again. But this will return if high
// level guys rush all to join mages, the only level-based guild.
// If anyone detects that kind of cheating, change to #if 1
// /Graah
#if 0
    //
    // Temporary limitation to prevent ridiculous levels,
    // because super high-level priests, having no skills,
    // really ruin all balance. /Graah
    //
    // This MIGHT be lifted if all skill-less guilds get skills.
    //
    // if (level >= 90 && e > 0) return;
    //
    if ((e > 0) &&
	(exp_times >= 2) &&
	(experience >= 617661762))  // Askur's current exp :)
      return;
#endif

    // Over 200k to one person at a time! /Graah
    // However, if there are mega-monsters, and you want more exp,
    // you can always party. Everyone in party could get that 200k!
    if ((query_coder_level() == 0) &&
   // Hope no one minds that I added the following line, but I thought it
  // might help if one could add lots of experience to one's testplayer *-(|:U)
  // Enjorlas
    (query_testplayer() == 0) &&
      (e > 200000))
	e = 200000;

    //MUDwide experience tuning.
    // Bull 27. 7. 1998
    e = TUNE_GENERAL_EXP(e);

#ifdef LOG_EXP
    if (TP && TP != this_object()
      && interactive(TP) && !IS_CODER(coder_level)
      && (e >= ROOM_EXP_LIMIT || IS_CODER_OB(TP))) {
	c = (int) TP->query_coder_level();
	log_file("EXPERIENCE", sprintf(
	    "%s %s (%d) %d exp by %s (%s%d)\n",
	    ctime(time()), name, level, e, (string) TP->query_real_name(),
	    (c ? "C" : ""), (c ? c : (int)TP->query_level())));
    }
#endif
    experience += e;

    // Those nerds get lots of exp! They should get a life too... /Graah
    if (experience >= 2100000000)
    {
	experience -= 2100000000;
	exp_times++;
#ifdef LOG_EXP
	log_file("EXPERIENCE",
	  sprintf("%s %s got 2.1 billion for %d. time!\n",
	    ctime(time()), name, exp_times));
#endif
    }

    if (experience < 0 && exp_times > 0)
    {
	experience += 2100000000;
	exp_times--;
    }

    if (e >= 0) session("add", "exp", e);

    if (!IS_CODER(coder_level))
	add_worth(e);

    if (GuildFlags & (1 << G_HOOK_ADD_EXP))
	funcall(GuildHooks[G_HOOK_ADD_EXP], this_object(), e);
}

void
add_intoxination(int i)
{
    if (i < 0 && -i > intoxicated / 10)
	i = -intoxicated / 10;
    intoxicated += i;
    if (intoxicated < 0) intoxicated = 0;
}

void
add_stuffed(int i)
{
    if (i < 0 && -i > stuffed / 10)
	i = -stuffed / 10;
    stuffed += i;
    if (stuffed < 0) stuffed = 0;
}

void
add_soaked(int i)
{
    if (i < 0 && -i > soaked / 10)
	i = -soaked / 10;
    soaked += i;
    if (soaked < 0) soaked = 0;
}

void add_bank_money(int amt)
{
    if(object_name(previous_object()) != (GENERIC_ROOM_BANK)[1..<1])
        return 0;
    // We'll leave all the logging to the bank.
    if(bank_money + amt < 0)
        amt = -bank_money;

    bank_money += amt;
}

/****************************************************************
*								*
*	Now so-called 'lfuns'; functions gamedriver calls.	*
*								*
****************************************************************/

void
reset()
{
    reset_living();
    update_conditions();	// Let's check our conditions are ok...
    update_stats();		// And same for stats... Perhaps not needed?
}

void
create()
{
    create_living();
    liv_Flags = F_LIV_IS_PLAYER;
    level = -1;
    set_conditions(([]));
    name = "logon";
    cap_name = "Logon";
    reset_msg(); // messages when going invis/vis moved in obj/messages.c
    set_move_msg("min", "arrives");
    set_move_msg("mout", "leaves");
    set_move_msg("mmin", newmmsgin[0]);	// default
    set_move_msg("mmout", newmmsgout[0]);
    title = "the title less";
    gender = -1; // Illegal value, so it will be changed!
    allow_shadow = 0;
    plr_Flags2 = F_PLR_LICH;
    LordTitles = (string *)LEVELS_D->query_lord_titles();
}

// Identify ourself.
// (well, if we want to... depending on who's asking!
public varargs int
id(string str, int lvl)
{
    // Can look at his disguise too. Kelgath.
    if (str == name || str == race ||
      (!race && str == "human") || str == disguise_mess) return 1;

    // Someone (mortal) does this and that but now others can affect him.
    // If they dare.

    if (!coder_level && INVIS_LEVEL && (str == "someone" || str == "Someone"))
	return 1;

    // Invisibles want to work in peace.
    if (INVIS_LEVEL && lvl <= level) return 0;
    if (ghost) return str == "ghost of " + name;
    if (str == name || str == race) return 1;
    return 0;
}

/****************************************************************
*								*
*		And then rest of the functions.			*
*								*
****************************************************************/

/**********************************

 Initialize guest's stats.

 **********************************/

nomask static void
init_guest()
{
    coder_level = frog = exp_times = experience = race = 0;
    set_alignment(0);
    level = 1;
    set_move_msg("min", "arrives");
    set_move_msg("mout", "leaves");
    title = "the visitor";
    // Lets prevent the good old "Guest_as_buffer" trick
    hit_point = max_hp = 10;
    spell_points = 0;
    max_sp = 30;
    Str = max_Str = Dex = max_Dex = Con = max_Con = Int = max_Int = 1;
    mailaddr = "Visitor@from.outer.space";
    gender = auto_load = liege = 0;
    // No more Guest the visitor (follower of Somelord)
    ghost = dead = 0; // lets revive the guest silently
    save_me(1);	//Frobozz 28.10.1991
}

/***************************************

 Recover link death. Find link death
 room and set cost for ld.

 ***************************************/

nomask static object
recover_link_death()
{
    object ob;
    string tmp;
    string dummy;
    int    ndummy;

    remove_call_out("throw_ld_out");

    ob = query_link_death_room();
    if (!ob && (tmp = query_link_death_roomfile())) {
	if(sscanf(tmp, "%s#%d", dummy, ndummy) == 2) {
	    ob = find_object(GENERIC_ROOM_CHURCH);
	    if(!ob) tmp = link_death_room = GENERIC_ROOM_CHURCH;
	    else return ob;
	}
	call_other(tmp, "query", 0);
	ob = query_link_death_room();
    }
    link_death_room = 0; //set ldroom to zero when we recover.

    spell_points -= (max_sp / 6);
    fatigue_points -= (max_fp / 6);
    if (spell_points < 0 ) spell_points = 0;
    if (fatigue_points < 0) fatigue_points = 0;

    // Graah 1-Apr-97
    if (name == "guest") init_guest();

    CHANNEL_D->check_channels(myself);

    return ob;
}


/*************************************
 Used with set_prompt in initialize()
**************************************/
nomask string
prompt_path()
{
    string p;

    if (strstr(p, PATH_USER_FNAME) == 0) {
        sscanf(p, PATH_USER_FNAME "%s", p);
	return sprintf("stick:~%s> ", p);
    }
    return sprintf("stick:/%s> ", p);
}

nomask string
prompt_time()
{
    int h, m;

    sscanf(ctime(time()), "%!s %!s %!d %d:%d:%!d %!d", h, m);
    if(m < 10)
	return sprintf("%d:0%d> ", h, m);
    return sprintf("%d:%d> ", h, m);
}


/*************************************

 Initialize player's values from login
 object when logging on.

 NOTE! This is a _very_ important for
	security... Leave a hole there
	and you'll end up having every
	coder log in as an admin when
	you're away. *grin*

 *************************************/

nomask void
initialize(string str, string passwd, int passtime,
  int lvl, string maddr, int gen, status interactive_copy,
	mapping attrs)
{
    string tmp;
    int n, i, j;
    object ldroom, crown;

    // No unauthorized calls!!!
    if (!str || !previous_object() || !(tmp = object_name(previous_object())))
	return;

    if (tmp != SWITCH_OB_FNAME && (sscanf(tmp, "%s#%d", tmp, n) < 2 ||
	tmp != LOGIN_OB_FNAME))
		return;

    set_connection_charset(_charset, 0);
    set(LIV_NIGHT, NATURE->query_night());

	// New: Let's make sure they are passed; if not, we'll default
	// to the old values in use.
    if (passwd)
	password = passwd; //password change possible when LD ++Tron
    if (passwd_time)
	passwd_time = passtime;

    // If this is the active copy of player, then avoid ld recovery
    // because it will return zero. 

    if (player_initialized) {
	if (!interactive_copy) {
	    ldroom = recover_link_death(); // link death
	    if(!ldroom) ldroom = environment(this_object());
	}
    } else {
	//not link dead.
	if (lvl != -1) {
		if (!restore_object(PATH_FOR_PLAYER_SAVE(str))) {
		destruct(this_object());
		return;
	    }
	    if (coder_level > 0) {
		set(PLR_WIZ_ON);
		set(PLR_FIZ_ON);
		if(coder_level >= 300)
		    set(PLR_AWIZ_ON);
	    } else {
		set(PLR_LICH_ON);
	    }
	    infravision = query_infravision();

	// This can be used for updating player objects when there
	// are big incompatible changes...
	// For now, it's dummy thing:
	    if (Version == 0) {
		Version = 1;
	    }

	    // We need to initialize these counters:
	    hp_delay = query_race_stat(RACE_HP_DELAY);
	    sp_delay = query_race_stat(RACE_SP_DELAY);
	    fp_delay = query_race_stat(RACE_FP_DELAY);
	} else {
	    set_stat(ST_STR, 1);
	    set_stat(ST_CON, 1);
	    set_stat(ST_INT, 1);
	    set_stat(ST_DEX, 1);
	    hit_point = max_hp;
	    spell_points = 0;
	    level = 1;
	    if (maddr)
		mailaddr = maddr;
	    gender = gen;
	    title = (string)LEVELS_D->query_title_for_level(level, gender);
	}

	name = str;		//Restore name +
        if(passwd)
	password     = passwd;	//password here so that they won't get messed up.
        if(passwd_time)
	passwd_time     = passtime;

	if (name == "guest") init_guest();
// No need to init any more:
//	init_quicktyper();
	weapon_class = query_race_stat(RACE_NATURAL_WC); // race.c
	armour_class = query_race_stat(RACE_NATURAL_AC);
	dead = ghost;

	myself = this_object();
	cap_name = capitalize(name);

	configure_object(myself, OC_COMMANDS_ENABLED, 1);
	efun::set_this_player(myself);
	set_living_name(name);
    }

/* If login-module wanted to pass us some data: */
    if (attrs) {
//tell_me(sprintf("DEBUG: attrs=%O.", attrs));
	if (i = attrs["window_x"]) {
	  if (i < TELL_MIN_X)
		i = TELL_MIN_X;
	  set_env("columns", i);
	}
	if (j = attrs["window_y"]) {
	  if (j < TELL_MIN_Y)
		j = TELL_MIN_Y;
	  set_env("rows", j);
	}
	if (i || j) {
	  tell_me(sprintf("DEBUG: Screen size set to %d x %d.",i,j));
	}
	// else tell_me("DEBUG: No window size passed.");
	i = set_client(attrs["client"]);
//tell_me(sprintf("DEBUG: Client id %d (0 means none).", i));
    }

    // move to start here
    if (!ldroom && !interactive_copy) {
	if (!(ldroom = query_start_location_obj())) {
	    if (!this_player()->query_coder_level()) {
		tell_me("No starting location can be found. Try again later.");
		destruct(this_object());
		return;
	    }
	}
    }

    // move only if this player was not active when connecting.
    // because active player has valid environment already.
    if (!interactive_copy)
	move_object(myself, ldroom);

    if (!this_object())
	return; // Shouldn't happen in the perfect world. However, 
		// login room may dest us if we're not allowed to
		// enter or so

    set(PLR_TIME_TO_SAVE, age + 500);
    set(PLR_LOGIN_TIME, time());
    update_conditions();	// Let's not forget this one...
    update_stats();		// Or this one.
    max_hp = HPS(query_stat(ST_CON));
    max_sp = SPS(query_stat(ST_INT));
    max_fp = FPS(query_stat(ST_STR), query_stat(ST_CON));

    configure_object(this_object(), OC_HEART_BEAT, 1);

    if (!Env)
	Env = ([]);
    if (INVIS_LEVEL && (!IS_CODER(coder_level) || level < 33))
	vis();

    // Hidden thieves must become visible here. /Graah
    if (!IS_CODER(coder_level) && (INVIS_LEVEL == IL_HIDDEN)) vis();

    set_crime_level(0); // remove these when crime system works
    set_crime_count(0);

    if (player_initialized) //coming from link death.
    {
	inform_us("reconnects");

	if (hunted && sizeof(hunted)) {
	    for(i = sizeof(hunted) - 1; i >= 0; i--)
		if (hunted[i] && objectp(hunted[i]) &&
		  environment() && present(hunted[i], environment())) {
		    attack_object(hunted[i], 1);
		    hunted[i] = 0;
		}
	}
	// Then let's inform the guild object, if it wants to know...
	if (GuildFlags & (1 << G_HOOK_RECOVER_FROM_LD))
	    funcall(GuildHooks[G_HOOK_RECOVER_FROM_LD], this_object());

    } else { //Normal login

	// Removed the changing of coders path. --Val
	if (!stringp(CURRENT_PATH)) set_env("cwd", "doc");

	add_standard_commands();

	more(NEWS_FILE);
	if (IS_CODER(coder_level)) {
	    string p;

	    add_action("vis", "vis");
	    add_action("invis", "invis");
	    more(CODERNEWS_FILE);

	    p = query_env("prompt");

	    if (p) {
		if (p == "PATH")
		    configure_interactive(this_object(), IC_PROMPT, #'prompt_path);
		else if(p == "TIME")
		    configure_interactive(this_object(), IC_PROMPT, #'prompt_time);
		else
		    configure_interactive(this_object(), IC_PROMPT, p);
	    }

	}

	// NEW: Guild object-loading; they don't need to be autoloadable anymore.
	initialize_guild();

	// This is in /basic/living/guild.c; uses 'catch' to prevent errors to
	// end login-sequence.

	load_auto_obj(auto_load);

	inform_us("enters " MUD_NAME );
#ifdef LOG_ENTER
	// Log entering too. 15.6.1995 //Frobozz
	log_file ("ENTER", sprintf("%s %s (enter) from %s(%s) %d,%d(%d),%d,\
%d/%d,%d/%d,%d/%d,%d/%d\n",
	    ctime(time())[4..15], capitalize(name), interactive_info(this_object(), II_IP_NAME),
	    interactive_info(this_object(), II_IP_NUMBER), level, experience, exp_times, money,
	    Str, max_Str, Con, max_Con, Dex, max_Dex, Int, max_Int));
#endif

	if (!INVIS_LEVEL && environment())
	    environment() -> tell_here(
	      sprintf("%s enters " MUD_NAME ".", cap_name), this_object());
	else tell_me("You are invisible.");

	if (tot_value) {
	    tell_me(sprintf("You find %d coins of your lost money!", tot_value));
	    money += tot_value;
	    tot_value = 0;
	}

	whimpy = query(PLR_USED_WIMPY);

	if (max_Str < 1) max_Str = Str;
	if (max_Dex < 1) max_Dex = Dex;
	if (max_Con < 1) max_Con = Con;
	if (max_Int < 1) max_Int = Int;

	if (Str > max_Str) set_stat(ST_STR, max_Str);
	if (Dex > max_Dex) set_stat(ST_DEX, max_Dex);
	if (Con > max_Con) set_stat(ST_CON, max_Con);
	if (Int > max_Int) set_stat(ST_INT, max_Int);

#define ISTL(x) log_file("STRANGE_STATS", name + ": " + (x) + "\n")
	// Ah, HERE! / Graah
	if (max_Str > (i = (int)LEVELS_D->query_max_stat(ST_STR, race)))
	{ ISTL(sprintf("Str %O -> %O", max_Str, i)); max_Str = i;}
	if (max_Dex > (i = (int)LEVELS_D->query_max_stat(ST_DEX, race)))
	{ ISTL(sprintf("Dex %O -> %O", max_Dex, i)); max_Dex = i;}
	if (max_Con > (i = (int)LEVELS_D->query_max_stat(ST_CON, race)))
	{ ISTL(sprintf("Con %O -> %O", max_Con, i)); max_Con = i;}
	if (max_Int > (i = (int)LEVELS_D->query_max_stat(ST_INT, race)))
	{ ISTL(sprintf("Int %O -> %O", max_Int, i)); max_Int = i;}

	if (!query_coder_level())
	{
	    // i = (int) LEVEL_D->check_level(experience);
	    i = (int)LEVEL_D->check_level(experience, exp_times);

	    if (level > i) {
		Ilg(sprintf("Level %O -> %O\n", level, i));
		/* Player killed in PK should lose their level   //Elena */
		if (i < 25 && level >= 25 && (crown = present("lord_crown",
		      this_object()))) destruct(crown);
		// Let's try to patch the "4 crowns" bug... -+ Doomdark +-
		level = i;
	    }
	}
    }

   CHANNEL_D->check_channels(myself);

    // Common for LD and normal login
   tell_me(MAIL_D->query_mail(name, 0));

    // Argh! Amylaar certainly hates this func:
    if (interactive_info(this_object(), II_IP_NUMBER) != called_from_ip && called_from_ip)
	tell_me(sprintf("Your last login was from %s.", called_from_ip));

    called_from_ip = interactive_info(this_object(), II_IP_NUMBER);
    called_from_ip_name = interactive_info(this_object(), II_IP_NAME);

    if (hit_point < 0) {
	tell_me("\nYou are mortally wounded and unconscious.");
	set_condition(C_UNCONSCIOUS | C_FORCE_FLAG, -1);
    } else if(query_condition(C_UNCONSCIOUS) == -1)
	set_condition(C_UNCONSCIOUS, 0);

	set_condition(RESISTANCE(C_STUNNED), -1);
    race_update();

    player_initialized = 1;
	if (_value_plr_client == CLIENT_MURDER) {
		PRINT_INV
	}
}

// Called by command 'save'
int
save_character()
{
    save_me(3);
    tell_me("Ok.");
    return 1;
}

/* Hmmh. Perhaps we should log calls... */
nomask void
set_level(int lev)
{
    if (!intp(lev) || lev < 1) return;

#if 0
    /* To catch the level change bug. Added nomask too.  --Val */
    if (!previous_object())
    {
	log_file("LEVEL", sprintf("%s %s: from %d to %d by %s in %s\n",
	    ctime(time()), name, level, lev,
	    (previous_object() ? object_name(previous_object()) : "--this--")
	    (environment() ? = object_name(environment()) : "--void--")));
    }
#endif
    level = lev;
}

/*
  New shout routines to allow earmuffs via catch_shout()

  Possible future extension:
     filter_objects() returns an array holding the players that accepted
     the shout(), that is those who heard it.
     These could be listed to the shouter if one wishes.
*/

// This is called for every shouted string to this player.
int
catch_shout(string str)
{
    if (coder_level && (this_player()->query_disguised() ||
	(sizeof(str) > 5 && str[0..6] == "Someone")))
	str = sprintf("%s=%s", (string)TP->query_real_name(), str);
    if (listen_to_shouts_from_level) {
	if ((int) TP->query_level() >= listen_to_shouts_from_level) {
	    tell_me(str);
	    return 1;
	} else return 0;
    }

    if (!environment() || environment()->query(PREVENT_SHOUT)) return 0;

    tell_me(str);
    return 1;
}

/* This is the earmuff hook. You can set the level of the players to which
   you want to listen to, to one more than your own.
   This means you can not stop higher level players from shouting to you,
   but you can stop lower levels and your own level.
 */
int
listen_shout(int lev)
{
    if (lev && lev <= level+1) listen_to_shouts_from_level = lev;
    return listen_to_shouts_from_level;
}

// THIS FUNCTION IS NO LONGER NEEDED! -+ Doomdark 26-feb-96 +-
int
dump_esc(string str)
{
    string tmp1, tmp2;
    int i, esc;
    return 0;
}

// If you are drunk, you mumble 7.4.1992 //Frobozz
// Recoded for 3.1 --Val

// ARGH! Slow as hell, this code. -+ Doomdark 17-jan-95 +-
// (better use global mappings, which would be queried from a daemon;
// efficient solution, still no much memory usage)

string
mumble(string str)
{
    string *words, *mixup;
    int    i, j;

    if (!str) return 0;

    /* "s" -> "sh" , "S" -> "Sh" */
    words = explode(str, "S");
    str = efun::implode(words, "Sh");
    words = explode(str, "s");
    str = efun::implode(words, "sh");
    words = explode(str, " ");
    // Test needed because of buggy explode
    if (!(words && sizeof(words) && words[0])) return str;

    // Change the places of two words
    i = random(sizeof(words));
    j = random(j);
    if (i != j) {
	str = words[j];
	words[j] = words[i];
	words[i] = str;
    }

    // Mix words with 'sounds like' counterparts
    mixup = ({
      "one","1",	"two","2",
      "too","2",	"to","2",
      "three","3",	"for","4",
      "five","5",	"6","sex",
      "you","U",	"here","hear",
      "there","their","are","R",
      "be","bee",	"right","write",
      "which","witch", "made","maid",
      "bare","bear",	"beer","bear",
      "where","wear",	"why","Y",
      "stair","stare", "eight","8",
      "ate","8",	"male","mail",
      "sale","sail",	"see","sea",
      "C","sea",	"B","bee",
      "no","know",	"die","dye",
      "tail","tale",	"hair","hare",
      "frobozz","froboss" /* :) //Frobozz */
    });
    for (i = sizeof(words) - 1; i >= 0; --i) {
	if (random(3) && (j = member(words[i], mixup) >= 0))
	    words[i] = mixup[j ^ 1];
	if (!random(5))
	    words[i]=upper_case(words[i]);
    }

    return efun::implode(words, " ");
}

/* call_out:ed when going ld. //Graah */
void
ld_forget_hunted()
{
    /* In case we go ld many times quickly */
    remove_call_out("ld_forget_hunted");

    /* Test if we returned on time? */
    if (!interactive()) hunted = ({ });
}

void
heart_beat()
{
    int i, cond;
    mixed tmp;

    // We won't immediately go ld if: we are
    // - just fighting someone here or
    // - mortally wounded
    //  Now there is a delay of 2 hbs before we go ld in those cases

    // Queued Commands ++Tron
    // Important to put it before interactivity is
    // handled in this function.

    command_queue();

    if (!interactive() && !link_death_room &&
      /* ((!check_fight(1) && query_hp() >= 0) || ++ld_delay > 2)) */
      /*  IF we are mortally wounded...no link death. ++Tron */
      (/* !is_fighting() || */ ++ld_delay > 5) && query_hp() >= 0) {
	configure_object(this_object(), OC_HEART_BEAT, 0);
	ld_delay = 0;
	// Let's not save here... too long evaluations occur Grrr. ++Tron
	// save_me(1);

	set(PLR_CONNECT_TIME, query(PLR_CONNECT_TIME) + time() - query(PLR_LOGIN_TIME));
	environment()->tell_here(sprintf(
	    "Link Death arrives on his %s %s and carries %s away!",
	    ({ "black", "red", "purple", "pink", "green",
	      "yellow", "blue", "gray", "white", "octarine"})[random(10)],
	    ({ "stallion", "horse", "dragon", "wyrm"})[random(4)], query_name())
	  , this_object());
	link_death_room = object_name(environment());

	move_object(this_object(), GENERIC_ROOM_PURGATORY);
	// Let's inform the guild object, in case it wants to know! -+ Doomdark +-
	if (GuildFlags & (1 << G_HOOK_GO_LD))
	    funcall(GuildHooks[G_HOOK_GO_LD], this_object());

	// Let's throw link-deaders out in two hours time 8.4.1992 //Frobozz
	// Would be much better using timerequester! -+ Doomdark +-
	call_out("throw_ld_out",7200);

#ifdef LOG_ENTER
	log_file ("ENTER", sprintf("%s %s (ld) %d,%d(%d),%d,%d/%d,%d/%d,%d/%d,%d/%d.\n",
	    ctime(time())[4..15], capitalize(name), level, experience,
	    exp_times,
	    money, Str, max_Str, Con, max_Con, Dex, max_Dex, Int, max_Int));
#endif
	/* We can't keep hunts if we go ld. Nasty, but it is
	   kind of a cheat to escape to ld all the time.
	   //Graah 26-Apr-96. */
	call_out("ld_forget_hunted", 60); /* should be 8+INT/2 but...*/

	inform_us("goes link dead");
	return;		// Let's not continue heart_beat.
    }

    // New. We need to clear the 'dirty stamp'. -+ Doomdark 17-dec-95 +-

    if (dirty_stamp && dirty_stamp < age) {
	tell_me("You can no longer be attacked in no-PK places.");
	dirty_stamp = 0;
	liv_Flags &= (~F_LIV_OK_TO_PK);
    }

    cons_moves = 0;

    // Who had removed this? /Graah
    if (coder_level == 0 && interactive(this_object()) &&
      interactive_info(this_object(), II_IDLE) >= (2 * 60 * 60))
    {
	tell_me("You've been idle for 2 hours. Now, get out.");
//	save_me(2);
//	destruct(this_object());
        command("quit");
	return;
    }

    if (ghost) return;
    age++;

    // Reduce criminal counter. Easier to have as a counter: we don't
    // want the crimes to get old during link death or the time the
    // player isn't logon. //Graah

    reduce_crime();

    if (age > time_to_save) {
	if (!query(LIV_BRIEF))
	    tell_me("Autosave.");
	save_me(1);
	set(PLR_TIME_TO_SAVE, age + 500);
    }

    // Let's update drunkness every 10th heart_beat:
    switch (age % 10) {
    case 0:
	if (intoxicated) {
	    if (!(--intoxicated)) {
		headache = max_headache;
		max_headache = 0;
		switch (random(6)) {
		case 0: tell_me("Suddenly without a reason you get a bad headache."); break;
		case 1: tell_me("Suddenly without a reason you get a terrible headache."); break;
		case 2: tell_me("Suddenly without a reason you get a pounding headache."); break;
		case 3: tell_me("Suddenly with a perfectly good reason you get a bad headache."); break;
		case 4: tell_me("Suddenly with a perfectly good reason you get a terrible headache."); break;
		default: tell_me("Suddenly with a perfectly good reason you get a pounding headache."); break;
		}
//		if ((hit_point -= 3) < 1)
                if ((hit_point > 0)  &&  (hit_point -= 3) < 1)
		    hit_point = 1;
	    }
	} else if (headache && !(--headache))
	    tell_me("You no longer have a headache.");
	break;
    case 2:
	// Death's door. We don't heal - we bleed!
	if (hit_point < 0) {
	    tell_me((string)DESC_D->query_rnd_bleed_desc());

	    // //Graah: lots of hits bleed somewhat quicker
	    hit_point -= (3 + random(6) + (max_hp / 40));
	    if (hit_point < -(DEATH_HP(max_hp))) {
		tell_me("\nYour injuries are fatal.");
		catch(death(2), this_object()); // Bleed death, not death trap
		return;
	    }
	}
    case 4:
	if (query_condition(C_POISONED)) {
	    /* Poison! */
	    switch (random(6)) {
	    case 0: tell_me("Poison burns your blood."); break;
	    case 1: tell_me("You are poisoned..."); break;
	    case 2: tell_me("Poison makes you feel sick."); break;
	    case 3: tell_me("You feel dizzy.");
	    case 4: tell_me("You feel very sick. It's the poison..."); break;
	    default: tell_me("Your head spins. It's the poison!"); break;
	    }
	    /* More poison damage! Graah */
	    /* TODO: there should be poisons of different potency */
	    hit_point -= (1 + random(8) + random(8));
	    if (hit_point < -(DEATH_HP(max_hp))) {
		tell_me("\nThe poison kills you!");
		catch(death(2, this_object()));
		return;
	    }
	}
	break;
    case 5:
	forget_hunted(); /* Forget old enemies slowly... */
	break;
    case 7:
	// Sometimes call server for neat drunk effects.
	if (intoxicated && !random(4))
	    DRUNK_D->drunk_effect(this_object());
    case 9:
	if (!random(2) && query_condition(C_HALLUCINATING))
	    HALLU_D->hallu_effect(this_object());
    }

    if (liv_Flags & F_LIV_RESTING) {
	sp_delay -= 4;
	hp_delay -= 4;
	fp_delay -= 4;
    } else if (liv_Flags & F_LIV_SITTING) {
	sp_delay -= 3;
	hp_delay -= 3;
	fp_delay -= 3;
    } else {
	sp_delay -= 2;
	hp_delay -= 2;
	fp_delay -= 2;
    }

    // HPs:
    // - Poison prevents healing, sickness halves it.
    // - Sitting speeds it up by 25%, resting by 50%
    // - Intoxication adds 1 points / healing beat.
    // - Level bonus is linear; in effect, 1 point / 25 levels.
    // - Delay between beats depends on race (check out /basic/living/race.c)
    if (hp_delay < 1) {
	if (!cond)
	    cond = 4 | (query_condition(C_SICK) ? 1 : 0) | (query_condition(C_POISONED) ? 2 : 0);
	if (hit_point < max_hp && hit_point >= 0 && !(cond & 2)) {
	    // Old:      i = 1 + (random(20) + level) / 25 + (intoxicated ? 1 : 0) + query_stat(ST_HP_HEAL_RATE);
	    i = query_race_stat(RACE_HP_HEAL_RATE) + (random(20) + level) / 25 + (intoxicated ? 1 : 0) + query_stat(ST_HP_HEAL_RATE);
	    if (cond & 1)
		i = (i + random(2)) / 2;

	    // Do we have that stupid "heal ob"?
#if 0
	    if (present("heal ob", this_object()))
		i += 2;
#endif

	    hit_point += i;
	    if (hit_point > max_hp) hit_point = max_hp;
	}
	hp_delay = query_race_stat(RACE_HP_DELAY) << 1;
    }

    // SPs. Same rules as with HPs. Should intoxication heal less?
    if (sp_delay < 1) {
	if (!cond)
	    cond = 4 | (query_condition(C_SICK) ? 1 : 0) | (query_condition(C_POISONED) ? 2 : 0);
	if (spell_points < max_sp && !(cond & 2)) {
	    // Old: i = 1 + (random(20) + level) / 25 + (intoxicated ? 1 : 0) + query_stat(ST_SP_HEAL_RATE);
	    i = query_race_stat(RACE_SP_HEAL_RATE) + (random(20) + level) / 25 + (intoxicated ? 1 : 0) + query_stat(ST_SP_HEAL_RATE);
	    if (cond & 1)
		i = (i + random(2)) / 2;
	    spell_points += i;
	    if (spell_points > max_sp) spell_points = max_sp;
	    if(spell_points < 0) spell_points = 0;
	}
	sp_delay = query_race_stat(RACE_SP_DELAY) << 1;
    }

    // FPs. Same rules as with HPs... Have to consider fighters a bit, perhaps?
    // In addition, being stuffed adds 1 fp / beat.
    if (fp_delay < 1) {
	if (!cond)
	    cond = 4 | (query_condition(C_SICK) ? 1 : 0) | (query_condition(C_POISONED) ? 2 : 0);
	if (fatigue_points < max_fp && !(cond & 2)) {
	    // i = 1 + (random(20) + level) / 25 + (intoxicated ? 1 : 0) + query_stat(ST_FP_HEAL_RATE) + (stuffed ? 1 : 0);
	    i = query_race_stat(RACE_FP_HEAL_RATE) + (random(20) + level) / 25 + (intoxicated ? 1 : 0) + query_stat(ST_FP_HEAL_RATE) + (stuffed ? 1 : 0);
	    if (cond & 1)
		i = (i + random(2)) / 2;
	    fatigue_points += i;
	    if (fatigue_points > max_fp) fatigue_points = max_fp;
	    if(fatigue_points < 0) fatigue_points = 0;
	}
	fp_delay = query_race_stat(RACE_FP_DELAY) << 1;
    }

    if (stuffed) stuffed--;
    if (soaked) soaked--;

    // Now... If we're not fighting, we can do up to ACTIONS_NON_COMBAT
    // (currently 2) actions, compared to 1 action / round when fighting:
    if (attackers)
	i = 1;
    else
	i = ACTIONS_NON_COMBAT;

    while (i && sizeof(tmp = query(PLR_FULL_COMBAT_QUEUE))) {
	// NEW: optional fourth parameter is action's duration
	if (pointerp(tmp[0]) && sizeof(tmp[0]) > 3) {
	    if (--tmp[0][3] > 0) {
	        i--;
	        continue;
	    }
	}
	tmp = query(PLR_COMBAT_QUEUE);
	if (pointerp(tmp) && sizeof(tmp) > 1) {
	  if (to_object(tmp[1])) {
	    if (sizeof(tmp) > 2) {
		if (funcall(tmp[1], this_object(), &(tmp[2])))
		{
		    if (attackers) i = 0;
		    else i--;
		}
	    } else {
		if (funcall(tmp[1], this_object(), 0))
		{
		    if (attackers) i = 0;
		    else i--;
		}
	    }
	  }
	}
	else if (to_object(tmp) && funcall(tmp, this_object(), 0))
	{
	    if (attackers) i = 0;
	    else i--;
	}
	if (sizeof(tmp = query(PLR_FULL_COMBAT_QUEUE)) &&
	  pointerp(tmp[0]) && sizeof(tmp[0]) > 1
	  && to_object(tmp[0][0])) {
	    if (sizeof(tmp[0]) < 3)
		funcall(tmp[0][0], this_object(), 0);
	    else
		funcall(tmp[0][0], this_object(), &(tmp[0][2]));
	}
    }
    // NEW! Guild objects can set a hook to prevent hitting, for example,
    // when the player is casting a spell:
    if (i && attackers && !funcall(_busy, this_object()))
	attack();

    if (attackers && whimpy && hit_point < (whimpy * max_hp / 100)
      && is_fighting() && query_can_move())
	run_away();

    if (query(PLR_HPSP_REPORT_ON)) {
	if (hit_point != last_hp || spell_points != last_sp
	  || fatigue_points != last_fp){
	    last_hp = hit_point;
	    last_fp = fatigue_points;
	    last_sp = spell_points;
            tell_me(sprintf("< HP %d/%d | SP %d/%d "+
                "| FP %d/%d >\n",
		hit_point, max_hp,
		spell_points, max_sp,
		fatigue_points, max_fp), TELL_NO_WRAP);
	}
    }

    if (NextConditionCheck && (age >= NextConditionCheck))
	update_conditions();
    if (nextStatCheck && (age >= nextStatCheck))
	update_stats();
}

// Let's throw link-deaders out in two hours time 8.4.1992 //Frobozz
private void
throw_ld_out2()
{
    int i, m;
    object *obs;
    object ob;
    if (interactive()) return;

#ifdef LOG_ENTER
    log_file ("ENTER", sprintf("%s %s (throw ld out)\n",
	ctime(time())[4..15], cap_name));
#endif
    // Well, no more players hanging in the shop. ++Tron
    obs = all_inventory(this_object());
    if (obs) {
	i = sizeof(obs);
	while (--i) {
	    m = (int)obs[i]->query_value();
	    if (m) {
		add_money((m/2 > 666) ? 666 : m/2);
		if (!obs[i]->drop())
		    destruct(obs[i]);
	    }
	}
    }
    command("quit");
}

// Get players out for sure finally.
//++Tron

void
throw_ld_out()
{
    if (catch(throw_ld_out2())) {
	if (catch(throw_ld_out2()))
	    command("quit");
    }
}

// INFO 19.8.1992 //Frobozz
void
inform_us(string what)
{
    object *ulist;
    int i, u_is_coder, troll, u_is_troll;
    string r;

    ulist = users();
   what = sprintf("[Inform] %s %s.", cap_name, what);
    troll = (query_race() == "troll" || query_race() == "dark elf");

    for(i = sizeof(ulist) - 1; i >= 0; i--) {
	if (ulist[i]) {
	    u_is_coder = (int)ulist[i]->query_coder_level();
	    r = (string)ulist[i]->query_race();
	    u_is_troll = (r == "dark elf" || r == "troll");
	    /* If they want to hear and (we are not coders or they are) we'll
	       tell them, and if we both are trolls/dark elves or non-trolls/
	       non-dark-elves */

	    if (ulist[i] != this_object() &&
	      ulist[i]->query(PLR_INFO_ON) &&
	      ((!coder_level && level < 33) || u_is_coder) &&
	      (!INVIS_LEVEL || ulist[i]->query_level() > 32 || u_is_coder) &&
	      (u_is_coder || (troll && u_is_troll) || (!troll && !u_is_troll)))
		/* Invisible admin - coders shouldn't see it. //Graah */
		// Umh. Let's say, _deep invis_ admins (level 1000 and up). But;
		// why the should (co)admins be hiding from coders... -+ Doomdark +-
		if (!(INVIS_LEVEL >= 1000 && IS_ADMIN(coder_level) && !IS_ADMIN(u_is_coder)))
		    ulist[i]->tell_me(what, TELL_TYPE_INFO);
	}
    }
}

varargs int
second_life(status pk, status nicemk, object killer_ob, object corpse)
{
    object death_mark;
    string killer,		// Name of killer in gravestone
    rkiller;	// Name of the real killer, used in /log/KILLS.
    // It might be a room or some other death trap //Frobozz
    int i;
    string cr;

    if (previous_object() && previous_object() != this_object())
	if (IS_CODER(coder_level))
	    return illegal_patch("second_life");

    make_scar();

    ghost = 1;

    // Don't reduce ALL stats any more, just one or two.
    // Now only one; high stats cost MUCH.

    if (!pk && !nicemk) {
	if (level > 1) level = level - 1;

	i = 1;
	// Was: i = (level > 18 && random(200) < level) ? 2 : 1;
	while (i--) {
	    switch(random(4))
	    {
	    case 0:
		if (max_Str > 1) add_stat(ST_BASE_STR, -1);
		break;
	    case 1:
		if (max_Con > 1) add_stat(ST_BASE_CON, -1);
		break;
	    case 2:
		if (max_Dex > 1) add_stat(ST_BASE_DEX, -1);
		break;
	    case 3:
		if (max_Int > 1) add_stat(ST_BASE_INT, -1);
		break;
	    default:
		break;
	    }
	}
    }

    set_move_msg("min", "drifts around");
    set_move_msg("mout", "drifts");
    headache = intoxicated = stuffed = soaked = 0;

    if (!killer_ob) killer_ob = query_attack();
    if (!killer_ob) killer_ob = previous_object();

    if (killer_ob) {
	rkiller = (string)killer_ob->query_real_name();
	if (!rkiller) rkiller = (string)killer_ob->short(1);
	if (!rkiller) rkiller = "Someone";
    } else rkiller = "Something";

    killer = rkiller;
    // In case of death traps

    if (killer_ob && !living(killer_ob)) {
	killer = "Something";
	rkiller = "death trap";
    }

    killer = capitalize(killer);

    // No more 'Foo killed by Foo' 29.9.1992 //Frobozz

    if (killer == cap_name)
	if (name_of_last_attacker)
	    killer = capitalize(name_of_last_attacker);
	else
	    killer = "Something";

#ifdef LOG_KILLS
    if (killer_ob) {
	if (!rkiller) rkiller = "Someone";
	if (!(cr = creator(killer_ob))) cr = "<UNKNOWN>";
	log_file("KILLS", sprintf("%s (%d) by %s (%s) creator: %s at %s\n",
	    name,
	    level,
	    rkiller,
	    object_name(killer_ob),
	    cr,
	    ctime(time())
	  ));
	/* For the gravestone in the burial ground */
	if (!query_testplayer())
	    GRAVE_STONE -> log_dead(capitalize(name),killer);
    } else {
	if (previous_object()) {
	    if (!(cr = creator(previous_object()))) cr = "<UNKNOWN>";
	    log_file("KILLS", sprintf(
		"%s (%d) by %s (%s), creator: %s at %s.\n",
		name, level, (string) previous_object()->short(1),
		object_name(previous_object()), cr, ctime(time())));
	} else log_file("KILLS", sprintf("%s (%d)\n", name, level));

	if (!query_testplayer())
	    GRAVE_STONE -> log_dead(capitalize(name),"something");
    }
#endif
    tell_me("\nYou die.\nYou have a strange feeling.\n\
You can see your own dead body from above.\n");

    if(!query_testplayer())
        move_object(death_mark = clone_object(DEATH_MARK), this_object());

    // See /basic/living/death.c, we are "still dying" when this is set.
    // This is required when they quit and do other hacks during death.

    set(LIV_DEATH_MARK);

    if (level > 19 && !query_testplayer())
    {
	inform_us("just died");
     }
       dirty_stamp = 0;
       liv_Flags &= (~F_LIV_OK_TO_PK);
    // NEW! Let's also inform guild object about dying! -+ Doomdark 11-oct-95 +-
    if (guild_object)
	guild_object->guild_member_died(this_object());

    return 1;
}

int
remove_ghost()
{
    int compare;

    if (!ghost) return 0;
    ghost = dead = 0;

    tell_me("You feel a very strong magic force.\nYou are sucked away...");
    if (environment()) environment() -> tell_here(sprintf(
	    "Some mist disappears.\n%s appears in a solid form.", cap_name), this_object());

    set_move_msg("min", "arrives");
    set_move_msg("mout", "leaves");
    save_me(1);

    tell_me("You re-appear in a more solid form.");
    // Let's be sure PK-flag is now off, and combat ended:

    attackers = 0;

    compare = age + 120 + (2 * query_level()) * 2;

    if (dirty_stamp && dirty_stamp < compare) {
	liv_Flags &= (~F_LIV_OK_TO_PK);
	dirty_stamp = 0;
    }

    return 1;
}

status
invis()
{
    if (INVIS_LEVEL) {
	tell_me("You are already invisible.");
	return 1;
    }
    tell_me("You are now invisible.");
    // Why mmsgout? There should be a new message for this...
    environment()->tell_here(sprintf("%s %s.",
	query_name(), query_move_msg("mmout")), this_object());
    // cap_name = "Someone";
    set_invis(IL_INVISIBLE);	// After msg so our name is ok.
    return 1;
}

status
vis()
{
    if (!INVIS_LEVEL) {
	tell_me("You are not invisible.");
	return 1;
    }
    set_invis(IL_VISIBLE);
    tell_me("You are now visible.");
    // cap_name = capitalize(name);
    environment()->tell_here(sprintf("%s %s.",
	query_name(), query_move_msg("mmin")), this_object());
    return 1;
}

// Recursively compute the values of the inventory.
// Beware that object may selfdestruct when asked for query_value().

int
compute_values(object ob)
{
    int v;
    while(ob) {
	int tmp;
	object next_ob;

	next_ob = next_inventory(ob);
	tmp = (int)ob->query_value();
	if(tmp > 700) // Taxes, used to be 1000 //Frobozz
	    tmp = 700;
	v += tmp;
	if (ob && first_inventory(ob))
	    v += compute_values(first_inventory(ob));
	ob = next_ob;
    }
    return v;
}

varargs int
save_me(int flag)
// bit 0 -> compute values of items player has;
// bit 1 -> print message of saving data.
// bit 2 -> should we remove object based conditions; ie. are we quitting
// bit 3 -> if we are going ld.
{
    if (flag & 1)
	tot_value = compute_values(first_inventory(this_object()));
    else tot_value = 0;

    // NEW: Let's also save guild data.

    save_guild(flag & 2);

    // Code found in /basic/living/guild.c

    compute_auto_str();
    last_saved = time(); // "Left the game 1 D ago." etc...

	save_object(PATH_FOR_PLAYER_SAVE(name));
}

nomask int
illegal_patch(string what)
{
    tell_me("You are struck by a mental bolt from the interior of the " MUD_NAME ".");
    log_file("ILLEGAL", ctime(time()) + ":\n" + name + " " + what + ".\n");
    return 0;
}

void
load_auto_obj(string str)
{
    string file, argument, rest;
    object ob;

    while(str && str != "") {
	if(sscanf(str, "%s:%s^!%s", file, argument, rest) != 3) {
	    tell_me("Auto load string corrupt!");
	    return;
	}
	str = rest;
	if (!(catch(ob = clone_object(file)))) {
	    if(argument)
		catch(ob->init_arg(argument));
	    move_object(ob, this_object());
	} else {
	    tell_me("ERROR: Couldn't clone one of autoloading objects!");
	}
    }
}

string
compute_auto_str()
{
    object ob;
    string str,tmp;

    tmp = "";

    ob = first_inventory(this_object());
    while(ob) {
	str = (string)ob->query_auto_load();
	ob = next_inventory(ob);
	if(!str) continue;
	sscanf(str, "%s^!", str);
	tmp = tmp + str + "^!";
    }
    auto_load = tmp;
}

void
make_scar()
{
    if (level < 10) return;
    scar |= 1 << random(MAX_SCAR);
}

varargs string
show_scar(status no_write_please)
{
    int i, j, first, old_value;
    string *scar_desc, *tmp;
    /* Added some new scars 30.3.1992 //Frobozz */

    scar_desc = ({ "left leg", "right leg", "nose", "left arm", "right arm",
      "left hand", "right hand", "forehead", "left cheek",
      "right cheek", "chest", "jaw", "left side", "right side",
      "back", "pelvis", "stomach", "neck" });
    j = first = 1;

    old_value = scar;
    tmp = ({});

    while(i < MAX_SCAR) {
	if(scar & j) {
	    old_value &= ~j;
	    if(first) {
		tmp = ({ sprintf("%s %s", Possessive(), scar_desc[i]) });
		first = 0;
	    } else
		tmp += ({ sprintf("%s %s", Possessive(), scar_desc[i]) });
	}
	j *= 2;
	i++;
    }

    if (no_write_please) {
	if (!sizeof(tmp)) return "";
	return sprintf("%s has a scar on %s.",
	  query_name(), implode(tmp, ", ", " and "));
    }

    if(sizeof(tmp) > 1) tell_me("%s has a scar on %s.",
	  query_name(), implode(tmp, ", ", " and "));
}

void clear_scar() { scar = 0; }

nomask void
add_standard_commands()
{
    Aa("communicate","say"); Aa2("communicate","'", 1);
    Aa("change_password", "password");
    Aa("save_character","save");
    Aa("quit","quit");

    add_action("command_driver","",1);
}

object
check_fight(int silently)
{
    object ob;
    if (ob = is_fighting()) {
	if (!silently) tell_me("Not now, you are fighting!");
	return ob;
    }
    return 0;
}

// this allows forcing TP, and makes different tools work with the
// static functions..	//Rincewind

int
force_us(string cmd) 
{
    object ob;
    status b;
    string err;

    if (!(ob = TP)) return 0;

    if (ob != this_object()) {
	if (query_coder_level()) {
	    tell_me(sprintf("%s failed to force you to %s.",
		ob->query_real_name(), cmd));
	    return 0;
	}
    }
    forcing++;
    if((err = catch(b = command(cmd)))) {
	tell_me(err);
	b = 0;
    }
    forcing--;
    return b;
}

// Show the list of new messages when going invis and vis
status
show_msg()
{
    int i;
    string s;
    s = "Entering:";
    for (i = 0; i < sizeof(newmmsgin); i++)
	s = sprintf("%s\n%d. %s", s, (i + 1), newmmsgin[i]);
    s += "\nExiting:";
    for (i = 0; i < sizeof(newmmsgout); i++)
	s = sprintf("%s\n%d. %s", s, (i + 1), newmmsgout[i]);
    tell_me(s);
    return 1;
}

// Pick new messages when going invis and vis from list 5.6.1992 //Frobozz
int
mmsgs(string str)
{
    int arg;
    if(!sscanf(str,"%d",arg) || arg<1 || arg>sizeof(newmmsgout)) {
	tell_me("USAGE: 'messages <arg>' where <arg> is a number between 1 and "+sizeof(newmmsgout)+".\n'messages' (without argument) shows you the list of messages.");
	return 1;
    }
    set_move_msg("mmout", newmmsgout[arg - 1]);
    set_move_msg("mmin", newmmsgin[arg - 1]);
    tell_me("Ok.");
    return 1;
}

mixed test() { return funcall(_busy, this_object()); }

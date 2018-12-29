/* New Guild of Thieves
 *
 * Skills
 * -    Backstabbing, Sneaking, Throwing, Lockpicking, Stealing, Appraising,
 *      Tumbling
 * -    range from 0 to 10000
 * -    almost compatible with New Stick skills
 * -    tlook is not a skill, but so what...
 *
 * Skill difficulties
 * -    average is 5000
 * -    check thief.h for more information
 *
 * What to do if the thief loses his mark?
 * -    find his last log line in GDIR/THIEF_LOG (/players/thieves/THIEF_LOG)
 * -    call the init_arg() function in his/her thief tattoo with the data
 * ("0#1#1#1111#1111#1111#1111#1111#1111#1111#0#0#title#0#tattoo desc")
 * as an argument.
 *
 */

// Tidied a bit 09-aug-95 by Doomdark
/* Rewrote several pieces Nov. 07 1995 - Kelgath */
/* Transfered skills to seperate files in ~thieves/skills and tidied a
   bit more 1/1/96 --Colt */
// Can't fucking quit & relogin reset stab cheat again. /Graah
// 21-Jul-96: Channel handles isolated rooms now. /Graah
// Now checking for idling before adding tumbling skill //Frobozz
// Tumble now costs some fp (same goes for dodge/parry) //Sumppen
// 5-Sep-96 / Graah: Warmup (like ninjas meditation). Takes time,
//     prevents other skill use (except tumbling) during the exercise.
// 07-oct-96 Doomdark; slowly rewriting to use new features like
//	mappings; code looks horrible as is.
// 04-nov-96: Doomdark; did some small changes, supplied by Kelgath; for example,
//	now most of the commands are in separate file (stupid_commands.c)
// 13-nov-96: updates ++Skarda	
// 15-Apr-97 / Graah: Show if/when Rank is Advanced.
// 20-Jun-97 / Graah: Support for GUILDLESS (/include/guild.h) added.
// 10-Jul-97 / Graah: Re-checked troll and half-dorc training.
// 10-Jul-97 / Graah: Skill training now has level maximum, to prevent
// cheating with low level thief!
// Sign command for the joining application coming up...
// Also 'join <name>' for enforcers/demons.
// 11-Jul-97 / Graah: Moving commands to bin dir...
// 23-Sep-97 / Graah: New system to allow multiple stabbing
// 25-Sep-97 / Graah: Tumble, stab and tth crits use same rates.
// 02-Apr-98 / Kieve: Took out the max level checks
#include <treasure.h>

#include <attack.h>

#include <tune.h>

/* This has to be moved to another location; inherits seem to have to come
 * before prototypes.
 */
#if 0
#include "/guilds/thieves/thief.h"
#endif

#include <room_defs.h>
#include <stats.h>

#define	TMARK_C

#define TUMBLE_COST	 1
#define TUMBSEC		30

#define SA(x) skill_adj(x)
#define GUILDD_D "/bin/daemons/guildd"

#define BIN_CHANNEL_D "/bin/daemons/channel_d"

/* Define this if crimes are active */
/* #define CRIME */

inherit "/guilds/thieves/skills/criticals";

// ... And this was moved from the earlier point by Doomdark
// 18-Dec-97
#if 1
#include "thief.h"
#endif
/* Bunch of variables that are needed... */
mapping skills;
int success, targ_lev, sneak, t_level, guildmaster, tth;
string pretitle, tattoo_desc;
int ban; /* Ban until time() >= ban...   fuck... */
int wea_sharp, delay_steal, show_tattoo, delay_stab, stab_cheat;
int delay_warmup;
int steal_tax, tax_time; /* Unpaid taxes and time last paid */
object owner, target, summoner,summon_room;
int summon_time;
status cook_flag;
int cantflag;
object LAST_STABBED; //Last victim tumble/stabbed
string last_save;
status already_mort;

/* And some non-saveable ones too */
static mapping thief_cmds;
static int tumbtime, top_checked, advantage;
static int moons; /* How light are moons? */
status cdr;
static string *setlines;
static string setmode;
static mapping stabbed; // Who have we stabbed lately?

static int hidden;
static int pk;

/* From whom we tried to steal last time? Prevents cheat. */
static object last_steal, item;

#include    "skills/gm_commands.c"
#include	"skills/stupid_commands.c"
#include	"skills/demon_commands.c"
int flg_tumble;
#include 	"skills/tumble.c"

void guild_mem_ld(object me);

int
still_thief(int silently)
{
    string name, temp;
    object new_owner, ob, att;
    int tmp;

    new_owner = environment();

    if (!new_owner)
	new_owner = TP;

    if (new_owner)
	owner = new_owner;

    if (!owner)
	return 0;

    if (owner && !living(owner))
    {
	destruct(this_object());
	return 0;
    }

    // Cheating with hide?
    if (!hidden && (int)owner->query_invis() == IL_HIDDEN)
      {
	owner->tell_me("Stopped hiding.");
	owner->set_invis(0);
      }

    /* Note: this is the level of thieven skills, not real level! */
    t_level = (int)owner->query_level();
    cdr = ((int)owner->query_coder_level() || (int)owner->query_testplayer());

    // Racial adjustments. Race may change suddenly, and this won't
    // be updated at that point, but who cares...

    switch ((string)owner->query_race())
    {
    case "hobbit":
	advantage = 4;
	t_level += 2;
	break;
    case "dwarf":
	advantage = 3;
	t_level += 1;	   
	break;
    case "elf":
	advantage = 2;
	break;
    case "half-orc":
	advantage = -1;
	break;
    case "troll":
	advantage = -3;
	break;
    default:
	advantage = 0;
	break;
    }
    /* Why have max when levels are unlimited or at least go
       to 70+ ? 
      if (t_level > 40)
	 t_level = 40;
    */
    // Cannot use any abilities when dead
    if (owner->query_ghost())
	return 0;

    if ((att = (object)owner->query_attack()) &&
      (ob = present("guild_guard", environment(owner))) &&
      !ob->is_fighting() && (string)att->query_guild() != GN_THIEF
      && !att->id("guild_guard"))
	ob->attack_object(owner->query_attack());

    // Call guild to check guildmaster & enforcer status

    return (GUILD->is_thief(owner) != 0);
}

string
do_short(int arg, object who)
{
    if (show_tattoo)  {
	if (tattoo_desc)
	    return tattoo_desc;
	return "Thieve's Tattoo";
    }
    return 0;
}

void
reset_treasure()
{
    still_thief(1);
    moon_light(); /* donna need to call to often */
}

void
create()
{
    sneak = hidden = delay_steal = top_checked = delay_warmup = 0;
    show_tattoo = cantflag = 1;
    tumbtime = time();
    tattoo_desc = "Thieve's Tattoo";
    set_long("The guild tattoo pictures a cloak and a dagger.  Type \
'thelp' for information about your skills ('skills' to list skills).");
    set_name("Thieve's Tattoo");
    set_short(#'do_short);
    set_weight(0);
    set_size(({ 5, 3, 0 }));
    set_id(({ "tmark", "tattoo", "guild_mark", "mark", "thieve's tattoo", "thief tattoo" }));
    skills = ([
      SK_STAB : 0,
      SK_SNEAK : 0,
      SK_TTH : 0,
      SK_PICK : 0,
      SK_STEAL : 0,
      SK_APPRAISE : 0,
      SK_POISON : 0,
      SK_TUMBLING : 0
    ]);
}

/* Let's allow getting but not dropping. Of course, there won't be n+1 thief marks
 * in a pile in the Tristeza market square to be taken but...
 */
int get() { return 1; }

int
drop(status silent)
{
#if 0
    /* This causes spam, and, tattoos can not exactly be dropped
       I think... /Graah */
    if (!silent && this_player() && this_player() == owner)
	this_player()->tell_me("Dropping your precious guild mark? Are you NUTS?");
#endif
    return 1;
}

/*********************************************************************

			   QUERY FUNCTIONS

**********************************************************************/

mixed query_guildmaster() { return guildmaster; }
mixed query_ban() { return ban; }
mixed query_adv() { return advantage; }
mixed query_last_steal() { return last_steal; }
mixed query_tth() { return tth; }
string query_tattoo_desc() { return tattoo_desc; }
nomask string query_pretitle() { return pretitle; }
int query_tax() { return steal_tax; }
int query_tax_time() { return tax_time; }
int query_steal_tax() { return steal_tax; }

int query_delay_stab() { return delay_stab; }
int query_advantage() { return advantage; }
query_thief_level() { return t_level; }
int query_sneak() { return (sneak ? sneak : 0); }
int query_tumbtime() { return (tumbtime + TUMBSEC - time()); }
object query_summoner() { return summoner; }
object query_summon_room() { return summon_room; }

int query_hidden() { return hidden; }
void set_hidden(int h) { hidden = h; }

/* This should be a bit more efficient than the old way...
   Not tons of functions to slow things down. */

int
query_skills(int type)
{
    if(type < 8 && type >= 0) return skills[type];
    else if(type = 8)
	return ((skills[SK_STAB] + skills[SK_SNEAK] + skills[SK_TTH] +
	    skills[SK_PICK] + skills[SK_STEAL] + skills[SK_APPRAISE] +
	    skills[SK_TUMBLING] + skills[SK_POISON])/8);
    //This better not happen.. if so.. the call was made incorrectly.	
    else return 0;

}

/********************************************************************

		   SET/ADD  FUNCTIONS

*********************************************************************/

void
set_guildmaster(mixed x)
{
    guildmaster = x;
    if (cdr)
    {
	if (guildmaster)
	    if (guildmaster >= TR_DEMON)
		CHANNEL_D->add_channel_member(TO,1,1,1);
	    else
		CHANNEL_D->add_channel_member(TO,1,1,-1);
//	else
//	    CHANNEL_D->add_channel_member(TO,1,-1,-1);
    }
}

void set_last_steal(object x) { last_steal = x; }
void set_ban(mixed x) { ban = x; }
void set_tth(mixed x) { tth = x; }
void set_tattoo_desc(mixed x) { if (valid_desc(x)) tattoo_desc = x; }

nomask void
set_pretitle(string str)
{
    if (stringp(str) && sizeof(str) < 15) pretitle = str;
}

void set_tax_time(mixed t) { tax_time = t; }
void set_steal_tax(mixed x) { steal_tax = x; if (steal_tax < 0) steal_tax = 0; }
void add_steal_tax(mixed x) { steal_tax += x; if (steal_tax < 0) steal_tax = 0; }

// Again.. seems a silly thing to have a function for each skill.
void set_skill(int skill, int amount) { skills[skill] = amount; }

void set_delay_stab(int x) { delay_stab = x; }
void set_sneak(int x) { sneak = x; }
void add_stab_skill() { skills[SK_STAB] = advance_skill(skills[SK_STAB], DIFF_STAB); }

void
set_summoner(object ob, object where, int when)
{
    summoner = ob;
    summon_room = where;
    summon_time = (when+90); // minute and 1/2 to respond
}

/***********************************************************************/

string
query_guild_save()
{
    return
    guildmaster + "#" +
    show_tattoo + "#" +
    cantflag + "#" +
    skills[SK_STAB]  + "#" +
    skills[SK_SNEAK] + "#" +
    skills[SK_TTH] + "#" +
    skills[SK_PICK]  + "#" +
    skills[SK_STEAL] + "#" +
    skills[SK_APPRAISE] + "#" +
    skills[SK_TUMBLING] + "#" +
    steal_tax + "#" +
    tax_time + "#" +
    pretitle + "#" +
    ban + "#" +
    tattoo_desc + "#"+
    skills[SK_POISON];
}

/*
 * Prevent normal melee attack if we are busy with other things.
 * //Graah  18-Jan-96
 */
status
guild_prevent_attack()
{
    return (tth || cook_flag || delay_warmup);
}

void
initialize_guild(string arg)
{
    string current_gm, *current_co_gm, *current_enforcers, *current_demons;
    int tmp_poison;
    string rn;

    /* Duh. Should use explode() here... -+ Doomdark +- */
    if (!arg ||
      sscanf(arg,"%d#%d#%d#%d#%d#%d#%d#%d#%d#%d#%d#%d#%s#%d#%s#%d",
	guildmaster, show_tattoo, cantflag, skills[SK_STAB], skills[SK_SNEAK],
	skills[SK_TTH], skills[SK_PICK], skills[SK_STEAL], skills[SK_APPRAISE],
	skills[SK_TUMBLING],
	steal_tax,tax_time,pretitle,ban,tattoo_desc,skills[SK_POISON]) <16)
    {
	write("Thief guild restoration might have failed!\n");
    }

    /* Just to make sure */
    if (environment())
    {
	environment()->set_guild("thief");
	environment()->set_guild_object(this_object());
	environment()->set_guild_object_name(GDIR + "thief_mark_new");
	environment()->set_guild_save(query_guild_save());
	environment()->set_guild_hook( ([
	    G_HOOK_HIT_PLAYER : #'query_guild_ac_rate,
	    G_HOOK_QUIT       : #'guild_mem_quit,
	    G_HOOK_DEATH      : #'guild_mem_died,
	    G_HOOK_GO_MORT    : #'guild_mem_mort,
	    G_HOOK_GO_LD      : #'guild_mem_ld,
	    G_HOOK_PREVENT_ATTACK : #'guild_prevent_attack,
	  ]) );
    }
    if (!tattoo_desc) tattoo_desc = "Thieve's Tattoo";

#ifdef OLD_GOVERNMENT
    current_gm = (string) GM_D->query_gm();
    current_co_gm = (string *) GM_D->query_co_gm();
#endif
    current_enforcers = (string *) GM_D->query_enforcers();
    current_demons = (string *) GM_D->query_demons();

    rn = (string) environment()->query_real_name();

#ifdef OLD_GOVERNMENT
    guildmaster = 0;
    if (rn == current_gm) {
	GM_D->set_gm_last_on();
	guildmaster = TR_GM;
    } else if (member_array(rn, current_co_gm) != -1)
	guildmaster = TR_CO_GM;
    else if (member_array(rn, current_enforcers) != -1)
	guildmaster = TR_ENFORCER;
    else  if (member_array(rn, current_demons) != -1)
	guildmaster = TR_DEMON;
    else
	guildmaster = 0;
#endif
#ifndef OLD_GOVERNMENT
    if(current_demons  &&  member_array(rn, current_demons) != -1)
	guildmaster = TR_DEMON;
#endif

#ifdef GUILDLESS
    // Testplayers and coders are still in guild.
    if (this_object()->query_coder_level() ||
	this_object()->query_is_testplayer())
      {
#endif
    if (guildmaster)
       {
        if (guildmaster >= TR_DEMON)
	    CHANNEL_D->add_channel_member(TO,1,1,1);
        else
	    CHANNEL_D->add_channel_member(TO,1,1,-1);
       }
#ifdef GUILDLESS
      }
#endif

    still_thief(1);
    tmp_poison = (900 + t_level * 80) + (int)environment()->query_stat(ST_INT) * 10 + random(100);
    if (skills[SK_POISON] < 1024) skills[SK_POISON] = tmp_poison;

    // Not at the moment. Return this if they start using it again. / Graah.
    //   environment()->tell_me(
    //     "A lot of thanks to the quit & relogin stab reset cheaters, \
    //you now have to wait full two minutes before you can even stab \
    //the first time. Isn't it nice you have such cheaters in your guild?");
    //   stab_cheat = 60 * 2;
    //   call_out("stab_cheat_over", stab_cheat);

}

void
init_arg(string arg)
{
    int x, i;

    if (!arg)
    {
	for(i=0;i<8;i++) skills[i] = 1024;
	pretitle = "1";
	tattoo_desc = "Thieve's Tattoo";
	guildmaster = steal_tax = 0;
	show_tattoo = cantflag = 1;
	tax_time = time();
	return;
    }

    if (sscanf(arg, "%d#%d#%d#%d#%d#%d#%d#%d#%d#%d#%d#%d#%s#%d#%s#%d",
	guildmaster,show_tattoo,cantflag, skills[SK_STAB], skills[SK_SNEAK],
	skills[SK_TTH], skills[SK_PICK], skills[SK_STEAL], skills[SK_APPRAISE],
	skills[SK_TUMBLING],
	steal_tax,tax_time,pretitle,ban,tattoo_desc,skills[SK_POISON]) < 16)
    {
	skills[SK_POISON] = 1024;
	if (sscanf(arg, "%d#%d#%d#%d#%d#%d#%d#%d#%d#%d#%d#%d#%s#%d#%s",
	    guildmaster, show_tattoo, cantflag, skills[SK_STAB], skills[SK_SNEAK],
	    skills[SK_TTH], skills[SK_PICK], skills[SK_STEAL], skills[SK_APPRAISE],
	    skills[SK_TUMBLING],
	    steal_tax, tax_time, pretitle, ban, tattoo_desc) < 15)
	{
	    tattoo_desc = "Thieve's Tattoo";

	    if (sscanf(arg, "%d#%d#%d#%d#%d#%d#%d#%d#%d#%d#%d#%d#%s#%d",
		guildmaster, show_tattoo, cantflag, skills[SK_STAB], skills[SK_SNEAK],
		skills[SK_TTH], skills[SK_PICK], skills[SK_STEAL], skills[SK_APPRAISE],
		skills[SK_TUMBLING],
		steal_tax, tax_time, pretitle, ban) < 14)
	    {
		ban = 0;

		if (sscanf(arg, "%d#%d#%d#%d#%d#%d#%d#%d#%d#%d#%d#%d#%s",
		    guildmaster, show_tattoo, cantflag, skills[SK_STAB], skills[SK_SNEAK],
		    skills[SK_TTH], skills[SK_PICK], skills[SK_STEAL], skills[SK_APPRAISE],
		    skills[SK_TUMBLING],
		    steal_tax, tax_time, pretitle) < 13)
		{
		    pretitle = "1";

		    if (sscanf(arg, "%d#%d#%d#%d#%d#%d#%d#%d#%d#%d#%d#%d",
			guildmaster, show_tattoo, cantflag, skills[SK_STAB], skills[SK_SNEAK],
			skills[SK_TTH], skills[SK_PICK], skills[SK_STEAL], skills[SK_APPRAISE],
			skills[SK_TUMBLING],
			steal_tax, tax_time) < 12)
		    {
			environment()->tell_me(
			  "TROUBLE: Thieve's Guild save has failed. Contact an admin or co-admin.");
		    }
		}
	    }
	}
    }

    /* Should start at 1024 */
    if (skills[SK_TUMBLING] < 500)
	skills[SK_TUMBLING] += 1024;

    /* Convert into new mark */
    environment()->set_guild("thief");
    environment()->set_guild_object(this_object());
    environment()->set_guild_object_name(GDIR + "thief_mark_new");
    environment()->set_guild_save(query_guild_save());
    environment()->set_guild_hook( ([
	    G_HOOK_HIT_PLAYER : #'query_guild_ac_rate,
	    G_HOOK_QUIT       : #'guild_mem_quit,
	    G_HOOK_DEATH      : #'guild_mem_died,
	    G_HOOK_GO_MORT    : #'guild_mem_mort,
	    G_HOOK_GO_LD      : #'guild_mem_ld,
	    G_HOOK_PREVENT_ATTACK : #'guild_prevent_attack,
      ]) );
    still_thief(1);
}

/* Can the tattoo short be accepted? */
int
valid_desc(string x)
{
    int i;

    if (!x)
	return 0;

    if (sizeof(x) > 30)
    {
	owner->tell_me("Too long description! Max 30 characters.");
	return 0;
    }

    for (i = (sizeof(x) - 1); i >= 0; i--)
	if (x[i] < ' ' || x[i] == '#')
	{
	    owner->tell_me("Illegal character in description!");
	    return 0;
	}

    return 1;
}

/* An old thief switches into new mark */
void
convert()
{
  string old_thief;
  int sb, kakka;
  int i, j, resign_date;

//  sb = 900 + t_level * 80;
  sb = 900 + 80;
//  kakka = (int)owner->query_level();
  kakka = 1;

  skills[SK_TTH]      = sb + 1 * 10 + random(100);
  skills[SK_STAB]     = sb + 1 * 10 + random(100);
  skills[SK_SNEAK]    = sb + 1 * 10 + random(100);
  skills[SK_PICK]     = sb + 1 * 20 + random(100);
  skills[SK_STEAL]    = sb + 1 * 20 + random(100);
  skills[SK_APPRAISE] = sb + 1 * 40 + random(100);
  /* Just a small level bonus */
  skills[SK_TUMBLING] = 1024 + 50 * (kakka > 20 ? (kakka - 20) : 0);

#if 0
  skills[SK_TTH]      = sb + (int)owner->query_stat(ST_DEX) * 10 + random(100);
  skills[SK_STAB]     = sb + (int)owner->query_str() * 10 + random(100);
  skills[SK_SNEAK]    = sb + (int)owner->query_stat(ST_DEX) * 10 + random(100);
  skills[SK_PICK]     = sb + (int)owner->query_int() * 20 + random(100);
  skills[SK_STEAL]    = sb + (int)owner->query_stat(ST_DEX) * 20 + random(100);
  skills[SK_APPRAISE] = sb + (int)owner->query_int() * 40 + random(100);
  /* Just a small level bonus */
  skills[SK_TUMBLING] = 1024 + 50 * (kakka > 20 ? (kakka - 20) : 0);
#endif

  steal_tax = 0;
  tax_time = time();

#if 0
  if (stringp(old_thief =
	      (string)GUILDD_D->restore_guild_data(
		(string)environment()->query_real_name(),GN_THIEF)))
#else
    if(0)
#endif
    {

write("Debug: old thief, it says: \"" + old_thief + "\".\n");

      if (sscanf(old_thief,
		 "%d#%d#%d#%d#%d#%d#%d#%d#%d#%d#%d#%d#%s#%d#%s#%d#%d",
		 guildmaster,show_tattoo,cantflag, skills[SK_STAB],
		 skills[SK_SNEAK], skills[SK_TTH], skills[SK_PICK],
		 skills[SK_STEAL], skills[SK_APPRAISE], skills[SK_TUMBLING],
		 steal_tax, tax_time, pretitle, ban, tattoo_desc,
		 skills[SK_POISON],
		 resign_date) != 17)
	{
	  environment()->tell_me("Old guild data restore unsuccessful.  \
Attempt logged.");
	}
      else
	if (resign_date > 800000000) {
	  for(i=0;i<8;i++) skills[i] -= skills[i]/10;
	  resign_date = time()- resign_date;
	  resign_date /= 86400;
	  if (resign_date > 100) resign_date = 100;

	  for(i=0;i < resign_date;i++)
	    for(j=0;j<8;j++) skills[j] = (skills[j] * 5)/100;

	  environment()->tell_me("Skills penalized for " +
				 resign_date + " days of absence.");
	}
    }

  initialize_guild(query_guild_save());
}

// Into internal skills
void
mkskills()
{
    if (!owner)
	return;

#define CONVRT(x) NEWSKILL((x) * 7 + (x > 10000 ? (x - 10000) * 50 : 0))

    owner->set_liv_Skill("backstab", CONVRT(skills[SK_STAB]));
    owner->set_liv_Skill("throw", CONVRT(skills[SK_TTH]));
    owner->set_liv_Skill("pick", CONVRT(skills[SK_PICK]));
    owner->set_liv_Skill("sneak", CONVRT(skills[SK_SNEAK]));
    owner->set_liv_Skill("steal", CONVRT(skills[SK_STEAL]));
    owner->set_liv_Skill("appraise", CONVRT(skills[SK_APPRAISE]));
    owner->set_liv_Skill("tumble", CONVRT(skills[SK_TUMBLING]));
}

int check_ban(int silently)
{
    int b;

    if (time() > ban)
	return ban = 0;

    b=ban-time();

    if (!silently)
	TM(sprintf("You can't stab nor tth due Wrath of Skert for %d %s.",
	    ((b / 60) ? b / 60 : b),
	    ((b / 60) ? "minutes" : "seconds")));

    return b;
}

mapping
query_guild_commands()
{

    return ([
      "appraise": "appraise", "app": "appraise",
      "peek": "peek",
      "consider": "consider", "cons": "consider",
      "pick": "pick",
      "disguise": "disguise",
      "skills": "thief_skills",
      "sneak": "sneak", "hide": "hide",
      "stab": "stab", "backstab": "stab",
      "steal" : "steal", "rob" : "steal",
      "brew":"brew_poison",
      "taunt": "taunt", "insult": "taunt",
      "thief": "thief_help", "thelp": "thief_help",
      "chant": "chant", "cant": "chant", "tchannel": "chant",
      "canttoggle": "canttoggle", "chanttoggle": "canttoggle",
      "gmctoggle": "gmctoggle",
      "canttail" : "cantread",
      "tpose": "tpose",
      "tth": "tth", "stth": "tth",
      "tlook": "tlook",
      "refresh": "refresh",
      "maletitles": "tits", "femaletitles": "tits",
      "neutertitles": "tits", "alltitles": "tits",
      "enforcers": "enforcerlist",
      "taxes": "taxes",
      "top": "top",
      "warmup": "warmup",

      /* Guildmaster commands */
      "gag": "gag_cmd",
      "ungag": "gag_cmd",
      "whogag": "gag_cmd",
      "promote": "manage_gm",
      "demote": "manage_gm",
      "promotegm": "manage_gm",
      "demotegm": "manage_gm",
      "tax": "tax_set",
      "set": "gmeditor",
      "kickout":"kickout",
      "wrath": "wrath",

      /* Enforcer/GM cmds */
      "resign": "resign_gm",
      "show": "showlogs",
      "gmc": "gmc", /* Damn GM/Enforcer channel */
      "gmctail" : "gmcread",

      /* Demon cmds */
      "demontail" : "demonread",
      "demon" : "demon", /* Demon channel */
      "demontoggle" : "demontoggle",
      "goto": "goto",

      "save": "save_cmd",
      "tweight": "thief_weight",
      "cook":"cook_corpse",

      "form": "thief_cmd",
      "sign": "thief_cmd",
      "join": "thief_cmd",
    ]);
}

void force_check() { TOP_TEN->check_top_ten(owner); }


/* Why on earth have 2 functions for this?.. now only one. */
status refresh(string arg)
{
    object new_tattoo;

    if(sneak)
        return notify_fail("Not while you're sneaking!\n"), 0;

    if (!arg) {
	environment()->tell_me("Refreshing thief commands.");
	move_object(this_object(), owner);
	still_thief(1);
	TM("Your tattoo glows brightly for a moment.");
	return 1;
    }
    else if (lower_case(arg) == "tattoo")
    {
	environment(TO)->tell_me("Updating into new version of tattoo.");
	new_tattoo = clone_object(GDIR + "thief_mark_new");
	if (!new_tattoo)
	{
	    TM("Something went wrong. Operation canceled.");
	    return 1;
	}

	if (!cdr && owner->query_invis())
	    owner->vis();

	move_object(new_tattoo, owner);
	new_tattoo->initialize_guild(query_guild_save());
	new_tattoo->set_stabbed(stabbed);
	owner->tell_me("Tattoo updated successfully.");
	destruct(this_object());
	return 1;
    }
    else
	return notify_fail("Usage: 'refresh' (no arguments), or 'refresh \
tattoo'.\n"), 0;
}

// Moving commands to bin/ dir...
status
thief_cmd(string arg)
{
  string c;

  c = lower_case(query_verb());

  if (file_size(GDIR + "bin/_" + c + ".c") < 1)
    return notify_fail("This command is currently unavailable.\n"), 0;

  return (status)call_other(GDIR + "bin/_" + c, c + "_cmd",
			    this_object(), owner, arg);
}


status thief_help(string str)
{
    string tmp1, tmp2;

    if (!str)
	return notify_fail("Type 'thelp index' to get the list of different \
helps.\n"), 0;

    if (sscanf(str, "%s/%s", tmp1, tmp2) == 2 ||
      sscanf(str, "%s.%s", tmp1, tmp2) == 2 ||
      sscanf(str, "%s %s", tmp1, tmp2) == 2)
	return notify_fail("Hey! What are you trying to do?\n"), 0;

#ifdef OLD_GOVERNMENT
    if (str == "enforcer" && guildmaster < TR_ENFORCER && !cdr)
    {
	TPT("You can't get this help.");
	return 1;
    }

    if (str == "guildmaster" && guildmaster < TR_CO_GM && !cdr)
    {
	TPT("You can't get this help.");
	return 1;
    }
#endif

    // Use players more to show help. --Val

    if (tmp1 = read_file(GDIR + "doc/" + str))
	TP->more(explode(tmp1, "\n"));
    else
	TPT("No help on that subject. Type 'thelp index' to get the list.");

    return 1;
}

static int rank_no(int s)
{
  int x;

  if (s <= 0) return 0;
  x = s / (10000 / ADJ_AMOUNT);
  if (x > (ADJ_AMOUNT - 1)) x = (ADJ_AMOUNT - 1);
  return x;
}

static string
rankname(int x)
{
  /* 39 */
  return ({
    "Ridiculous", "Very Terrible", "Terrible",
      "Harmless", "Mostly Harmless",
      "Quite Awful", "Awful", "Not Awful",
      "Extremely Bad", "Very Bad", "Quite Bad", "Bad", "Somewhat Bad",
      "Very Poor", "Poor", "Somewhat Poor", "Not Poor",
      "Fair", "Much Less Than Average",
      "Below Average", "Slightly Below Average", "Average",
      "Slightly Above Average", "Above Average",
      "Much Better Than Average",
      "Almost Good", "Quite Good", "Rather Good", "Good",
      "Very Good", "Extremely Good",
      "Great", "Excellent",
      "Mighty", "Legendary", "Elite",
      "Master", "Grand Master",
      "Ultimate" })[x];
}

string skill_adj(int sk)
{
  int x;

  x = sk / (10000 / ADJ_AMOUNT);

  if (x > (ADJ_AMOUNT - 1))
    x = ADJ_AMOUNT - 1;

  return rankname(x);
}

string EV(int sk)
{
    int x, high, hipsis;
    string tmp;

    if (sk >= 10000)
    {
	x = (1 + ((sk - 10000) / 21));
	hipsis = ((x * 21 + 10000) - sk);
	hipsis = (100 - ((hipsis * 100) / 21));
	return "Ultimate, " + hipsis + "% of Rank " + x;
    }

    x = sk / (10000 / ADJ_AMOUNT);

    if (x > (ADJ_AMOUNT - 1))
	x = (ADJ_AMOUNT - 1);

    if (x > (ADJ_AMOUNT - 2))
	high = 10000;
    else
	high = (x + 1) * (10000 / ADJ_AMOUNT);

    hipsis = (high - sk);

    return SA(sk) + ", about " +
    (100 - ((100 * hipsis) / (10000 / ADJ_AMOUNT))) +
    "% of rank done";
}

int thief_skills(string arg)
{
  object ob, tm;
  string s;

  if (arg)
    {
      if (guildmaster < TR_CO_GM && !cdr)
	{
	  notify_fail("You can't see other thieves' skills.\n");
	  return 0;
	}

	ob = find_player(lower_case(arg));
	if (ob)
	    tm = (object) ob->query_guild_object();

	if ((!ob || !tm) || (!owner->query_coder_level() &&
	    (ob->query_coder_level() || ob->query_testplayer())) ||
	  ((string)ob->query_guild() != GN_THIEF))
	{
	    notify_fail("You don't seem to detect that thief in game.\n");
	    return 0;
	}

	TPT("Ok, skills of " + capitalize(arg) + " look like this:");
	tm->thief_skills(0);

	return 1;
    }

    s = sprintf(
      "Your thieven skills are:\n\n\
Backstabbing:\t%s\n\
Sneak/Hiding:\t%s\n\
Throwing:\t%s\n\
Lockpicking:\t%s\n\
Stealing:\t%s\n\
Appraising:\t%s\n\
Poison:\t\t\%s\n\
Tumbling:\t%s\n\
- Average -:\t%s",
      EV(skills[SK_STAB]),EV(skills[SK_SNEAK]),EV(skills[SK_TTH]),EV(skills[SK_PICK]),
      EV(skills[SK_STEAL]),EV(skills[SK_APPRAISE]),EV(skills[SK_POISON]),
      EV(skills[SK_TUMBLING]),EV(query_skills(SK_AVERAGE)));

    if (sneak || (!show_tattoo))
    {
	if (sneak)
	    s += "\nYou are currently sneaking.";
	if (!show_tattoo)
	    s += "\nYou have hidden your tattoo.";
    }
    else
	s += "\nYou are not using any skills at the moment.";

#if 0
    s += "\nMaximum skill level for you is " +
      EV(M_MAX_SK_LVL(t_level)) + ".";
#endif

    if (guildmaster > 0)
    {
#ifdef OLD_GOVERNMENT
	if (guildmaster < TR_CO_GM)
	    s += "\nYou are one of the ENFORCERS.";
	else
	if (guildmaster < TR_GM)
	    s += "\nYou are an ASSISTANT GUILDMASTER.";
	else
#endif
	if (guildmaster < TR_DEMON)
//	    s += "\nYou are the one and only GUILDMASTER.";
	    s += "\nYou are one of the ENFORCERS.";
	else
	    s += "\nYou are a Thieven Demi-God!";
    }

    TPT(s);

    if (ban > 0)
	check_ban(0);

    return 1;
}

/*
 * Function   : advance_skill
 * Description: add point(s) to skill
 *            One point adds less and less to the skill, depending
 *            on the difficulty and how high the skill already is.
 * Arguments  : int sk  : skill value (0..10000)
 *            int d   : difficulty (0..10000)
 *            int p   : (optional) how many points to add (default 1)
 * Returns    : The new value of the skill.
 * Notes      : This function can't reduce skill yet. Perhaps later....
 *            It adds points in busy loop, so we don't allow adding
 *            more than 100 points at once :-(
 * NEW: Show some info now and then. / Graah
 * NEW: Trolls and Half-Dorcs train really badly! / Graah
 * NEW: Level affects max skill. Really! / Graah
 *      Someone had removed that limitation?
 * NEW: More cheats removed with magic kludges. /Graah
 * NEW: Show every time skill gets trained even a bit. /Graah
 *      - Not currently : got to make that toggleable feature.
 */
static object moremagic;
static int kludge;

varargs int advance_skill(int sk, int d, int p)
{
  object weird;
  int magic, nsk, i, dp;
  string old_desc, new_desc;
  int oldr, newr;

  // Hah hah haa! But don't make it less than 8.
  if (query_idle(owner) > 10) return sk;

  // Not obfuscated enough!
  if ((weird = environment(owner)) &&
      (moremagic == weird) &&
      (++kludge > (6+random(6)))) return sk;
  else if (weird != moremagic) { kludge = 0; moremagic = weird; }

  // Trolls and Half-Dorks SUCK! / Graah
  // Trolls have -3, dorks -1. So, (-25 * a) = 75 or 25.
  // Trolls: Only 17.5% chance to actually train, Dorks: 37.5%.
  if ((advantage < 0) && ((-25 * advantage) > random(201)))
    return sk;

  // Humans: slightly worse, 50% chance to not advance.
  if (!advantage && (random(101) < 50)) return sk;

  if (sk < 0) sk = 0;

  // Max skill level for level. See thief.h.
  // Got the inspiration when a certain cheater again started killing
  // himself to level 1. :-D / Graah
  // April 2, 1998 - Kieve -> Took out the max level for training.
  //   The bug which allowed that seems to have been fixed, and
  //   also, demons imposed higher training requirements for lower
  //   level players.
#if 0
  if (sk >= (M_MAX_SK_LVL(t_level))) return sk;
#endif

  if (p < 1)
    p = 1; /* Default */
  else if (p > 100)
    p = 100; /* Can't add more than 100 at once */

  old_desc = EV(sk);
  oldr = rank_no(sk);

  nsk = sk;

  /* Difficulty %, ranges from 25% to 175% */

  if (d >= 5000)
    dp = (100 - ((d - 5000) / 66)); /* Change that 66 */

  if (d  < 5000)
    dp = (100 + ((5000 - d) / 66)); /* to change range */

  for (i = 0; i < p; i++)
    {
      magic = 10000 - nsk;

      if (magic < 1)
	{
	  /* REALLY slow advancement for ultimate skills. */

	  magic = (-(magic));

	  if (random(magic) < 50)
	    {
	      owner->tell_me("You feel a bit more skilled.");

	      new_desc = EV(nsk + 1);

	      if (new_desc != old_desc)
		{
		  owner->tell_me("Your skill advances to " + new_desc + "!");
		}

	      return (nsk + 1);
	    }

	  return nsk;
	}

      magic = ((dp * magic) / (100 * THF_DIV));
      if (magic < 1)
	magic = 1; /* At least one point. */
      if (magic > 4)
	magic = 4; /* Maximum of 4 points at once. */

      nsk += magic;
    }

#if 0
  new_desc = EV(nsk);

  if (new_desc != old_desc)
    {
      owner->tell_me("Your skill advances to " + new_desc + "!");
    }
#endif

  newr = rank_no(nsk);

  if (newr > oldr)
    {
      owner->tell_me("Skill just rised from " + rankname(oldr) +
                     " to " + rankname(newr) + "!");
    }

  return nsk;
}


/*******************************************************
			STEALTH
The ability to do things with out being seen mainly. This
should effect steal and stab mostly. I guess tumble as well
as harder to see target, easier for target to dodge.

	Max_stealth = 100;
Kelgath, Oct 25 1996
********************************************************/

int stealth()
{
    int stealth, night, fog, clouds, rain, sneakyness;
    int *weather;

    weather = (mixed *) nature()->query_thief_weather();

    night = weather[0];
    fog = weather[1];
    clouds = weather[2];
    rain = weather[3];

    sneakyness = (skills[SK_SNEAK]/1000 > 10 ? 10 : skills[SK_SNEAK] / 1000);

    stealth = ( sneakyness + thief_dex() + advantage
      + 16*night
      + clouds*(1 + night)
      + fog*(5 + 10*night)
      + rain*(3+ 2*night)
      -night*moons );

    if(stealth > 100)
	stealth = 100;

    if(stealth < 0)
	stealth = 0;

    return stealth;
}

/* Moonlight... needed for stealth() */
int moon_light()
{
    string mal, cel;
    mapping vals;

    vals = ([ "dark":0, "waxing crescent":1, "waxing gibbous":2,
      "full":3,"waning gibbous":2, "waning crescent":1 ]);

    mal = (string) nature()->moonphase(MALERANA_CIRCUIT);
    cel = (string) nature()->moonphase(CELERANA_CIRCUIT);

    moons = vals[mal] + vals[cel];
}

status steal(mixed str)
{
    if (delay_steal)
    {
	notify_fail("You must wait to steal again.\n");
	return 0;
    }

    if (delay_warmup)
    {
	notify_fail("You are just doing your exercises!\n");
	return 0;
    }

    DELAY_STEAL == (int) STEAL->steal(owner,str) ? delay_steal++ : delay_steal = 0;
    if (delay_steal)
	call_out("steal_again",DELAY_STEAL);
    return 1;
}

status steal_again()
{
    delay_steal = 0;
    return 1;
}

void test_notice(mixed arg, object targ)
{
    status yep;
    string itm, tmp;
    object guard;

    /* A coder notices always */
    target = targ;

    if (!arg || !objectp(arg))
	itm = 0;
    else
    {
	itm = arg->short();
	if (!itm)
	    itm = "something";
    }

    yep = (targ->query_coder_level()) && !targ->is_npc();
    if (!yep)
    {
	if ((string) targ->query_guild() == GN_THIEF)
	    /* Let's make other thieves notice more.. */
	    yep = (random(10) + random(10) < targ_lev);
	else
	    yep = ((random(20) + random(20) + random(140))
	      < (targ_lev + (100 - stealth())));
    }

    if (yep)
    {
	if (t_level < 4)
	    if (random(5) < (3 - t_level))
		return;

	    /* And there is still a fair chance to go unnoticed */
	// if (random(100) < (31 + advantage * 5)) return;
	// new formula -Kel

	if (random(100) < stealth() )
	    return;

	/* We were invisible: even harder to notice */
	if (owner->query_invis() && random(100) < 44)
	    return;

	/* Blind target never notices */
	if (!target->query_can_see())
	    return;

	if (itm)
	{
	    if (success > 10)
		tmp = " stole " + itm;
	    else
		tmp = " tried to steal " + itm;
	}
	else
	{
	    if (success > 0)
		tmp = " stole some gold";
	    else
		tmp = " tried to steal gold";
	}

	target->tell_me("You notice that " +
	  (random(150) < target->query_level() ?
	    Realname(owner) : owner->query_name()) +
	  tmp + " from you!\n");
	environment(owner)->tell_here(owner->query_name() + tmp + " from " +
	  target->query_name() + "!\n", ({ owner, target }));

	/*Make thief dirty if noticed */
	if(target->query(LIV_IS_PLAYER))
	    owner->set(LIV_OK_TO_PK);
	/* Sometimes the thief notices that victim noticed. */
	if ((random(100) + random(100)) < t_level)
	    TM("OOPS! " + target->query_name() +
	      " noticed your attempt!\n");

	/* I'm a monster! Attack the thief! */
	if (target->is_npc())
	{
	    target->attack_object(owner);
	    /* If in town, and near guard, he notices!! */
	    if((guard=present("city_guard",environment(target))))
	    {
		guard->attack_object(owner);
		guard->command("say Stop thief!!");
	    }

	    /* We learn from this mistake */
	    skills[SK_STEAL] = advance_skill(skills[SK_STEAL], DIFF_STEAL);
	}

#ifdef CRIME
	/* Noticed theft is a crime in city. Not theft from coder, nor
	   from another thief. */
	if (!owner->query_coder_level() &&
	  environment(owner)->query(IN_CITY) &&
	  !present("tmark", target))
	    CRIME_D->crime(owner, CR_THEFT);
#endif
    }
}

status brew_poison()
{
    POISON->make_poison(owner);
    return 1;
}

status sneak()
{
    if (sneak)
    {
	if (sneak <= 1)
	    notify_fail("You must rest a bit before sneaking again.\n");
	else
	    notify_fail("You are already sneaking.\n");
	return 0;
    }
    sneak = SNEAK->sneak(owner);
    if (sneak)
	call_out("stop_sneak",sneak);
    return 1;
}

void
stop_sneak()
{
    string tmp;

    remove_call_out("stop_sneak");
    tmp = (string)owner->query_move_msg("mmin");
    owner->set_move_msg("mmin",
      "was hiding in the shadows, but can now be seen again");
    owner->vis();
    TM("You stopped sneaking.");
    owner->set_move_msg("mmin", tmp);
    sneak = 0;
}


status
hide()
{
  HIDE->hide(owner);
  return 1;
}


mapping query_stabbed() { return stabbed; }
void set_stabbed(mapping x) { stabbed = x; }


// Stab skill asks this function: can we stab again (or first time?)
// Return 0 if we can.
int
stab_delay(object victim)
{
  if (!victim) return 1;
  if (!mappingp(stabbed)) stabbed = ([]);

  m_delete(stabbed, 0); // Remove dead monsters

  if (victim->query_dead())
    {
      m_delete(stabbed, victim);
      return 1;
    }

  if (!member(stabbed, victim))
    {
      stabbed += ([ victim: (time() + (int)victim->query_stat(ST_INT)) ]);

      // First try. Already in combat? Then we can't stab.
      if (victim->is_fighting(owner) ||
	  member_array(owner, victim->query_hunted()) != -1)
	return 1;

      return 0;
    }

  // Already been stabbing this one. Has it forgotten?
  if (stabbed[victim] < time())
    {
      stabbed[victim] = (time() + (int)victim->query_stat(ST_INT));
      return 0;
    }

  return 1;
}


status stab(string str)
{
  if (stab_cheat)
    {
	owner->tell_me("You have to wait some more.");
	return 1;
    }

    if (delay_warmup)
    {
	notify_fail("You are just doing your exercises!\n");
	return 0;
    }

    STAB->stab_cmd(str,owner);
    call_out("stab_delay_over",DELAY_STAB);
    return 1;
}

status stab_delay_over()
{
    delay_stab = 0;
    return 1;
}

status stab_cheat_over()
{
    stab_cheat = 0;
    return 1;
}

status appraise(string str)
{
    APPRAISE->appraise(owner, str);
    return 1;
}

status pick(string str)
{
    PICK->pick_cmd(str, owner);
    return 1;
}


/* Get our pretitle */
string thief_pretitle()
{
    int i, x, y, z;

    if (!still_thief(1))
	return "";

    if ((x = (int) query_guildmaster()) > 0)
    {
#ifdef OLD_GOVERNMENT
	if (x < TR_CO_GM)
	    return "ENFORCER";
	if (x < TR_GM)
	    return "CO-GUILDMASTER";
	if (x < TR_DEMON)
	    return "GUILDMASTER";
#endif
	/* Demon */
        if(x == TR_ENFORCER) return "ENFORCER";
	if (pretitle == "0")
	    return "";
	if (pretitle != "1")
	    return pretitle;
	return "DEMON";
    }

    x = y = -1;
    z = 0;

    for (i = 0; i < 6; i++)
    {
	if (skills[i] < 2500)
	    z++;
	if (skills[i] > 8400)
	{
	    if (x != -1)
		x = 99;
	    else
		x = i;
	}
	else
	if (skills[i] > 7400)
	{
	    if (y != -1)
		y = 99;
	    else
		y = i;
	}
    }
    if (z >= 6)
	return "Junior Thief";

    /* Test too low skills here */
    if (skills[SK_TTH] < (owner->query_level() * 155))
	return "Lazy Thrower";

    if (x == -1)
    {
	if (y == -1)
	    return "";
	if (y == 99)
	    return "Senior Thief";

	return ({ "Great Stabber", "Great Sneaker", "Great Thrower",
	  "Great Picker", "Great Stealer", "Great Fence" })[y];
    }

    if (x == 99)
	return "Grand Master";

    return ({ "Master Stabber", "Master Sneaker", "Master Thrower",
      "Master Picker", "Master Stealer", "Master Fence" })[x];
}

status canttoggle()
{
  return "/bin/pub/_channel"->channel_cmd("thief toggle", owner);
}

varargs status chant(string str)
{
  object ob, tm;
  string d1, d2, d3, nm;

  // Best place I could think to put it.... -Kel
  if (owner)
    {
      if (top_checked < 1)
	{
	  top_checked = 10;
	  call_other(TOP_TEN, "check_top_ten", owner);
	}
      else
	top_checked--;
    }

  if (str)
    {
      if (((sscanf(lower_case(str), "%s %s %s", d1, d2, d3) == 3) &&
	   (d1 == d2) && (d2 == d3)))
	{
	  TM("You chant a name three times, trying to summon a demon...");
	  if (environment(owner)->query(PREVENT_TELEPORT_TO) ||
	      environment(owner)->query(PREVENT_TELEPORT))
	    {
	      TM("...but you know it is hopeless from here.");
	      return 1;
	    }

	  if ((ob = find_player(d1)) && !ob->query_dead() &&
	      (tm = present("tmark", ob)) && tm->query_guildmaster() >=
	      TR_DEMON)
	    {
	      if (ob == owner)
		{
		  TM("Summoning yourself sounds pretty silly.");
		  return 1;
		}

	      if (environment(ob)->query(PREVENT_TELEPORT) ||
		  environment(ob)->query(PREVENT_TELEPORT_FROM))
		{
		  TM("...but you know it is hopeless from there.");
		  return 1;
		}

	      //
	      if (ob->is_fighting())
		{
		  TM("...but that demon is in middle of a fight just now.");
		  return 1;
		}

	      nm = (string) owner->query_real_name();
	      ob->tell_me(sprintf(
				  "You hear %s calling your name three times somewhere! If you wish to help %s, \
type 'goto %s'.", nm, (string) owner->Objective(), nm));
	      tm->set_summoner(owner,environment(owner),time());
	    }

	  return 1;
	}
    }

  if (!str) return "/bin/pub/_channel"->channel_cmd("thief list", owner);
  return "/bin/pub/_channel"->channel_cmd("thief send " + str, owner);
}

/*
Newer Thief Dex: Higher importance on race, how much the poor
bastard is carrying, and, of course, dexterity.
*/

int thief_dex()
{
    int dex;

    dex = (int) owner->query_stat(ST_DEX);
    dex += advantage; // racial bonus++

    // Scale it by how much free strength they have left...
    // Give them 15% for free-- was too low before
    // Thieves are only ones who cannot basicly use any eq!why?
    // let them use cloaks and daggers ++skarda  
    // and lets correct this. ++graah
    dex = (dex * (165 - (int)owner->query(LIV_CARRY_RATE))) / 100;
    // Too generous version was:
    // dex = dex * (125 - 2*((int)owner->query(LIV_CARRY_RATE))/3)/100;
    // Too harsh, however, was:
    // dex = (dex * (150 - ((int)owner->query(LIV_CARRY_RATE) * 2))) / 100;

    if (dex < 1) dex = 1;

    return dex;
}

status fix_title(string arg)
{
    object g;
    int le;

    le = owner->query_level();
    if (le < 1 || le > 20)
	return 1;
    le--;

    if (!arg)
	owner->set_title(GUILD->get_new_title(le));

    return 1;

}

void backup_log(string filename)
{
  string tmp;

  /* Coders/testplrs must not fill our log. They don't need backup. */
  if (cdr)
    return;

  if (filename != "THIEF_LOG" && filename != "KICKED_LOG")
    return;

  if (file_size(GDIR + filename) >= 256000)
    {
      rename(GDIR + filename, GDIR + filename + ".old");
/*
      LOG_FILE(filename, "New log file started at " + ctime(time()) + "\n");
*/
    }

  tmp = query_guild_save();

/*
  LOG_FILE(filename,
	   ctime(time()) + ":" + Realname(owner) + ": " + tmp + "\n");
*/
}

/* When he _types_ "save", then log. Don't log when autosaving. */

save_cmd()
{
    if (last_save != (string) TO->query_guild_save())
    {
	last_save = (string) TO->query_guild_save();
	backup_log("THIEF_LOG");
    }
    return 0;
}

/* Stop sneaking before quitting */

void
guild_mem_quit(object who)
{
    if (owner != who)
	return;

    if (!owner->query_coder_level())
	owner->vis();

    if (guildmaster == TR_GM)
	GM_D->set_gm_last_on();
    CHANNEL_D->remove_channel_member(TO);

    /* Log thief's data when quitting to backup his/her skills. */
    backup_log("THIEF_LOG");
    // Shouldn't really be needed, this one:
    who->set_guild_hook(0);
    // (Didn't comment it out though. -+ Doomdark +-)

}

void guild_mem_died(object died, object killer)
{
  object killer_mark;

  if (died != owner) return;
  remove_call_out("done_mort");
  already_mort = 0;
  BIN_CHANNEL_D->send_channel_message("SKERT", "thief", sprintf(
	"My child %s, has been murdered by %s. May %s death be avenged!",
	capitalize((string)owner->query_real_name()),
	capitalize((string)killer->query_real_name()),
	(string)owner->Possessive()));
  if (guildmaster != TR_GM)
    return;

#ifdef OLD_GOVERNMENT
  if ((string)killer->query_guild() == GN_THIEF)
    {
      killer_mark = (object) killer->query_guild_object();
      if (killer_mark->query_guildmaster() == TR_DEMON)
	return;
      BIN_CHANNEL_D->send_channel_meesage("SKERT","thief",
	capitalize((string)killer->query_real_name()) +
		" is making " + killer->query_possessive() +
					  " bid for GM by killing " +
		capitalize((string)GM_D->query_gm()) + ".");
      GM_D->start_vote((string)killer->query_real_name());
    }
#endif
}

void
done_mort()
{
    already_mort = 0;
}

void
guild_mem_mort(object mort, object killer)
{
  string place;

  if (mort != owner) return;
  if (already_mort) return;
  already_mort = 1;

// None of the guilds supports this message any more.
// Bull
#if 0
  if (!owner->query_coder_level())
    {
      place = (string)environment(owner)->query_short(2, owner);
      if (!stringp(place)) place = "Dark place";
      BIN_CHANNEL_D->send_channel_message("SKERT", "thief",
	sprintf("My child %s, is mortally wounded at %s!",
		capitalize((string)mort->query_real_name()),
		place));
    }
#endif

  call_out("done_mort",150);
}

void
guild_mem_ld(object me)
{
  int il;

  if (me != owner) return;

  // Must not stay hidden to ld
  il = (int)owner->query_invis();
  if (il == IL_HIDDEN) owner->vis();
}

status
tth(string arg)
{
    string tmp1, tmp2;

    if (!arg)
    {
	TM("Usage: tth <item> [at <target>].\n" +
	  "The default is your current opponent.");
	return 1;
    }

    if (check_ban(0))
	return 1;

    if (arg && (sscanf(arg, "%s at %s", tmp1, tmp2) == 2) &&
      (!present(tmp2, environment(owner))))
    {
	TM("You can't see any \"" + tmp2 + "\" around here...");
	return 1;
    }

    /* Necros - I think they are going to tth =) (the stab went off already)
       if (delay_stab)
	 {
	   TM("Ok, wait, hey...are you going to stab or tth...?");
	   return 1;
	 }
    */

    if (tth)
    {
	TM("You are already about to throw.");
	return 1;
    }

    if (delay_warmup)
    {
	TM("You are just doing your exercises!");
	return 1;
    }

    if (this_player()->query_hp() < 0) {
	TM("You can't tth when you are on the ground bleeding. ");
	return 1;
    }

    if (!this_player()->query_can_move())
    {
	owner->tell_me("You can't move!");
	return 1;
    }

    remove_call_out("thetth");
    call_out("thetth", 0, arg);

    tth = 1;

    return 1;
}

void remove_tth()
{
    remove_call_out("thetth");
    tth = 0;
}

/*
 * Throw small weapons and items.
 */
status thetth(string arg)
{
    tth = 0;
    TTH->tth_cmd(arg,owner);
    return 1;
}


int tits()
{
    return (int)call_other(GUILD, query_verb());
}

int enforcerlist()
{
    string *enf,*fen;
    int i;

#ifdef OLD_GOVERNMENT
    if ((string)GM_D->query_gm())
	TM("Guildmaster: " + capitalize((string)GM_D->query_gm()) +
	  ".");
    else
    {
	TM("There is no Guildmaster at the moment.");
	return 1;
    }
#endif
    enf = (string *)GM_D->query_demons();
    fen = ({});

    if (sizeof(enf) < 1)
	TM("There are no Demons at the moment.");
    else
    {
	for (i = sizeof(enf) - 1; i >= 0; i--)
	    if (stringp(enf[i]))
		fen += ({ capitalize(enf[i]) });
	TM("Demons: ");
	if (sizeof(fen) == 1)
	    TM(fen[0]);
	else
	    TM(implode(fen, ", ", " and "));
    }

    enf = (string *)GM_D->query_enforcers();
    fen = ({});

    if (sizeof(enf) < 1)
	TM("There are no Enforcers at the moment.");
    else
    {
	for (i = sizeof(enf) - 1; i >= 0; i--)
	    if (stringp(enf[i]))
		fen += ({ capitalize(enf[i]) });
	TM("Enforcers: ");
	if (sizeof(fen) == 1)
	    TM(fen[0]);
	else
	    TM(implode(fen, ", ", " and "));
    }
    return 1;
}

int peek(string arg)
{
    APPRAISE->peek(owner, arg);
    return 1;
}

int taxes()
{
    int d, h;

    TM("Your tax is " +
      (int)GUILD->query_tax((int)owner->query_level()) +
      " per cent.\n");
    if (query_tax() > 0)
    {
	TM("Yor tax debt is " + query_tax() +
	  " gc, which you have to pay to Guild cash box.\n");
	d = time() - tax_time;
	h = d / 3600;
	d = h / 24;
	if (d > 0)
	    h -= (d * 24);
	if (h < 1 && d < 1)
	    TM("You have just recently paid your taxes.\n");
	else
	{
	    write("You last paid your taxes ");
	    if (d > 0)
	    {
		write(d + " day"); if (d > 1) write("s");
		if (h > 0)
		    write(" and ");
	    }
	    if (h > 0)
		write(h + " hour"); if (h > 1) write ("s");
	    write (" ago.\n");
	}
    }
    else
	TM("Very good. You have paid all your taxes already.\n");

    return 1;
}


status cantread(string arg)
{
  if (environment(owner)->query(ROOM_ISOLATED))
    {
      owner->tell_me("Nothing on channel log.");
      return 1;
    }

  return "/bin/pub/_channel"->channel_cmd("thief hist", owner);
}


/* All other damage goes through this */
void hit_victim(mixed ar)
{
  int d;
  string dummy;

  if (!ar[0] || !objectp(ar[0]) || !intp(ar[1]))
    return;

#ifdef OLD_GOVERNMENT
  if (sscanf(file_name(environment(owner)),"guilds/thieves/%s",
	     dummy) == 1 && (string)ar[0]->query_guild() != GN_THIEF)
    {
	if (guildmaster && guildmaster != TR_DEMON)
	    owner->tell_me("You fight extra hard in \
defense of your guild!");
	if (guildmaster == TR_ENFORCER)
	    ar[1] += ar[1] * 10 / 100;
	else
	if (guildmaster == TR_CO_GM)
	    ar[1] += ar[1] * 20 / 100;
	else
	if (guildmaster == TR_GM)
	    ar[1] += ar[1] * 30 / 100;
    }
#endif
#ifndef OLD_GOVERNMENT
    if(sscanf(file_name(environment(owner)), "guilds/thieves/%s",
	dummy) == 1 && (string)ar[0]->query_guild() != GN_THIEF)
    {
	if(guildmaster >= TR_ENFORCER) {
	    owner->tell_me("You fight extra hard in defense of your guild!");
	    ar[1] += ar[1] * 10 / 100;
	}
    }
#endif

#ifdef GN_PRIEST
    if (member_array((string)ar[0]->query_guild(),
		     ({ GN_PRIEST })) != -1)
      ar[1] = ar[1] * 2;
#endif

#ifdef GN_NINJA
    if (member_array((string)ar[0]->query_guild(),
		     ({ GN_NINJA })) != -1)
      ar[1] = ((ar[1] * 3) / 2);
#endif


    
    d = (int)ar[0]->hit_player(TUNE_THIEF_DAMAGE(ar[1]), 0, DT_SLASH, owner);

    if (cdr)
	TM("(Damage: " + d + " hits)");

    if (ar[0] && !ar[0]->is_npc())
      {
	THIEF_LOG("tth_dmg",
		  owner->query_real_name() + " threw for " +
		  d + " pts dmg.\n");
      }
}

/* All stab damage goes through this */
void stab_victim(mixed ar)
{
    int h, d;
    int d1;
    string dummy;

    if (!ar[0] || !objectp(ar[0]) || !intp(ar[1]))
	return;

#ifdef OLD_GOVERNMENT
    if (sscanf(file_name(environment(owner)),"guilds/thieves/%s",
	dummy) == 1 && (string)ar[0]->query_guild() != GN_THIEF)
    {
	if (guildmaster && guildmaster != TR_DEMON)
	    owner->tell_me("You fight extra hard in \
defense of your guild!");
	if (guildmaster == TR_ENFORCER)
	    ar[1] += ar[1] * 10 / 100;
	else
	if (guildmaster == TR_CO_GM)
	    ar[1] += ar[1] * 20 / 100;
	else
	if (guildmaster == TR_GM)
	    ar[1] += ar[1] * 30 / 100;
    }
#endif
#ifndef OLD_GOVERNMENT
    if(sscanf(file_name(environment(owner)), "guilds/thieves/%s",
	dummy) == 1 && (string)ar[0]->query_guild() != GN_THIEF &&
      guildmaster) {
	owner->tell_me("You fight extra hard in defense of your guild!");
	ar[1] += ar[1] * 10/100;
    }
#endif
    d = ar[1];
    d1 = d;

    /* Not needed (I hope)
	    if (!query_once_interactive(ar[0]))
    */
    d = (int)ar[0]->hit_player(TUNE_THIEF_DAMAGE(d), DT_STAB, 0, owner);
    /*
       else
	  d = (int)ar[0]->hit_player(d,DT_STAB,0,owner);
    */
    if (cdr) TM("(Damage: " + d + " hits)");
    if (cdr)
    {
	if (!ar[0])
	{
	    owner->tell_me("Enemy Killed");
	}
	else
	{
	    if (!ar[0]->is_npc())
	    {
		owner->tell_me("Actual Percentage: " + ((d * 100) /
		    ar[0]->query_max_hp()) + "%");
	    }
	}
    }
    if (ar[0])
      {
	if (!ar[0]->is_npc() && !cdr)
	  {
	    THIEF_LOG("PK_STAB_DMG",
		      capitalize((string)owner->query_real_name())+"("+
		      owner->query_race()+
		      ") stabbed for "+((d*100) / ar[0]->query_max_hp())+
		      "% of "+capitalize((string)ar[0]->query_real_name())+
		      "'s("+ar[0]->query_race()+") hit points. ("+
		      d+")\n");
	  }
      }
}

void delayed_hit(int delay, mixed ar)
{
    call_out("hit_victim", delay, ar);
}

void delayed_stab(int delay, mixed ar)
{
    call_out("stab_victim", delay, ar);
}


int top(string arg)
{
    int w;
 
    if(guildmaster < TR_DEMON)
        notify_fail("Usage: top <1..9> (and 'thelp top' for more info)\n");
    else
        notify_fail("Usage: top <1..12> (and 'thelp top' for more info)\n");
 
    if (!arg)
        return 0;
    if(sscanf(arg, "%d", w) != 1)
        return 0;
    if(w < 1)
        return 0;
    if(guildmaster < TR_DEMON  &&  w > 9)
        return 0;
    if(guildmaster >= TR_DEMON  &&  w > 12)
        return 0;
 
/*
    if(guildmaster < TR_DEMON)
    {
        TM("Those lists are restricted to Demons. ");
        return 1;
    }
    if ((int)this_player()->query_level() < 20)
    {
        TM("You must be at least level 20 to read lists.");
        return 1;
    }
*/
 
    TOP_TEN->list_top_ten(w - 1);
    return 1;
}


status resign(string arg)
{
#ifndef OLD_GOVERNMENT
    return 0;
#endif
    if (guildmaster != TR_GM)
	return 0;
    if (!arg)
    {
	notify_fail("To be absolutely sure, use 'resign gm'.\n");
	return 0;
    }
    if (arg != "gm")
    {
	notify_fail("Use 'resign gm'.\n");
	return 0;
    }
    TP->tell_me("Ok, you resign from GM.");
    GM_D->resign_gm(1);
    return 1;
}

// Now shows "effective dex"
status thief_weight()
{
  int weight, d, td;

  weight = (int)owner->query(LIV_CARRY_RATE);
  weight -= 5;
  if (weight < 0) weight = 0;

  switch (weight)
    {
    case 0..10:
      owner->tell_me("You're carrying almost nothing! Your tumbling \
will be great!");
      break;
    case 11..20:
      owner->tell_me("Wow!, that's a light load.  You should have \
no problem tumbling!");
      break;
    case 21..30:
      owner->tell_me("Your load is still pretty light, but starting \
to get in your way.");
      break;
    case 31..40:
      owner->tell_me("You should probably be carrying less, at least \
if you wanna tumble better.");
      break;
    case 41..60:
      owner->tell_me("Your load is starting to get heavy now.  \
You probably won't tumble to well.");
      break;
    case 61..100:
      owner->tell_me("Your load is way too heavy for you to \
perform your skills well.");
      break;
    default:
      owner->tell_me("You are carrying wierdly. Contact a (co-)admin.");
      break;
    }

  d = (int)TP->query_stat(ST_DEX);
  td = thief_dex();

  if (td > d)
    owner->tell_me("With this load, your effective dexterity is \"" +
		   (string)DESC_D->query_dex_desc(td) +
		   "\", which is better than your normal dexterity (\"" +
		   (string)DESC_D->query_dex_desc(d) + "\").");
  else if (td < d)
    owner->tell_me("With this load, your effective dexterity is \"" +
		   (string)DESC_D->query_dex_desc(td) +
		   "\", which is worse than your normal dexterity (\"" +
		   (string)DESC_D->query_dex_desc(d) + "\").");

  return 1;
}
status end_cook()
{
    cook_flag = 0;
    return 1;
}
status cook_corpse(string what)
{
    object corpse;
    if (cook_flag)
    {
	notify_fail("You are already busy preparing a delicious \
meal!\n");
	return 0;
    }
    if (!what || what != "corpse")
    {
	notify_fail("You can only cook corpses!\n");
	return 0;
    }
    if (!corpse = present(what,environment(owner)))
	if (corpse = present(what,owner))
	{
	    notify_fail("You gotta drop it first!\n");
	    return 0;
	}
	else
	{
	    notify_fail("There are no corpses here!\n");
	    return 0;
	}
    if ((int)corpse->query_decay() < 4)
    {
	notify_fail("That corpse is too decayed to cook!\n");
	return 0;
    }
    cook_flag = 1;
    COOK->cook_cmd(owner,corpse,environment(owner),0);
    return 1;
}

status
warmup(string arg)
{
    int am, sp;

    if (delay_steal || tth || cook_flag)
    {
	notify_fail("Not now.\n");
	return 0;
    }
    if(this_player()->query_hp() < 0)
    {
	TM("You cannot move! You are bleeding, mortally wounded!");
	return 0;
    }

    if (delay_warmup)
    {
	notify_fail("You are already warming up!\n");
	return 0;
    }

    if (!arg || sscanf(arg, "%d", am) < 1)
    {
	TM("Warm up how many points?");
	return 1;
    }

    if (am < 1)
    {
	TM("Yeah, sure.");
	return 1;
    }

    if (am < 4)
    {
	TM("That would be useless. You got to use at least 4 points.");
	return 1;
    }

    sp = (int)owner->query_sp();

    if (am > sp)
    {
	TM("You are not fresh enough to do that much, try " + sp + " points.");
	return 1;
    }

    owner->add_sp(-(am));
    TM("Ok, you start stretching and doing gymnastics...");
    environment(owner)->tell_here((string)owner->query_name() +
      " starts to stretch and to do gymnastics.", ({ owner }));
    delay_warmup = (3 + (am / 50));
    call_out("end_warmup", delay_warmup, am);
    return 1;
}

void
end_warmup(int am)
{
    int x, l;

    l = (100 + ((int)owner->query_level() * 20));
    if (l > 1000) l = 1000;

    x = (int)BALANCE_D->healing(l, GN_THIEF, am);

    TM("You finish the exercise. That felt good!");
    environment(owner)->tell_here((string)owner->query_name() +
      " has finished the exercise.", ({ owner }));
    delay_warmup = 0;

    owner->add_fp((x * 4) / 3);
    owner->add_hp(x / 2);
}

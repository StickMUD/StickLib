/****************************************************************
*								*
* Module:							*
*	/lib/living.c						*
* Description:							*
*	The base 'living' object of Sticklib distribution. Is	*
*	used by the other derived objects such as player.c	*
*	and npc.c, and should never be used directly.		*
* Version:							*
*	1.0a (for StickLib), Aug-98				*
* Last update:							*
*	04-Sep-98 by Doomdark					*
*								*
* Modifications:						*
*								*
* 04-Sep-98, Doomdark: Have to check for existence of		*
*	GN_ - defines as most of the Stick-guilds are _not_	*
*	included in StickLib, but the guild-specific checks	*
*	still need to be included in case new guilds are	*
*	added a later point.					*
****************************************************************/

#include "/sys/interactive_info.h"

#include <logging.h>
#include <conditions.h>
#include <daemons.h>
#include <lights.h>
#include <npc_defs.h>
#include <room_defs.h>
#include <party.h>
#include <player_defs.h>
#include <guild.h>
#include <tell_me.h>
#include <client_defs.h>
#include <tune.h>
#include <generic_items.h>
#include <corpse_defs.h>
#include <skills_defs.h>
#include <spell_skills.h>
#include <attack.h>

#include "/basic/living/living_private.h"

#define NAME_OF_GHOST		"Some mist"
#define MAX_HUNTED		7
#define ATTACKER_ARRAY_SIZE	4

#define GET_LIGHT	((int)environment()->query_total_light())
#define	INVIS_LEVEL	invis_level
#define MY_NAME(x)      query_name(0,x,0) // capitalized name
#define MY_NAME_NORMAL(x) query_name(0,x,1) // non-capitalized, unless
// it's a proper name - Chopin 15-May-98

//#define	TIME_TO_STAY_DIRTY	150
// Means we'll be PK:able for 150 hbs == 5 minutes after first pk-hit.
#define TIME_TO_STAY_DIRTY (120 + (2 * level))
#define	IDLE_LIMIT_FOR_DODGE	120
// Ie. will dodge unless we've been more than 2 mins idle!

// We _NEED_ to define this, do NOT remove!!!
#define	LIVING_C

inherit "/basic/id";
inherit "/basic/object/dimensions";
inherit "/basic/living/alignment";
inherit "/basic/living/channels";
inherit "/basic/living/gender";
inherit "/basic/living/pets";
inherit "/basic/living/guild";
inherit "/basic/living/resistance";

#include "/basic/living/set_n_query.c"

varargs int tell_me(string s, int flags);

status query_can_move();
status query_can_see();
varargs mixed query_condition(int c, int mode);
string query_real_name();
object query_attack();
varargs int query_wc(status left);
public int query_ac();
status query_ghost();
object is_wearing(mixed x);
varargs object is_fighting(object a);
int query_level();
status query_sit();
status query_rest();
status test_dark();
status test_dark2(int a, int b, int c);
varargs status attack_object(object o, status no_hit);
varargs int attacked_by(object o, int i, int x);
void transfer_all_to(object x);
varargs status add_weight(int w, status no_recount);
void recount_weight();
void race_update();
varargs status set_sit(status a, status silent);
varargs status set_rest(status a, status silent);
int query_age();
varargs void stop_fight(object o, int flags);
varargs string get_short(object o, object o2, int sil);
void start_hunting(object o);
void purge_hunt();

// Funcs defined to be overridden:
int query_coder_level() { return 0; }
int query_testplayer() { return 0; }
int query_is_testplayer() { return 0; }
int attack() { return 0; }

// 'Prototypes' of functions that reside only in player.c
int guild_melee_damage_rate(int x) { return x; }
void set_crime_count(int x) { return; }
void set_crime_level(int x) { return; }

/****************************************************************
*								*
*	Global variables; more defined in modules		*
*		included later on.				*
*								*
****************************************************************/

int intoxicated, stuffed, soaked; // How many HBs to stay intoxicated/stuffed/soaked
int headache, max_headache;	// How long does it last...
int money;			// Amount of money on creature
string name;			// Name of creature
int level;			// Level of creature
int max_hp, hit_point;		// Hp are "point", sp and fp "points"... :-(
int max_sp, spell_points;	// Spell points (sp)
int max_fp, fatigue_points;	// Fatigue points (fp)
int experience;			// Experience (xp)
int exp_times;			// 2,100,000,000 times additional exp gained
status ghost;			// Used for monsters that can leave a ghost
int age;			// Age in HBs
int frog;			// Frog curse
int whimpy;			// Automatic flee when HP low
string auto_load;		// Autoloading objects
status dead;			// Alive or dead?
int player_kills;     		// amount of player kills. but why here?
int time_to_heal;		// A count-down variable; used by npcs & players
int dirty_stamp;

static int infravision;		// Infravision level (how well we can see in dark)
static int local_weight;	// weight of items
static object *hunted;		// Enemies we hunt
static object *attacker;	// Enemies we are currently fighting
static int attackers;		// Number of attackers in that array (if less than 4)
static object attacker_ob, alt_attacker_ob; // NOT USED!
static int forget_hunt;		// Counter
static string cap_name;		// Capital version of "name"
static string name_of_last_attacker; // If we have been slain unconscious
static object last_attacker;    // the object too.
static object hitter;		// Who or what called hit_player?
static object kill_ob;		// Living we'll attack on next hb (NPCs?)
static int cons_moves;		// # of consecutive moves in 1 round

#if 0
static string spell_name;
static int spell_cost, spell_dam;
#endif

// These are used by player.c only but need to be here still. :-(
static int GuildFlags;
static mapping GuildHooks;

static closure HitFunc;		// 'Pointer' to weapon's 'hit'-function.
static closure ExtraHitFunc;	// 'Pointer' to weapon's 'extra_hit'-function.
static closure LeftHitFunc, LeftExtraHitFunc;

/****************************************************************
*								*
*	Then the modules we include. They contain some		*
*	more global variables and many functions.		*
*								*
****************************************************************/

#include "/basic/living/conditions.c"
#include "/basic/living/race.c"
#include "/basic/living/weapon_handling.c"
#include "/basic/living/armour_handling.c"
#include "/basic/living/abilities.c"
#include "/basic/living/luck.c"
#include "/basic/living/moving.c"
#include "/basic/living/death.c"

/****************************************************************
*								*
*	And then all the query_-functions and alike		*
*								*
****************************************************************/

status query_is_immune_vs_obj(object obj, int type) { return 0; }
nomask string query_real_name() { return name; }

varargs string
query_name(int mode, object x, status non_cap)
{
    if (INVIS_LEVEL && (!intp(mode) || mode < INVIS_LEVEL))
        return non_cap ? "someone" : "Someone";
    return non_cap ? name : cap_name;
}

status is_npc() { return liv_Flags & F_LIV_IS_NPC; }

int query_hp() { return hit_point; }
int query_max_hp() { return max_hp; }
int query_sp() { return spell_points; }
int query_max_sp() { return max_sp; }
int query_fp() { return fatigue_points; }
int query_max_fp() { return max_fp; }
int query_ep() { return experience; }
int query_exp_times() { return exp_times; }
int query_level() { return level; }
int query_money() { return money; }

status query_dead() { return dead; }
status query_ghost() { return ghost; }

object *query_attackers() { return attacker; }

object
query_attack()
{
    int i;

   // Sometimes this happens, got to test. /Graah
   if (!environment()) return 0;

    while (i < attackers) {
	if (attacker[i] && present(attacker[i], environment()))
	    return attacker[i];
	i++;
    }

    return 0;
}

object
query_alt_attack()
{
    int i;

	if (!environment()) return 0;
    while (i < attackers) {
	if (attacker[i] && present(attacker[i], environment())) {
	    while (++i < attackers)
		if (attacker[i] && present(attacker[i], environment()))
		    return attacker[i];
	    return 0;
	}
	i++;
    }

    return 0;
}
nomask object query_hitter() { return hitter; }
object *query_hunted() { return hunted; }
status query_rest() { return liv_Flags & F_LIV_RESTING; }
status query_sit() { return liv_Flags & F_LIV_SITTING; }
int query_age() { return age; }
status query_frog() { return frog; }
int query_wimpy() { return whimpy; }

// This is WRONG! Should return weight of this object, not weight of items!
int query_weight() { return local_weight; }
// Actually, query_weight is defined in /basic/size.c... :-(

// In the places where the above function is used, it must be changed
// to the next one, then the above one must be removed!
int query_local_weight() { return local_weight; }

int query_intoxication() { return intoxicated; }
int query_stuffed() { return stuffed; }
int query_soaked() { return soaked; }

// Is it night around us? (Reduce light by returned value if so)

int
query_night()
{
    int ll;

    if (environment() && (liv_Flags & F_LIV_NIGHT) &&
      ((int) environment()->query(ROOM_WD) & (WD_OUTDOORS | WD_CITY) == WD_OUTDOORS))
	ll = (int) environment() -> query_light_level();
    // This is how much night reduces light level!
    // That's because not all rooms have light-level of LT_LIGHTS.
    // (especially old rooms tend to use hard-coded value 1 == LT_DIM)
    if (ll > 2) ll = 2;	// Kludge, so that there's no night in city.
    return ll;
}

varargs object
is_fighting(object who)
{
    int i;
    if (!environment()) return 0;
    if (!who) {
	while (i < attackers) {
	    if (attacker[i] && present(attacker[i], environment()))
		return attacker[i];
	    i++;
	}
	return 0;
    }
    if (!present(who, environment())) return 0;
    if (member(attacker, who) >= 0) return who;
    return 0;
}

/****************************************************************
*								*
*		And now set_-functions.				*
*								*
****************************************************************/

void set_hunted(object *h) {
    if (!pointerp(h)) hunted = ({ }); else hunted = h;
}

// Set the wimpy mode
varargs void
set_wimpy(int arg)
{
    whimpy = arg;
    if (whimpy > 70) whimpy = 70;
    if (whimpy < 0) whimpy = 0;
}

// Note that this function does NOT check the PK-restrictions! -+ Doomdark +-
void
set_attacker(object ob)
{
    int i;
    object x;
    // Let's do this fast; though the order won't stay the same for other enemies.
    if ((i = member(attacker, ob)) >= 0) {
	attacker[i] = attacker[0];
	attacker[0] = ob;
    } else {
	// Wow. So a new attacker... Neat-o.
	if (sizeof(attacker) > attackers) {
	    i = attackers;
	    while (--i> 0) {
		attacker[i] = attacker[i - 1];
	    }
	    attacker[0] = ob;
	} else {
	    attacker = ({ ob }) + attacker;
	}
	attackers++;
    }
}

varargs status
set_rest(status arg, status silent)
{
  string b;

  if (silent) {
    liv_Flags = arg ? ((liv_Flags | F_LIV_RESTING) & (~F_LIV_SITTING))
      : (liv_Flags & (~F_LIV_RESTING));
    return arg;
  }
  if (liv_Flags & F_LIV_SITTING) return 0;
  if (!arg) {
    tell_me("You stop resting.");
    if (!INVIS_LEVEL && environment())
      environment() -> tell_here(sprintf("%s stops resting and \
clambers to %s feet.", MY_NAME(this_object()), Possessive()), this_object());
  } else {
/* I hate this: Starks Jan 28, 1999
    b =  ({
      "acromions", "ankle bones", "ankles", "arm-bone", "auditory ossicles",
      "bones", "bones", "bones", "bones", "bones",
      "breastbone", "capitate bones", "carpal bones", "clavicle", "coccygeal bone",
      "costal cartilages", "cuboid bones", "distal interphalangeal joints", "elbow joints", "elbow-bone",
      "fibulas", "finger bones", "frontal bone", "greater multangular bones", "hip bone",
      "humeral articulations", "humeruses", "iliac bones", "intervertebral disks", "jaw bone",
      "kneecaps", "mandibles", "medial malleoluses", "metacarpal bones", "metacarpophalangeal joints",
      "metatarsal bones", "nasal bone", "navicular bones", "proximal interphalangeal joints", "pubic bone",
      "radius bones", "rib bones", "sacral bone", "scaphoid bones", "scapulas",
      "shin bones", "shoulder blade", "skeleton", "skull", "spinal column",
      "sternum", "tarsal bones", "thigh bones", "toe bones", "toe bones",
      "vertebral arches", "vertebras", "vomer", "zygomatic bones",
    }) [random(59)];
*/
  b = "bones";
    tell_me("You rest your tired " + b + ".");
    if (!INVIS_LEVEL && environment())
      environment() -> tell_here(sprintf("%s rests %s tired " + b + ".",
	MY_NAME(this_object()), Possessive()), this_object());
  }
  liv_Flags = arg ? ((liv_Flags | F_LIV_RESTING) & (~F_LIV_SITTING))
    : (liv_Flags & (~F_LIV_RESTING));
  return arg;
}

varargs status
set_sit(status arg, status silent)
{
    if (silent) {
	liv_Flags = arg ? ((liv_Flags | F_LIV_SITTING) & (~F_LIV_RESTING))
	: (liv_Flags & (~F_LIV_SITTING));
	return arg;
    }
    if (liv_Flags & F_LIV_RESTING)
	return 0;
    if (!arg) {
	tell_me("You stand up.");
	if (!INVIS_LEVEL && environment())
	    environment() -> tell_here(sprintf("%s stands up.",
		MY_NAME(this_object())), this_object());
    } else {
	tell_me("You sit down.");
	if (!INVIS_LEVEL && environment())
	    environment() -> tell_here(sprintf("%s sits down.",
		MY_NAME(this_object())), this_object());
    }
    liv_Flags = arg ? ((liv_Flags | F_LIV_SITTING) & (~F_LIV_RESTING))
    : (liv_Flags & (~F_LIV_SITTING));
    return arg;
}

/****************************************************************
*								*
*	And now some 'query-like'-functions.			*
*								*
****************************************************************/

// We can get things, but can't be put inside anything.
status can_put_and_get(string str) { return str != 0; }

// Ask short descs through this to get hallucination effects.
varargs public string
get_short(object ob, object me_ob, int see_ilvl)
{
    string s;
    if (random(100) < 77 || !query_condition(C_HALLUCINATING))
	s = (string) ob->short(see_ilvl, me_ob ? me_ob : this_object());
    else if (living(ob))
	s = (string) HALLU_D->random_short(1);
    else s = (string) HALLU_D->random_short(2);
    return s;
}

public varargs
string get_name(object ob, object me_ob)
{
    string s;
    if (random(100) < 77 || !query_condition(C_HALLUCINATING))
	s = (string) ob -> query_name(0, me_ob ? me_ob : this_object());
    else if (living(ob))
	s = (string) HALLU_D->random_short(1);
    else s = (string) HALLU_D->random_short(2);
    return stringp(s) ? s : "Someone";
}

// NEW: Returns other values than 1, 0 or -1 now....
// Otherwise same return values as test_dark2 except that
// 0 means full visibility instead of 7/-7.
// Reason for difference is compatibility, as usual.

nomask int
test_dark()
{
    int night, ll;

    if (!query_can_see()) return 1; // Query_can_see sets the fail-msg.

    night = (liv_Flags & F_LIV_NIGHT);
    ll = (int) environment()->query_light(night);
    night = (night ? -1 : 1);

    // First we'll check trolls:
    if (infravision == 5) {
	if (ll > LT_DIM && (night > 0)
	  && (((int)environment()->query(ROOM_WD) & (WD_CITY | WD_OUTDOORS))
	    == WD_OUTDOORS))
	    return notify_fail("It's too bright here!\n"), 3;
	return 0;
	// Then others:
    } else {
	if (ll < LT_DIM)
	    switch (infravision) {
	case 0: case 1:
	    notify_fail("It's too dark in here!\n");
	    return 2 * night;	// 2/-2 -> too dark
	case 2:
	    notify_fail("It's too dark in here!\n");
	    return 4 * night;
	case 3:
	    notify_fail("It's too dark in here!\n");
	    return 5 * night;
	case 4: // Dwarves!
	    // notify_fail("It's too dark here!\n");
	    // return 6 * night;
	// this should return 0 instead 7, right?
	    return 0;
	default:
	    return 0;
	}
    }
    return 0;
}

// Test_dark2 is bit faster than test_dark; used by move_player for example.
// Arguments:
// 1st: Total light level of the room we're in (except for night)
// 2nd: Intrinsic light level of the room we're in (without night & item lights)
// 3rd: Outdoor level of the room we're in

// Return value:
// sign: negative if it's night now, positive otherwise
// 1(/-1): Nope, we can't see anything due to blindness and such
// 2(/-2): Nope, it's too dark to see anything
// 3(/-3): Nope, it's too bright to see anything
// 4(/-4): We can see living, non-undead objects, but not anything else
// 5(/-5): We can see - "" - and in addition short desc without exit list
// 6(/-6): We can see - "" - and short desc with exit list.
// 7(/-7): Ok, we can see 100% ok (including undead monsters)

// Graah 23-Jun-96: Dwarves now always see at (+7), unless it is
// ridiculously bright or dark.

nomask varargs status
test_dark2(int total_light, int light_level, int outdoorness)
{
    int iv, night;

    // Exception #1: City. Always visible...
    if (outdoorness & WD_CITY) {
	night = ((liv_Flags & F_LIV_NIGHT)) ? -1 : 1;
	return query_can_see() ? (7 * night) : (night);
    }

    // If it's night out there and we're outdoors, we'll adjust the total
    // light value...
    if (liv_Flags & F_LIV_NIGHT) {
	if (!query_can_see()) return -1;
	night = -1;
	if (light_level > 0 && (outdoorness & WD_OUTDOORS)) {
	    if (light_level < 3) {
		total_light -= light_level;
	    } else {
		total_light -= 2;
	    }
	}
    } else {
	if (!query_can_see()) return 1;
	night = 1;
    }

    // It's dark (night or not)?
    if (total_light < LT_DIM) {
	switch (infravision) {
	case 0: case 1:
	    return 2 * night;	// 2/-2 -> too dark
	    break;
	case 2:
	    return 4 * night;
	    break;
	case 3:
	    return 5 * night;
	    break;
	case 4:
	    // Infra 4 is only used by dwarves (and undead, but
	    // if Liches are installed, they should have some other
	    // infravision level or troll level?)
	    // Was 6 * night
	    return 7; // Dwarves always see.
	    break;
	default:
	    return 7 * night;
	    break;
	}
    } else if (infravision == 5 && total_light > LT_DIM
      && (night > 0) && (outdoorness & WD_OUTDOORS)) {
	// Troll under the sun?
	return 3 * night;
    }

    return 7 * night;
}

// If no _player_ is here (except possibly ourself), we'll return 0, otherwise 1.
// Used by monsters, mainly (to turn off heart beat when no players are in
// same room... saves lots of evaluation time).

status
test_if_any_here()
{
    object ob;
    if (!(ob = environment())) return 0;
    ob = first_inventory(environment());
    while(ob) {
	if (ob != this_object() && interactive(ob)) return 1;
	ob = next_inventory(ob);
    }
    return 0;
}

/****************************************************************
*								*
*		Then some useful add-functions.			*
*								*
****************************************************************/

/* Added flag to make "hit_player" simulations etc. //Graah */
varargs int
add_hp(int x, int flag)
{
    if (hit_point > 0) {
	hit_point += x;

	if (hit_point <= 0) {
	    if (flag == 1) {
		if (hit_point < 0 && !(liv_Flags & F_LIV_IS_NPC)) {
		    // We are nearly dead. Terminal unconsciousness.
		    // Let's make sure we _do_ add condition. -> let's force it.
		    if (query_condition(C_UNCONSCIOUS) >= 0) {
			set_condition(C_UNCONSCIOUS | C_FORCE_FLAG, -1);
			tell_me("You fall down, unconscious!");
			environment() -> tell_here(sprintf("%s  falls down.",
			    MY_NAME(this_object())), this_object());
			stop_fight(attacker_ob);
		    }
		}
	    } else {
		hit_point = 1;
	    }
	}
	if (hit_point >= max_hp)
	    hit_point = max_hp;
	// Now add_hp can recover from unconsciousness too!
    } else if (flag == 1) {
	if ((hit_point += x) > 0)
	    set_condition(C_UNCONSCIOUS, 0);
    }
    return hit_point;
}

int
add_sp(int x)
{
    if ((spell_points += x) > max_sp) spell_points = max_sp;
    if (spell_points < 0) spell_points = 0;
    return spell_points;
}

int
add_fp(int x)
{
    if ((fatigue_points += x) > max_fp) fatigue_points = max_fp;
    if (fatigue_points < 0) fatigue_points = 0;
    return fatigue_points;
}

int
add_player_kills(int a)
{
    player_kills += a;
    if(player_kills < 0) player_kills = 0;
    return player_kills;
}

// Returns true if we can carry that weight too.
// NEW! 2nd argument to prevent recount_weight.
// Can be used for efficiency if you know what you're doing.

varargs status
add_weight(int w, status no_recount)
{
    int max;

    if (!no_recount)
	recount_weight();

    // Coders still drop stuff when carrying too much? //Graah
    // Yup. And let's leave it that way; easier to test/debyg! -+ Doomdark +-
    //	if (query_coder_level() && !(liv_Flags & F_LIV_IS_NPC)) return 1;

    max = query_race_stat(RACE_CARRY_RATE) * (query_stat(ST_STR) + 10) / 111;

    if (w > 0 && w + local_weight > max) return 0;
    if (w + local_weight < 0) return 0;
    local_weight += w;
    return 1;
}

// This is used by the shop etc.
void
add_money(int m)
{
    int x;
#ifdef LOG_EXP
    if (this_player() && this_player() != this_object() &&
      interactive_info(this_player(), II_IP_NUMBER) && interactive_info(this_object(), II_IP_NUMBER) &&
      (m >= ROOM_EXP_LIMIT || this_object()->query_coder_level())) {
	x = (int) this_player()->query_coder_level();
	log_file("EXPERIENCE", sprintf("%s %s(%d) %d money by %s(%s%d)\n",
	    ctime(time()), name, level, m,
	    (string) this_player()->query_real_name(),
	    (x  ? "C" : ""), (x ? x : (int)this_player()->query_level())));
    }
#endif
    money += m;
    if ((liv_Flags & F_LIV_IS_PLAYER) && (m > 200 || m < -200))
	this_object()->save_me(0);
}

/********************************
*	Few internal funcs:	*
********************************/

void
update_attackers(int silent)
{
    int i, j, k;
    while (i < attackers) {
	if (attacker[i] && present(attacker[i], environment()))
	    i++;
	break;
    }
    if (i == attackers) return;
    k = i;

    j = i++;
    while (i < attackers) {
	if (attacker[i] && present(attacker[i], environment()))
	    attacker[j++] = attacker[i];
	i++;
    }
    if (attackers > 3) {
	if (j <= ATTACKER_ARRAY_SIZE)
	    attacker = attacker[0..ATTACKER_ARRAY_SIZE - 1];
	else attacker = attacker[0..j - 1];
    }
    attackers = j;
    while (j < sizeof(attacker))
	attacker[j++] = 0;
    if (!silent && !k && attackers)
	tell_me(sprintf("You turn to attack %s!",
	    (string) attacker[0] -> query_name(0, this_object())));
}

/****************************************************************
*								*
*	Now some setup-functions; these are to be called by	*
*	any module that inherits us...				*
*								*
****************************************************************/

void
reset_living()
{
    int t;

#ifdef OLD_ALIGNMENT
    reset_alignment();
#endif

    // Then, we'll do some cleaning up.
    if (hunted && sizeof(hunted))
	hunted -= ({ 0 });

    t = 0;

    // Let's hope this helps in really removing destructed objects from game. :-)
    while (t < sizeof(attacker))
	if (!attacker[t++]) {
	    update_attackers(1);
	    break;
	}
}

void
create_living()
{
    attacker = ({ });
    hunted = ({ });
    attackers = 0;
}

/****************************************************************
*								*
*	Now all the remaining functions...			*
*								*
****************************************************************/

/* This routine is called when we are attacked by a player.
 * Use "to_top" value -1 if you only want to check if it's possible,
 * not to start combat! */

varargs int
attacked_by(object ob, int to_top, int enemy_flags)
{
    int i;
    object tmp;

    if (!objectp(ob) || ob == this_object()) return 0;

    if (!enemy_flags)
	enemy_flags = (int) ob->query(LIV_FLAGS);

    /* Are both players? */
    if (liv_Flags & enemy_flags & F_LIV_IS_PLAYER) {
	/* Players shouldnt be able to attack coders! -+ Doomdark 26-feb-96 +- */
	if (query_coder_level() > 0) return 0;
	/* Is there Peace? /Graah */
	if (PEACE_D->query_peace())
	{
	    ob->tell_me("Peace is upon the world. You cannot \
currently attack other players, nor can they attack you.");
	    return 0;
	}

#ifdef GN_HEALER
	// Log PK attacks on healers. Don't bother to fix the guilds
	// that allow cheat training - we want to catch and punish
	// the cheaters with this :) /Graah
	if (query_guild() == GN_HEALER)
	  log_file("ATTACK_HEALER",
		   sprintf("%s attacked %s at %s in %s\n",
			   capitalize((string)ob->query_real_name()),
			   cap_name,
			   ctime(time())[4..15], object_name(environment(ob))));
#endif

	if (!dirty_stamp && environment()->query(NO_PK)) {
	    if (to_top < 0) return 0;
	    ob->tell_me(sprintf("Your attempt to kill an innocent player,\
 in a place that doesn't allow it, was noticed. You immediately regret your\
 cruel attempt to kill and you quickly pat %s on the shoulder like nothing\
 happened.", MY_NAME(ob)));
	    tell_me(sprintf("%s pats you on the shoulder.",
		(string) ob->query_name(0, this_object())));
	    environment()->tell_here(sprintf("%s pats %s on the shoulder.",
		ob->query_name(), MY_NAME(ob)), ({ob, this_object()}));
	    log_file("PLAYER_KILLER", sprintf("%s tried to kill %s at\
 %s in %s\n", capitalize((string)ob->query_real_name()), cap_name,
		ctime(time())[4..15], object_name(environment(ob)) ));
	    stop_fight(ob);
	    ob->stop_fight(this_object());
	    return 0;
	}

	if (to_top < 0) return 1;

	if (!dirty_stamp)
	    set(LIV_OK_TO_PK);
	else
	  {
	    // Dirty stamp shouldn't be set on char who is already
	    // dead. /Graah
	    if (!dead)
	      dirty_stamp = age + TIME_TO_STAY_DIRTY;
	  }

	/* How about enemy... let's be sure he/she can be attacked too! */
	if (!(enemy_flags & F_LIV_OK_TO_PK))
	    ob->set(LIV_OK_TO_PK);

	log_file("PLAYER_KILLER",
	  sprintf("%s started to kill %s at %s in %s\n",
	    capitalize((string)ob->query_real_name()),
	    cap_name,
	    (ctime(time())) [4..15], object_name(environment(ob)) ));
    }

    if (to_top < 0) return 1;

    // 24-Aug-96 / Graah: New flag: monster can cause PK flag to set
    if ((enemy_flags & F_LIV_IS_PLAYER) &&
      query(NPC_SET_PK_FLAG))
    {
	if (!(enemy_flags & F_LIV_OK_TO_PK))
	    ob->set(LIV_OK_TO_PK);
    }

    if ((i = member(attacker, ob)) >= 0) {
	if (!to_top) return 1;
	attacker[i] = attacker[0];
	attacker[0] = ob;
	return 1;
    }
    if (!attackers && (liv_Flags & F_LIV_IS_NPC))
	configure_object(this_object(), OC_HEART_BEAT, 1);

    if (!sizeof(attacker))
	attacker = allocate(ATTACKER_ARRAY_SIZE);
    if (to_top) {
	if (sizeof(attacker) > attackers) {
	    i = attackers;
	    while (--i> 0) {
		attacker[i] = attacker[i - 1];
	    }
	    attacker[0] = ob;
	} else {
	    attacker = ({ ob }) + attacker;
	}
    } else {
	if (sizeof(attacker) > attackers) {
	    attacker[attackers] = ob;
	} else {
	    attacker += ({ ob });
	}
    }
    attackers++;
    return 1;
}

// With this we can stop fighting some or all enemies...
// Note that we won't do "update_attackers"; let's do that later...
varargs void
stop_fight(object ob, int flags)
{
    int i;
    object tmp;
    if (ob) {
	// Flag; bit 0 -> move as last enemy, no complete stop
	if (flags & 1) {
	    if ((i = member(attacker, ob)) >= 0
	      && attackers > (i + 1)) {
		tmp = attacker[attackers - 1];
		attacker[attackers - 1] = ob;
		attacker[i] = tmp;
	    }
	    return;
	}
	while (i < attackers)
	    if (attacker[i] == ob)
		attacker[i++] = 0;
	    else i++;
	if ((i = member(hunted, ob)) >= 0) {
	    hunted[i] = 0;
	    hunted -= ({ 0});
	}
    } else {
	while (i < attackers)
	    attacker[i++] = 0;
	attackers = 0;
	kill_ob = 0;
    }
}

// This routine is called from other routines to drop one specified object.
// We return true if success.
varargs int
drop_one_item(mixed ob, int force)
{
    int wt;

    if (!ob || ob->drop(force)) return 0;

    if (!ob) return 1;
    // This may look silly, but is needed in case 'drop()' destructed
    // the object in question! In that case, we consider it
    // to have been a succesfull drop.
    // Armours worn only weigh 1/2 of normal weight! -+ Doomdark +-

    if (wt = (int)ob->query_weight())
	if (is_wearing(ob))
	    add_weight(- ((wt + 1) / 2), 1);
	else add_weight(-wt, 1);

    move_object(ob, environment());
    call_other(environment(), "extra_move_object", ob, environment());

    return 1;
}

void
drop_all(int silent)
{
    mixed *inv, *inv2;
    string it, out, myn;
    int i,size, x;
    int verbose;

    if (silent & 1) verbose = 0; else verbose = 1;
    inv = all_inventory();

    if ((size = sizeof(inv)) < 1) return;
    inv2 = allocate(sizeof(inv));

    if (verbose) myn = MY_NAME(this_object());

    for (i = 0; i < size; i++) {
	if (drop_one_item(inv[i], silent) && verbose &&
	  (it = (string)inv[i]->short(0, this_object()))) {
	    inv[i] = sprintf("drop: %s.", get_short(inv[i], this_object()));
	    inv2[i] = sprintf("%s drops %s.", myn, it);
	    x++;
	} else {
	    inv[i] = 0;
	    inv2[i] = 0;
	}
    }
    if (verbose && x) {
	tell_me(efun::implode(inv, "\n"));
	environment()->tell_here(efun::implode(inv2, "\n"), this_object());
    }
    recount_weight();	// Let's add this as drop_one_item won't recount.
}

status
drink_alco(int strength)
{
    if ((intoxicated + strength) > (query_stat(ST_CON)
	+ query_race_stat(RACE_DRUNKNESS_RATE))) {
#if 0
	if (random(2)) tell_me("You fail to reach the drink with your mouth.");
	else tell_me("You missed.");
	return 0;
#endif
	tell_me("You feel perfectly relaxed and content, and then "
	  "black out. ");
	// Was C_UNCONSCIOUS, but this is better! /Graah
	set_condition(C_PASSED_OUT, (intoxicated+strength)/2);
    }
    // Players get braver when they are drunk :) ++Tron
    // Another piece of code in run_away() function.
    if ((intoxicated + strength) > (query_stat(ST_CON) / 2)) {
	tell_me("You feel absolutely invincible and mumble something\
 about being very BRAVE from now on.");
    }
    if ((intoxicated += strength / 2) < 0) intoxicated = 0;
    if (!intoxicated) tell_me("You are completely sober.");
    if (intoxicated > 0 && headache) {
	headache = 0;
	if (random(2)) tell_me("Your headache disappears.");
	else tell_me("Your hands stop shaking.");
    }
    if (intoxicated > max_headache)
	max_headache = intoxicated;
    if (max_headache > 8)
	max_headache = 8;
    return 1;
}
status drink_alcohol(int str) { return drink_alco(str); }

status
drink_soft(int strength)
{
    if ((soaked + strength) > (query_stat(ST_CON) * 8)) {
	tell_me("You can't possibly drink that much right now!\n\
You feel crosslegged enough as it is.");
	return 0;
    }
    if ((soaked += strength * 2) < 0) soaked = 0;

    if (!soaked)
	tell_me("Your mouth feels a bit dry.");

    return 1;
}

status
eat_food(int strength)
{
    if ((stuffed + strength) > (query_stat(ST_CON) * query_race_stat(RACE_EAT_RATE))) {
	if (random(2)) tell_me("This is much too rich for you right now!\
 Perhaps something lighter?");
	else tell_me("Your stomach is too full to eat that now.");
	return 0;
    }
    if ((stuffed += strength * 2) < 0) stuffed = 0;
    if (!stuffed) tell_me("Your stomach makes a rumbling sound.");
    return 1;
}

void
purge_hunt()
{
    int i, x;

    if (!hunted) hunted = ({});
    if (sizeof(hunted) < 2) return;

    for (i = 0; i < sizeof(hunted); i++) {
	if (!objectp(hunted[i]) || hunted[i]->query_ghost()) {
	    hunted[i] = 0;
	    x++;
	}
    }

    if (x) hunted -= ({ 0 }); // Let's remove all 0s.
    if (sizeof(hunted) > MAX_HUNTED)
	hunted = hunted[0..MAX_HUNTED - 1];
}

// This is called from move_player() when we leave a room some attackers are
void
start_hunting(object h)
{
    if (!hunted)
	hunted = ({ h });
    else hunted = ({ h }) + hunted;
    //	purge_hunt();
    // Let's not call this all the time... :-/
}

// Redefinable event function. This is called when someone or something
// kills us. This can get a zero argument or even an non-living object,
// so check before you do your stuff. /areas/tristeza/npc/guard.c has
// an example of how it can be used.
// //Graah

// void killed_by(object ob) { }

// This function is called from other players when they want to make
// damage to us. We return how much damage we received.
//
// Arguments:
// dam: Amount of dmg in hps
// t: Bitfield containing the damage type(s) of this attack. Default is
// slashing (edged weapons). Always use DT_xxx defines from /include/attack.h.
// hc: Hit class of the weapon (or attack) used
// NEW! Can be a closure... Guess what it'll do then! -+ Doomdark 27-jun-96
// enemy: Object who does damage to us. If not specified, this_player()
// or previous_object() will be used.

// Note that enemy_flags always != 0 if enemy really is an NPC or player;
// this is used to prevent fighting 'dummy' enemies; rooms doing damage
// and such.

varargs int
hit_player(int dam, int t, mixed hc, object enemy)
{
    int dodge, c, aw, x, y, z;
    string killer;
    int enemy_player, us_player, enemy_flags;
    int pk_checked, old_hp, orig_dam;
    mixed func;

    if (!enemy) enemy = this_player();
    if (!enemy) enemy = previous_object();

    if (!enemy) return 0;

    hitter = enemy;
    if (living(enemy)) {
	enemy_flags = (int) enemy->query(LIV_FLAGS);
	if (enemy_flags & F_LIV_IS_PLAYER)
	    enemy_player = 1;
    }

   if(enemy_player && (query(LIV_FLAGS) & F_LIV_IS_PLAYER) && dam > 0) {
     if(dam > 900) dam = 225;
     else {
      float dam2;
        dam2 = dam/2.0 - (dam/60.0 * dam/60.0);
       dam = to_int(dam2);
     }
   }
    orig_dam = dam;
    // First, let's update attacker list and call attacked_by, which may
    // be masked by NPCs.

    us_player = liv_Flags & F_LIV_IS_PLAYER;

    // If we're not yet fighting that enemy, let's call attacked_by!
    if (enemy && enemy != this_object()
      && (enemy_flags & (F_LIV_IS_NPC | F_LIV_IS_PLAYER))) {
	if (member(attacker, enemy) < 0) {
	    if (enemy != this_object()) {
		if (!us_player)
		    configure_object(this_object(), OC_HEART_BEAT, 1);
		if (!attacked_by(enemy, 0, enemy_flags))
		    return 0;
		pk_checked = 1;
	    }
	}
    }

    // Prevent cheats (maybe?)

    if (name == "guest" && enemy_player) {
	tell_me("\n\nThings are getting too violent here. Goodbye.\n\n");
	destruct(this_object());
	return 0;
    }

    // Some guilds seem to cheat too...

    if (!pk_checked && enemy != this_object() && us_player
      && enemy_player && !dirty_stamp	&& environment()->query(NO_PK)) {
	tell_me(sprintf("%s tried to damage you but this is not a\
 playerkilling area.", (string)enemy->query_name()));
	enemy->tell_me(sprintf("You can't hurt %s because this is not\
 a playerkilling area.", query_name()));
	enemy->stop_fight(this_object());
	stop_fight(enemy);
	return 0;
    }

    // Don't damage coders too much!

    if (us_player && query_coder_level()
      && dam >= (hit_point + DEATH_HP(max_hp))) {

	// Stop fighting. we would have died.

	stop_fight(enemy);
	if (enemy && enemy_flags)
	    enemy->stop_fight(this_object());
	tell_me("You would have died, but since you are immortal you don't.");
	set_condition(C_UNCONSCIOUS, 0);
	hit_point = max_hp;
	return 0;
    }

    // Dead people can't be hit. Should we log these incidents??
    if (dead) {
	stop_fight(enemy);
	if (enemy && enemy_flags)
	    enemy->stop_fight(this_object());
	return 0;
    }

    // A new feature: redefinable function - is the monster
    // immune to damage from this enemy? //Graah
    // Let's also pass the damage type! -+ Doomdark +-

    if (query_is_immune_vs_obj(enemy, t)) {
	funcall(hc, this_object(), 0, t);
	return 0;
    }

#if 0
    // Prevent cheating guilds from causing damage when it is impossible
    // 18.7.1994 //Graah
    // Let's not waste resources here; call_others eat time _in vain_
    // Guilds need to be fixed instead. -+ Doomdark +-
    if (enemy && enemy != this_object() && enemy_flags
      && !enemy->query_can_move()) {
	return 0;
    }
#endif

    // Amount to avoid due to AC:
    // NEW! Now reduces 25% of AC plus random(AC / 2)... Same amount than previously,
    // but not as random!
    y = (armour_class + 2 + random(2 * (armour_class + 2))) >> 2;
    if (!query_can_move())
	y >>= 1;

    // Can avoid some damage only if able to move
    // NO! Guilds MUST check this themselves!
    // And why's that?? -+ Doomdark +-
    // Temporary solution for Healers. Other guilds must be checked first.

    // Removed the kludge that Graah had added for healers, and added a check
    // here so that healers can idle as much as they want and still stop fights.
    // Chopin 3-May-98
    if (query_can_move()
#ifdef GN_HEALER
	|| guild == GN_HEALER
#endif
    )
    {
	// Guild bonus/penalty applied before armor class, only for players.
	if (us_player && (GuildFlags & (1 << G_HOOK_HIT_PLAYER)) &&
	  interactive(this_object()) &&
	  (interactive_info(this_object(), II_IDLE) < IDLE_LIMIT_FOR_DODGE
#ifdef GN_HEALER
	  || guild == GN_HEALER
#endif
	) && (((int)this_object()->query_skill(SK_DODGE) == SKILL_NO_EXIST) ||
		query_condition(C_NO_DODGE)) &&
		(func = GuildHooks[G_HOOK_HIT_PLAYER])) {
	    if (to_object(func) && intp(x = funcall(func, dam, t, hc, enemy)))
		dam = x;
	}
	// We _SHOULD_ consider different damage types here, btw!
	// Stupid racial modifiers... Begone:
	// dam = dam * query_race_stat(RACE_AC_RATE) / 100;
    }

    // Cheating people march around with level 20 and dex 1.
    // So, they are too slow to avoid hits: hits go through armor.
    z = query_stat(ST_DEX);
    if ((z >= 10) ||
	(level < 15) ||
	((level - z) < (10 + random(10))))
      // AC softens hit in any case. :-p (even if we can't move)
      dam -= y;
	if(t > DT_FORCE) {
	if((int)this_object()->query_skill(SK_M_DEFENSE) != SKILL_NO_EXIST)
		if(this_object()->query_skill_hook(SK_M_DEFENSE, 0)) {
			dodge = (int)this_object()->use_skill(SK_M_DEFENSE,
				SK_TRAIN_STD, ({
				dam, t, hc, enemy }));
				if(dodge > HOOK_RESOLVED)
					dam -= dodge;
				dodge = 0;
			}
	}
	if(t <= DT_PHYS && t > DT_FORCE && !query_condition(C_STUNNED)
		&& !query_condition(C_NO_DODGE)) {
		// Armour just gets in the way
		// But strength is a factor, too
		if(c = sizeof(query_armour("weight")))
		{
			while(c--)
				aw += query_armour("weight")[c];
			aw -= (query_stat(ST_STR)/8);
			if(aw < 0) aw = 0;
		}
		if(query_fp() < COST_FOR_ASKILL + (aw/4) ||
		query_condition(C_UNCONSCIOUS) ||
		query_condition(C_STUNNED))
			dodge = SKILL_NO_EXIST;
		else
	dodge = (int)this_object()->use_skill(SK_DODGE,
		SK_TRAIN_X_HARD);
	if(dodge > SKILL_NO_EXIST)
		if(add_fp(-(COST_FOR_ASKILL + (aw/4))) <
			(COST_FOR_ASKILL + (aw/4)))
			tell_me(
			"Your last attempt at dodging has worn you out."
			);
	if(dodge > 0) {
		dodge = dodge * (int)this_object()->query_skill_lev("dodge");
		dodge = to_int((float)dodge/2);
		dodge -= aw;
		// Small bonus for dex
		dam += (query_stat(ST_DEX) / 10);
		if(dodge < 0) dodge = 0;
		dam -= dodge;
		if(dam < 0) dam = 0;
		}
	}


    // There is something weird in the following piece of code so I
    // added #if 0:
    // The greater the hit_class, the smaller the chance to hit ?!
    // Another reason is, that I want to be able to call that hc-closure
    // later in the code. Chopin 3-May-98 
#if 0
    // This is where we check hit class (accuracy).
    // It must not affect damage. Strange.

    if (!intp(hc))
	hc = 0;
    if ((dam - hc) <= 0) return 0;
#endif

    //MUDwide tuning of damage. This will be removed from here when all
    //the guilds implement it in their own code.
    // Bull 27. 7. 1998
    if( !( enemy->query_npc() ) )
      dam = TUNE_GENERAL_DAMAGE(dam);

    // We always do at least 1 point of damage, if we hit.

    /* We can now resist some damage */
    dam = dam - (dam*query_resistance(t))/1000;
    if (dam < 1) dam = 1;

    // Hitter may want to know how much damage was actually done.
    // This funcall was done before resistances were checked, but I moved
    // it here so that the real damage that is done can be told
    // to the hitter (but before this object possibly dies)
    // Chopin 3-May-98
    
    if(closurep(hc) && to_object(hc))
        funcall(hc, this_object(), dam, t, orig_dam);

    old_hp = hit_point;
    hit_point -= dam;

    // Hobbits are sneaky! //Graah
    // Uhhhh?!?!?! -+ Doomdark +-

   // old_hp should prevent multiple mort-messages +- Flagg

    if (hit_point < 0 && us_player) {
      // We are nearly dead. Terminal unconsciousness.
	last_attacker = enemy;
	name_of_last_attacker = (string) enemy->query_real_name();
      // give message only once ...
        if (old_hp >= 0){
	   set_condition(C_UNCONSCIOUS | C_FORCE_FLAG, -1);
	   tell_me("You are mortally wounded and fall down, unconscious!");
	   environment() -> tell_here(sprintf("%s is mortally wounded and\
 falls down.", MY_NAME(this_object())), this_object());
      // Let's make enemies skip us for a second... 8-)
	   x = 0;
	   while (x < attackers) {
	       if (attacker[x] && present(attacker[x], environment()))
		   attacker[x]->stop_fight(this_object(), 1);
	        x++;
	  }
      // Let's inform guild object about going mortally wounded as well!
	  if ((GuildFlags & (1 << G_HOOK_GO_MORT))
	   && (func = GuildHooks[G_HOOK_GO_MORT])) {
	      funcall(func, this_object(), enemy);
	  }
	 }
    }

    if ((!us_player && (hit_point < 0))
      || (us_player && (hit_point < -(DEATH_HP(max_hp))))) {
	if (!enemy_flags) enemy = last_attacker;
	// Let's reward the one who did the last strike (and his/her party)
	catch(this_object()->death(1, enemy));
    }

    return dam;
}


void
transfer_all_to(object dest)
{
    object *ob;
    object mon;
    int i;

    if (ob = all_inventory())
	for (i = sizeof(ob) - 1; i >= 0; i--)
	    // Beware that drop() might destruct the object.
	    if (!ob[i] -> drop(1) && ob[i])
	    {
		move_object(ob[i], dest);
		call_other(environment(), "extra_move_object",
		  ob[i], dest);
	    }
    local_weight = 0;
    if (!money) return;
    if (mon = present(" money", dest)) {
	mon -> set_money((int) mon->query_money() + money);
    } else {
	mon = clone_object(GENERIC_MONEY_OBJECT);
	mon -> set_money(money);
	move_object(mon, dest);
    }
    money = 0;
}

// Fleeing in panic? In that case we might drop some things. //Graah
// Or even some coins :-D

void
drop_things_in_panic()
{
    object *inv, ob;
    int num,i,x, dx;

    if (level < (2 + random(3))) return;

    inv = all_inventory(this_object());

    dx = query_stat(ST_DEX);

    // Change this algorithm if it is too cruel or too nice /Graah
    num = random(12) +
	  random(sizeof(inv) / 2) - random(dx / 2);

    // Spill coins
    if (num > 1 && !random(4) && money > 10) {
	x = random(money) + 1;
	if (ob = present(" money", environment())) {
	    ob -> set_money((int)ob->query_money() + x);
	} else {
	    ob = clone_object(GENERIC_MONEY_OBJECT);
	    ob -> set_money(x);
	    move_object(ob, environment());
	}
	money -= x;
	tell_me("You spill some coins while fleeing.");
	environment() -> tell_here(MY_NAME(this_object())
	  + " spills some coins while fleeing.",this_object());
	return; // It's enough when this happens...
    }

    if (num > 1) for (i = sizeof(inv)-1; i >= 0; i--) {
      x = random(sizeof(inv));
      // Worn and wielded (= used) things are not dropped easily.
      if (present(inv[x], this_object())
	  && !inv[x]->is_used()
	  && !inv[x]->drop()) {
	tell_me(sprintf("You dropped %s while fleeing.", get_short(inv[x])));
	environment() -> tell_here(sprintf("%s dropped %s while fleeing.",
MY_NAME(this_object()), (string)inv[x]->query_name(0, this_object())),
				   this_object());
	move_object(inv[x], environment());
	call_other(environment(),
		   "extra_move_object", inv[x], environment());
	add_weight(-((int) inv[x] -> query_weight()));
	return;
      }
    }
}

void
drop_all_money(status verbose)
{
    object mon;
    if (!money) return;
    if (mon = present("money", environment())) {
	mon -> set_money((int)mon->query_money() + money);
    } else {
	mon = clone_object(GENERIC_MONEY_OBJECT);
	mon -> set_money(money);
	move_object(mon, environment());
    }
    if (verbose) {
	tell_me(sprintf("You drop %d gold coins.", money));
	environment() -> tell_here(sprintf("%s drops %d gold coins.",
	    MY_NAME(this_object()), money), this_object());
    }
    money = 0;
    if (liv_Flags & F_LIV_IS_PLAYER)
	this_object()->save_me(0);
}

// Added recount Val 20/11/91
void
recount_weight()
{
    int new_weight, max, i, x;
    object *ob;
    string str;

    ob = all_inventory(this_object());
    new_weight = 0;

    if (ob)
	for(i = sizeof(ob) - 1; i >= 0; i--) {
	    // For worn armour, we'll only use half of normal weight! -+ Doomdark +-
	    if (worn_armour && (x = member(worn_armour, ob[i])) >= 0)
		new_weight += (worn_armour_weight[x] + 1) / 2;
	    else
		new_weight += (int)ob[i]->query_weight();
	}

    local_weight = new_weight;

    // test that the weight is legal
    // max = query_stat(ST_STR) + 10;
    max = query_race_stat(RACE_CARRY_RATE) * (query_stat(ST_STR) + 10) / 111;

    // Was "... / 100" but as worn armour now weighs only 1/2 of normal
    // weight, I lowered max. capacity a bit. -+ Doomdark +-

    i = 0;
    while (local_weight > max && ob[i]) {

	new_weight = local_weight - (int) ob[i]->query_weight();
	if (new_weight >= local_weight) { i++; continue; }

	if (!ob[i]->drop() && ob[i]) {
	    if (str = get_short(ob[i])) {
		tell_me(str + " drops through your fingers.");
		environment() -> tell_here(sprintf("%s has problems carrying %s items.",
		    MY_NAME(this_object()), Possessive()), this_object());
	    }
	    move_object(ob[i],environment());
	    call_other(environment(), "extra_move_object", ob[i],environment());
	}
	if (!ob[i] || environment(ob[i]) != this_object())
	    local_weight = new_weight;
	i++;
    }
}

void
heal_self(int h)
{

    if (h <= 0) return;

    // Back from death's door. It's time to wake up soon.
    if (hit_point < 0 && (hit_point + h) >= 0)
	set_condition(C_UNCONSCIOUS, 0);

    hit_point += h;

    if (hit_point > max_hp) hit_point = max_hp;
    if ((spell_points += h) > max_sp) spell_points = max_sp;
    // Let's add fps too. -+ Doomdark 15-feb-96 +-
    // Upped rate to h from h/2  -- Sumppen 25-jun-96 --
    if ((fatigue_points += (h)) > max_fp) fatigue_points = max_fp;
}

// Now returns whether we succeeds in attack!
varargs status
attack_object(object ob, status no_hit)
{
    int ret_val, i;
    if (!ob || ob == this_object()) return 0;
    if (is_fighting(ob)) return 1;
    if (ob -> query_ghost()) return 0;
    if (ret_val = (int) ob -> attacked_by(this_object())) {
	while (i < attackers) {
	    if (!attacker[i] || !present(attacker[i], environment()))
		break;
	    i++;
	}
	if (i < attackers) {
	    attacker[i] = ob;
	} else if (attackers < sizeof(attacker)) {
	    attacker[attackers++] = ob;
	} else {
	    attacker += ({ ob });
	    attackers++;
	}
	if (!no_hit) attack();
    }
    return ret_val;
}

varargs string
show_age(status no_write)
{
    int i, x;
    string txt;

    txt = "Age: ";
    i = age;
    if (x = i / 43200) {
	txt = sprintf("%s %d days ", txt, x);
	i -= x * 43200;
    }
    if (x = i / 1800) {
	txt = sprintf("%s %d hours ", txt, x);
	i -= x * 1800;
    }
    if (x = i / 30) {
	txt = sprintf("%s %d minutes ", txt, x);
	i -= x * 30;
    }
    txt = sprintf("%s %d seconds.", txt, i * 2);
    if (no_write) return txt;
    if (this_player())
	this_player() -> tell_me(txt);
    return 0;
}

// This function remains only because of compatibility, as command() now
// can be called with an object as argument.
// A security hole here, I say. :-/
void
force_us(string cmd)
{
    if (!this_player() || (int) this_player()->query_level() <= level ||
      !interactive_info(this_player(), II_IP_NUMBER)) {
	tell_me(sprintf("%s failed to force you to '%s'.",
	    this_player()->query_real_name(), cmd));
	return;
    }
    if (this_player() && !this_player()->query(LIV_IS_NPC)) {
	if (!(liv_Flags & F_LIV_IS_NPC)
	  && (int) this_player()->query_level() < level ||
	  !interactive_info(this_player(), II_IP_NUMBER)) {
	    tell_me(sprintf("%s failed to force you to '%s'.",
		(string) this_player()->query_real_name(), cmd));
	    return;
	}
    }
    tell_me(sprintf("%s forces you to '%s'.", (string) this_player()->
	query_real_name(), cmd));
    command(cmd);
}

status
frog_curse(status arg)
{
string r;
    if (arg) {
	if (frog) return 1;
	tell_me("You turn into a frog!");
	// Guest is protected against the curse. Obvious reasons. /Graah
	if (name == "guest") {
	    tell_me("But you, as a guest, are immune.");
	    return 1;
	}
	frog = 1;
	recount_weight();
	return 1;
    }
   r = query_race();
	if (!r) r = "human";
    if (query_gender() != G_NEUTER)
        tell_me(sprintf("You turn into a %s once again.", r));
    else tell_me("You turn into a worm once again.");
    return (frog = 0);
}

int
run_away2(int idx, string *myexits)
{
    object here;
    int tries;

    if (hit_point < 0) return 0;

    here = environment();
    tries = sizeof(myexits);
    do {
	command(myexits[idx]);
	if (!idx--) idx = sizeof(myexits) - 1;
	if (here != environment()) return 1;
    } while (--tries);
    if (here) here -> tell_here(sprintf("%s looks around in panic,\
 as if not knowing where to escape.", MY_NAME(this_object())), this_object());
    tell_me("You failed to run away!");
    return 0;
}

int
run_away()
{
    string *my_exits;
    mapping exits;
    int dir, x;
    string err;

    /* Skip it if mortally wounded or unable to move */
    if (hit_point < 0 || !query_can_move()) return 0;

    /* We are very brave when drunk :) ++Tron */
    if (!query_verb() && intoxicated > (query_stat(ST_CON) / 2)
      && !random(4))
    {
	tell_me("You think: Haaaa! I can take it!");
	return 0;
    }

    tell_me("You flee in PANIC!");

    /* Players might drop some money or an item fleeing. //Graah */
    if (!is_npc() && random(10) < 4)
	drop_things_in_panic();

    // New: Poor wimps may now flee several times in a row //Graah
    // Hmmh. And cause too_long_eval_cost occasionally?!?!
    // Now max. 2 flees; was 3 earlier. -+ Doomdark +-

    x = 1;
    if (random(10) < 3) x += random(2);

    while (x && environment()) {

	if (!(exits = (mapping) environment()->query_exits())) {
	    my_exits = ({ });
	    dir = random(4);
	} else {
	    dir = random(sizeof(my_exits = m_indices(exits)));
	}

	// No more zillions of exits. If no visible exits exist (nice put, eh?),
	// we should print "can't flee" etc., but let's make a compromise; only
	// 4 main dirs will be tried...

	my_exits += ({
	  "north", "east", "south", "west"
	  // , "up", "down",	"northeast", "southeast", "southwest", "northwest",
	});

	// The first try is from room exits, if any exists...

	set(LIV_IS_FLEEING);
	if (err = catch(dir = run_away2(dir, my_exits)))
	    tell_me(err);

	x--;

    }

    set(LIV_NOT_FLEEING);

    return dir;
}

// Our race has changed: update all race-dependent stats
void
race_update()
{
    if (liv_Flags & F_LIV_IS_PLAYER) {
	if(Int > max_Int)
	    set_stat(ST_INT, max_Int);
	if(Con > max_Con)
	    set_stat(ST_CON, max_Con);
	if(Dex > max_Dex)
	    set_stat(ST_DEX, max_Dex);
	if(Str > max_Str)
	    set_stat(ST_STR, max_Str);
	if(Fatigue > max_Fatigue)
	    set_stat(ST_FAT, max_Fatigue);
	if (hit_point > max_hp) hit_point = max_hp;
	if (spell_points > max_sp) spell_points = max_sp;
    }
}

void
forget_hunted()
{
    if (!hunted || sizeof(hunted) < 5) return;
    if (++forget_hunt > (8 + query_stat(ST_INT) / 2)) {
	forget_hunt = 0;

	if (hunted && sizeof(hunted)) {
	    purge_hunt();
	    if (sizeof(hunted) == 1) hunted = ({});
	    else hunted = hunted[0..<2];
	}
    }
}

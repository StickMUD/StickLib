/****************************************************************
*								*
* Module: attack.c (player-version)				*
* Author: Doomdark						*
* Version: 1.1							*
* Last update: 17-jun-96					*
* Description:							*
*	Contains functions used when we are fighting normally;	*
*	ie. hitting enemy with a weapon.			*
*								*
* CHANGES:							*
*								*
* - Limits for different hit messages changed.			*
* - Support for guild-specific 'normal' hits added.		*
* - Added few new descs for good hits; suggested by Shadow. -DD.*
*								*
/***************************************************************/

#ifndef PLAYER_C

#include <living_defs.h>
#include <room_defs.h>
#include <daemons.h>
#include <tell_me.h>
#include <guild.h>
#include "/basic/living/living_private.h"

inherit "/basic/player/skills";
static int attackers, liv_Flags, weapon_class;
static object *attacker, *hunted, name_of_weapon, left_weapon;
static closure HitFunc, LeftHitFunc, ExtraHitFunc, LeftExtraHitFunc;
static int damage_type;
static int hit_class;
static int GuildFlags;
static mapping GuildHooks;

int level;
int spell_points, experience;

void set(int x);
varargs int tell_me(string s, int tell_type);
void start_hunting(object ob);
void update_attackers(status x);
void stop_fight(object x);
status query_can_move();
int query_wc(status left);
int query_random_luck();
string query_name(int mode, object who);
string Possessive();
string get_name(object x, object y);
void set_sit(status x);
void set_rest(status x);
int query_stat(int stat);
object query_attack();

#endif

// NEW! The 'real' attack-function; is only used by the attack() from
// player.c, but can be called more than once; and will only be called
// if the guild in which player belongs to doesn't override the attack(s).

// Return values:
// >0 -> hitting done, enemy alive, damage done
//  0 -> missed
// -1 -> hitting done, enemy dead
// -2 -> fumbled...

int
do_attack(int hit_amount, int hit_class, int dmg_type, object enemy,
  object wp)
{
    string enemy_n, how, what, what2, txt;
    int tmp, x;
    if (!enemy) return 0;
    enemy_n = (string) enemy->query_name(0, this_object());
    tmp = (int) enemy -> hit_player(random(hit_amount + 1),
      dmg_type, hit_class, this_object());


    if (tmp <= 0) {

	if (!enemy) return -1;	// Shouldn't happen, really...

	// Fumbles are only for players at the moment. //Graah
	// How often should they happen?
	if (random(300) < 3) {
	    FUMBLE_D->fumble(this_object(), enemy, wp);
	    return -2;
	}

	if (random(2)) {
	    x = 0;
	    switch (random(3)) {
	    case 0: txt = "You missed by a mile."; break;
	    case 1: txt = "You missed."; break;
	    case 2:
		switch (random(5)) {
		case 0:
		    txt = "You took your best hit"; break;
		case 1:
		    txt = "You attack valiantly";
		    x = 1; break;
		case 2:
		    txt = "You swing wildly";
		    x = 1; break;
		case 3:
		    txt = "You attack ferociously"; break;
		case 4:
		    txt = "You aim carefully"; break;
		}
		txt = sprintf("%s, but %s", txt, get_name(enemy, this_object()));
		switch (random(5)) {
		case 0:
		    txt += (x ? " ducked." : " ducks."); break;
		case 1:
		    txt += (x ? " stepped aside." : " steps aside."); break;
		case 2:
		    txt += (x ? " danced back." : " dances back."); break;
		case 3:
		    txt += (x ? " dodged." : " dodges."); break;
		case 4:
		    txt += (x ? " ducked." : " ducks."); break;
		}
	    }
	} else {
	    txt = get_name(enemy, this_object());
	    if (!txt) txt = "Someone";
	    if (random(2)) txt += " ignores";
	    else {
		switch (random(3)) {
		case 0: txt += " easily"; break;
		case 1: txt += " narrowly"; break;
		case 2: txt += " barely"; break;
		}
		switch (random(6)) {
		case 0: txt += " escapes"; break;
		case 1: txt += " blocks"; break;
		case 2: txt += " evades"; break;
		case 3: txt += " parries"; break;
		case 4: txt += " dodges"; break;
		case 5: txt += " ducks"; break;
		}
	    }
	    if (random(2)) txt += " your feeble flailings.";
	    else {
		switch (random(3)) {
		case 0: txt += " your clumsy"; break;
		case 1: txt += " your feeble"; break;
		case 2: txt += " your"; break;
		}
		switch (random(5)) {
		case 0: txt += " blow."; break;
		case 1: txt += " swing."; break;
		case 2: txt += " strike."; break;
		case 3: txt += " assault."; break;
		case 4: txt += " attack."; break;
		}
	    }
	}
	tell_me(txt, TELL_TYPE_MISSING);

	// These are still quite boring, should we put new messages here too?
	// //Frobozz

	if (!(txt = query_name(0, enemy))) txt = "Someone";
	if (environment())
	    environment() -> tell_here(sprintf("%s missed %s.",
		txt, enemy_n), ({ enemy, this_object()}));
	if (enemy)
	    enemy -> tell_me(sprintf("%s missed you.", txt), TELL_TYPE_MISSED_BY);
	return 0;
    } else {
	// No negative combat exp, nor zillion points from a zap!
	// Some more exp from hits //Graah  [was: random(1 + tmp)]
	if (tmp > 200) experience += (100 + random(100));
	else experience += (tmp / 2 + random(1 + tmp / 2));
    }

    if (enemy && !enemy -> query_ghost()) {
	if (tmp == 1) {
	    switch (random(6)) {
	    case 0: how = " in the side"; break;
	    case 1: how = " on the head"; break;
	    case 2: how = " in the chest"; break;
	    case 3: how = " in the stomach"; break;
	    case 4: how = " on the arm"; break;
	    default: how = " on the leg"; break;
	    }
	    what = "tickle";
	    what2 = "tickles";
	} else if (tmp < 3) {
	    how = "";
	    what = "graze";
	    what2 = "grazes";
	} else if (tmp < 4) { 
	    switch (random(6)) {
	    case 0: how = " in the side"; break;
	    case 1: how = " on the head"; break;
	    case 2: how = " in the chest"; break;
	    case 3: how = " in the stomach"; break;
	    case 4: how = " on the arm"; break;
	    default: how = " on the leg"; break;
	    }
	    what = "scratch";
	    what2 = "scratches";
	} else if (tmp < 6) {
	    how = " lightly";
	    switch (random(3)) {
	    case 0: what = "hit"; what2 = "hits"; break;
	    case 1: what = "strike"; what2 = "strikes"; break;
	    default: what = "beat"; what2 = "beats";
	    }
	} else if (tmp < 9) {
	    how = "";
	    switch (random(3)) {
	    case 0: what = "hit"; what2 = "hits"; break;
	    case 1: what = "strike"; what2 = "strikes"; break;
	    default: what = "beat"; what2 = "beats";
	    }
	} else if (tmp < 13) {
	    how = " hard";
	    switch (random(3)) {
	    case 0: what = "hit"; what2 = "hits"; break;
	    case 1: what = "strike"; what2 = "strikes"; break;
	    default: what = "beat"; what2 = "beats";
	    }
	} else if (tmp < 20) {
	    how = " very hard";
	    switch (random(3)) {
	    case 0: what = "hit"; what2 = "hits"; break;
	    case 1: what = "strike"; what2 = "strikes"; break;
	    default: what = "beat"; what2 = "beats";
	    }
	} else if (tmp < 27) {
	    how = " extremely hard";
	    switch (random(3)) {
	    case 0: what = "hit"; what2 = "hits"; break;
	    case 1: what = "strike"; what2 = "strikes"; break;
	    default: what = "beat"; what2 = "beats";
	    }
	} else if (tmp < 35) {
	    how = " with a bone crushing sound";
	    what = "smash";
	    what2 = "smashes";
	} else if (tmp < 47) {
	    how = " spraying blood everywhere";
	    what = "slaughter";
	    what2 = "slaughters";
	} else if (tmp < 60) {
	    how = " into tiny fragments";
	    what = "massacre";
	    what2 = "massacres";
	} else {
	    how = " with a colossal strike";
	    what = "utterly obliterate";
	    what2 = "utterly obliterates";
	}
	if (!(txt = get_name(enemy, this_object()))) txt = "Someone";
	tell_me(sprintf("You %s %s%s%s",
	    what, txt,
	    how, (random(2) ? "." : "!")), TELL_TYPE_HITTING);
	if (!(txt = query_name(0, enemy)))
	    txt = "Someone";
	if (enemy) enemy->tell_me(sprintf("%s %s you%s%s",
		txt, what2, how, (random(2) ? "." : "!")), TELL_TYPE_HIT_BY);
	if (environment()) environment() -> tell_here(sprintf("%s %s %s%s%s",
		txt, what2, enemy_n, how,
		(random(2) ? "." : "!")), ({ enemy, this_object() }) );
	return 1;
    }
    if (enemy)
	stop_fight(enemy);
    tell_me(sprintf("You killed %s.", enemy_n));
    return -1;
}

// Return true if there still is a fight.

status
attack()
{
    int tmp, enemy_flags;
    object ob, curr_wp;
    string name_of_attacker;

    if (!attackers) return 0;

    ob = query_attack();
    if (!ob) {
	update_attackers(1);
	if (attackers)
	    ob = attacker[0];
    } else if (!present(ob, environment())) {
	stop_fight(ob);
	if (!hunted) hunted = ({});
	// Start hunting enemy, if he/she/it fled.
	if (member(hunted, ob) < 0) {
	    name_of_attacker = (string) ob->query_name(0,this_object());
	    if (name_of_attacker != "someone" && name_of_attacker != "Someone")
		tell_me(sprintf("You are now hunting %s.", name_of_attacker));
	    start_hunting(ob);
	}
	update_attackers(1);
	if (attackers)
	    ob = attacker[0];
    }

    if (!attackers)
	return 0;

    if (liv_Flags & F_LIV_RESTING)
	set_rest(0);
    if (liv_Flags & F_LIV_SITTING)
	set_sit(0);

    enemy_flags = (int) ob->query(LIV_FLAGS);

    // PK checks:
    if (enemy_flags & F_LIV_IS_PLAYER) {

	// Auto-stop player fight in non-PK area
	if (!(enemy_flags & F_LIV_OK_TO_PK)
	  && environment(ob)->query(NO_PK)) {
	    tell_me("Player killing is not allowed here. Stopped fighting.");
	    set(LIV_NOT_JUST_MOVED);
	    stop_fight(ob);
	    return 0;
	}
    }

    // Check if we are able to attack
    if (!query_can_move()) {
	tell_me("You cannot fight back!");
	return 1;
    }

    // NEW! Guilds can 'mask' the 'normal' attack!
    if (GuildFlags & (1 << G_HOOK_PRIMARY_ATTACK)) {
	tmp = funcall(GuildHooks[G_HOOK_PRIMARY_ATTACK],
	  this_object(), ob, name_of_weapon, left_weapon);
	// Return value of 0 means that 'normal' primary attack should be done instead;
	// -1 means that we won't do primary attack at all (ie. most of the time
	// that indicates that the guild mark handled the attack ok!)
	if (tmp > 0)
	    do_attack(tmp, hit_class, damage_type, ob,
	      name_of_weapon ? name_of_weapon : left_weapon);
    }
    if (!tmp) {
	if (name_of_weapon) {
	    tmp = (mixed) funcall(HitFunc, ob) + query_wc(0);
	    if (!ob) return 1;
	    curr_wp = name_of_weapon;
	    funcall(ExtraHitFunc, ob);
	} else if (left_weapon) {
	    tmp = (mixed) funcall(LeftHitFunc, ob) + query_wc(1);
	    if (!ob) return 1;
	    curr_wp = left_weapon;
	    funcall(LeftExtraHitFunc, ob);
	} else tmp = query_wc(0);
	do_attack(tmp, hit_class, damage_type, ob, curr_wp);
    }

    // Let's return 1, we can check existance of enemies next round.
    if (!ob || !present(ob, environment())) return 1;

    if (GuildFlags & (1 << G_HOOK_SECONDARY_ATTACK)) {
	// Note; if the enemy dies with primary attack, we _won't_ do the secondary
	// attack. This is meant to be logical, so that it "takes 1 round" to change
	// the primary enemy.
	if (!ob)
	    return 0;
	tmp =  funcall(GuildHooks[G_HOOK_SECONDARY_ATTACK],
	  this_object(), ob, name_of_weapon, left_weapon);
	if (tmp >= 0)
	    do_attack(tmp, hit_class, damage_type, ob, left_weapon);
    } else if (name_of_weapon && left_weapon) {
	tmp = (mixed) funcall(LeftHitFunc, ob) + query_wc(1);
	if (!ob) return 1;
	curr_wp = left_weapon;
	funcall(LeftExtraHitFunc, ob);
	do_attack(tmp, hit_class, damage_type, ob, left_weapon);
    }

    return 1;
}

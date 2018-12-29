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

#include <skills_defs.h>
#include "/basic/player/skills.c"
#include <weapon_defs.h>
#include <auto_skills.h>
#ifndef PLAYER_C

#include <living_defs.h>
#include <room_defs.h>
#include <daemons.h>
#include <tell_me.h>
#include <guild.h>
#include "/basic/living/living_private.h"

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
int add_fp(int arg);
int query_fp();

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
/* random is a great function.......when used correctly
	Starks - Jan 19, 1999
	tmp = (int) enemy -> hit_player(random(hit_amount + 1),
*/
	tmp = (int) enemy -> hit_player((3*hit_amount/4) + random((hit_amount/4)+1),
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
#if 0
	 } else {
		how = " to small fragments";
		what = "massacre";
		what2 = "massacres";
#endif
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
int tmp, offhand, enemy_flags;
object ob;
string name_of_attacker, skill;
status sharp, handy;

	if (!attackers) return 0;
	if (this_object()->query_queued_event())	return 0;

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

// New... the 'busy flag'. Is used by guilds to prevent 'normal' hits
// when casting spells etc:

#if 0
// No need; checked at /obj/living.c::heart_beat already! -+ Doomdark +-
	if (funcall(_busy, this_object()))
		return 1;
#endif

// NEW! Guilds can 'mask' the 'normal' attack!
/* Guilds are goin g'bye: we won't need this crap
	Starks - Jan 19, 1999
*/
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
// Things added from here down- Starks, Jan 19, 1999
			sharp = (status)name_of_weapon->query(WP_SHARP);
			handy = (status)name_of_weapon->query(WP_TWO_HANDED);
			handy && sharp && skill = SK_2H_SHARP;
			handy && (sharp || skill = SK_2H_BLUNT);
			handy || (sharp && skill = SK_1H_SHARP);
			handy || sharp || skill = SK_1H_BLUNT;
				if(query_fp() < COST_FOR_ASKILL) {
					tmp = -2; // weary attack
			if(to_int(query_skill(skill)) > SKILL_NO_EXIST)
			tell_me("You're too tired to strike effectively.");
				}
			if(!tmp)
			tmp = to_int(use_skill(skill, SK_TRAIN_X_HARD, 0));
			if(tmp >= SKILL_NO_EXIST) {
				tmp > SKILL_NO_EXIST &&
					tmp *= query_skill_lev(skill);
				if(tmp > SKILL_NO_EXIST)
				add_fp(-COST_FOR_ASKILL);
				tmp += (mixed) funcall(HitFunc, ob) + query_wc(0);
				tmp < 1 && tmp = 1;
// That's the end of the skills addition - Starks
				if (!ob) return 1;
				funcall(ExtraHitFunc, ob);
				do_attack(tmp, hit_class,
				damage_type, ob,
				name_of_weapon);
			}
		}
		if(left_weapon) {
			sharp = (status)left_weapon->query(WP_SHARP);
			handy = (status)left_weapon->query(WP_TWO_HANDED);
			handy && sharp && skill = SK_2H_SHARP;
			handy && (sharp || skill = SK_2H_BLUNT);
			handy || (sharp && skill = SK_1H_SHARP);
			handy || sharp || skill = SK_1H_BLUNT;
			if(query_fp() >= COST_FOR_ASKILL) 
			{
			tmp = to_int(use_skill(skill, SK_TRAIN_X_HARD, 0));
			if(tmp >= SKILL_NO_EXIST) {
				tmp > SKILL_NO_EXIST &&
					tmp *= query_skill_lev(skill);
				tmp += (mixed) funcall(LeftHitFunc, ob) + query_wc(0);
				offhand = to_int(use_skill(
				SK_OFFHAND, SK_TRAIN_X_HARD, 0));
				offhand != SKILL_NO_EXIST &&
				offhand *= query_skill_lev(SK_OFFHAND);
				tmp = (tmp + offhand) / 2;
				tmp < 1 && tmp = 1;
				add_fp(-COST_FOR_ASKILL);
				if (!ob) return 1;
				funcall(LeftExtraHitFunc, ob);
				do_attack(tmp, hit_class,
				damage_type, ob,
				left_weapon);
			}
			}
			else tell_me(
			"You are too tired to swing your offhand weapon.");
		}
		if(!tmp) {
			if(query_fp() < COST_FOR_ASKILL) {
			tmp = -2;
			if(to_int(query_skill(SK_UNARMED)) > SKILL_NO_EXIST)
		tell_me("You are too tired to strike effectively.");
			}
			else
			tmp = to_int(use_skill(
			SK_UNARMED, SK_TRAIN_X_HARD, 0));
			if(tmp >= SKILL_NO_EXIST) {
				tmp > SKILL_NO_EXIST &&
				tmp *= query_skill_lev(SK_UNARMED);
				if(tmp > SKILL_NO_EXIST)
				add_fp(-COST_FOR_ASKILL);
				tmp += query_wc(0);
				do_attack(tmp, hit_class, damage_type, ob, 0);
			}
		}
	}

/* Nope, we've been handled above, thankyou very much- Starks, Jan 19, 1999
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

*/
	return 1;
}

/***************************************************************************

			End of useful code...

***************************************************************************/

// ANCIENT: (spells that were once in-built to living.c!)

#if 0
// We are casting a spell!!!!
	if (spell_cost) {
	  spell_points -= spell_cost;

// New spell messages (shock and fireball provided by Dolandar)
// 28.8.1992 //Frobozz

	  my_name = query_name(0, ob);
	  switch(spell_name) {
	    case "magic missile":
		ob->tell_me(sprintf("%s points at you and a magic\
 missile hits you exploding.", my_name));
		tell_me(sprintf("You point your finger and a magic missile\
 explodes at %s.", name_of_attacker));
		environment() -> tell_here(sprintf("%s points at %s and a magic\
 missile hits %s exploding.",
my_name, name_of_attacker, (string)ob->Objective()),
			({ this_object(),ob }));
		break;
	    case "shock":
		ob->tell_me(sprintf("%s points at you and you feel icy\
 fingers grasping at your very heart.", my_name));
		tell_me(sprintf("You say the magical words and point at %s\
 who recoils in pain.", name_of_attacker));
		environment() -> tell_here(sprintf("%s utters a word and\
 points at %s who recoils in pain.", my_name, name_of_attacker),
		({ this_object(),ob }));
		break;
	    case "fire ball":
		ob->tell_me(sprintf("%s points at you and mutters\
 something. You are engulfed in searing hot flames.", my_name));
		tell_me(sprintf("You point at %s and invoke magical words.\
 Huge ball of fire explodes around %s.",
		name_of_attacker, (string)ob->Objective()));
	environment() -> tell_here(sprintf("%s points %s finger and a\
 ball of fire explodes around %s.", my_name, Possessive(), name_of_attacker),
		({ this_object(),ob }));
		break;
	  default:

// These were the old descs, not much imagination, they are here in case
// someone uses initiate_spell() //Frobozz

		ob->tell_me(sprintf("You are hit by a %s.", spell_name));
		tell_me(sprintf("You cast a %s.", spell_name));
		environment() -> tell_here(sprintf("%s casts a %s at %s.",
		my_name, spell_name, name_of_attacker),
		({ this_object(),ob }));
		break;
	  }
	}
#endif

#if 0
// And still ancient stuff; check for breaking the weapon sometimes:

	if ((liv_Flags & F_LIV_IS_PLAYER) && name_of_weapon && tmp > 20
	&& random(100) < (weapon_class - level * 2 / 3 - 14 - query_random_luck()))
	{
		tell_me(
"You fumble with your great weapon. CRACK! You managed to smash it\
 into hundreds of small pieces. Nice.");
		log_file("BAD_SWORD", sprintf("%s, %s, %s\n",
		name_of_weapon->query_short(), creator(name_of_weapon),
		object_name(name_of_weapon)));
		destruct(name_of_weapon);
		weapon_class = 3;
		return 1;
	}


#endif

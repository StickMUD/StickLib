/****************************************************************
*								*
* Module: attack.c (NPC-version)				*
* Author: Doomdark						*
* Version: 1.00							*
* Last update: 17-jun-96					*
* Description:							*
*	Contains functions used when we are fighting normally;	*
*	ie. hitting enemy with a weapon.			*
*								*
* CHANGES:							*
*								*
* - Monsters won't get any messages, unless flag NPC_CONTOLLED	*
*   is set.							*
* - Limits for different hit messages changed.			*
* - Updated descs; same as with player-version.			*
* - Moved combat message creation to COMBAT_D, saves A LOT of	*
*   memory!!! //Graah 26-Jun-96					*
* - Check, if our weapon_class should be used instead of	*
*   weapon's is now done in query_wc() rather than here.	*
*   //Chopin 17-dec-97						*
*								*
/***************************************************************/

#ifndef NPC_C

#include <living_defs.h>
#include <room_defs.h>
#include <daemons.h>
#include <tell_me.h>
#include "/basic/living/living_private.h"
#include "/basic/npc/npc_private.h"

static int attackers, liv_Flags, npc_Flags, weapon_class;
static object *attacker, *hunted, name_of_weapon;
int spell_points, experience;
static closure HitFunc, ExtraHitFunc;
static int damage_type;
static int hit_class;
int level;

void set(int x);
varargs int tell_me(string s, int tell_type);
void start_hunting(object ob);
void update_attackers(status x);
void stop_fight(object x);
status query_can_move();
int query_wc();
string query_name(int mode, object who);
string Possessive();
string get_name(object x, object y);
void set_sit(status x);
void set_rest(status x);
int query_stat(int stat);
object query_attack();

#endif

// Return true if there is still a fight.

status
attack()
{
    int tmp, enemy_flags, x;
    mixed whit;
    object ob;
    string name_of_attacker, my_name, txt;
    status do_tell;

    if (!attackers)
	return 0;

    do_tell = npc_Flags & F_NPC_CONTROLLED;

    ob = query_attack();
    if (!ob) {
	update_attackers(1);
	if (attackers) ob = attacker[0];
    } else if (!present(ob, environment())) {
	stop_fight(ob);
	if (!hunted) hunted = ({});
	// Start hunting enemy, if he/she/it fled.
	name_of_attacker = (string) ob->query_name(0,this_object());
	if (member(hunted, ob) < 0) {
	    if (do_tell && name_of_attacker != "Someone" &&
	      name_of_attacker != "someone")
		tell_me(sprintf("You are now hunting %s.", name_of_attacker));
	    start_hunting(ob);
	}

	update_attackers(1);
	if (attackers) ob = attacker[0];
    }

    if (!attackers) return 0;

    name_of_attacker = (string) ob->query_name(0, this_object());

    if (liv_Flags & F_LIV_RESTING)
	set_rest(0);
    if (liv_Flags & F_LIV_SITTING)
	set_sit(0);

    enemy_flags = (int) ob->query(LIV_FLAGS);

    // Check if we are able to attack
    if (!query_can_move()) {
	tell_me("You cannot fight back!");
	return 1;
    }

    my_name = query_name(0, ob);

    if (name_of_weapon) {
	whit = (mixed) funcall(HitFunc, ob);
	if (!ob) return 1;
    }

    if ((string) whit != "miss") {

	if (name_of_weapon && ob) {
	    funcall(ExtraHitFunc, ob);
	}

	if (!ob) return 1;

	// Str & dex is modified by weapon.c now...
	if((x = whit + query_wc()) < 1)
	    x = 1;

	// New: Hit Class (accuracy) of the weapon considered
	//
	// Attack type (2nd argument) is currently 0 */

	tmp = (int) ob -> hit_player(random(x + 1),
	  damage_type, hit_class, this_object());
    } else tmp = x = 0;

    if (tmp <= 0) {

	// Try to prevent 'You missed' after the foe is dead
	// Added new ways to "miss" (told to player)
	// 25.2.1992 //Frobozz&&Elena
	// And again some new ways //Frobozz 29.8.1993

	if (ob) {

	    if (do_tell)
		tell_me(sprintf("You miss %s.", name_of_attacker), TELL_TYPE_MISSING);

	    // These are still quite boring, should we put new messages here too?
	    // //Frobozz

	    if (!(txt = query_name(0, ob))) txt = "Someone";
	    if (environment())
		environment() -> tell_here(sprintf("%s missed %s.",
		    txt, name_of_attacker), ({ob,this_object()}));
	    if (ob)
		ob -> tell_me(sprintf("%s missed you.", txt), TELL_TYPE_MISSED_BY);
	} 
	return 1;
    }

    if (ob && !ob->query_ghost())
    {

	if (do_tell)
	    tell_me(sprintf("You hit %s (damage %d, of which %d taken).",
		name_of_attacker, x, tmp), TELL_TYPE_HITTING);

	COMBAT_D->npc_hit_desc(this_object(), ob, tmp);

	return 1;
    }

    stop_fight(ob);
    tell_me(sprintf("You killed %s.", name_of_attacker));

    // Let's return 1, we'll still check enemies next round.
    return 1;
}

// Basic module that contains just 2 functions; set() and query().
// This is to be included/inherited by standard living objects;
// namely, NPCs (/lib/npc.c) and players (/obj/player.c).

// Version 1.2c 9-May-98
//
// - Now relies on "off"-values to be negated "on"-values
// (ie. for example that NPC_NOT_AGGRESSIVE == - NPC_AGGRESSIVE
// - Internal flags defined in npc_private.h; only to be used by
//   mudlib modules (other than npc.c too!)
//
// 9-May-98 / Graah: New flag NPC_NO_CORPSE, npc won't leave a corpse.

static int npc_Flags;
static object possessed_by;
int healRate;
static mixed move_area;
static mixed *follow_data;
static status set_pk_flag, no_corpse;

#include "npc_private.h"

#ifndef	NPC_C
#include <npc_defs.h>

int time_to_heal;
static int liv_Flags;
static int damage_type;
static int attackers;
static object *attacker;

int query_stat(int x);
int query_race_stat(int x);
void update_attackers();

int local_weight;
#endif

mixed
query(int x)
{
    status n;
    if (x < 0) {
	x = -x;
    } else {
	n = 1;
    }
    if (x < _FIRST_NPC_VALUE)
	switch(x) {
    case LIV_IS_NPC:
	return n ? 1 : 0;
    case LIV_IS_PLAYER_OR_NPC:
	return 1;
    case LIV_UNDEAD:
	return n ? (liv_Flags & F_LIV_UNDEAD) :
	!(liv_Flags & F_LIV_UNDEAD);
    case LIV_JUST_MOVED:
	return n ? (liv_Flags & F_LIV_JUST_MOVED) :
	!(liv_Flags & F_LIV_JUST_MOVED);
    case LIV_IS_FLEEING:
	return n ? (liv_Flags & F_LIV_IS_FLEEING) :
	!(liv_Flags & F_LIV_IS_FLEEING);
    case LIV_OK_TO_PK:
	return 1;
    case LIV_NIGHT:
	return liv_Flags & F_LIV_NIGHT;
    case LIV_MAX_CARRY:
	return (query_race_stat(RACE_CARRY_RATE) *
	  query_stat(ST_STR) + 10) / 111;
    case LIV_CARRY_RATE:
	return (local_weight * 100) / (query_race_stat(RACE_CARRY_RATE)
	  * query_stat(ST_STR) + 10) / 111;
    case LIV_HEAL_RATE:
	return healRate;
    case LIV_DAMAGE_TYPE:
	return damage_type;
    case LIV_ATTACKERS:
	return attacker;
    case LIV_REAL_ATTACKERS:
	update_attackers();
	return attacker[0..attackers-1];
    case LIV_WC_FORMULA:
	// Dummy formula for NPCs... should we use the 'real' one??
	return lambda(({ 'x }), ({ #'return, 'x }) );
    }
    else
	switch (x) {
    case CREATE_ORIGINAL:
	return n ? (npc_Flags & F_NPC_CREATE_ORIGINAL) :
	!(npc_Flags & F_NPC_CREATE_ORIGINAL);
    case NPC_AGGRESSIVE:
	return n ? (npc_Flags & F_NPC_AGGRESSIVE) :
	!(npc_Flags & F_NPC_AGGRESSIVE);
    case NPC_MONS_AGGRESSIVE:
	return n ? (npc_Flags & F_NPC_MONS_AGGRESSIVE) :
	!(npc_Flags & F_NPC_MONS_AGGRESSIVE);
    case NPC_NO_CONDITION:
	return n ? (npc_Flags & F_NPC_NO_CONDITION) :
	!(npc_Flags & F_NPC_NO_CONDITION);
    case NPC_MOVE_AT_RESET:
	return n ? (npc_Flags & F_NPC_MOVE_AT_RESET) :
	!(npc_Flags & F_NPC_MOVE_AT_RESET);
    case NPC_CONTROLLED:
	return npc_Flags & F_NPC_CONTROLLED;
    case NPC_FOLLOW_ATTACKER:
	return follow_data;
    case NPC_POSSESSED_BY:
	return possessed_by;
    case NPC_SET_PK_FLAG:
	return set_pk_flag;
    case NPC_NO_CORPSE:
	return no_corpse;
    case NPC_MOVE_AREA:
	return move_area;
    }
    if (x == NPC_FLAGS) return npc_Flags;
    if (x == LIV_FLAGS) return liv_Flags;
    return 0;
}

varargs int
set(int x, mixed value)
{
    status n;
    if (x < 0) {
	x = -x;
    } else {
	n = 1;
    }
    if (x < _FIRST_NPC_VALUE)
	switch (x) {
    case LIV_UNDEAD:
	liv_Flags = n ? (liv_Flags | F_LIV_UNDEAD) :
	(liv_Flags & (~F_LIV_UNDEAD));
	break;
    case LIV_JUST_MOVED:
	liv_Flags = n ? (liv_Flags | F_LIV_JUST_MOVED) :
	(liv_Flags & (~F_LIV_JUST_MOVED));
	break;
    case LIV_IS_FLEEING:
	liv_Flags = n ? (liv_Flags | F_LIV_IS_FLEEING) :
	(liv_Flags & (~F_LIV_IS_FLEEING));
	break;
    case LIV_NIGHT:
	if (!value) liv_Flags &= (~F_LIV_NIGHT);
	else liv_Flags |= F_LIV_NIGHT;
	break;
    case LIV_HEAL_RATE:
	if (intp(value)) {
	    healRate = value;
	    if (value < 0) time_to_heal = value;
	    else time_to_heal = 0;
	} else return 0;
	break;
    case LIV_DAMAGE_TYPE:
	if (intp(value)) damage_type = value;
	break;
    default:
	return 0;
    } else
	switch (x) {
    case CREATE_ORIGINAL:
	npc_Flags = n ? (npc_Flags | F_NPC_CREATE_ORIGINAL) :
	(npc_Flags & (~F_NPC_CREATE_ORIGINAL));
	break;
    case NPC_AGGRESSIVE:
	npc_Flags = n ? (npc_Flags | F_NPC_AGGRESSIVE) :
	(npc_Flags & (~F_NPC_AGGRESSIVE));
	break;
    case NPC_MONS_AGGRESSIVE:
	npc_Flags = n ? (npc_Flags | F_NPC_MONS_AGGRESSIVE) :
	(npc_Flags & (~F_NPC_MONS_AGGRESSIVE));
	break;
    case NPC_NO_CONDITION:
	npc_Flags = n ? (npc_Flags | F_NPC_NO_CONDITION) :
	(npc_Flags & (~F_NPC_NO_CONDITION));
	break;
    case NPC_MOVE_AT_RESET:
	npc_Flags = n ? (npc_Flags | F_NPC_MOVE_AT_RESET) :
	(npc_Flags & (~F_NPC_MOVE_AT_RESET));
	break;
    case NPC_CONTROLLED:
	npc_Flags = (value ? (npc_Flags | F_NPC_CONTROLLED) :
	  (npc_Flags & (~F_NPC_CONTROLLED)));
	// Let's use 'standard base data', if we use this old way of following:
	// (delay = 1 sec, max. follow path = 1 moves, max. follow distance = 1
    case NPC_FOLLOW_ATTACKER:
	follow_data = ({ 0, 1, 0, 1, 1, 0, 0, 0 });
	break;
    case NPC_POSSESSED_BY:
	npc_Flags |= F_NPC_CATCH_TELL_USED;
	possessed_by = value; break;
    case NPC_SET_PK_FLAG:
	set_pk_flag = value; break;
    case NPC_NO_CORPSE:
	no_corpse = value; break;
    case NPC_MOVE_AREA:
	move_area = value; break;
    default:
	return 0;
    }
    return 1;
}

// Basic module that contains just 2 functions; set() and query().
// This is to be included/inherited by standard living objects;
// namely, NPCs (/lib/npc.c) and players (/obj/player.c).

// Version 1.2d 10-mar-96
//
// - Now relies on "off"-values to be negated "on"-values
// (ie. for example that PL_INFO_OFF == - PLR_INFO_ON
// - Now uses player_private.h; defines flag so other mudlib modules
//   can use them. Dirty, but efficient way. :-)
// - New closure; returned with query(LIV_WC_FORMULA)

#include "/basic/player/player_private.h"

int plr_Flags;		// Saveable player flags.
static int plr_Flags2;	// Non-saveable player flags.
int Version;		// Version of player.c; currently 0 (22-nov-95)
	// Used for compatibility; when recoding stats etc.

static int login_time;
static int total_connect_time;
static int time_to_save;
static int damage_type;
int used_whimpy;

static closure *_combat_queue;
			// Queue of things to be done on heart_beat... *grin*

// A threshold constant to add efficiency...
#define _FIRST_PLAYER_VALUE	2000
// #define	TIME_TO_STAY_DIRTY	150
// Means we'll be PK:able for 150 hbs == 5 minutes after first pk-hit.
#define TIME_TO_STAY_DIRTY (120 + this_object()->query_level() * 2)

#ifndef	PLAYER_C
#include <player_defs.h>
#include <room_defs.h>

int time_to_heal;	// Count-down variable. Really should be a static one,
			// but that could allow cheats... :-/ (probably?)
static int liv_Flags;
static int dirty_stamp;
static int attackers;
static object *attacker;
int age;
static int local_weight;
static int infravision;

int query_stat(int x);
int query_race_stat(int x);
void update_attackers();
void tell_me(string s);
#endif

nomask mixed
query(int x)
{
mixed n;
	if (!intp(x)) return 0;
	if (x < 0) {
		x = -x;
	} else {
		n = 1;
	}
	if (x < _FIRST_PLAYER_VALUE)
	switch(x) {
// First, the values common for all living objects:
	case LIV_IS_NPC:
		return n ? 0 : 1;
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
	case LIV_DEATH_MARK:
		return n ? (plr_Flags & F_PLR_DEATH_MARK) :
			!(plr_Flags & F_PLR_DEATH_MARK);
	case LIV_BRIEF:
		return n ? (plr_Flags & F_PLR_BRIEF) :
			!(plr_Flags & F_PLR_BRIEF);
	case LIV_BRIEF_WEATHER:
		return n ? (plr_Flags & F_PLR_BRIEF_WEATHER) :
			!(plr_Flags & F_PLR_BRIEF_WEATHER);
	case LIV_MAX_CARRY:
		return (query_race_stat(RACE_CARRY_RATE) * (query_stat(ST_STR)
			+ 10)) / 111;
	case LIV_OK_TO_PK:
		return dirty_stamp;
	case LIV_NIGHT:
		return liv_Flags & F_LIV_NIGHT;
	case LIV_CARRY_RATE:
		return (100 * local_weight) / (query_race_stat(RACE_CARRY_RATE)
			* (query_stat(ST_STR) + 10) / 111);
	case LIV_DAMAGE_TYPE:
		return damage_type;
	case LIV_ATTACKERS:
		return attacker;
	case LIV_REAL_ATTACKERS:
		update_attackers();
		return attacker[0..attackers - 1];
	case LIV_INFRAVISION:
		return infravision;
	case LIV_WC_FORMULA:
// This formula is: ret_val = ((str + 20) * str_fct + (dex + 20) * dex_fct)
//			* base_wc / (35 * 100);
// In effect, it returns weighted wc, weighted by its arguments.
		return lambda(({ 'wc, 'str_f, 'dex_f }),
		 ({ #'/,
		  ({ #'*,
		   ({ #'+,
		    ({ #'*, ({ #'+, ({ #'query_stat, ST_STR }), 20 }), 'str_f }),
		    ({ #'*, ({ #'+, ({ #'query_stat, ST_DEX }), 20 }), 'dex_f })
		   }),
		   'wc
		  }),
		  3500
		 })
		);
        } else
        switch (x) {
// Then, values specific for players, non-saveable ones:
	case PLR_BUSY_KILL_ON:
		return n ? (plr_Flags2 & F_PLR_BUSY_KILL) :
			!(plr_Flags2 & F_PLR_BUSY_KILL);
	case PLR_ALLOW_SUMMON:
		return n ? (plr_Flags2 & F_PLR_ALLOW_SUMMON) :
			!(plr_Flags2 & F_PLR_ALLOW_SUMMON);
	case PLR_WIZ_ON:
		return n ? (plr_Flags2 & F_PLR_WIZ):
			!(plr_Flags2 & F_PLR_WIZ);
	case PLR_FIZ_ON:
		return n ? (plr_Flags2 & F_PLR_FIZ) :
			!(plr_Flags2 & F_PLR_FIZ);
	case PLR_LICH_ON:
		return n ? (plr_Flags2 & F_PLR_LICH) :
			!(plr_Flags2 & F_PLR_LICH);
// Someone forgot this:
	case PLR_AWIZ_ON:
		return n ? (plr_Flags2 & F_PLR_AWIZ) :
			!(plr_Flags2 & F_PLR_AWIZ);
// And finally, saveable values.
	case PLR_INFO_ON:
		return n ? (plr_Flags & F_PLR_INFO_ON) :
			!(plr_Flags & F_PLR_INFO_ON);
	case PLR_TIME_TO_HEAL:
		return time_to_heal;
	case PLR_HPSP_REPORT_ON:
		return n ? (plr_Flags & F_PLR_HPSP) :
			!(plr_Flags & F_PLR_HPSP);
	case PLR_LOGIN_TIME:
		return login_time;
	case PLR_USED_WIMPY:
		return used_whimpy;
	case PLR_TIME_TO_SAVE:
		return time_to_save - age;
	case PLR_VERSION:
		return Version;
	case PLR_CONNECT_TIME:
		return total_connect_time + time() - login_time;
	case PLR_COMBAT_QUEUE:
		if (!sizeof(_combat_queue))
			return 0;
		n = _combat_queue[0];
		_combat_queue = _combat_queue[1..];
		return n;
	case PLR_FULL_COMBAT_QUEUE:
		return _combat_queue;
	}
	if (x == LIV_FLAGS) return liv_Flags;
	if (x == PLR_FLAGS) return plr_Flags2;
	if (x == PLR_SAVE_FLAGS) return plr_Flags;
	return 0;
}

varargs nomask int
set(int x, mixed value) // mixed for _combat_queue ??
{
status n;
	if (!intp(x)) return 0;
	if (x < 0) {
		x = -x;
	} else {
		n = 1;
	}
	if (x < _FIRST_PLAYER_VALUE)
        switch (x) {
// First, the values common for all living objects:
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
	case LIV_DEATH_MARK:
		plr_Flags = n ? (plr_Flags | F_PLR_DEATH_MARK) :
			(plr_Flags & (~F_PLR_DEATH_MARK));
		break;
	case LIV_BRIEF:
		plr_Flags = n ? (plr_Flags | F_PLR_BRIEF) :
			(plr_Flags & (~F_PLR_BRIEF));
		break;
	case LIV_BRIEF_WEATHER:
		plr_Flags = n ? (plr_Flags | F_PLR_BRIEF_WEATHER) :
			(plr_Flags & (~F_PLR_BRIEF_WEATHER));
		break;
	case LIV_DAMAGE_TYPE:
		if (intp(value)) damage_type = value;
		break;
	case LIV_OK_TO_PK:
		liv_Flags |= F_LIV_OK_TO_PK;
		if (!dirty_stamp && !value) tell_me(
"You can now be attacked by another player everywhere, for the next 5 minutes!");
// the police can give their own msg for this in their command:
		if(value && value > TIME_TO_STAY_DIRTY) dirty_stamp = age + value;
		else dirty_stamp = age + TIME_TO_STAY_DIRTY;
		break;
	case LIV_NIGHT:
		if (!value) liv_Flags &= (~F_LIV_NIGHT);
		else liv_Flags |= F_LIV_NIGHT;
		break;
        default:
                return 0;
	}
        else
        switch (x) {
// Then, values specific for players, non-saveable ones:
	case PLR_BUSY_KILL_ON:
		plr_Flags2 = n ? (plr_Flags2 | F_PLR_BUSY_KILL) :
			(plr_Flags2 & (~F_PLR_BUSY_KILL));
		break;
	case PLR_ALLOW_SUMMON:
		plr_Flags2 = n ? (plr_Flags2 | F_PLR_ALLOW_SUMMON) :
			(plr_Flags2 & (~F_PLR_ALLOW_SUMMON));
		break;
	case PLR_WIZ_ON:
		plr_Flags2 = n ? (plr_Flags2 | F_PLR_WIZ) :
			(plr_Flags2 & (~F_PLR_WIZ));
		break;
	case PLR_FIZ_ON:
		plr_Flags2 = n ? (plr_Flags2 | F_PLR_FIZ) :
			(plr_Flags2 & (~F_PLR_FIZ));
		break;
	case PLR_LICH_ON:
		plr_Flags2 = n ? (plr_Flags2 | F_PLR_LICH) :
		(plr_Flags2 & (~F_PLR_LICH));
		break;
// Someone forgot this:
	case PLR_AWIZ_ON:
		plr_Flags2 = n ? (plr_Flags2 | F_PLR_AWIZ) :
		(plr_Flags2 & (~F_PLR_AWIZ));
		break;
// And finally, saveable values.
	case PLR_INFO_ON:
		plr_Flags = n ? (plr_Flags | F_PLR_INFO_ON) :
			(plr_Flags & (~F_PLR_INFO_ON));
		break;
	case PLR_HPSP_REPORT_ON:
		plr_Flags = n ? (plr_Flags | F_PLR_HPSP) :
			(plr_Flags & (~F_PLR_HPSP));
		break;
	case PLR_USED_WIMPY:
		used_whimpy = value; break;
	case PLR_LOGIN_TIME:
		login_time = value; break;
	case PLR_TIME_TO_SAVE:
		time_to_save = value; break;
	case PLR_CONNECT_TIME:
		total_connect_time = value; break;
	case PLR_COMBAT_QUEUE:
		if (!_combat_queue)
			_combat_queue = ({ value });
		else
			_combat_queue += ({ value });
		return sizeof(_combat_queue);
	case PLR_COMBAT_QUEUE_OPTIONAL:
		if (!sizeof(_combat_queue))
			return 0;
		_combat_queue += ({ value });
		return sizeof(_combat_queue);
	case PLR_COMBAT_QUEUE_CLEAR:
		if (sizeof(_combat_queue) > 1) {
			n = sizeof(_combat_queue) - 1;
			_combat_queue = _combat_queue[0..0];
			return n;
		}
		return 0;
	default:
		return 0;
  	}
	return 1;
}

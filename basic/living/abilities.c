/****************************************************************
*								*
* Module: abilities.c						*
* Description: Contains code handling statistics like "strength"*
*	"intelligence" etc., commonly referred to as "stats" or	*
*	abilities.						*
*	Also, now contains other boostable stats now, like	*
*	hp/sp/fp heal rates.					*
* Version: 1.3							*
* Date: 01-mar-96, Doomdark.					*
*								*
* Changes:							*
* - Patched is_npc() to liv_Flags & F_LIV_IS_NPC. 14-nov-95 DD	*
* - Added 3 new stats; hp/sp/fp heal rate to allow boosts.	*
*	-+ 01-mar-96 Doomdark +-				*
*								*
****************************************************************/

int Str, Int, Con, Dex, Fatigue;	// Current stats (may be drained)
int max_Str, max_Int, max_Con, max_Dex, max_Fatigue;	// Permanent stats

int statFlags;		// Bitfield telling which 'boosts' we have:
mapping statBoosts;	// Mapping holding info about 'boosts':
static int nextStatCheck; // Timer; used for efficiency. Tells when to
// call update_stats next time.

// Let's define this so we can start using variable directly, if necessary

#ifndef	AGE
#define	AGE	query_age()
#endif

#ifndef LIVING_C

#include "/basic/living/living_private.h"
#include <race.h>
#include <stats.h>

static int max_sp, max_hp, max_fp;
int hit_point, spell_points, fatigue_points;
static int liv_Flags;

int query_race_stat(int x);
void tell_me(string s);
int query_age();
#endif

// We'll define these to allow handling bitfields more easily.
#define	_STAT_VALUE		0
#define	_STAT_TIMED		8
#define	_STAT_OBJECT_BASED	16

#define	_NR_OF_STATS	8

int
set_stat(int st, int value)
{
    if (st == ST_CLEAR_ALL) {
	statFlags = nextStatCheck = statBoosts = 0;
	return 1;
    }
    if (value < 1) {
	// Kludge because fatigue can be 0. -+ Doomdark +-
	if ((st == ST_FAT || st == ST_BASE_FAT) && !value) ;
	else value = 1;
    }
    else if (!(liv_Flags & F_LIV_IS_NPC) && (value > MAX_STAT))
	value = MAX_STAT;

    switch (st) {
    case ST_STR:
	if (liv_Flags & F_LIV_IS_NPC)
	    Str = max_Str = value;
	else
	    Str = value;
	max_fp = FPS(Str,Con);
	return Str;
    case ST_INT:
	if (liv_Flags & F_LIV_IS_NPC)
	    Int = max_Int = value;
	else
	    Int = value;
	max_sp = SPS(Int);
	return Int;
    case ST_CON:
	if (liv_Flags & F_LIV_IS_NPC)
	    Con = max_Con= value;
	else
	    Con = value;
	max_hp = HPS(Con);
	max_fp = FPS(Str,Con);
	return Con;
    case ST_DEX:
	if (liv_Flags & F_LIV_IS_NPC)
	    Dex = max_Dex = value;
	else
	    Dex = value;
	return Dex;
    case ST_FAT:
	if (liv_Flags & F_LIV_IS_NPC)  // Fatigue is based on Str and Con
	    Fatigue = max_Fatigue = value; // Sumppen 25.06.96
	else
	    Fatigue = value;
	/*
	    max_fp = 60 + Fatigue * query_race_stat(RACE_FP_RATE);
	*/
	return Fatigue;
    case ST_BASE_STR:
	return (max_Str = value);
    case ST_BASE_INT:
	return (max_Int = value);
    case ST_BASE_CON:
	return (max_Con = value);
    case ST_BASE_DEX:
	return (max_Dex = value);
	/*  case ST_BASE_FAT:
	    return (max_Fatigue = value); */
    }

    return 0;
}

// Let's make this bit more efficient (I hope)
mixed
query_stat(int st)
{
    if (st < 0) {
	switch (st) {
	case ST_ALL_STATS:
	    return ([
	      ST_STR  : Str,
	      ST_CON  : Con,
	      ST_DEX  : Dex,
	      ST_INT  : Int,
	      ST_FAT  : Fatigue,
	      ST_BASE_STR  : max_Str,
	      ST_BASE_CON  : max_Con,
	      ST_BASE_DEX  : max_Dex,
	      ST_BASE_INT  : max_Int,
	      ST_BASE_FAT  : max_Fatigue
	    ]);
	case ST_STAT_BOOSTS: return statBoosts;
	case ST_DEBUG_DATA:	return statFlags;
	}
	return 0;
    }
    if (st & ST_DRAINED) {
	st ^= ST_DRAINED;
	switch (st) {
	case ST_STR:	return Str;
	case ST_INT:	return Int;
	case ST_CON:	return Con;
	case ST_DEX:	return Dex;
	case ST_FAT:	return Fatigue;
	}
	return 0;
    }
    switch (st) {
    case ST_STR:
	if (statFlags & (1 << (ST_STR + _STAT_VALUE)))
	    return Str + statBoosts[ST_STR + _STAT_VALUE];
	return Str;
    case ST_INT:
	if (statFlags & (1 << (ST_INT + _STAT_VALUE)))
	    return Int + statBoosts[ST_INT + _STAT_VALUE];
	return Int;
    case ST_CON:
	if (statFlags & (1 << (ST_CON + _STAT_VALUE)))
	    return Con + statBoosts[ST_CON + _STAT_VALUE];
	return Con;
    case ST_DEX:
	if (statFlags & (1 << (ST_DEX + _STAT_VALUE)))
	    return Dex + statBoosts[ST_DEX + _STAT_VALUE];
	return Dex;
    case ST_FAT:
	if (statFlags & (1 << (ST_FAT + _STAT_VALUE)))
	    return Fatigue + statBoosts[ST_FAT + _STAT_VALUE];
	return Fatigue;
    case ST_HP_HEAL_RATE:
	if (statFlags & (1 << (ST_HP_HEAL_RATE + _STAT_VALUE)))
	    return Fatigue + statBoosts[ST_HP_HEAL_RATE + _STAT_VALUE];
	return 0;
    case ST_SP_HEAL_RATE:
	if (statFlags & (1 << (ST_SP_HEAL_RATE + _STAT_VALUE)))
	    return Fatigue + statBoosts[ST_SP_HEAL_RATE + _STAT_VALUE];
	return 0;
    case ST_FP_HEAL_RATE:
	if (statFlags & (1 << (ST_FP_HEAL_RATE + _STAT_VALUE)))
	    return Fatigue + statBoosts[ST_FP_HEAL_RATE + _STAT_VALUE];
	return 0;
    case ST_BASE_STR:	return max_Str;
    case ST_BASE_INT:	return max_Int;
    case ST_BASE_CON:	return max_Con;
    case ST_BASE_DEX:	return max_Dex;
    case ST_BASE_FAT:	return max_Fatigue;
    }
}

// With this we can use both object based, and timed, stat boosts/drains.

// Basically we'll have 4 types of calls:

// a) Timed drains/boosts; will be added to the list of timed d/bs. In
//    this case, item = 0 and duration > 0.
// b) Adding an object-based drain/boosts. Item != 0, duration = -1
// c) Removing - "" -. Item != 0, duration = 0.
// d) Adding/Subtracting stat permanently; in this case item = 0 and
//    duration -1. For compatibility, duration may be 0 too...
//    (ie. only 2 arguments).

varargs int
add_stat(int stat, int delta, int duration, object item)
{
    int i, x, y;
    mixed *timed;

    if (!statBoosts) statBoosts = ([ ]);	// Let's initialize it now.
    // First let's check case d):
    // Note that if we change the base value, we'll also have to modify
    // the current value.
    // Let's use standard set functions, so we can modify sp/hp values too,
    // if needed.
    if (!item) {
	if (duration <= 0) {
	    switch (stat) {
	    case ST_STR:
		return set_stat(ST_STR, Str + delta); 
	    case ST_INT:
		return set_stat(ST_INT, Int + delta); 
	    case ST_CON:
		return set_stat(ST_CON, Con + delta); 
	    case ST_DEX:
		return set_stat(ST_DEX, Dex + delta); 
	    case ST_FAT:
		return set_stat(ST_FAT, Fatigue + delta);
	    case ST_BASE_STR:
		set_stat(ST_BASE_STR, max_Str + delta);
		return set_stat(ST_STR, Str + delta); 
	    case ST_BASE_INT:
		set_stat(ST_BASE_INT, max_Int + delta);
		return set_stat(ST_INT, Int + delta);  
	    case ST_BASE_CON:
		set_stat(ST_BASE_CON, max_Con + delta); 
		return set_stat(ST_CON, Con + delta); 
	    case ST_BASE_DEX:
		set_stat(ST_BASE_DEX, max_Dex + delta); 
		return set_stat(ST_DEX, Dex + delta); 
	    case ST_BASE_FAT:
		set_stat(ST_BASE_FAT, max_Fatigue + delta); 
		return set_stat(ST_FAT, Fatigue + delta); 
	    }
	    return 0;

	    // Next, let's check case a) (timed stat boost/drain):

	} else {
	    if (stat < ST_FIRST_STAT || stat > ST_LAST_STAT || duration < 1)
		return 0;
	    // Can only drain boost current value; not base ('real') value. Also,
	    // duration has to be over 0...
	    duration += AGE;
	    if (!(statFlags & (1 << (stat + _STAT_TIMED)))
	      || !(timed = statBoosts[stat + _STAT_TIMED])) {
		// Didn't have timed boost/drain for this stat. So, we'll add it!
		statFlags |= ((1 << (stat + _STAT_TIMED))
		  | (1 << (stat + _STAT_VALUE)));
		statBoosts[stat + _STAT_TIMED] = ({ duration, delta });
		// Already have a drain/boost. Have to modify the array...
	    } else {
		i = 0;
		// So, we'll loop and combine new drain/boost with old timed d/bs.
		while (i < sizeof(timed) && duration <= timed[i]) {
		    timed[i + 1] += delta;
		    i += 2;
		}
		// If d/b continues longer than the last one, let's add 2 elements.
		if (i == sizeof(timed) && duration > timed[i - 2]) {
		    timed += ({ delta, duration });
		}
		statBoosts[stat + _STAT_TIMED] = timed;
	    }
	    // Let's also add this to the current value...
	    statBoosts[stat + _STAT_VALUE] += delta;
	    if (!nextStatCheck || nextStatCheck > duration)
		nextStatCheck = duration;
	}
    } else {
	// Otherwise, it's either case b) or c):
	if (stat < ST_FIRST_STAT || stat > ST_LAST_STAT) return 0;
	// Case b) (adding object based d/b):
	if (duration) {
	    if (!(statFlags & (1 << (stat + _STAT_OBJECT_BASED)))
	      || !(timed = statBoosts[stat + _STAT_OBJECT_BASED])) {
		// Didn't have timed boost/drain for this stat. So, we'll add it!
		statFlags |= ((1 << (stat + _STAT_OBJECT_BASED))
		  | (1 << (stat + _STAT_VALUE)));
		statBoosts[stat + _STAT_OBJECT_BASED] = ({ item, delta });
		// Already had an object based boost/drain. Let's not bother checking
		// whether they're still ok... faster this way. *grin*
		// (we could easily check it. just don't think it's needed)
	    } else {
		timed += ({ item, delta });
		statBoosts[stat + _STAT_OBJECT_BASED] = timed;
	    }
	    // And let's also update total boost value.
	    statBoosts[stat + _STAT_VALUE] += delta;

	    // Case c) (removing object based d/b):
	} else {
	    // Let's check we really have a boost/drain for this object...
	    if (!(statFlags & (1 << (stat + _STAT_OBJECT_BASED)))
	      || !(timed = statBoosts[stat + _STAT_OBJECT_BASED])) return 0;

	    // Now... we have to check for destructed items as we want to use
	    // "array -= ({ 0 })". Otherwise we might get into trouble if there
	    // was a dested item. :-/
	    // As we have to do this anyway, we may as well check that item is where
	    // it's supposed to be; ie. in this object's inventory.
	    // We can search for the item in very same loop too...

	    i = sizeof(timed);
	    x = 0;
	    while ((i -= 2) >= 0) {
		if (timed[i] == item) {
		    x = y = 1;
		    statBoosts[stat + _STAT_VALUE] -= timed[i + 1];
		    timed[i] = 0;
		    timed[i + 1] = 0;
		} else if (!timed[i] || !present(timed[i], this_object())) {
		    x = 1;
		    statBoosts[stat + _STAT_VALUE] -= timed[i + 1];
		    timed[i + 1] = 0;
		}
	    }
	    if (x) {
		timed -= ({ 0 });
		if (!sizeof(timed)) {
		    timed = 0;
		    statFlags &= (~(1 << (stat + _STAT_OBJECT_BASED)));
		    m_delete(statBoosts, stat + _STAT_OBJECT_BASED);
		    if (!(statFlags & (1 << (stat + _STAT_TIMED)))) {
			statFlags &= (~(1 << (stat + _STAT_VALUE)));
			m_delete(statBoosts, stat + _STAT_VALUE);
		    }
		} else statBoosts[stat + _STAT_OBJECT_BASED] = timed;
	    }
	}
    }

    if (stat == ST_CON) {
	max_hp = HPS(Con + statBoosts[ST_CON + _STAT_VALUE]);
	max_fp = FPS(Str,Con + statBoosts[ST_CON + _STAT_VALUE]);
    } else if (stat == ST_INT) {
	max_sp = SPS(Int + statBoosts[ST_INT + _STAT_VALUE]);
    } else if (stat == ST_STR) {
	max_fp = FPS(Str + statBoosts[ST_STR + _STAT_VALUE],Con);
    }

    return 1;
}

void
update_stats()
{
    int i, j, *timed, a, old_stat, n_check;
    mixed *obj_based;
    string s, s2;
    int do_remove;

    // If we don't have any boosts/drains, let's skip checks.
    // Might speed things up.
    if (!statFlags) {
	nextStatCheck = statBoosts = 0;
	return;
    }
    if (!mappingp(statBoosts)) {
	statBoosts = statFlags = nextStatCheck = 0;
	return;
    }
    nextStatCheck = (a = AGE) + 10;	// Just to be sure...
    n_check = 0;

    i = ST_FIRST_STAT - 1;
    while (i++ < ST_LAST_STAT) {

	if (!(statFlags & (1 << i))) continue;	// No boosts.
	timed = obj_based = 0;

	// Has a timed stat boost/drain?
	if (statFlags & (1 << (i + _STAT_TIMED))) {
	    if (!(timed = statBoosts[i + _STAT_TIMED])
	      || !pointerp(timed)) {
		statFlags &= (~(1 << (i + _STAT_TIMED)));
	    } else {
		j = 0;
		old_stat = timed[1];
		while (j < sizeof(timed) && timed[j] <= a)
		    j += 2;
		// Boost/drain ended...
		if (j) {
		    if (j == sizeof(timed)) {
			timed = 0;
			statFlags &= (~(1 << (i + _STAT_TIMED)));
			statBoosts = m_delete(statBoosts, i + _STAT_TIMED);
		    } else {
			timed = timed[j - 1 .. sizeof(timed) - 1];
			old_stat -= timed[i];
			if (!n_check || n_check > timed[0]) {
			    n_check = timed[0];
			    nextStatCheck = n_check;
			}
			statBoosts[i + _STAT_TIMED] = timed;
		    }
		    if (old_stat) {
			// So; if our stat changes due to a timed stat boost/drain
			// ending, let's inform player.
			if (old_stat < 0) {
			    switch (i) {
			    case ST_STR:
				s2 = "You feel stronger."; break;
			    case ST_DEX:
				s2 = "You feel nimbler."; break;
			    case ST_CON:
				s2 = "You feel tougher."; break;
			    case ST_INT:
				s2 = "You feel brighter."; break;
			    case ST_FAT:
				s2 = "You feel briskier."; break;
			    case ST_HP_HEAL_RATE:
			    case ST_SP_HEAL_RATE:
			    case ST_FP_HEAL_RATE:
				s2 = 0; break;
			    }
			} else {
			    switch (i) {
			    case ST_STR:
				s2 = "You feel weaker."; break;
			    case ST_DEX:
				s2 = "You feel clumsier."; break;
			    case ST_CON:
				s2 = "You feel frailer."; break;
			    case ST_INT:
				s2 = "You feel dumber."; break;
			    case ST_FAT:
				s2 = "You feel more fatigue."; break;
			    case ST_HP_HEAL_RATE:
			    case ST_SP_HEAL_RATE:
			    case ST_FP_HEAL_RATE:
				s2 = 0;
				break;
			    }
			}
			if (!s) s = s2;
			else
			{
			    if (s2)
				s = sprintf("%s\n%s", s, s2);
			    else s = 0;
			}
		    }
		}
	    }
	}

	// Has an object-based stat boost/drain?
	if (statFlags & (1 << (i + _STAT_OBJECT_BASED))) {
	    if (!(obj_based = statBoosts[i + _STAT_OBJECT_BASED])
	      || !pointerp(obj_based)) {
		statFlags &= (~(1 << (i + _STAT_OBJECT_BASED)));
	    } else {
		j = sizeof(obj_based);
		do_remove = old_stat = 0;
		while ((j -= 2) >= 0) {
		    // Has the object been destructed or moved to some other place?
		    if (!obj_based[j] || environment(obj_based[j]) != this_object()) {
			do_remove++;
			obj_based[j] = 0;
			obj_based[j + 1] = 0;
		    } else {
			old_stat += obj_based[j + 1];
		    }
		}
		if (do_remove) {
		    obj_based -= ({ 0 });
		    if (!sizeof(obj_based)) {
			// If no object-based boosts/drains left, let's remove
			// entry from mapping...
			obj_based = 0;
			statBoosts = m_delete(statBoosts, i + _STAT_VALUE);
			statFlags &= (~(1 << (i + _STAT_OBJECT_BASED)));
		    } else {
		    }
		    statBoosts[i + _STAT_OBJECT_BASED] = obj_based;
		}
	    }
	}
	if (!obj_based) {
	    // No boosts for this stat any more!
	    if (!timed) {
		statFlags &= (~(1 << (i + _STAT_VALUE)));
		statBoosts = m_delete(statBoosts, i + _STAT_VALUE);
		continue;	// Let's skip rest of checks.
	    }
	    old_stat = 0;
	}
	if (timed) old_stat += timed[1];
	statBoosts[i + _STAT_VALUE] = old_stat;

	// Let's also keep max hp & sp amounts up to date...
	if (i == ST_CON) {
	    max_hp = HPS(Con + statBoosts[ST_CON + _STAT_VALUE]);
	    max_fp = FPS(Str,Con + statBoosts[ST_CON + _STAT_VALUE]);
	} else if (i == ST_INT) {
	    max_sp = SPS(Int + statBoosts[ST_INT + _STAT_VALUE]);
	} else if (i == ST_STR) {
	    max_fp = FPS(Str + statBoosts[ST_STR + _STAT_VALUE],Con);
	}
    }
    if (s) tell_me(s);
}

// Abilities as arrays (as they should have been ages ago)
int* query_abilities() { return ({ Str, Dex, Con, Int, Fatigue }); }
int* query_max_abilities() { return ({ max_Str, max_Dex, max_Con, max_Int, max_Fatigue }); }

string
query_stats()
{
    return sprintf("str: %d, int: %d, con: %d, dex: %d, Fatigue: %d",
      Str, Int, Con, Dex, Fatigue);
}

// Module: conditions.c.
//
// Used by: Standard player object (/obj/player.c) and standard
//	NPC object (/lib/npc.c). Configured bit different, if constant
//	PLAYER_C is defined. Same core for both, however
//
// Version: 3.1
// Date: 7-May-98 / Graah
//
// New:
//
// Completely rewritten. Wasn't checking conditions except when
// queried. This resulted in not knowing when player sees, hear etc.
// again. This is now fixed, but this still works much more
// efficiently than the old version. Also, you can now add resistances
// to all conditions. This is done by negating the condition number.
// To be able to use resistances, however, please include header file
// "conditions.h", which defines macro "RESISTANCE(condition)" and
// use it (like "player_ob -> add_condition(RESISTANCE(C_BLIND))").
//
// Also, new function "add_condition" can be used to extend or shorten
// duration of a timed condition.
//
// Now same file for both player.c and npc.c; configuration handled by
// preprocessor directives. If you want to use 'full version', define
// PLAYER_C prior to including this file.
//
// Actually, there aren't much differences between the two any more... :-)
//
// 24-aug-95:
//
// Once again, rewritten... Now uses heart beat - based system; no need
// for pack/unpack anymore. Should also be "fairer" under lag...
//
// 7-May-98: Added C_PASSED_OUT for alcohol and possibly some poisons.
//           Resisting unconsciousness becomes more reasonable.

// Now, let's define this... so we can switch back to using time()
// if necessary. -+ Doomdark +-

#define	AGE	query_age()

#ifndef PLAYER_C
#ifndef NPC_C
#ifndef LIVING_C

#include <conditions.h>

status dead;

int query_age();

varargs void tell_me(string s, mixed ob, status n);
#endif
#endif
#endif

#define	CONDITION_MASK(x)	(1<<x)
#define	_COND_PACK_MARK		(1<<0)
#define	LAST_FAST_CONDITION	30
#define	IS_FAST_CONDITION(x)	(x>0&&x<31)

#define	COND_OK(x,y)		(force_on || ((y > -1) && ((x > y) || (x < 0))))
#define	COND_CHECK(x)		((x > 0) && (!NextConditionCheck || (x < NextConditionCheck)))

// Conditions (C_BLIND, C_DETECT_INVIS etc.). Also resistances to conditions!
// Structure: ([ condition_element1, condition_element2, ... ])
// where each element is of form "condition_or_resistance : value"
// in which "condition_or_resistance" is a constant defined in
// /include/conditions.h and "value" either:
//	o integer that defines duration of the condition
//	o an object causing permanent condition
//	o array of int(s) and/or object(s) if there is at least one
//	  such object and either a timed condition/resistance or another object.
// Note that if there's a timed value in array, it's always the 1st element!

mapping Conditions;

// Next one tells us when will next condition check be needed.
// This helps us to prevent calling of condition updates when not
// necessary.

static int NextConditionCheck;

// And this integer is used as bitfield to allow quick queries;
// it contains current status of conditions (only 32 first, if we have more).

int ConditionsOn;	// Let's save this as it contains pack flag!

// Condition queries. Can be used to check resistances as well...
// Note that conditions aren't checked when queried any more.

// NEW! 2nd optional argument: query-type. Defines the type of the
// answer. Returned types are:

// 0 ->	Integer; either time left, or -1 if object based condition.
// 1 ->	Real value; either integer, object or array of those.

// NOTE! Returned value is in heart beats, just like values used in
// set/add_condition!

varargs mixed
query_condition(int c, int qtype)
{
    mixed x;

    // First let's check if it's a "fast" condition/resistance;
    // if so, we don't have to check mapping, but only a bitfield!

    if (IS_FAST_CONDITION(c)) {
	if (!(ConditionsOn & CONDITION_MASK(c)))
	    return 0;
    }

    if (!Conditions || !mappingp(Conditions))
	return 0;

    x = Conditions[c];

    if (!qtype) {
	if (intp(x = Conditions[c])) {
	    if (x > 0) x -= AGE;
	    return x;
	}
	if (objectp(x)) return -1;
	if (x[0] && intp(x[0]))
	    return x[0] - AGE;
	return -1;
    }

    return x;
}

// Was: "Each point to v adds one heart beat (2 seconds)."
// Not anymore. Now this really _sets_ the condition, given that
// there's no resistance to the condition. Use "add_condition" if
// you want to add or subtract duration of a condition (or resistance).
// Still, each unit of duration means 1 heart beat -> 2 seconds.
// Note however that actual time a heart beat takes may be more or
// less than 2 seconds, depending on lag etc.

// Arguments:
//
// int condition:	Condition/resistance to set/clear.
// int duration:	Duration of c/r; if 0 will clear c/r, if -1 then
//			set the c/r permanent.
// [object item]:	Optional. Object that 'causes' permanent c/r.
//			has to be in inventory of this_object() (ie. player)
//
// Returns:
//
// When clearing c/r:	1 if c/r was cleared (ie. no other c/r's of same type),
//			0 if still one or more c/r's of same type left
// When setting c/r:	1 if setting was succesful; for resistances (almost)
//			  always, for conditions if there weren't resistance
//			  to the condition in question.
//			0 if setting was unsuccesful; usually due to resistance
//			  blocking a condition.

varargs status
set_condition(int condition, int duration, object item)
{
    mixed old_value, old_value2;
    int is_on, i, on_flag, force_on;
    status resisting;

    if (!Conditions)
	Conditions = ([ ]);

    if (condition > 0 && (condition & C_FORCE_FLAG)) {
	condition &= (~C_FORCE_FLAG);
	force_on = 1;
    }
    // Let's use this cheap kludge to try to prevent buggy code causing
    // permanent conditions. -+ Doomdark 25-aug-95 +-
    else if (duration < -1)
	duration = 0;

    old_value = Conditions[condition];

    if (IS_FAST_CONDITION(condition)) {
	on_flag = CONDITION_MASK(condition);
	is_on = ConditionsOn & on_flag;
    }

    if (!duration) {	// Removing a condition/resistance

	if (intp(old_value)) {	// Already had a timed condition.
	    if (objectp(item)) return 0; // Tried to remove object based condition.
	} else if (objectp(old_value)) {
	    if (item != old_value) {	// Not same object!
		if (environment(old_value) != this_object()) {
		    // Item not in our inventory any more!!!
		    // So, we'll remove condition but _not_ tell player as this is bug;
		    // should have been informer earlier.
		    Conditions = m_delete(Conditions, condition);
		    if (is_on) ConditionsOn &= (~on_flag);
		}
		return 0; // Not same object; condition remains.
	    }	// Otherwise, let's continue, ie. we'll clear condition ok.
	} else {
	    for (i = sizeof(old_value) - 1; i >= 0; i--) {
		if (objectp(old_value[i]) && (environment(old_value[i]) != this_object()
		    || old_value[i] == item))
		    old_value[i] = 0;
	    }
	    old_value -= ({ 0 });
	    if (i = sizeof(old_value)) {
		if (i == 1)	// If only 1 element, no need to be an array!
		    Conditions[condition] = old_value[0];
		else
		    Conditions[condition] = old_value;
		return 0;
	    }
	}

	Conditions = m_delete(Conditions, condition);

	if (is_on) {
	    ConditionsOn &= (~on_flag);

	    if (condition > 0) switch (condition) {
		// Now, let's inform player. Should we also inform about resistances ending?!?
		// But. Should we inform when object based condition ended?!?!

	    case C_BLIND:	tell_me("You can see again."); break;
	    case C_DEAF:	tell_me("You can hear again."); break;
	    case C_STUNNED:	tell_me("Your muscles start to work again."); break;
	    case C_UNCONSCIOUS: tell_me("You become conscious again."); break;
	    case C_CONFUSED: tell_me("You feel less confused."); break;
	    case C_POISONED: tell_me("You feel less poisoned and sick."); break;
	    case C_DETECT_INVIS: tell_me("Your eyes do not tingle any more."); break;
	    case C_SLOWED:	tell_me("The world slows down."); break;
	    case C_HASTED:	tell_me("The world quickens up."); break;
	    case C_HALLUCINATING: tell_me("Everything looks SO boring now."); break;
	    case C_PASSED_OUT: tell_me("You wake up."); break;
	    default: tell_me("You feel very strange."); break;
	    }
	}

	/*************************************************

	 If resistance to condition was removed,
	 we have to check for possible _new_ condition!!!

	*************************************************/

	if (condition < 0) {
	    if (!(ConditionsOn & (on_flag = CONDITION_MASK(-condition)))) {
		if (old_value = Conditions[-condition]) {
		    if (objectp(old_value)) {
			if (environment(old_value) != this_object()) {
			    Conditions = m_delete(Conditions, -condition);
			    return 1;	// Removal ok nevertheless.
			}
		    } else if (pointerp(old_value)) {
			for (i = sizeof(old_value) - 1; i >= 0; i--) {
			    if (objectp(old_value[i]) && environment(old_value[i]) != this_object())
				old_value[i] = 0;
			}
			old_value -= ({ 0 });
			if (!(i = sizeof(old_value)))
			    old_value = 0;
			else if (i == 1)
			    old_value = old_value[0];
			if (old_value) Conditions[-condition] = old_value;
			else Conditions = m_delete(Conditions, 0);
		    }

		    if (old_value) {	// Will get condition after resistance ended!
			ConditionsOn |= on_flag;
			// Also, let's inform player about this happening!
			switch (-condition) {
			case C_BLIND:	tell_me("You suddenly can't see a thing!"); break;
			case C_DEAF:	tell_me("You suddenly can't hear a thing!"); break;
			case C_STUNNED:	tell_me("You suddenly can't move your muscles!"); break;
			case C_UNCONSCIOUS: tell_me("You suddenly lose your consciousness!"); break;
			case C_CONFUSED: tell_me("You suddenly feel very confused!"); break;
			case C_POISONED: tell_me("You suddenly feel poisoned and sick!"); break;
			case C_DETECT_INVIS: tell_me("Your eyes suddenly begin to tingle!"); break;
			case C_SLOWED:	tell_me("The world suddenly quickens up!"); break;
			case C_HASTED:	tell_me("The world suddenly slows down!"); break;
			case C_HALLUCINATING: tell_me("Suddenly, world looks really COSMIC!"); break;
			case C_PASSED_OUT: tell_me("Eveything goes dark!"); break;
			default: tell_me("You suddenly feel very strange."); break;
			}
		    }
		}
	    }
	}

	/********************************************

	 Ok. Removing and associated stuff completed.

	********************************************/

	return 1;	// So, we were succesful in removing it.
    }

    // Can only add object based conditions/resistances if objects is in
    // our inventory. Is this check needed?

    if (item && environment(item) != this_object())
	return 0;

    if (duration > 0) {
	duration = duration + AGE;
	// Let's calculate when condition/resistance ends...
    }

    // We're adding condition/resistance...

    if (condition < 0) {	// Adding a new resistance.

	if (intp(old_value)) {  // Had a timed resistance
	    if (!item) {	// Adding timed resistance as well.
		if (COND_OK(duration, old_value)) {
		    old_value = duration;	// Will last longer, so we'll use it!
		} else return 1;	// Could add it ok...
	    } else old_value = ({ old_value, item });
	} else if (objectp(old_value)) {	// Had an object-based resistance.
	    if (!item) old_value = ({ duration, old_value });
	    else if (item == old_value) return 1;	// Trying to add same object-based res.!
	    else old_value = ({ old_value, item });
	} else {	// Had an array...
	    for (i = sizeof(old_value) - 1; i >= 0; i--) {
		if (objectp(old_value[i]) && (environment(old_value[i]) != this_object()
		    || old_value[i] == item))
		    old_value[i] = 0;
	    }
	    old_value -= ({ 0 });
	    if (!sizeof(old_value)) {
		if (item) old_value = item;
		else old_value = duration;
	    } else {
		if (!item && intp(old_value[0])) {
		    if (COND_OK(duration, old_value[0]))
			old_value[0] = duration;
		    else
			return 1;	// Could add resistance nevertheless.
		} else {
		    if (item) old_value = ({ item }) + old_value;
		    else old_value = ({ duration }) + old_value;
		}
	    }
	}
	if (old_value) Conditions[condition] = old_value;
	else Conditions = m_delete(Conditions, 0);

	if (!item && COND_CHECK(duration))
	    NextConditionCheck = duration;
	// And, let's also set next checkpoint if necessary.

	// No need to set any flags as resistances don't use them.

	return 1;	// Adding resistance went ok.

    } else {		// Adding a new condition.

	// Now let's check if we have resistance against this condition!
	// Except if we are forcing the condition, like when going mortally
	// wounded. -+ Doomdark 27-oct-95 +-

	if (!force_on && (old_value2 = Conditions[RESISTANCE(condition)])) {
	    if (objectp(old_value2)) {
		if (environment(old_value2) != this_object()) {
		    Conditions = m_delete(Conditions, RESISTANCE(condition));
		    old_value2 = 0;
		}
	    } else if (pointerp(old_value2)) {
		for (i = sizeof(old_value2) - 1; i >= 0; i--) {
		    if (objectp(old_value2[i]) && environment(old_value2[i]) != this_object())
			old_value2[i] = 0;
		}
		old_value2 -= ({ 0 });
		if (!(i = sizeof(old_value2))) {
		    Conditions = m_delete(Conditions, RESISTANCE(condition));
		    old_value2 = 0;
		} else if (i == 1)
		    Conditions[RESISTANCE(condition)] = old_value2[0];
		else
		    Conditions[RESISTANCE(condition)] = old_value2;
	    }

	    if (old_value2) {	// Resistance exists; let's set flag!
		if (!item) return 0;
		// If we had a _timed_ condition, we simply discard it as
		// it most probably was a spell, and thus was completely
		// prevented by the resistance!
		resisting = 1;
	    }
	}

	// Now, whether we have resistance, let's add condition normally but
	// let in only _affect_ us if we don't have resistance. That's what above
	// flag is used for.

	if (intp(old_value)) {  // Had a timed condition
	    if (!item) {	// Adding timed condition as well.
		if (COND_OK(duration, old_value)) {
		    old_value = duration;	// Will last longer, so we'll use it!
		} else return 1;
	    } else old_value = ({ old_value, item });
	} else if (objectp(old_value)) {	// Had an object-based resistance.
	    if (!item) old_value = ({ duration, old_value });
	    else if (item == old_value) return !resisting;
	    // Trying to add same object-based cond.!
	    else old_value = ({ old_value, item });
	} else {	// Had an array...
	    for (i = sizeof(old_value) - 1; i >= 0; i--) {
		if (objectp(old_value[i]) && (environment(old_value[i]) != this_object()
		    || old_value[i] == item))
		    old_value[i] = 0;
	    }
	    old_value -= ({ 0 });
	    if (!sizeof(old_value)) {
		if (item) old_value = item;
		else old_value = duration;
	    } else {
		if (!item && intp(old_value[0])) {
		    if (COND_OK(duration, old_value[0]))
			old_value[0] = duration;
		    else
			return 1;	// Could add condition nevertheless
		} else {
		    if (item) old_value = ({ item }) + old_value;
		    else old_value = ({ duration }) + old_value;
		}
	    }
	}

	Conditions[condition] = old_value;

	if (COND_CHECK(duration))
	    NextConditionCheck = duration;
	// And, let's also set next checkpoint if necessary.

	if (resisting)
	    return 0;		// Was a resistance, didn't set condition!

	if (!is_on)
	    ConditionsOn |= (on_flag);

	return 1;		// Adding condition went ok.

    }
}

// NEW! With this you can extend duration of a condition.
// Please do use this instead of set_condition, if you do use
// timed conditions.

// Arguments:
//   int condition:	Condition/resistance of which duration you want to extend.
//   int amount:	Amount with which you want to extend duration
//			in heart beats (ie. 2 seconds). This means that value
//			of 1 will extend duration by 2 seconds. Note however that
//			actual time heart beat takes varies because of lag etc.
//
// Returns:		Duration of the condition in question after (im HBs)
//			add_condition, or 0 if condition went off (or was off),
//			or was blocked by a resistance.

int
add_condition(int condition, int amount)
{
    mixed old_value;
    int i, x;

    if (!amount)
	return query_condition(condition, 0);

    if (!(old_value = Conditions[condition]) || (!intp(old_value) &&
	(!pointerp(old_value) || !intp(old_value[0])))) {
	if (amount <= 0) return 0;
	return set_condition(condition, amount, 0) ? amount : 0;
    }

    if (intp(old_value))
	x = old_value;
    else
	x = old_value[0];

    if (x < 0) return -1;	// No need to add; permanent.

    x -= AGE;
    if (x < 0) x = 0;

    if ((amount += x) < 0)
	amount = 0;

    return set_condition(condition | C_FORCE_FLAG, amount, 0)
    ? amount : 0;
}

void
update_conditions()
{
    int *cond, i, j, curr_time, next_time, cond_nr;
    mixed x, y;
    string txt;

    NextConditionCheck = next_time = 0;

    if (!mappingp(Conditions)) return;

    curr_time = AGE;

    NextConditionCheck = curr_time + 10;	// Just to play this safe!

    for (cond = m_indices(Conditions), i = sizeof(cond) - 1; i >= 0; i--) {
	cond_nr = cond[i];
	x = Conditions[cond_nr];
	if (intp(x)) {
	    if (x > -1) {
		if (x <= curr_time)
		    x = 0;
		else if (!next_time || next_time > x)
		    next_time = x;
	    }
	} else if (objectp(x)) {
	    if (environment(x) != this_object())
		x = 0;
	} else if (pointerp(x)) {
	    for (j = sizeof(x) - 1; j >= 0; j--) {
		y = x[j];
		if (objectp(y)) {
		    if (environment(y) != this_object())
			x[j] = 0;
		} else {
		    if (y > -1) {
			if (y <= curr_time)
			    x[j] = 0;
			else if (!next_time || next_time > y)
			    next_time = y;
		    }
		}
	    }
	    x -= ({ 0 });
	    if (!(j = sizeof(x)))
		x = 0;
	    else if (j == 1)
		x = x[0];
	}
	if (x)
	    Conditions[cond_nr] = x;
	else {
	    Conditions = m_delete(Conditions, cond_nr);
	    if (IS_FAST_CONDITION(cond_nr))
		ConditionsOn &= ~(CONDITION_MASK(cond_nr));
	    if (cond_nr > 0) {
		if (!txt) txt = "";
		else txt += "\n";
		// Also, let's inform player about this happening!
		switch (cond[i]) {
		case C_BLIND:	txt += "You can see again!"; break;
		case C_DEAF:	txt += "You can hear again!"; break;
		case C_STUNNED:	txt += "You can move again!"; break;
		case C_UNCONSCIOUS: txt += "You regain your consciousness!"; break;
		case C_CONFUSED: txt += "You feel less confused!"; break;
		case C_POISONED: txt += "You feel less poisoned and sick!"; break;
		case C_DETECT_INVIS: txt += "Your eyes stop to tingle."; break;
		case C_SLOWED:	txt += "The world suddenly slows down!"; break;
		case C_HASTED:	txt += "The world suddenly quickens up!"; break;
		case C_HALLUCINATING: txt += "Suddenly, everything looks very dull..."; break;
		case C_PASSED_OUT: txt += "You wake up."; break;
		default:	txt += "You suddenly feel very strange."; break;
		}
	    } else {	// Let's also inform about ending resistances?
		if (!txt) txt = "You feel less resistant...";
		else txt += "\nYou feel less resistant...";
	    }
	}
    }
    if (txt) tell_me(txt);
    NextConditionCheck = next_time;
}

void
set_conditions(mapping c)
{
    if (!c) {
	Conditions = NextConditionCheck = 0;
	ConditionsOn = 0;
    } else if (mappingp(c)) {
	Conditions = c;
	if (!sizeof(c))
	    ConditionsOn = NextConditionCheck = 0;
    }
}

mapping
query_conditions()
{
    return Conditions;
}

// Condition-based queries

status
query_can_move()
{
    if (dead || (ConditionsOn & (CONDITION_MASK(C_STUNNED) |
	  CONDITION_MASK(C_PASSED_OUT) |
	  CONDITION_MASK(C_UNCONSCIOUS) |
	  CONDITION_MASK(C_CASTING))))
	return notify_fail("You cannot move a muscle!\n"), 0;
    return 1;
}

status
query_can_see()
{
    if (ConditionsOn & (CONDITION_MASK(C_BLIND)
	| CONDITION_MASK(C_UNCONSCIOUS)
	| CONDITION_MASK(C_PASSED_OUT)))
    {
	if (ConditionsOn & (CONDITION_MASK(C_HALLUCINATING)))
	    notify_fail("Wow, man! The colours! (They block your sight).\n");
	else notify_fail("You cannot see anything.\n");
	return 0;
    }

    return 1;
}

#ifdef DEBUG
string
debug_conditions()
{
    return sprintf("Next check: %d, Packed: %s, Conditionmask: %d",
      NextConditionCheck, (ConditionsOn & _COND_PACK_MARK ? "YES": "NO"),
      ConditionsOn);
}
#endif

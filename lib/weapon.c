/****************************************************************
*								*
* Module:							*
*      /lib/weapon.c						*
* Description:							*
*	The base weapon object that should be used as the base	*
*	for all the weapons in the game, either by cloning it	*
*	and setting the values, or by inheriting it.		*
* Version:							*
*	1.0a, Aug-98.						*
* Last update:							*
*	19-Aug-98 by Doomdark					*
*								'
* Modifications:						*
*								*
****************************************************************/

#include <conditions.h>
#include <curse.h>
#include <weapon_defs.h>
#include <attack.h>
#include <living_defs.h>

#include "/basic/weapon/weapon_private.h"

#define NO_DROP_MESSAGE "Your weapon refuses to be dropped."
#define NO_REMOVE_MESSAGE "Your weapon doesn't want to be removed."

#define	MAX_POISON_DURATION	1000
// So, max. duration for poisoning will be 2000 seconds ~33 mins.

#define	UPDATE_WEAPON wielded_by->update_weapon(this_object(),#'query_wc,\
query_hc(),query(WP_DAMAGE_TYPE))
#define	IS_WIELDED	(wielded&&wielded_by==environment())

inherit "/basic/curse";
inherit "/basic/object/dimensions";

// Prototypes:

void new_value();

/****************************************************************
*								*
*	And then global variables. Please use set_funcs		*
*		instead of direct use.				*
*								*
****************************************************************/

int class_of_weapon, hit_class;	// Weapon & hit class of the weapon
int damage_type;		// What kind of damage this weapon does?
int bluntness;			// Amount of bluntness
static int poisoned;		// How much poison left in weapon.
static int poison_str;		// Poison amount, poison strength
string read_msg;		// Optional read-message for players.
string info;			// Optional info-string for other coders.
int value, orig_value;		// Current / original value of this weapon.
object hit_func, wield_func, unwield_func;
string weapon_type, weapon_subtype;
int _str_factor, _dex_factor;	// Factors determining relative 'weights' of
				// wielder's strength and dexterity.

// Objects in which special 'hit', 'wield' and 'unwield'-functions will be
//  called. If they return 1, that action won't be done, except for 'hit' that
// returns amount of extra damage done.
static object wielded_by;	// Object that's wielding this weapon.
static string wielded;		// String used by query_short (usually, 0 or "wielded").

static closure _wc_formula;	// Formula used when determining 'real wc'
				// when this weapon is wielded by someone.

private static int _wp_flags;	// Private variable used by this module.
private static closure initFunc;

private static string *_types = ({
	"sword", "club", "mace",
	"spear", "dagger", "bow",
	"knife", "duster", "staff",
	"hammer", "whip", "axe"
});

/****************************************************************
*								*
*		Now all the query-functions.			*
*								*
****************************************************************/

// New!!!

closure
query_closure(string func)
{
	return symbol_function(func, this_object());
}


mixed
query(int x)
{
string *foo, s, t;
	switch (x) {
	case WP_TYPE:
// We do need to kludge a fair bit here; because most code doesn't
// yet set explicitly the type! -+ Doomdark +-

// If weapon_type has already been set, there's no need for kludges! - Chopin
                if(weapon_type)
                    return weapon_type;

		if (s = query_name()) {
			foo = explode(s, " ");
			if (member(_types, foo[<1]) >= 0)
				weapon_type = foo[<1];
		    else if (strstr(s, "sword") >= 0 || strstr(s, "blade") >= 0)
			weapon_type = "sword";
		    else if (strstr(s, "axe") >= 0) weapon_type = "axe";
		    else if (strstr(s, "hammer") >= 0) weapon_type = "hammer";
		    else if (strstr(s, "duster") >= 0) weapon_type = "duster";
		    else if (strstr(s, "whip") >= 0) weapon_type = "whip";
		    else if (strstr(s, "bow") >= 0) weapon_type = "bow";
		}
		if (!weapon_type) {
		    if (_wp_flags & F_WP_BLUNT) {
			weapon_type = "club";      // Basic blunt type. :-/
		    } else {
			weapon_type = "sword";     // Basic sharp type.
		    }
		}
		return weapon_type;
	case WP_SUBTYPE:
		return weapon_subtype;
	case WP_BLUNT:
		return (_wp_flags & F_WP_BLUNT);
	case WP_SHARP:
		return !(_wp_flags & F_WP_BLUNT);
	case WP_TWO_HANDED:
		return (_wp_flags & F_WP_TWO_HANDED);
	case WP_ONE_HANDED:
		return !(_wp_flags & F_WP_TWO_HANDED);
	case WP_TROLL_WEAPON:
		return (_wp_flags & F_WP_TROLL_WEAPON);
	case WP_NOT_TROLL_WEAPON:
		return !(_wp_flags & F_WP_TROLL_WEAPON);
	case WP_STAYS_SHARP:
		return (_wp_flags & F_WP_STAYS_SHARP);
	case WP_GOES_BLUNT:
		return !(_wp_flags & F_WP_STAYS_SHARP);
	case WP_TROLL_1H:
		return (_wp_flags & F_WP_TROLL_1H);
	case WP_TROLL_2H:
		return !(_wp_flags & F_WP_TROLL_1H);
	case WP_DAMAGE_TYPE:
// Now, if we haven't defined damage type, let's try to 'guess' it.
		if (!damage_type) {
		  if (weapon_type == "axe")
			damage_type = DT_CLEAVE;
		else if (weapon_type == "dagger" || weapon_type == "knife"
		  || weapon_type == "spear")
			damage_type = DT_PUNCTURE;
		else
		if (_wp_flags & F_WP_BLUNT)
			damage_type = DT_BLUNT;
		else
			damage_type = DT_SLASH;
		}
		return damage_type;
	case WP_STR_FACTOR:
		return _str_factor;
	case WP_DEX_FACTOR:
		return _dex_factor;
	case WP_FLAGS:
		return _wp_flags;
	}
	return 0;
}

int query_poison_strength() { return poison_str; }
int query_bluntness() { return bluntness; }

// NEW! Now we'll also make sure we modify the actual wc with the
// values of str & dex the living wielding us has.
varargs int
query_wc(int return_real)
{
int i;
/* We need to take care we have the weapon type set... */
	if (!weapon_type)
		query(WP_TYPE);
	if (!_str_factor) {
	  switch (weapon_type) {
	  case "sword":
		if (_wp_flags & F_WP_TWO_HANDED) {
			_str_factor = 50; _dex_factor = 50;
		} else {
			_str_factor = 30; _dex_factor = 70;
		}
		break;
	  case "dagger":
		_str_factor = 20; _dex_factor = 80;
		break;
	  case "knife":
		_str_factor = 25; _dex_factor = 75;
		break;
	  case "club":
		if (_wp_flags & F_WP_TWO_HANDED) {
			_str_factor = 90; _dex_factor = 10;
		} else {
			_str_factor = 80; _dex_factor = 20;
		}
		break;
	  case "hammer":
		if (_wp_flags & F_WP_TWO_HANDED) {
			_str_factor = 90; _dex_factor = 10;
		} else {
			_str_factor = 80; _dex_factor = 20;
		}
		break;
	  case "mace":
		if (_wp_flags & F_WP_TWO_HANDED) {
			_str_factor = 80; _dex_factor = 20;
		} else {
			_str_factor = 65; _dex_factor = 35;
		}
		break;
	  case "axe":
		if (_wp_flags & F_WP_TWO_HANDED) {
			_str_factor = 75; _dex_factor = 25;
		} else {
			_str_factor = 60; _dex_factor = 40;
		}
		break;
	  case "spear":
		if (_wp_flags & F_WP_TWO_HANDED) {
			_str_factor = 60; _dex_factor = 40;
		} else {
			_str_factor = 40; _dex_factor = 60;
		}
		break;
	  case "bow":
		_str_factor = 10; _dex_factor = 90;
		break;
	  case "duster":
		_str_factor = 60; _dex_factor = 40;
		break;
	  case "whip":
		_str_factor = 20; _dex_factor = 80;
		break;
	  case "staff":
		_str_factor = 40; _dex_factor = 60;
		break;
	  case "scythe":
		_str_factor = 30; _dex_factor = 70;
		break;
	  default:
		_str_factor = _dex_factor = 50;
	  }
	}
	if (return_real || !(i = funcall(_wc_formula, class_of_weapon
		+ query_curse_level() - bluntness, _str_factor, _dex_factor)))
		return (class_of_weapon + query_curse_level() - bluntness);
	return i;
}

int query_hc() { return (hit_class + query_curse_level() / 2); }
	/* Hit class (accuracy), -10 to 10 */

string query_info() { return info; }
string query_wielded() { return wielded; } /* requested by Franco */
status query_hit_func() { return hit_func ? 1 : 0; }  /* 22.12.1991 //Frobozz */
object query_wielded_by() { return wielded_by; } //Frobozz
int query_value() { return value; }
int query_orig_value() { return orig_value; }

#if 0
// Huh? Name should NOT have any suffix! -+ Doomdark 05-mar-95 +-
varargs string
query_name(int mode, object who)
{
	if (!mode || ! wielded) return ::query_name(mode, who);
	return ::query_name(mode, who) + " ("+wielded+")";
}
#endif

varargs string
query_short(int mode, object who)
{
string s;
	if (!wielded) return ::query_short(mode, who);
	if (s = ::query_short(mode, who))
		return sprintf("%s (%s)", ::query_short(), wielded);
	return s;
}

varargs string
query_long(string what, object who)
{
string tmp;
	if (class_of_weapon) {
	  switch (bluntness * 10 / class_of_weapon) {
		case    5: tmp = "\nIt's heavily damaged."; break;
		case 3..4: tmp = "\nIt's quite damaged."; break;
		case    2: tmp = "\nIt has taken some damage."; break;
		case    1: tmp = "\nIt's slightly used."; break;
		case    0: tmp = ""; break;
		default  : tmp = "\nIt's almost useless."; break;
	  }
	} else {
		tmp = "";
	}

// Todo (perhaps): different poison descs.
	if (poisoned) return sprintf("%s%s It's coated with icky green stuff.",
		::query_long(what, who), tmp);

// Temporary kludge here... :-)
	return sprintf("%s%s\n[Info: It seems to be a %d-handed %s weapon\
 of type '%s'.]",
		::query_long(what, who),
		tmp, query(WP_TWO_HANDED) ? 2 : 1,
		query(WP_BLUNT) ? "blunt" : "sharp", query(WP_TYPE));
}

/****************************************************************
*								*
*		Then all the set-functions.			*
*								*
****************************************************************/

// NEW!

varargs void
set(int x, mixed value)
{
	switch (x) {
	case WP_TYPE:
		weapon_type = value;
		break;
	case WP_SUBTYPE:
		weapon_subtype = value;
		break;
	case WP_BLUNT:
		_wp_flags |= F_WP_BLUNT;
		break;
	case WP_SHARP:
		_wp_flags &= (~F_WP_BLUNT);
		break;
	case WP_TWO_HANDED:
		_wp_flags |= F_WP_TWO_HANDED;
		break;
	case WP_ONE_HANDED:
		_wp_flags &= (~F_WP_TWO_HANDED);
		break;
	case WP_TROLL_WEAPON:
		_wp_flags |= F_WP_TROLL_WEAPON;
		break;
	case WP_NOT_TROLL_WEAPON:
		_wp_flags &= (~F_WP_TROLL_WEAPON);
		break;
	case WP_STAYS_SHARP:
		_wp_flags |= F_WP_STAYS_SHARP;
		break;
	case WP_GOES_BLUNT:
		_wp_flags &= (~F_WP_STAYS_SHARP);
		break;
	case WP_TROLL_1H:
		_wp_flags |= F_WP_TROLL_1H;
		break;
	case WP_TROLL_2H:
		_wp_flags &= (~F_WP_TROLL_1H);
		break;
	case WP_DAMAGE_TYPE:
		damage_type = value;
		if (IS_WIELDED)
			UPDATE_WEAPON;
// Let's update wielder's damage type as well, if needed.
		break;
	case WP_STR_FACTOR:
		if (value < 0) value = 0;
		else if (value > 100) value = 100;
		_str_factor = value;
		_dex_factor = 100 - value;
		break;
	case WP_DEX_FACTOR:
		if (value < 0) value = 0;
		else if (value > 100) value = 100;
		_dex_factor = value;
		_str_factor = 100 - value;
		break;
	}
}

// Poison coated weapons //Graah 20-May-93
void
set_poison_strength(int x)
{
	if (x > 20 || x < 0) return;
	else poison_str = x;
}
void set_poison_str(int x) { set_poison_strength(x); }

// It's nice to be able to set posioned variable 18-Apr-98 Chopin
void set_poisoned(int x) { poisoned = x; }

void
set_bluntness(int arg)
{
	bluntness = arg; 
	new_value();
//	if (IS_WIELDED) UPDATE_WEAPON;
// Not needed any more. -+ 10-mar-96 Doomdark +-
}
void
set_wc(int x)
{
	if (x < 2) x = 2;
	set_max_curse_level(class_of_weapon = x);
}
void set_hc(int h) { if (h >= -10 && h <= 10) hit_class = h; }

void set_value(int v) { value = orig_value=v; }

void set_hit_func(object o) { hit_func = o; }
void set_wield_func(object o) { wield_func = o; }
void set_unwield_func(object o) { unwield_func = o; }

void set_wielded_by(object ob) { wielded_by = ob; } // 30.11.1992, Frobozz
void set_wielded(string s) { wielded = s; }	// 23-nov-94, Doomdark

void set_read(string str) { read_msg = str; }
void set_info(string i) { info = i; }

/****************************************************************
*								*
*	Now "new" query-a-like-functions; they are used to	*
*	check whether a specific condition is met; names of	*
*	the functions should be clear enough to tell what	*
*	they actually return... :-)
*								*
****************************************************************/

status is_weapon() { return 1; }	/* Is a weapon all right. */
status is_wielded() { return IS_WIELDED; }
status is_used() { return is_wielded(); }
status is_blunt() { return query(WP_BLUNT); }
status is_two_handed() { return query(WP_TWO_HANDED); }
status is_poisoned() { return poisoned; }
status is_troll_weapon() { return query(WP_TROLL_WEAPON); }

/****************************************************************
*								*
*	Then the lfuns (functions called by gamedriver).	*
*								*
****************************************************************/

status get() { return 1; }		/* Can be picked up. */

// NEW:
// Silent; bit 0 -> no messages;
//	   bit 1 -> force
varargs status
drop(status silent)
{
     if (IS_WIELDED) {
	if (!(silent & 2) && IS_CURSED) {
	  if (query_verb() != "quit") {
		if (!(silent & 1)) wielded_by->tell_me(NO_DROP_MESSAGE);
	    return 1;
	  } else set_curse(0, CT_NONE);
	}
	if (wielded_by && wielded_by->unwield(this_object(),silent)) return 1;
    }
    return (wielded = wielded_by = 0);
}

void create_weapon() { }

void
create()
{
string t;
int i;
	poison_str = 2; // Default poison strength.
	if (function_exists("reset_weapon")) _wp_flags |= F_WP_RESET_USED;
	if (sscanf(object_name(this_object()), "%s#%d", t, i) < 2) {
		_wp_flags |= (F_WP_IS_ORIGINAL | F_WP_NOT_CREATED_YET);
		return;
	}
	initFunc = symbol_function("init_weapon", this_object());
	create_weapon();
}

void
reset()
{
	if (_wp_flags & F_WP_RESET_USED)
		this_object() -> reset_weapon();
}

void
init()
{
	if ((_wp_flags & F_WP_NOT_CREATED_YET) && environment()) {
		initFunc = symbol_function("init_weapon", this_object());
		_wp_flags &= (~F_WP_NOT_CREATED_YET);
		this_object()->create_weapon();
	}
	if (initFunc)
		funcall(initFunc);
}

/****************************************************************
*								*
*	Now two functions needed when saving weapons over	*
*	reboots; save_variables and restore_variables.		*
*								*
****************************************************************/

void
restore_variables(mapping m)
{
string s;
	class_of_weapon = m["class_of_weapon"];
	hit_class = m["hit_class"];
	bluntness = m["bluntness"];
	poison_str = m["poison_str"];
	value = m["value"];
	orig_value = m["orig_value"];
	set_weight(m["local_weight"]);
	set_size(m["size"]);
	read_msg = m["read_msg"];
	info = m["info"];
	poisoned = m["poisoned"];
	_wp_flags = m["_wp_flags"];
	damage_type = m["damage_type"];
	weapon_type = m["weapon_type"];
	weapon_subtype = m["weapon_subtype"];
	_str_factor = m["_str_factor"];
	_dex_factor = m["_dex_factor"];
	if (s = m["hit_func"]) {
	s -> load_me_now();
	hit_func = find_object(s);
	}
	if (s = m["wield_func"]) {
	s -> load_me_now();
	 wield_func = find_object(s);
	}
	if (s = m["unwield_func"]) {
	s -> load_me_now();
	unwield_func = find_object(s);
	}
	::restore_variables(m);
}

mapping
save_variables()
{
mapping m;
string s, t;
int i;

	if (!mappingp(m = ::save_variables())) m = ([ ]);
	m += ([
	 "class_of_weapon" : class_of_weapon
	 ,"hit_class" : hit_class
	 ,"bluntness" : bluntness
	 ,"poison_str" : poison_str
	 ,"value" : value
	 ,"orig_value" : orig_value
	 ,"local_weight" : query_weight()
	 ,"size": query_size()
	 , "poisoned" : poisoned
	 , "_wp_flags" : _wp_flags & ~(F_WP_NOT_CREATED_YET | F_WP_IS_ORIGINAL)
	 , "damage_type" : damage_type
	 , "weapon_type" : weapon_type
	 , "weapon_subtype" : weapon_subtype
	 , "_str_factor" : _str_factor
	 , "_dex_factor" : _dex_factor
	]);
// Then, let's only save these if they have any useful value.
	if (read_msg) m["read_msg"]  = read_msg;
	if (info) m["info"]  = info;
// Let's be sure we save filenames of the object, not objectpointer!
	if (hit_func) {
	if (objectp(hit_func)) {
      s = object_name(hit_func);
      if (sscanf(s, "%s#%d", t, i) == 2)
	m["hit_func"]  = t;
      else m["hit_func"]  = s;
	} else if (stringp(hit_func)) m["hit_func"] = hit_func;
	}
	if (wield_func) {
	if (objectp(wield_func)) {
      s = object_name(wield_func);
      if (sscanf(s, "%s#%d", t, i) == 2)
	m["wield_func"]  = t;
      else m["wield_func"]  = s;
	} else if (stringp(wield_func)) m["wield_func"] = wield_func;
	}
	if (unwield_func) {
	if (objectp(unwield_func)) {
      s = object_name(unwield_func);
      if (sscanf(s, "%s#%d", t, i) == 2)
	m["unwield_func"]  = t;
      else m["unwield_func"]  = s;
	} else if (stringp(unwield_func)) m["unwield_func"] = unwield_func;
	}
	return m;
}

/****************************************************************
*								*
*	Now all the commands that players/monsters can use.	*
*	Note that we don't need any add_actions in init; nor	*
*	any checks for the string we get as input since those	*
*	have already been taken care of by /bin/pub/_<cmd>.c.	*
*								*
****************************************************************/

varargs status
wield_cmd(string str, object me, status left)
{
	if (!me && !(me = this_player())) return 0;
	me -> wield(this_object(), 0, left);
	return 1;
}

varargs status
unwield_cmd(string str, object me)
{
object tp;

	if (!me && !(me = this_player())) return 0;
	if (!str && !wielded) me -> tell_me("You are not wielding a weapon.");
	else me -> unwield(this_object(), 0);
	return 1;
}

varargs status
read_cmd(string str, object me)
{
	if (!me && !(me = this_player())) return 0;
	if (read_msg) me -> tell_me(read_msg);
	else return 0;
	return 1;
}

/****************************************************************
*								*
*		And then all the rest functions.		*
*								*
****************************************************************/

// New functions; are used by /lib/living to set the weapon
// wielded/not wielded.
// You can use the silent-mode; it might be bit more efficient.
// Returns 1 if it could be wielded/removed, 0 otherwise.
// If you use this, remember also to set living's values accordingly.

// NEW!
// arg 3: string-suffix for short desc ("(wielded in left hand)" and such)
// arg 4: closure from living.c which expects 3 arguments; the base wc,
//	the strength and dexterity modifiers (in percents, sum should
//	be 100). It returns the actual wc this weapon has, when it's wielded
//	by the wielder (arg #1 for this function).

varargs status
wield_me(object who, status silent, string where, closure wc_func)
{
object ob;
	if (wield_func && wield_func -> wield(this_object(), who))
		return 0;	// Can't be wielded by us.
// Other checks are taken care of in 'wield' in /obj/living.
// You should remember that this_player may not be ok... *shrug*

	if (IS_WIELDED && wielded_by == who) {
	    if (!silent) 
	      who -> tell_me(sprintf("You are already wielding the %s!", query_name()));
		return 0;
	}
	wielded_by = who;
// NPCs should perhaps use some dummy formula? Or perhaps they oughta use
// the same wc-system?
	if (!wc_func) {
		_wc_formula = (closure) who->query(LIV_WC_FORMULA);
	} else {
		_wc_formula = wc_func;
	}
	if (stringp(where)) {
		_wp_flags &= (~F_WP_LEFT_WIELDED);
		wielded = where;
	} else {
		_wp_flags |= F_WP_LEFT_WIELDED;
		wielded = "wielded";
	}
	return 1;
}

varargs status
unwield_me(object who, status silent)
{
	if (!who) return 0;
	if (unwield_func && unwield_func -> unwield(this_object(), who))
		return 0;	// Can't be unwielded by us.

	if (!IS_WIELDED) {
		if (!silent) who -> tell_me("You are not wielding it!");
		return 1;
	}
	if (IS_CURSED && query_verb() != "quit") {

// Let's check if we are quitting; if so, let's unwield it...
// This way this weapon won't get destructed... *grin*

		if (!silent) who -> tell_me(NO_REMOVE_MESSAGE);
		return 0;
	}
	wielded_by = wielded = _wc_formula = 0;
	_wp_flags &= (~F_WP_LEFT_WIELDED);
	return 1;
}

// This function gets called when the curse is changed.
void
update_curse()
{
// Not needed any more. -+ 10-mar-96 Doomdark +-
//	if (IS_WIELDED) UPDATE_WEAPON;
}

// every bluntness takes value down 5% from the original
void
new_value()
{
    if((value = orig_value - bluntness * orig_value / 20) < orig_value / 10)
	value = orig_value / 10;
}

status
wear_out()
{
	if (!wielded_by) return 1;	// Just to be sure.
// This used to be 100, too often //Frobozz
	if ((random(200) == 1) && !(_wp_flags & (F_WP_BLUNT | F_WP_STAYS_SHARP))
	  && interactive(wielded_by)) {
		bluntness++;
		if ((class_of_weapon - bluntness) < 3) {
		  set_curse(0, CT_NONE);	// Just to be sure.
		  wielded_by -> tell_me("CRACK! Your weapon broke!");
		  wielded_by -> unwield(this_object(), 1);
		  destruct(this_object());
		  return 0;
		} else {
// Not needed any more. -+ 10-mar-96 Doomdark +-
//			UPDATE_WEAPON;
		}
		if (wielded_by) wielded_by -> tell_me(
			"Your weapon seems to have taken some damage.");
		new_value();
	}
	return 1;
}

status
hit(object attacker)
{
	if (wear_out() && hit_func) return (status)
		hit_func -> weapon_hit(attacker, wielded_by);
	return 0;
}

/* This function gets called only when we successfully hit our opponent.
   It doesn't need to return anything. //Graah
 */

void
extra_hit(object attacker)
{
int dmg;
string s, t, u;
// Poisoned weapons
    if (!attacker) return;
    if (poisoned > 0) {
	poisoned--;
	if (!(s = (string) attacker->query_name(0, wielded_by)))
		s = "Someone";
	if ((int)attacker -> query_level() < random(20 + poison_str)) {
		dmg = (int) attacker -> query_condition(C_POISONED);
		if (!intp(dmg) || (dmg < MAX_POISON_DURATION))
		  if (!attacker->add_condition(C_POISONED, 1 + random(poison_str))) {
			t = sprintf("The poison doesn't seem to harm %s!", s);
			u = "The poison doesn't seem to harm you!";
		  } else {
			t = sprintf("%s is poisoned!", s);
			u = "You are poisoned!";
		  }
		  attacker -> tell_me(u);
		if (wielded_by) wielded_by -> tell_me(t);
	} else {
		attacker -> tell_me("The poison doesn't seem to harm you!");
	    if (wielded_by) wielded_by -> tell_me(sprintf(
"The poison doesn't seem to harm %s.", s));
	}

	if (!poisoned && wielded_by) wielded_by -> tell_me(
		"The poison has been worn out from the weapon.");
    }
}

int
clean_up(int refs)
{
	if (!refs) return 0;
	if (refs > 1 || environment() || first_inventory()) return 1;
	destruct(this_object());
	return 0;
}

/****************************************************************
*								*
*	Compatibility-section; NOT TO BE USED ANY MORE!		*
*								*
****************************************************************/

// Use "query_hc" instead.
int query_hit_class() { return query_hc(); }

status query_blunt() { return query(WP_BLUNT); }
status query_two_handed() { return query(WP_TWO_HANDED); }
// Use 'is_poisoned' instead.
status query_poisoned() { return poisoned; }
status query_troll_weapon() { return query(WP_TROLL_WEAPON); }

// Use set_curse instead!
int query_cursed() { return -query_curse_level(); }

varargs status remove_me(object who, status silent) { return unwield_me(who, silent); }
varargs status remove_cmd(string str, object me) { return unwield_cmd(str, me); }

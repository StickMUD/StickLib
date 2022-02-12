/****************************************************************
*								*
* Standard armour file. To be inheritted by _every_ armour	*
* in StickMud.							*
*								*
* Version 2.2d	8-May-98					*
*								*
* Last modifications by Doomdark 25-mar-96.			*
* 07-Dec-94 //Graah:						*
* - weight (and size) now in /basic/object/dimensions.c		*
* - and yes, I also fixed saves and restores.			*
* 15-dec-94 Doomdark; fixed curse_level-handling.		*
* - 07 -oct-95 Added support for resistances & stat boosts.	*
* 5-May-98 / Graah: race added: armours that only fit one race. *
*		Use set_race() and query_race().		*
*		Elves and dark elves can use the same armours.	*
*		Half-orcs are treated as "orc".			*
*		Many-raced: set_race("dwarf#orc") etc.		*
* 8-May-98 / Graah: Tired of illegal armour classes. Now even	*
*		Uniques will obey the rules, they are forced.	*
*								*
* NEW:								*
*								*
* - Now armours can be cursed too (just like weapons);		*
*   curse does reduce ac by 1/2 points / curselevel.		*
* - Note that curse_level is added as is (except for dividing	*
*   by two); positive values are bonuses, negative penalties.	*
* - Curse handled by module /basic/curse.c (same as weapons).	*
* - Short description can be modified using "set_worn"; the	*
*   argument is added (in parenthesis) at the end of the	*
*   short description; if argument is an integer-value other	*
*   than 0, string "worn" is used instead.			*
* - New functions; "set_wear_func" and "set_unwear_func" (like	*
*   "set_wield_func" etc. in weapon.c) can be used; they will	*
*   be called when players or monster tries to wear or remove	*
*   this armour. If function returns 1, the action won't be	*
*   done (can't be worn/removed, or armour handles wearing all	*
*   by itself.							*
* - Armours can now offer resistance against some condition, or	*
*   cause a condition or boost/drain a stat.			*
* - Now create_armour isn't called for blueprint objects; unless*
*   they are moved to environment() of some living object. This	*
*   may result in some odd compatibility problems... :-/	*
****************************************************************/

// Only Val may use type "free magic". Guild owners that want to use it,
// ask (co)admins //Frobozz

#define MAXLIST ([\
    "armour":5, "helmet":2, "leggings":2, "shield":2,\
    "amulet":2, "belt":1,   "boot":1,     "bracers":2,\
    "cloak":2,  "gloves":1, "mask":1,     "ring":2,\
    "free magic":6\
    ])

#define NO_DROP_MESSAGE "Your armour refuses to be dropped."
#define NO_REMOVE_MESSAGE "Your armour doesn't want to be removed."
#define	IS_WORN	(worn&&objectp(worn_by)&&environment()==worn_by)
#define	UPDATE_ARMOUR worn_by->update_armour(this_object(),query_ac(),type);

#define	F_ARM_RESET_USED	1
#define	F_ARM_INIT_USED		2
#define	F_ARM_ORIGINAL		4
#define	F_ARM_NOT_CREATED	8

#include <armour_defs.h>
#include <conditions.h>
#include <curse.h>
#include <daemons.h>
#include <living_defs.h>
#include <stats.h>

inherit "/basic/curse";	// Curse.c inherits /basic/id.c too
inherit "/basic/object/dimensions";

// Prototypes:

private void check_armour();
varargs status wear_me(object who, status silent, string where);
varargs status unwear_me(object who, status silent);
int query_ac();

/****************************************************************
*								*
*			 Global variables:			*
*								*
****************************************************************/

int orig_value, value;	// Original and current value of this armour
int ac;			// How well this armour protects us.
int damage;		// How damaged this armour is?
string type;		// Type of the armour; "helmet", "ring" etc.
string read_msg;	// If we have some text written on us.
string info;		// Some special info about the armour...
static string worn;	// Are we worn by someone/something?
// 0 -> no, string -> yes; usually "worn".
static object worn_by;	// Who are we worn by?
object wear_func, unwear_func;
// Objects in which functions "wear" and "unwear"
// are to be called when a player or monster tries to
// wear or remove this armour.
int *specialProperties;	// Stat boosts/drains, conditions, resistances to conds.

private int armourFlags; // Internal flags.
string race;		// Armour only fits certain race

/****************************************************************
*								*
*	Set-functions; used to 'set up' the armour:		*
*								*
****************************************************************/

void set_value(int v) { orig_value = value = v; }
void set_ac(int a)	// set type before ac because of checks!
{
    check_armour();
    if (a < 0) set_max_curse_level((ac = 0) + 1);
    else set_max_curse_level(ac = a);
    if (IS_WORN) UPDATE_ARMOUR;
}
void set_type(string t) {
    if (t == "boots") t = "boot";
    type = t; check_armour();
}
void set_info(string i) { info = i; }
void set_read_msg(string s) { read_msg = s; }
void set_read(string s) { read_msg = s; }
void set_worn(string s)
{
    if (!s || stringp(s)) worn = s;
    else worn = "worn";
}
void set_worn_by(object ob) { worn_by = ob; }
varargs void set_wear_func(object o) { wear_func = o; }
varargs void set_unwear_func(object o) { unwear_func = o; }
void set_race(string r) { race = r; }

/****************************************************************
*								*
* Add-function(s); used to add some property for the armour.	*
*								*
****************************************************************/

// Note that we can also remove special properties with this
// function!
int
add_special_property(int p_type, int p_stat1, int p_stat2)
{
    int i, x;
    x = p_stat1 + p_type;
    if (p_type == ARM_SP_DRAIN_STAT && p_stat2 > 0)
	p_stat2 = -p_stat2;
    if (!p_stat2) {
	if (!specialProperties || !(i = sizeof(specialProperties)))
	    return 0;
	while ((i -= 2) >= 0) {
	    if (specialProperties[i] == x) {
		// Now, if we're worn, we have to remove conditions etc!
		if (worn_by) {
		    switch (p_type) {
		    case ARM_SP_BOOST_STAT:
		    case ARM_SP_DRAIN_STAT:
			worn_by->add_stat(p_stat1, 0, 0, this_object());
			break;
		    case ARM_SP_CAUSE_CONDITION:
			worn_by->set_condition(p_stat1, 0, this_object());
			break;
		    case ARM_SP_RESIST_CONDITION:
			worn_by->set_condition(RESISTANCE(p_stat1),
			  0, this_object());
			break;
		    }
		}
		specialProperties[i] = 0;
		specialProperties[i + 1] = 0;
		specialProperties -= ({ 0 });
		if (!sizeof(specialProperties))
		    specialProperties = 0;
		return 1;
	    }
	}
	return 0;	// Special property not found.
    } else if (!specialProperties)
	specialProperties = ({ p_type + p_stat1, p_stat2 });
    else
	specialProperties += ({ p_type + p_stat1, p_stat2 });
    return 1;
}

/****************************************************************
*								*
* Query-functions; used to get information about this armour.	*
*								*
****************************************************************/

string query_type() { return type; }
int query_value() { return value; }
int query_orig_value() { return orig_value; }
int query_ac() { return ac - damage + query_curse_level() / 2; }
int query_damage() { return damage; }   /* How much the armour has taken damage */
string query_info() { return info; }
object query_worn_by() { return worn_by; }
string query_worn() { return worn; }
string query_race() { return race; }

nomask string
query_special_properties()
{
    string ret;

    for (int i = 0; i < sizeof(specialProperties); i++) {
	if (!i) {
	    ret = sprintf("%d", specialProperties[i]);
	} else {
	    ret += sprintf("#%d", specialProperties[i]);
	}
    }

    return ret;
}

// Is there really a need to secure these? This is purely player stuff. /Graah
nomask int *query_specialProperties() { return specialProperties; }

varargs string
query_short(int mode, object who)
{
    string s;
    if (!worn) return ::query_short(mode, who);
    s = ::query_short(mode, who);
    if (s) return sprintf("%s (%s)", ::query_short(mode, who), worn);
    else return 0;
}

varargs string
query_long(string str, object who)
{
    if (!damage) return ::query_long(str, who);
    return sprintf("%s\nIt has taken some damage.",
      ::query_long(str,who));
}

/****************************************************************
*								*
*	Query-like functions; these should be used when we	*
*	just want to know whether a special condition is met.	*
*								*
****************************************************************/

status is_armour() { return 1; }
status is_worn() { return IS_WORN; }
status is_worn_by(object x) { return worn && worn_by == x; }
status is_used() { return is_worn(); }

/****************************************************************
*								*
* Lfuns; functions directly used by gd in some situations.	*
*								*
****************************************************************/

varargs status id(string str, object who) { return str == type || ::id(str, who); }
/* Let's also allow players to use generic names (= armour_type). */

status get() { return 1; }	/* Can be picked up ok. */

// New; silent-flags:
// bit 0 -> silent mode
// bit 1 -> force
status
drop(status silent) /* Can only be called if not cursed (or when quitting). */
{
    if (IS_WORN) {
	if (!(silent & 2) && IS_CURSED) {
	    if (query_verb() != "quit") {
		if (!(silent & 1)) worn_by -> tell_me(NO_DROP_MESSAGE);
		return 1;
	    } else set_curse(0, CT_NONE);
	}
	if (worn_by && worn_by -> unwear(this_object(), silent)) return 1;
    }
    return (worn = worn_by = 0);
}

void create_armour() { }

void
reset(int x)
{
    string foo;
    int bar;
    if (!x) {
	type = "armour";
	if (function_exists("init_armour")) armourFlags |= F_ARM_INIT_USED;
	if (function_exists("reset_armour")) armourFlags |= F_ARM_RESET_USED;
	if (sscanf(object_name(this_object()), "%s#%s", foo, bar) == 2) {
	    create_armour();
	} else armourFlags |= (F_ARM_ORIGINAL | F_ARM_NOT_CREATED);
    } else if (armourFlags & F_ARM_RESET_USED)
	this_object() -> reset_armour();
}


void
init()
{
    // In case we use the blueprint object, we need to call create_weapon;
    // normally we don't call it as only few blueprints are ever used! -+ Doomdark +-
    if (armourFlags & F_ARM_NOT_CREATED) {
	armourFlags &= (~F_ARM_NOT_CREATED);
	this_object()->create_armour();
    }
    if (armourFlags & F_ARM_INIT_USED)
	this_object() -> init_armour();
}

/****************************************************************
*								*
*	Now all the commands that players/monsters can use.	*
*								*
****************************************************************/

varargs status
wear_cmd(string str, object me)
{
    if (!me && !(me = this_player())) return 0;
    me->wear(this_object(), 0);

    // Let's update our inventory on GMCP clients too.
    if (me->query(LIV_IS_PLAYER) && me->query_env("gmcp")) {
        TELOPT_D->send_char_items_update(me, this_object());
    }

    return 1;
}

varargs status
unwear_cmd(string s, object who)
{
    if (objectp(who))
	who -> unwear(this_object(), 0);
    else this_player() -> unwear(this_object(), 0);

    // Let's update our inventory on GMCP clients too.
    if (who && who->query(LIV_IS_PLAYER) && who->query_env("gmcp")) {
        TELOPT_D->send_char_items_update(who, this_object());
    }

    return 1;
}

varargs status
read_cmd(string s, object who)
{
    if (!who) who = this_player();
    who -> tell_me(read_msg ? read_msg :
      sprintf("You can't see any writing on the %s.", query_name(0, who)));
    return 1;
}

/****************************************************************
*								*
* Now 2 functions needed when saving armours over reboot.	*
*								*
*****************************************************************/

mapping
save_variables()
{
    mapping m;
    string s, t, i;
    if (!mappingp(m = ::save_variables())) m = ([]);
    m += ([
      "type"			: type
      ,"weight"		: query_weight()
      ,"size"			: query_size()
      ,"orig_value"		: orig_value
      ,"value"		: value
      ,"ac"			: ac
      ,"damage"		: damage,
      "specialProperties"	: specialProperties
    ]);
    // Let's save following variables only if they have a value.
    if (info) m["info"] = info;
    if (read_msg) m["read_msg"] = read_msg;
    if (race) m["race"] = race;
    // Now, let's save filenames of objects having their special "wear" and
    // "unwear" functions (if this armour has those).
    // Let's be sure we save filenames of the objects, not objectpointers!
    if (wear_func) {
	if (objectp(wear_func)) {
	    s = object_name(wear_func);
	    if (sscanf(s, "%s#%d", t, i) == 2)
		m["wear_func"]  = t;
	    else m["wear_func"]  = s;
	} else if (stringp(wear_func)) m["wear_func"] = wear_func;
    }
    if (unwear_func) {
	if (objectp(unwear_func)) {
	    s = object_name(unwear_func);
	    if (sscanf(s, "%s#%d", t, i) == 2)
		m["unwear_func"]  = t;
	    else m["unwear_func"]  = s;
	} else if (stringp(unwear_func)) m["unwear_func"] = unwear_func;
    }
    return m;
}

void
restore_variables(mapping m)
{
    string s;
    info = m["info"];
    read_msg = m["read_msg"];
    type = m["type"];
    set_weight(m["weight"]);
    set_size(m["size"]);
    orig_value = m["orig_value"];
    value = m["value"];
    ac = m["ac"];
    damage = m["damage"];
    race = m["race"];
    specialProperties = m["specialProperties"];
    if (s = m["wear_func"]) {
	s -> load_me_now();
	wear_func = find_object(s);
    }
    if (s = m["unwear_func"]) {
	s -> load_me_now();
	unwear_func = find_object(s);
    }
    ::restore_variables(m);
}

/****************************************************************
*								*
*		And then rest of the functions.			*
*								*
*****************************************************************/

varargs status
wear_me(object who, status silent, string where)
{
    int i, x;
    string r, *rcs;

    if (!who) return 0;

    if (IS_WORN && worn_by == who) {
	if (!silent)who->tell_me(sprintf(
		"You are already wearing the %s!", query_name(0, who)));
	return 0;
    }

    if (race)
    {
	r = (string)who->query_race();
	if (r == "dark elf") r = "elf";
	else if (r == "half-orc") r = "orc";
	else if (!r) r = "human";

	// Test all races this thing fits
	rcs = explode(race, "#");
	for (i = sizeof(rcs) - 1, x = 0; i >= 0; i--)
	{
	    if (rcs[i] == "dark elf") rcs[i] = "elf";
	    else if (rcs[i] == "half-orc") rcs[i] = "orc";
	    if (rcs[i] == r) x++;
	    // Prepare to desc:
	    else if (rcs[i] == "elf") rcs[i] = "elves";
	    else if (rcs[i] == "dwarf") rcs[i] == "dwarves";
	    else rcs[i] += "s";
	}

	if (!x)
	{
	    who->tell_me(sprintf("You try and try, but it doesn fit. \
It seems that %s only fits on %s!",
		query_name(0, who),
		implode(rcs, ", ", " and ")));
	    return 0;
	}
    }

    if (wear_func && wear_func -> wear(this_object(), who))
	return 0;	// Can't be worn by us.

    worn_by = who;
    if (stringp(where))
	worn = where;
    else worn = "worn";
    // So, now we can set other postfixes to short_desc than plain "(worn)".

    // Now. Let's add special property effects!
    if (!specialProperties)
	return 1;
    i = sizeof(specialProperties);
    while ((i -= 2) >= 0) {
	x = specialProperties[i];
	if (!intp(x)) continue;
	if (x & ARM_SP_BOOST_STAT) {
	    x &= (~ARM_SP_BOOST_STAT);
	    worn_by->add_stat(x, specialProperties[i + 1], -1, this_object());
	} else if (x & ARM_SP_DRAIN_STAT) {
	    x &= (~ARM_SP_DRAIN_STAT);
	    worn_by->add_stat(x, specialProperties[i + 1], -1, this_object());
	} else if (x & ARM_SP_CAUSE_CONDITION) {
	    x &= (~ARM_SP_CAUSE_CONDITION);
	    worn_by->set_condition(x, -1, this_object());
	} else if (x & ARM_SP_RESIST_CONDITION) {
	    x &= (~ARM_SP_RESIST_CONDITION);
	    worn_by->set_condition(RESISTANCE(x), -1, this_object());
	}
    }

    return 1;
}

varargs status
unwear_me(object who, status silent)
{
    int i, x;

    if (!who) return 0;

    if (!IS_WORN) {
	if (!silent) who ->tell_me("You are not wearing it!");
	return 0;
    }
    if (IS_CURSED && query_verb() != "quit") {
	if (!silent) who -> tell_me(NO_REMOVE_MESSAGE);
	return 0;
    }
    if (unwear_func && unwear_func->unwear(this_object(), who))
	return 0;

    // Now we'll have to remove special property effects...
    if (specialProperties && (i = sizeof(specialProperties)))
	while ((i -= 2) >= 0) {
	    x = specialProperties[i];
	    if (x & ARM_SP_BOOST_STAT) {
		x &= (~ARM_SP_BOOST_STAT);
		worn_by->add_stat(x, 0, 0, this_object());
	    } else if (x & ARM_SP_DRAIN_STAT) {
		x &= (~ARM_SP_DRAIN_STAT);
		worn_by->add_stat(x, 0, 0, this_object());
	    } else if (x & ARM_SP_CAUSE_CONDITION) {
		x &= (~ARM_SP_CAUSE_CONDITION);
		worn_by->set_condition(x, 0, this_object());
	    } else if (x & ARM_SP_RESIST_CONDITION) {
		x &= (~ARM_SP_RESIST_CONDITION);
		worn_by->set_condition(RESISTANCE(x), 0, this_object());
	    }
	}

    worn_by = worn = 0;
    return 1;
}

// This function gets called when the curse is changed.
void update_curse() { if (IS_WORN) UPDATE_ARMOUR; }

// Reduces the ac with 'amount' and destructs the armour if total ac < 0.
// Call with negative value (query_damage()) to fix the armour. //Elena
status
add_damage(int amount)
{
    if ((damage += amount) < 0) damage = 0;
    if(query_ac() < 0) {
	if (IS_WORN) worn_by->tell_me(
	      sprintf("The %s was destroyed!", query_name(0, worn_by)));
	set_curse(0, CT_NONE); /* So we can drop it ok. */
	drop(3);
	destruct(this_object());
	return 1;
    }
    value = orig_value - orig_value * damage / ac;
    if (value < orig_value / 10)
	value = orig_value/10;
    if (IS_WORN) UPDATE_ARMOUR;
    return 1;
}

private void
check_armour()
{
    if (ac <= 0) return;
    if (type && (ac > MAXLIST[type]))
    {
	ac = MAXLIST[type];

	/* Don't even bother to log. No one checked that log anyway. /Graah */
	/* log_file("BAD_ARMOUR", ctime(time()) + " type: " + type
	   + ", ac: " + ac + ", name(" + query_name(0, this_object())
	   + ") creator(" + creator(this_object())+ ")\n");
	   */
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
*		Compatibility section...			*
*								*
*		DO _NOT_ USE ANYMORE!!!				*
*								*
****************************************************************/

// Use 'unwear_me' instead.
varargs status
remove_me(object who, status silent, string s) { return unwear_me(who,silent,s);}

// Use 'unwear_cmd' instead... Or 'ob -> unwear(x)'.
varargs status remove_cmd(string str,object who) { return unwear_cmd(str,who); }

// Don't use this one any more...
status remove_armour(string str) { return unwear_cmd(str); }

// Use 'wear_cmd' instead.
varargs status wear(string s) { return wear_cmd(s); }

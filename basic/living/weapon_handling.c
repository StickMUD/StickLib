/****************************************************************
* The part of the /obj/living that handles weapons; wielding	*
* and removing them.						*
*								*
* v1.3, 22-mar-96						*
*								*
* Graah 31-jul-94. Added query_race_natural_wc().		*
* Graah 5-aug-94. Trolls can only wield clubs.			*
* Doomdark 21-sep-94. Added "update_weapon"; used when some	*
*	attribute of the wielded weapon changes. Also, added	*
*	attribute damage_type of the weapon.			*
* Doomdark 04-mar-95: Changed "query_race_natural_wc" to	*
*			"query_race_stat(RACE_NATURAL_WC)	*
* Graah 20-Apr-95: Troll weapon check now works with new weapons...
* Doomdark 02-aug-95: Now uses new features added by weapon.c	*
* Doomdark 10-mar-96: Now uses query_wc() dynamically, to check	*
*	bonuses from str & dex-boosts; also wc depends on	*
*	str & dex of the wielder otherwise.			*
* Doomdark 22-mar-96: Now allows wielding 2 1-handed weapons	*
*	simultaneously.						*
* Doomdark 08-apr-96: Now passes 3 useful closures for guild	*
*	objects on wield hooks.					*
* Doomdark 13-dec-96: Fixed a bug that allowed wielding 2-h	*
*	weapon and a 1-handed weapon in left hand.		*
* Chopin 17-dec-97: query_wc() now returns weapon_class		*
*	instead of weapon's wc if we are an npc.		*
* Graah  24-Apr-98: NPC/monster wc won't drop by wielding       *
*								*
****************************************************************/

#include <weapon_defs.h>
#include <auto_skills.h>
#include <skills_defs.h>

#include "/basic/weapon/weapon_private.h"

static object name_of_weapon, left_weapon;
static int weapon_class;
static int hit_class, left_hit_class;
static int damage_type, left_damage_type;
static int _weapon_flags, _left_weapon_flags;

static closure _query_wc, _left_query_wc;

static int orig_wc; // For NPCs

// The following is merely for debugging (and when this module is compiled alone);
// /obj/living has these variables (and functions).

#ifndef LIVING_C

#include "/basic/living/living_private.h"
#include <weapon_defs.h>
#include <living_defs.h>

status dead;
string cap_name;
string race;

static int liv_Flags;

static closure HitFunc, ExtraHitFunc;
static closure LeftHitFunc, LeftExtraHitFunc;

static mapping GuildHooks;
static int GuildFlags;

#include <race.h>
#include <guild.h>

string Possessive();
varargs void tell_me(string txt, mixed excl, status no_wrap);
object is_wearing(mixed x);
int query_race_stat(int x);
varargs string query_name(int mode, object who);
string query_race();
mixed query(int x);
#endif

#ifndef INVIS_LEVEL
int query_invis();
#define	INVIS_LEVEL query_invis()
#endif

varargs int
query_wc(status left)
{
    int i;

    if (liv_Flags & F_LIV_IS_NPC)
    {
	// Dunno, but the above is wrong in any case. /Graah 24-Apr-98
	if (name_of_weapon) i = funcall(_query_wc);
	else i = 0;
	return (weapon_class > i ? weapon_class : i);
    }

    if (left) {
	if (left_weapon)
	    return funcall(_left_query_wc);
	return 0;
    }

    if (name_of_weapon)
	return funcall(_query_wc);
    return weapon_class;
}

varargs int
query_hc(status left)
{
    if (left)
	return left_hit_class;
    return hit_class;
}

varargs int
query_damage_type(status left)
{
    if (left)
	return left_damage_type;
    return damage_type;
}

varargs object
query_weapon(status left)
{
    if (left)
	return left_weapon;
    return name_of_weapon;
}

// Please use "update_weapon" instead!

varargs void
recount_wc(status left)
{
    object w;
    int i;

    // Remember monster's/NPC's wc
    if (liv_Flags & F_LIV_IS_NPC) orig_wc = weapon_class;

    if (left) {
	if (left_weapon) {
	    left_hit_class = (int) left_weapon -> query_hc();
	    left_damage_type = (int) left_weapon -> query(WP_DAMAGE_TYPE);
	} else {
	    left_damage_type = 0;	// Hmmm... DT_BARE_HANDED???
	    left_hit_class = _left_query_wc = LeftHitFunc =
	    LeftExtraHitFunc = 0;
	}
    }

    if (name_of_weapon) {
	hit_class = (int) name_of_weapon -> query_hc();
	damage_type = (int) name_of_weapon -> query(WP_DAMAGE_TYPE);
    } else {
	weapon_class = query_race_stat(RACE_NATURAL_WC);
	damage_type = 0;	// Hmmm... DT_BARE_HANDED???
	hit_class = _query_wc = HitFunc = ExtraHitFunc = 0;
    }

    // Would we be worse with the weapon?
    if ((liv_Flags & F_LIV_IS_NPC) && (weapon_class < orig_wc))
	weapon_class = orig_wc;
}

varargs status
unwield(object w, status silent)
{
    closure foo;

    if (liv_Flags & F_LIV_IS_NPC) orig_wc = weapon_class;

    if (!w) return 1;
    if (w == name_of_weapon) {
	if (!w -> unwield_me(this_object(), silent))
	    return 1;
	name_of_weapon = hit_class = damage_type = _query_wc
	= HitFunc = ExtraHitFunc = 0;
	if (!silent) {
	    tell_me("You remove your wielded weapon.");
	    if (!INVIS_LEVEL && environment())
		environment() -> tell_here(sprintf(
		    "%s removes %s wielded weapon.", query_name(), Possessive()), this_object());
	}
    } else if (w == left_weapon) {
	if (!w -> unwield_me(this_object(), silent))
	    return 1;
	left_weapon = left_hit_class = left_damage_type = _left_query_wc
	= LeftHitFunc = LeftExtraHitFunc = 0;
	if (!silent) {
	    tell_me("You remove the weapon you wielded in the left hand.");
	    if (!INVIS_LEVEL && environment())
		environment() -> tell_here(sprintf(
		    "%s removes %s left-wielded weapon.", query_name(), Possessive()), this_object());
	}
    }

    // Let's inform the guild too, if it wants to know about this...
    // (those guilds that allow leftwield, certainly should)
    if ((GuildFlags & (1 << G_HOOK_REMOVE_WEAPON))
      && (foo = GuildHooks[G_HOOK_REMOVE_WEAPON])) {
	funcall(foo, this_object(), w, silent);
    }

    return 0;
}

// Don't use stop_wielding() any more. Use 'unwield(ob, silent)' instead;

varargs status
stop_wielding(status left)
{
    status tmp;
    return !unwield(name_of_weapon, 1);
}

varargs status
wield(object w, status silent, status left)
{
    string s;
    int new_wp_flags;
    closure foo, foo2;

    if (!w) return 1;

    if (name_of_weapon == w || left_weapon == w) {
	if (!silent) tell_me("You are already wielding it!");
	return 1;
    }
    new_wp_flags = (int) w->query(WP_FLAGS);
    if ((new_wp_flags & F_WP_TROLL_1H) && race == "troll")
	new_wp_flags &= (~F_WP_TWO_HANDED);

    if (left) {
	// At the moment, guilds need to allow wielding...
	/* Starks sez (Jan 20, 1999) We have no guilds that use this, so let's
	   throw in offhand
	    if (!(GuildFlags & (1 << G_HOOK_LWIELD_WEAPON))
		|| !(foo = GuildHooks[G_HOOK_LWIELD_WEAPON])) {
	*/
	if((int)this_object()->query_skill(SK_OFFHAND) == SKILL_NO_EXIST)
	{
	    tell_me(
	      "You are not skilled enough to wield more than one weapon simultaneously.");
	    return 1;
	}

	if (new_wp_flags & F_WP_TWO_HANDED) {
	    if (!silent)
		tell_me("You can't wield a two-handed weapon in your left hand!");
	    return 1;
	}

	if (is_wearing("shield")) {
	    if (!silent) tell_me("You cannot wield it because you are already \
wearing a shield on your left hand!");
	    return 1;
	}

	if (name_of_weapon && (_weapon_flags & F_WP_TWO_HANDED)) {
	    tell_me("You are wielding a two-handed weapon, so you can't wield \
a weapon in left hand.");
	    return 1;
	}
    } else {
	if ((new_wp_flags & F_WP_TWO_HANDED)) {
	    if (is_wearing("shield")) {
		if (!silent) tell_me("You cannot wield it because it is a \
two-handed weapon, and you are already wearing a shield on your left hand.");
		return 1;
	    }
	    if (left_weapon) {
		if (!silent) tell_me("You cannot wield it because it's a \
two-handed weapon and you are already wielding a weapon on your left hand.");
		return 1;
	    }
	}
    }

    // Trolls are stupid! //Graah
    if ((liv_Flags & F_LIV_IS_PLAYER) && race == "troll")
    {
	if (!(new_wp_flags & F_WP_TROLL_WEAPON)) {
	    s = (string) w->query(WP_TYPE);
	    if (s != "club" && s != "mace" && s != "duster" &&
	      s != "staff" && !w->id("scythe")) {
		if (!silent) tell_me(
		      "That weapon is too complex for you to wield!");
		return 1;
	    }
	}
    }

    // If we were already wielding a weapon there, we'll first unwield it
    // ... If we can.
    if (left) {
	if (left_weapon && unwield(left_weapon, silent))
	    return 1;
    } else {
	if (name_of_weapon && unwield(name_of_weapon, silent))
	    return 1;
    }

    // If the weapon 'doesn't want to be wielded', we won't wield it.
    // We assume the weapon did indicate the reason why it couldn't be wielded.
    if (new_wp_flags & WP_TWO_HANDED) {
	if (!w -> wield_me(this_object(), silent, "wielded in both hands",
	    query(LIV_WC_FORMULA)))
	    return 1;
    } else if (left) {
	if (!w -> wield_me(this_object(), silent, "wielded in left hand",
	    query(LIV_WC_FORMULA)))
	    return 1;
    } else {
	if (!w -> wield_me(this_object(), silent, 0, query(LIV_WC_FORMULA)))
	    return 1;
    }

    if (left) {
	_left_query_wc = (closure)w->query_closure("query_wc");
	LeftHitFunc = (closure) w->query_closure("hit");
	LeftExtraHitFunc = (closure) w->query_closure("extra_hit");
	// Also, for the time being, guild has to approve the leftwield... :-/
	/* Nope.....not at all, see above: Starks
	    if (!funcall(foo, this_object(), w, _left_query_wc,
			 LeftHitFunc, LeftExtraHitFunc)) {
	      // Alas, we may need to remove if we did 'physically wield' it already
	      _left_query_wc = LeftHitFunc = LeftExtraHitFunc = 0;
	      w->unwield_me(this_object(), 1);
	      return 1;
	    }
	*/
	left_weapon = w;
	_left_weapon_flags = new_wp_flags;
    } else {
	_query_wc = (closure)w->query_closure("query_wc");
	HitFunc = (closure)w->query_closure("hit");
	ExtraHitFunc = (closure)w->query_closure("extra_hit");
	// Guilds might be interested in 'normal' wield too; perhaps priests won't
	// wield sharp weapons and such... ?
	if ((GuildFlags & (1 << G_HOOK_WIELD_WEAPON))
	  && (foo = GuildHooks[G_HOOK_WIELD_WEAPON])
	  && !funcall(foo, this_object(), w,
	    _query_wc, HitFunc, ExtraHitFunc)) {
	    // Alas, we may need to remove if we did 'physically wield' it already
	    HitFunc = ExtraHitFunc = _query_wc = 0;
	    w->unwield_me(this_object(), 1);
	    return 1;
	}
	name_of_weapon = w;
	_weapon_flags = new_wp_flags;
    }

    if (!silent) {
	s = (string) w -> query_name();
	if (left) {
	    tell_me(sprintf("You wield the %s in your left hand.", s));
	    if (!INVIS_LEVEL && environment())
		environment() -> tell_here(sprintf(
		    "%s wields the %s in the left hand.", query_name(), s), this_object());
	} else {
	    tell_me(sprintf("You wield the %s.", s));
	    if (!INVIS_LEVEL && environment())
		environment() -> tell_here(sprintf(
		    "%s wields the %s.", query_name(), s), this_object());
	}
    }

    return 0;
}

// NEW! Second argument isn't really used...
// (except if it's a closure; in which case it replaces the _query_wc!)
varargs status
update_weapon(object wp, mixed wc, int hc, int dt)
{
    if (!wp) wp = previous_object();
    if (wp == name_of_weapon) {
	if (!wc) {
	    hit_class = (int) wp -> query_hc();
	    damage_type = (int) wp -> query(WP_DAMAGE_TYPE);
	} else {
	    if (closurep(wc))
		_query_wc = wc;
	    hit_class = hc;
	    damage_type = dt;
	}
    } else if (wp == left_weapon) {
	if (!wc) {
	    left_hit_class = (int) wp -> query_hc();
	    left_damage_type = (int) wp -> query(WP_DAMAGE_TYPE);
	} else {
	    if (closurep(wc))
		_left_query_wc = wc;
	    left_hit_class = hc;
	    left_damage_type = dt;
	}
    }
    return 1;
}

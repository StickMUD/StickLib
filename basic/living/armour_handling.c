/************************************************************************
*									*
* Module name:								*
*	armour-handling		(/basic/living/armour_handling.c)	*
*									*
* Version:								*
*	2.0c								*
*	25-mar-96							*
*									*
* Last modifications by Doomdark 14-nov-95.				*
* Graah 17-sep-94: Race natural AC, trolls can't wear "armour" armour   *
* Doomdark 22-aug-94: Modified "update_armour" slightly.		*
* Graah 11-Oct-94: Fixed object *query_worn_armour_ac() to int *	*
*		   Amylaar driver noticed that bug :-)!			*
* Doomdark 16-nov-94: Fixed "update_armour"; now it should even work.	*
* Doomdark 04-mar-95: Changed "query_race_natural_ac" to		*
*			query_race_stat(RACE_NATURAL_AC)		*
* Doomdark 06-may-95: Worn armour now only weighs half of normal weight.*
*	Also, replaced few funcs with "query_armour".			*
* Doomdark 12-may-95: Removing is prevented, if player couldn't carry	*
*	armour, as this sets its weight back to normal.			*
* Doomdark 15-may-95: Added level bonus for players. I's now 1% / level	*
*	for levels 20 - 29, and 2% / level from thereon.		*
* Doomdark 25-mar-96: Now checks if we have a left-wielded weapon...	*
* Graah    24-Apr-98: NPC's AC must not be reduced by wearing armor!    *
*									*
* Description:								*
*									*
* The part of the /obj/living that handles armours; wearing and		*
* removing them.							*
*									*
************************************************************************/

static object	*worn_armour;
static int	*worn_armour_ac;
static string	*worn_armour_type;
static int	*worn_armour_weight;	// NEW!

static int	armour_class;

static int	orig_ac; // For NPCs

// The following is merely for debugging or when compiling this module alone;
// /obj/living has these variables and functions.

#ifndef LIVING_C

#include "/basic/living/living_private.h"
#include "/basic/weapon/weapon_private.h"

status	dead;
int level;
static int liv_Flags;
static object left_weapon, name_of_weapon;
static int _weapon_flags;

#include <race.h>

varargs void tell_me(string txt, mixed excl, status no_wrap);
object query_weapon();
varargs string query_name(int mode, object x);
string query_race();
int query_race_stat(int x);
status add_weight(int x, status y);
#endif

#ifndef INVIS_LEVEL
int query_invis();
#define INVIS_LEVEL	query_invis()
#endif

// NEW! Level bonus to ac for players.
// Levels 20 - 29: 1% / level
// Levels 30+	   2% / level
// -+ Doomdark +-
public int query_ac() {
	if (level < 20 || (liv_Flags & F_LIV_IS_NPC))
		return armour_class;
	if (level < 30)
		return armour_class * (81 + level) / 100;
	return armour_class * (51 + level * 2) / 100;
}

mixed *
query_armour(string s)
{
	switch (s) {
	case "list":
		return pointerp(worn_armour) ? worn_armour : ({ });
	case "type":
		return pointerp(worn_armour) ? worn_armour_type : ({ });
	case "ac":
		return pointerp(worn_armour) ? worn_armour_ac : ({ });
	case "weight":
		return pointerp(worn_armour) ? worn_armour_weight : ({ });
	case "all":
		return pointerp(worn_armour) ? ({ worn_armour, worn_armour_type,
		worn_armour_ac, worn_armour_weight }):({ ({ }) });
	}

	return 0;
}

void
recount_ac()
{
  object ob, to;
  int i, size;
  object *temp_armour;
  int *temp_ac;
  string *temp_type;
  int *temp_weight;

  // NPCs don't need recount /Graah
  if (liv_Flags & F_LIV_IS_NPC) return;

  armour_class = query_race_stat(RACE_NATURAL_AC);
  i = 0;
  to = this_object();
  if (!worn_armour || !(size = sizeof(worn_armour))) return;
  do {
    if (!(ob = worn_armour[i]) || environment(ob) != to) {
      if (!i) {
	temp_armour = ({});
	temp_ac = ({});
	temp_type = ({});
	temp_weight = ({});
      } else {
	temp_armour = worn_armour[0..i-1];
	temp_ac = worn_armour_ac[0..i-1];
	temp_type = worn_armour_type[0..i-1];
	temp_weight = worn_armour_weight[0..i-1];
      }
      while (++i < size)
	if ((ob = worn_armour[i]) && environment(ob) == to) {
	  temp_armour += ({ worn_armour[i] });
	  temp_ac += ({ worn_armour_ac[i] });
	  temp_type += ({ worn_armour_type[i] });
	  temp_weight += ({ worn_armour_weight[i] });
	  if (worn_armour_type[i] == "shield")
	    armour_class += 3 * worn_armour_ac[i];
	  else
	    armour_class += worn_armour_ac[i];
	}
    }
    else {
      if (worn_armour_type[i] == "shield")
	armour_class += 3 * worn_armour_ac[i];
      else armour_class += worn_armour_ac[i];
    }
  } while (++i < size);
  if (temp_armour) {
    if (!sizeof(temp_armour))
      worn_armour = worn_armour_ac = worn_armour_type = worn_armour_weight = 0;
    else {
      worn_armour = temp_armour;
      worn_armour_ac = temp_ac;
      worn_armour_type = temp_type;
      worn_armour_weight = temp_weight;
    }
  }
}

//	So, are we wearing:
// a) a specific object (given as the argument),
// b) an armour of type 'type' (given as the argument),
// c) or any armour at all (no argument).

// Return the object in question if so (or, in case c, first worn object).

object
is_wearing(mixed type)
{
int i;
	if (!pointerp(worn_armour)) return 0;
	if (!type) return sizeof(worn_armour) ? worn_armour[0] : 0;
	if (objectp(type)) return
		(member(worn_armour, (object)type) < 0)
		? 0 : (object) type;
	return
		((i = member(worn_armour_type, (string)type)) < 0)
		? 0 : worn_armour[i];
}

// Wear some armour.

varargs object
wear(object armour, status silent)
{
  string tmp_type, n;
  string n2;
  int tmp_ac, tmp_weight;
  object old;

  recount_ac();	// Let's check armours are worn ok.

  if (liv_Flags & F_LIV_IS_NPC) orig_ac = armour_class;

  if (is_wearing(armour)) {
    if (!silent) tell_me("You are already wearing it!");
    return armour;
  }

  if (!(tmp_type = (string) armour -> query_type()))
    tmp_type = "armour";

  if (old = is_wearing(tmp_type)) {
	if (!silent) {
		n = (string) armour->query_name(0, this_object());

		n2 = (string) old->query_name(0, this_object());
		if (!n2) n2 = "<invisible>";
		if (!n) n = "<invisible>";
		tell_me(sprintf(
"You can't wear the %s because you are already wearing %s %s (the %s).",
n,
(tmp_type=="armour"?"an":"a"), tmp_type,
n2));
	}
    return armour;
  }

  /* Troll limits //Graah
     TODO: Armour size!!! */
  if (tmp_type == "armour" && query_race() == "troll") {
    if (!silent)
      tell_me(sprintf("Sorry, but %s is too small for you.",
		      (string)armour->query_name(0, this_object())));
    return armour;
  }

  if (tmp_type == "shield") {
    if (name_of_weapon && environment(name_of_weapon) == this_object()
	&& (_weapon_flags & F_WP_TWO_HANDED)) { 
      if (!silent) tell_me("You can't wear a shield \
because you are already wielding a two-handed weapon!");
      return name_of_weapon;
    }
    if (left_weapon && environment(left_weapon) == this_object()) {
      if (!silent) tell_me("You can't wear a shield \
because you are already wielding a weapon in your left hand!");
      return left_weapon;
    }
  }
  if (!armour -> wear_me(this_object(), silent))
    return armour;

  // Kludge! To boost shields' ac... -+ Doomdark +-
  if (tmp_type == "shield")
    armour_class += (tmp_ac = (int) armour->query_ac()) * 3;
  else
    armour_class += (tmp_ac = (int) armour -> query_ac());

  tmp_weight = (int) armour->query_weight();

  if (!pointerp(worn_armour)) {
    worn_armour = ({ armour });
    worn_armour_type = ({ tmp_type });
    worn_armour_ac = ({ tmp_ac });
    worn_armour_weight = ({ tmp_weight });
  } else {
    worn_armour += ({ armour });
    worn_armour_type += ({ tmp_type });
    worn_armour_ac += ({ tmp_ac });
    worn_armour_weight += ({ tmp_weight });
  }
  if (!silent) {
    tmp_type = (string) armour -> query_name(0, this_object());
    tell_me(sprintf("You wear the %s.", tmp_type));
    if (!INVIS_LEVEL && (old = environment()))
      old -> tell_here(sprintf("%s wears the %s.",
			       query_name(0, this_object()), tmp_type),
		       this_object());
  }

  // Now it'll only weigh half the normal weight. -+ Doomdark +-
  add_weight(- (tmp_weight / 2), 1);

  // Never reduce NPC ac by wearing armour. /Graah
  if ((liv_Flags & F_LIV_IS_NPC)
      && (armour_class < orig_ac)) armour_class = orig_ac;

  return 0;
}

varargs int
unwear(object armour, status silent)
{
int i;
string s;
	if (!pointerp(worn_armour) || (i = member(worn_armour,
	  armour)) < 0) {
		if (!silent) tell_me("You are not wearing it!");
		return 1;
	}

// Let's set it normal weight... -+ Doomdark +-
// If can't do that, we won't remove it. Simple.

	if (!add_weight(worn_armour_weight[i] / 2, 1)) {
		if (!silent) tell_me("But you can't carry anything\
 more (drop something first or use command 'remove/unwear <armour> and drop it.'");
		return 1;
	}

	if (!worn_armour[i] -> unwear_me(this_object(), silent))
		return 1;

	if (!silent) {
		s = (string) armour -> query_name(0, this_object());
		tell_me(sprintf("You remove the %s.", s));
		if (!INVIS_LEVEL && environment()) environment() ->
		tell_here(sprintf("%s removes the %s.",
			query_name(0, this_object()), s),this_object());
	}
	worn_armour[i] = 0;
	recount_ac();

	return 0;
}

void
stop_wearing(string name)
{
int i, s;
	if (!pointerp(worn_armour) || !(s = sizeof(worn_armour)))
		return;
	for (i = 0; i < s; i++)
		if (worn_armour[i] -> id(name)) break;
	if (i == s) return;
	unwear(worn_armour[i], dead);
}

varargs void
update_armour(object armour, int aclass, string type, int weight)
{
  int i, ac_old, weight_old;

  if (!pointerp(worn_armour)) return;

  if (liv_Flags & F_LIV_IS_NPC) orig_ac = armour_class;

  if (!armour)
    if (!(armour = previous_object())) return;
    else {
      if ((i = member(worn_armour, armour)) < 0) return;
      ac_old = worn_armour_ac[i];
      weight_old = worn_armour_weight[i];
      worn_armour_ac[i] = (int) armour -> query_ac();
      worn_armour_type[i] = (string) armour -> query_type();
      worn_armour_weight[i] = (int) armour -> query_weight();
      add_weight(worn_armour_weight[i] / 2 - weight_old / 2, 1);
    }
  else {
    if ((i = member(worn_armour, armour)) < 0) return;
    ac_old = worn_armour_ac[i];
    worn_armour_ac[i] = aclass;
    if (type) worn_armour_type[i] = type;
    else worn_armour_type[i] = "armour";
    if (weight) {
      weight_old = worn_armour_weight[i];
      worn_armour_weight[i] = weight;
      add_weight(weight / 2 - weight_old / 2, 1);
    }
  }

  // Let's update the whole armour class in case ac of this armour changed.
  if (ac_old -= worn_armour_ac[i]) {
    if (worn_armour_type[i] == "shield")
      armour_class -= 3 * ac_old;
    else armour_class -= ac_old;
  }

  if ((liv_Flags & F_LIV_IS_NPC) && (armour_class < orig_ac))
    armour_class = orig_ac;
}

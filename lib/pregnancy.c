#include "/sys/object_info.h"

#include <treasure.h>

#include <conditions.h>
#include <invis_levels.h>

// Duration factor, increase if want to slow pregnancy.
#define DUR 1750

#define E environment()
#define HER (string)E->query_possessive()

static string father, father_race;
static int counter;

void
create_treasure()
{
  set_id(({ "pregnancy" }));
  set_name("pregnancy");
  set_invis(IL_TOTALLY_INVISIBLE);
  set_short("Pregnancy");
  counter = -1;
  configure_object(this_object(), OC_HEART_BEAT, 1);
}

int query_counter() { return counter; }
void set_counter(int c) { counter = c; }
string query_father() { return father; }
string query_father_race() { return father; }
void set_father(string n) { father = n; }
void set_father_race(string r) { father_race = r; }

int get() { return 1; }
int drop() { return 1; }

string
query_auto_load()
{
  return "lib/pregnancy:" + father + "#" + father_race + "#" + counter;
}

void
init_arg(string arg)
{
  sscanf(arg, "%s#%s#%d", father, father_race, counter);
}

string
extra_look()
{
  if (counter >= 45) return capitalize(HER) + " stomach looks round.";
}

void
labour(int c)
{
  int x;
  object ob;

  E->set_condition(C_STUNNED, -1);

  if (c == 2)
    {
      E->tell_me("OUUUCH! Your stomach spasms painfully!");
      environment(E)->tell_here((string)E->query_name() +
	" drops to the ground, stunned!", ({ E }));
      // Hurts.
      x = (int)E->query_hp();
      x = ((2 * x) / 10);
      E->add_hp(-(x));
      return;
    }
  else if (c == 6)
    {
      E->tell_me("OUUUCH! Your stomach REALLY HURTS!");
      environment(E)->tell_here((string)E->query_name() +
	" writhes on the floor in pain!", ({ E }));
      // Hurts.
      x = (int)E->query_hp();
      x = ((3 * x) / 10);
      E->add_hp(-(x));
      return;
    }
  else if (c == 8)
    {
      E->tell_me("You are delivering a baby!!!");
      environment(E)->tell_here((string)E->query_name() +
	" is delivering a baby!", ({ E }));
      // Hurts.
      x = (int)E->query_hp();
      x = ((3 * x) / 10);
      E->add_hp(-(x));
      return;
    }
  else if (c < 12) return;

  // Hurts.
  x = (int)E->query_hp();
  x = ((6 * x) / 10);
  E->add_hp(-(x));

  E->tell_me("A baby pops out from you! OUCH!");
  environment(E)->tell_here((string)E->query_name() +
    " delivers a baby!", ({ E }));
  E->tell_me("Baby cries: WAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAHHH!");
  environment(E)->tell_here((string)E->query_name() +
    "'s baby cries: WAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAHHH!",
			    ({ E }));

  ob = clone_object("/lib/baby");
  move_object(ob, E);
  ob->set_stuff(father, father_race,
	       	(string)E->query_real_name(), (string)E->query_race());

  E->set_condition(C_STUNNED, 0);
  destruct(this_object());
}

void
heart_beat()
{
  if ((counter == -1) || !E || !interactive(E)) return;

  counter++;

  if (counter == 6 * DUR || counter == 8 * DUR)
    {
      E->tell_me(
	"\nYou have definitely gained some weight lately.\n\
Your stomach looks somewhat bigger.\n");
    }
  else if (counter == 10 * DUR || counter == 12 * DUR)
    {
      E->tell_me("\nYour stomach looks a lot bigger!\n");
    }
  else if (counter == 14 * DUR || counter == 16 * DUR)
    {
      E->tell_me("\nYour stomach is HUGE!\n");
    }
  else if (counter >= 18 * DUR)
    {
      labour(counter - 18 * DUR);
      return;
    }

  // Random symptoms (rare)

  if (random(1000) > 4) return;

  if (counter < 6 * DUR)
    {
      E->tell_me("You feel sick!");
      E->force_us("puke");
      return;
    }

  if (counter < 12 * DUR)
    {
      E->tell_me("You feel you would like to eat some " +
		({ "oysters", "bean sprouts", "apricots",
		   "Baltic herring", "biscuits", "chipped potatoes",
		   "peas", "pickled cucumbers", "cabbage", "garlic",
		   "ice cream", "lemons", "sardines", "stuffed olives"
		})[random(14)] + "!");
      return;
    }

  switch(random(3))
    {
      case 1:
	E->tell_me("Your legs hurt!");
	break;
      case 2:
	E->tell_me("Someone kicks your stomach! OUCH!");
	break;
      default:
	E->tell_me("You feel " + ({ "sad", "happy", "angry",
		"depressed", "horny", "extremely energic",
		"dirty" })[random(7)] + "!");
	break;
    }
}

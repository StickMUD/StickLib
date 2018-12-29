inherit "/basic/id";

#include <conditions.h>
#include <invis_levels.h>
#include <living_defs.h>

// Growth time, increase if want to slow it down
#define GROWTH 15000

#define E environment()
#define NAME (string)E->query_name()
#define HER (string)E->query_possessive()
#define HIM (string)E->query_objective()

void check_size();

static string father, father_race;
static string mother, mother_race;
static string baby_name;
static string baby_race;
static int counter;
static int gender;

string
get_race()
{
  if (father_race == mother_race)
    return (mother_race ? mother_race : "human");

  switch (father_race)
    {
    case "human": case 0:
      switch(mother_race)
	{
	case "troll": return "half-troll"; break;
	case "half-orc": return "quarter-orc"; break;
	case "dwarf": return "half-dwarf"; break;
	case "elf": return "half-elf"; break;
	case "dark elf": return "half-dark-elf"; break;
	case "hobbit": return "half-hobbit"; break;
	default: return "human"; break;
	}
      break;
    case "troll":
      switch(mother_race)
	{
	case "human": case 0: return "half-troll"; break;
	case "half-orc": return "troll-orc"; break;
	case "dwarf": return "troll-dwarf"; break;
	case "elf": return "troll-elf"; break;
	case "dark elf": return "dark troll-elf"; break;
	case "hobbit": return "troll-hobbit"; break;
	default: return "troll"; break;
	}
      break;
    case "half-orc":
      switch(mother_race)
	{
	case "human": case 0: return "quarter-orc"; break;
	case "troll": return "troll-orc"; break;
	case "dwarf": return "dwarf-orc"; break;
	case "elf": return "elf-orc"; break;
	case "dark elf": return "dark elf-orc"; break;
	case "hobbit": return "orc-hobbit"; break;
	default: return "half-orc"; break;
	}
      break;
    case "dwarf":
      switch(mother_race)
	{
	case "human": case 0: return "half-dwarf"; break;
	case "troll": return "dwarf-troll"; break;
	case "half-orc": return "dwarf-orc"; break;
	case "elf": return "dwarf-elf"; break;
	case "dark elf": return "dark dwarf-elf"; break;
	case "hobbit": return "dwarf-hobbit"; break;
	default: return "dwarf"; break;
	}
      break;
    case "elf":
      switch(mother_race)
	{
	case "human": case 0: return "half-elf"; break;
	case "troll": return "troll-elf"; break;
	case "dwarf": return "dwarf-elf"; break;
	case "half-orc": return "elf-orc"; break;
	case "dark elf": return "dark elf-orc"; break;
	case "hobbit": return "elf-hobbit"; break;
	default: return "elf"; break;
	}
      break;
    case "dark elf":
      switch(mother_race)
	{
	case "human": case 0: return "half-dark-elf"; break;
	case "troll": return "troll-dark-elf"; break;
	case "dwarf": return "dwarf-dark-elf"; break;
	case "half-orc": return "dark elf-orc"; break;
	case "elf": return "gray elf"; break;
	case "hobbit": return "dark elf-hobbit"; break;
	default: return "dark elf"; break;
	}
      break;
    case "hobbit":
      switch(mother_race)
	{
	case "human": case 0: return "half-hobbit"; break;
	case "troll": return "troll-hobbit"; break;
	case "dwarf": return "dwarf-hobbit"; break;
	case "half-orc": return "orc-hobbit"; break;
	case "elf": return "elf-hobbit"; break;
	case "dark elf": return "dark elf-hobbit"; break;
	default: return "hobbit"; break;
	}
      break;
    default: return (mother_race + "-" + father_race);
      break;
    }
}

void
create()
{
  set_name("baby");
  set_id(({ "baby", "child" }));
  set_short("A little baby");
  set_long("This is your little cute baby! You could 'feed' it if you want.");
  check_size();
  counter = -1;
  set_heart_beat(1);
}

void
init()
{
  add_action("feed_cmd", "feed");
}

int query_weight() { return 1; }
int query_gender() { return gender; }
void set_gender(int g) { gender = g; }
int query_counter() { return counter; }
void set_counter(int c) { counter = c; }
string query_father() { return father; }
string query_father_race_race() { return father_race; }
void set_father(string n) { father = n; }
void set_father_race(string r) { father_race = r; }
string query_mother() { return mother; }
string query_mother_race() { return mother_race; }
void set_mother(string n) { mother = n; }
void set_mother_race(string r) { mother_race = r; }
void set_race(string r) { baby_race = r; }
string query_race() { return baby_race; }

// Baby is born!
void
set_stuff(string f, string fr, string m, string mr)
{
  father = f;
  father_race = fr;
  mother = m;
  mother_race = mr;
  set_counter(1);
}

void
check_size()
{
  string r;

  baby_race = get_race();
  set_id(baby_race);
  
  if (counter < 75 * GROWTH)
    {
      if (counter < 5 * GROWTH)
	set_short("A little " + baby_race + " baby");
      else if (counter < 9 * GROWTH)
	set_short("A small " + baby_race + " baby");
      else if (counter < 12 * GROWTH)
	set_short("A " + baby_race + " baby");
      else if (counter < 20 * GROWTH)
	set_short("A large " + baby_race + " baby");
      else if (counter < 30 * GROWTH)
	set_short("A huge " + baby_race + " baby");
      else if (counter < 50 * GROWTH)
	set_short("An ENORMOUS " + baby_race + " baby");
      set_long("This is your own " + baby_race + " baby! \
You can 'feed baby' if you want to.");
      return;
    }

  set_long("This is your own " + baby_race + " child! \
You can 'feed child' if you want to.");

  if (counter < 100 * GROWTH) set_short("A small " + baby_race + " child");
  if (counter < 150 * GROWTH) set_short("A " + baby_race + " child");
  if (counter < 200 * GROWTH) set_short("A big " + baby_race + " child");
  else set_short("An ENORMOUS " + baby_race + " child");
}

int get() { return 1; }
int drop() { return 1; }

string
query_auto_load()
{
  return "lib/baby:" + father + "#" + father_race + "#" +
		       mother + "#" + mother_race + "#" +
		       baby_name + "#" +
		       gender + "#" + counter;
}

void
init_arg(string arg)
{
  sscanf(arg, "%s#%s#%s#%s#%s#%d#%d",
    father, father_race, mother, mother_race, baby_name, gender, counter);
  check_size();
	environment(this_object())->recount_weight();
}

status
feed_cmd(string str)
{
  check_size();

  if (!id(str)) return 0;

  E->tell_me("You feed the baby.");
  environment(E)->tell_here(NAME + " feeds " + HER +
    " " + baby_race + " baby.", ({ E }));
  return 1;
}

void
heart_beat()
{
  string s, t;
  int i;

  if ((counter == -1) || !E || !interactive(E)) return;

  counter++;

  // Child, not baby.
  if (counter >= 50 * GROWTH) return;

  if (random(1000) > 4) return;

  t = "The " + baby_race + " baby ";

  switch(random(9))
    {
      case 0:
	E->tell_me(t + "pukes on you!");
	environment(E)->tell_here(NAME + "'s baby pukes on " + HIM + "!");
	break;
      case 1:
	E->tell_me(t + "shits a HUGE amount of pale brown crap on you!");
	environment(E)->tell_here(NAME + "'s baby shits a HUGE amount \
of pale brown crap upon " + HIM + "!",
		({ E }));
	break;
      case 2:
	E->tell_me(t + "pisses on you!");
	environment(E)->tell_here(NAME + "'s baby pisses upon " + HIM + "!",
		({ E }));
	break;
      case 3:
	E->tell_me(t + "burps loudly!");
	environment(E)->tell_here(NAME + "'s baby burps loudly!",
		({ E }));
	break;
      case 5:
	E->tell_me(t + "cries: \
WAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
	environment(E)->tell_here(NAME + "'s baby cries: \
WAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA", ({ E }));
	break;
      default:
	s = "";
	for (i = 4 + random(18); i >= 0; i--)
	  s += ({ "gug", "aggla", "guggle", "ta ta", "do", "pa", "mummum",
		"blub", "mgbhl", "gagga", "lal", "ka", "baa" })[random(13)] +
		" ";
	E->tell_me(t + "says: " + s);
	environment(E)->tell_here(NAME + "'s baby says: " + s,
		({ E }));
	break;
    }
}

/****************************************************************
*								*
* Standard inheritable room for StickMud.			*
* 								*
* Version 1.43 10-Jan-98				        *
*								*
* Last Modifications by Doomdark 14-feb-96.			*
* 18-Jan-96: extra_move_object(object ob, object to) //Graah	*
* 17-Jul-96: ROOM_ISOLATED flag (takes a player "outside" the	*
*  game, no who list, no tells, no channels...nothing!)		*
*  This automatically prevents shout, where and teleporting.    *
* 10-Jan-98, Doomdark: Added ROOM_SHORT_EXIT_LIST, with which	*
*	you can query exit-list in form "n,e,w"; returns 0 if	*
*	no obvious exits present.				*
*								*
* NEW:								*
*								*
****************************************************************/

#include "/sys/object_info.h"

#include <weatherdesc.h>
#include <room_defs.h>
#include <daemons.h>
#include <player_defs.h>
#include <tell_me.h>
#include <client_defs.h>
#include <npc_defs.h>
#include <treasure_defs.h>

#define	TYPE_MONSTER	1
#define	TYPE_OBJECT	2

#define	INDEX_OBJECT	0
#define INDEX_FNAME	1
#define	INDEX_MSG	2
#define	INDEX_RESETTING	3
#define	INDEX_TYPE	4

private static mapping Exits; // Visible exits of the room.

// Long and short descriptions of the room.
static string short_desc, long_desc, night_long_desc;

// Special items players can examine; usually obvious from room's long description.
private static mapping Items;

// Special commands room has. These don't need add_action.
private static mapping commands;

// Special permanent objects; can't be taken, also, trying to make so
// that these would always be first objects in room's desc.
private static string *permanent_names;
private static object *permanent_objects;

// Monsters and objects room has; usually these will be cloned at reset, if
// a) monster has been killed or b) object has been taken away from this room.

// Structure is: ({ object_or_mon1, object_or_mon2... })
// where object_or_mon1 is either an object, array defining the object.
private static mixed *room_residents;

/* Fact about this room. ex: "no_fight", "no_steal" */
// Old. Is this still being used???
private static mixed property;

// Is this outdoors or indoors room, weather descriptions will be shown
// depending on this value. Values are WD_INDOORS, WD_OUTDOORS, WD_NONE
// Base light level of the room.
private static int light_level;

// Strings made of obvious exits, to be used with "query_long" & "query_short"
private static string short_exit_list, long_exit_list;

private mixed *room_effects;        // NEW!

private static closure ExtraLookFunc;

#define	ROOM_C

#include "/basic/room/set_n_query.c"
#include "/basic/room/tell_here.c"	/* We'll include it, easier to edit... */

// Prototypes; let's order functions to speed things up (I hope):
public void init();
public void heart_beat();

/****************************************************************
*								*
*		Query-functions.				*
*								*
****************************************************************/

// Note that 2 first are the same, but not the 3rd func!
// NEW! Can take an argument; 0 if it's day, non-zero if night.
// This reduces light level... *grin*
varargs int
query_total_light(int x)
{
  // Have to add a kludge for city rooms. *SIGH*
  if (x && (outdoorness & WD_OUTDOORS) && !(outdoorness & WD_CITY)) {
    if (light_level < 1)
      return set_light(0);
    if (light_level < 3)
      return set_light(0) - light_level;
    return set_light(0) - 2;
  }
  return set_light(0);
}

varargs int
query_light(int x)
{
  if (x && (outdoorness & WD_OUTDOORS) && !(outdoorness & WD_CITY)) {
    if (light_level < 1)
      return set_light(0);
    if (light_level < 3)
      return set_light(0) - light_level;
    return set_light(0) - 2;
  }
  return set_light(0);
}

int query_light_level() { return light_level; }

// For converting

// This used to be of type string*...
mapping query_items() { return Items; }

// With this you can query the exit list as a mapping; you should now use
// this function instead of the old "query_dest_dir".
mapping
query_exits()
{
  return Exits;
}

// With this you can check where does exit "dir" lead.
string
query_exit(string dir)
{
  int i;
  if (!mappingp(Exits) || !stringp(dir)) return 0;
  return Exits[dir];
}

mapping
query_commands()
{
  return commands;
}

// Does this room has a special property ?
// The 'property' variable can be both a string and array of strings.
// If no argument is given, return the 'property' variable.

mixed
query_property(string str)
{
  int i;

  if (!str) return property;
  if (!property) return 0;

  if (pointerp(property)) return member(str, property) != -1;
  return str == property;
}


// Bit 0 ($01): Return exits too
// Bit 1 ($02): Skip testing of darkness
// Bit 2 ($04): Test both darkness and brightness of the room!
//                  (and take race into account as well)

varargs string
query_short(int mode, object who)
{
  int i, night;
  string *dirs;

  if (mode & 4) {
    if (!who) who = this_player();
    if (who && (i = (int) who->test_dark2(query_light(),
					  light_level, outdoorness, 0))) {
      if (i < 0) {
	i = -i;
	night = 1;
      }
      // Values are the same as ones used in query_long...
      switch (i) {
      case 1: return "Somewhere";
      case 2: case 4:
	if (outdoorness & WD_INDOORS)
	  return "Dark room";
	if (night && (outdoorness & WD_OUTDOORS))
	  return "Dark place (night)";
	return "Dark place";
      case 3:
	if (night || (outdoorness & (WD_OUTDOORS | WD_CITY))
	    != WD_OUTDOORS) break;
	if (outdoorness & WD_INDOORS)
	  return "Bright room";
	if (!night)
	  return "Bright place (sunlight)";
	return "Bright place";
      }
    }
  } else if (!(mode & 2) && query_total_light() < LT_DIM)
    return (outdoorness & WD_INDOORS) ?
	   "Dark room" : "Dark place";
  
  // Used to skip exits if set(SKIP_EXITS) was used. But, don't think that's
  // really how it should be. -+ Doomdark +-
  if (!(mode & 1) || !Exits) {
    // Let's check if we're using closure here as well:
    if (closurep(short_desc))
      return funcall(short_desc, mode, who, night);
    return short_desc;
  }

  // Next block of code used to be function "update_exit_lists":

  if (!(Flags & F_ROOM_EXIT_LISTS_UPDATED)) {
    if (!Exits || !(sizeof(Exits))) {
      long_exit_list = "No obvious exits.";
      short_exit_list = 0;
    } else {
      dirs = m_indices(Exits);
      if (sizeof(Exits) == 1)
	long_exit_list = sprintf("The only obvious exit is %s.", dirs[0]);
      else long_exit_list = sprintf("The obvious exits are %s.",
				    implode(dirs, ", ", " and "));
      for (i = 0; i < sizeof(Exits); i++)
	switch (dirs[i]) {
	case "north": dirs[i] = "n"; break;
	case "east": dirs[i] = "e"; break;
	case "south": dirs[i] = "s"; break;
	case "west": dirs[i] = "w"; break;
	case "up": dirs[i] = "u"; break;
	case "down": dirs[i] = "d"; break;
	case "northeast": dirs[i] = "ne"; break;
	case "northwest": dirs[i] = "nw"; break;
	case "southeast": dirs[i] = "se"; break;
	case "southwest": dirs[i] = "sw"; break;
	}
      short_exit_list = efun::implode(dirs, ",");
    }
    Flags |= F_ROOM_EXIT_LISTS_UPDATED;
  }

  // End of old "update_exit_lists".

  if (!short_exit_list) {
    if (closurep(short_desc))
      return funcall(short_desc, mode, who, night);
    return short_desc;
  }
  if (!short_desc) return "<void room>";
  // Let's check if it's a closure...
  if (closurep(short_desc))
    return sprintf("%s (%s)", funcall(short_desc, mode, who, night),
		   short_exit_list);
  return sprintf("%s (%s)", short_desc, short_exit_list);
}

varargs string
query_long(string str, object who)
{
  string temp, temp2;
  string s, *dirs;
  object tp;
  int i, night;

  if (!who) who = this_player();

  if (who && (i = (int) who->test_dark2(query_light(), light_level,
					outdoorness))) {
    // Depending on return value, let's "prevent" seeing the room.
    // If return value < 0, it's night time; otherwise day...
    if (i < 0) {
      i = -i;
      night = 1;
    }

    // Check the return values from test_dark2() in living.c; anyway,
    // levels (+/-)1-3 mean we don't see anything, 4-6 that we see
    // something and 7 means full visibility.

    // We may later want to use more values...
    switch (i) {
    case 1:
      return "You can't see anything!";
    case 2:
      return "It is dark.";
    case 3:
      return "It is (too) bright here.";
    case 4:
      return "It is dark.";
    case 5:
      return query_short(2, who);	// Short desc, no exits.
    case 6:
      return query_short(3, who);	// Short desc, exits too.
    }
  }

  if (!str) {
    if (!long_desc) long_desc = "";
    // Now; we might want to use a closure instead of a long description;
    // when executed, this closure would return the long description. So,
    // let's allow it. First 2 arguments for the closure are the same as
    // the ones to quey_long; third is night-flag; it's 0 if it's day now,
    // otherwise 1.
    if (closurep(long_desc))
      temp = funcall(long_desc, str, who, night);
    else if (ExtraLookFunc) {
      if (night && night_long_desc)
	temp = sprintf("%s\n%s", night_long_desc,
		       (string) funcall(ExtraLookFunc, this_object()));
      else
	temp = sprintf("%s\n%s", long_desc,
		       (string) funcall(ExtraLookFunc, this_object()));
      if (temp[<1] == '\n') temp[<1] = ' ';
      // Let's remove possible ending linefeed...
    } else {
      if (night && night_long_desc) temp = night_long_desc;
      else temp =  long_desc;
    }
    if ((outdoorness & WD_OUTDOORS) && who->query(LIV_NOT_BRIEF_WEATHER))
      if ((temp2 = NATURE_D -> query_short_weather()) && sizeof(temp2))
	temp = sprintf("%s\n%s", temp, temp2);

    if (Flags & F_ROOM_EXITS_SKIPPED)
      return temp;

    // Next block of code used to be function "update_exit_lists":

    if (!(Flags & F_ROOM_EXIT_LISTS_UPDATED)) {
      if (!Exits || !sizeof(Exits)) {
	long_exit_list = "No obvious exits.";
	short_exit_list = 0;
      } else {
	dirs = m_indices(Exits);
	if (sizeof(Exits) == 1)
	  long_exit_list = sprintf("The only obvious exit is %s.", dirs[0]);
	else long_exit_list = sprintf("The obvious exits are %s.",
				      implode(dirs, ", ", " and "));
	for (i = 0; i < sizeof(Exits); i++)
	  switch (dirs[i]) {
	  case "north": dirs[i] = "n"; break;
	  case "east": dirs[i] = "e"; break;
	  case "south": dirs[i] = "s"; break;
	  case "west": dirs[i] = "w"; break;
	  case "up": dirs[i] = "u"; break;
	  case "down": dirs[i] = "d"; break;
	  case "northeast": dirs[i] = "ne"; break;
	  case "northwest": dirs[i] = "nw"; break;
	  case "southeast": dirs[i] = "se"; break;
	  case "southwest": dirs[i] = "sw"; break;
	  }
	short_exit_list = efun::implode(dirs, ",");
      }
      Flags |= F_ROOM_EXIT_LISTS_UPDATED;
    }

    return sprintf("%s\n%s", temp, long_exit_list);
  }

  if (!mappingp(Items)) return 0;

  while (temp = Items[str])
    if (stringp(temp) && temp[0] == '=')
      str = temp[1..<1];
    else break;

  if (closurep(temp)) {
    temp = funcall(temp, str, who, night);
  }
  if (!stringp(temp)) return 0;

  return temp;
}

// These should be overridden; added here to speed up call_others
// if func is missing (always cache_miss!!!)
varargs string query_msgin(object x) { return 0; }
varargs string query_msgout(object x) { return 0; }
void set_short(mixed s) { short_desc = s; }
void set_long(mixed s) { long_desc = s; }

// Because of the old kludgy efun "set_light", this isn't too tidy... :-(
void
set_light_level(int x)
{
  int t;

  t = x - light_level;
  light_level = x;
  set_light(t);
}

void
set_exits(mapping x)
{
  if (!mappingp(x)) return;
  Exits = x;
  Flags &= (~F_ROOM_EXIT_LISTS_UPDATED);
}

void
set_items(mapping x)
{
  int s, i, j;
  string *tmp, s2, desc;
  mixed t;

  if (!mappingp(x) || !(s = sizeof(tmp = m_indices(x)))) {
    Items = 0;
    return;
  }

  Items = x;

  for (i = 0; i < s; i++) {
    if (pointerp(t = tmp[i]) && (j = sizeof(t))) {
      s2 = t[0];
      Items[s2] = x[t];
      s2 = "=" + s2;

      while (--j) Items[t[j]] = s2;

      Items = m_delete(Items, t);

    }
  }
}

void
set_commands(mapping x)
{
  int i;
  string *c, *f;

  if (!x) commands = 0;
  // Let's not make commands use closures _yet_ in case room
  // wants to use 'replace_program'! -+ Doomdark 14-feb-96 +-
  else if (mappingp(x))
    commands = x;
#if 0
  {
    commands = x;
    c = m_indices(x);
    f = m_values(x);
    commands = allocate_mapping(sizeof(c));
    while (i < sizeof(c)) {
      if  (stringp(f[i]))
	commands[c[i]] = symbol_function(f[i], this_object());
      else commands[c[i]] = f[i];
      i++;
    }
  }
#endif
}

/****************************************************************
*								*
*		Add-functions.					*
*								*
****************************************************************/

// Can use closure as 2nd argument; should do the moving when called
// and return status; 1 if going to exit succeeded, 0 otherwise.
void
add_exit(string x, mixed y)
{
  if (!y) {
    if (Exits)	
      Exits = m_delete(Exits, x);
  }
  else if (!Exits) Exits = ([ x : y ]);
  else Exits[x] = y;
  Flags &= (~F_ROOM_EXIT_LISTS_UPDATED);
}

// 1st argument can be either a string or array of strings,
// can use closures as 2nd argument; should return description for
// the item (which will get printed to the player).
status
add_item(mixed s, mixed d)
{
  if (!stringp(d) && !closurep(d)) return 0;
  if (stringp(s))
    if (!Items) Items = ([ s : d ]);
    else Items[s] = d;
  else if (pointerp(s)) {
    int x;
    string t;
    if (!Items) Items = ([ (string*) s[0] : d ]);
    else Items[(string*) s[0]] = d;

    x = sizeof(s);
    t = "=" + s[0];

    while (--x) Items[s[x]] = t;
  }
  else return 0;
  return 1;
}

// Add a room-resident monster.
status
add_monster(mixed filename, string appearmsg, int reset_chance)
{
  object m;
  int i;

  // If 1st (and only!) argument is object pointer, it's simply to be added
  // to the list of room-resident objects. This is used in clean_up to make
  // things work more efficiently...

  if (objectp(filename)) {
    if (!room_residents)
      room_residents = ({({ filename, 0, 0, 0, TYPE_MONSTER })});
    else
      room_residents += ({({ filename, 0, 0, 0, TYPE_MONSTER })});
    return 1;
  }

  // This is needed for compatibility...
  if (reset_chance == 1)
    reset_chance = 100;

  if (reset_chance && reset_chance < random(100))
    ;
  else if (pointerp(filename)) {

    // First, let's check whether 1st element is a string; if so, it
    // defines object that we want to use as "base"; usually a monster
    // of some kind. If not, we'll use standard monster object, currently
    // "/lib/npc". It's recommended we don't explicitly define
    // filename, if we wish to use standard monster.

    if (!stringp(filename[0])) {
      m = clone_object(NPC_FILE); // Use std monster.
      i = 0;
    } else {
      m = clone_object(filename[0]); // Use custom monster.
      i = 1;
    }

    if (!m) return 0;

    // Now, we can customize our monster with call_others. Each element of
    // the array consists of function name (1st arg), and optional arguments

    for (; i < sizeof(filename); i++) {
      if (!pointerp(filename[i])) continue;
      if (sizeof(filename[i]) < 2)
	call_other(m, filename[i][0]);
      else
	apply(#'call_other, m, filename[i][0], filename[i][1..<1]);
    }

      } else if (stringp(filename)) {
		if (!(m = clone_object(filename))) return 0;
	} else return 0;

	if (!room_residents) room_residents = ({});

	if (m) move_object(m, this_object());

	room_residents += ({({ m, filename, appearmsg, reset_chance, TYPE_MONSTER })});

	if (m && appearmsg) {
		tell_here(appearmsg, m);
// This is stupid but...
		this_object() -> extra_add_monster(m);
	}
	return 1;
}

// Add a room-resident object.
varargs int
add_object(mixed filename, string appearmsg, status reset_chance)
{
  object m;
  int i;

  if (objectp(filename)) {
    if (!room_residents)
      room_residents = ({({ filename, 0, 0, 0, TYPE_OBJECT })});
	else
      room_residents += ({({ filename, 0, 0, 0, TYPE_OBJECT })});
    return 1;
  }

// This is needed for compatibility...
	if (reset_chance == 1)
		reset_chance = 100;

	if (reset_chance && reset_chance < random(100))
		;
	else if (pointerp(filename)) {

// First, let's check whether 1st element is a string; if so, it
// defines object that we want to use as "base"; usually an object
// of some kind. If not, we'll use standard monster object (NPC_FILE).
// It's recommended we don't explicitly define
// filename, if we wish to use standard object.

		if (!stringp(filename[0])) {
			m = clone_object(TREASURE_FILE);// Use std base object.
			i = 0;
		} else {
			m = clone_object(filename[0]); // Use custom object.
			i = 1;
		}

		if (!m) return 0;

// Now, we can customize our monster with call_others. Each element of
// the array consists of function name (1st arg), and optional arguments

		for (; i < sizeof(filename); i++) {
			if (!pointerp(filename[i])) continue;
			if (sizeof(filename[i]) < 2)
				call_other(m, filename[i][0]);
			else
		apply(#'call_other, m, filename[i][0], filename[i][1..<1]);
		}

	} else if (stringp(filename)) {
		if (!(m = clone_object(filename))) return 0;
	} else return 0;

	if (!room_residents) room_residents = ({});

	if (m) move_object(m, this_object());

	room_residents += ({({ m, filename, appearmsg, reset_chance, TYPE_OBJECT })});

	if (m && appearmsg)
		tell_here(appearmsg, m);

	return 1;
}

// Add a special command player can use in this room.
status
add_command(string cmd_name, mixed cmd_func)
{
	if (!stringp(cmd_name)) return 0;
	if (!cmd_func) {
		if (!mappingp(commands)) return 0;
		m_delete(commands, cmd_name);
		return 1;
	}
// Let's not make commands use closure _yet_ in case room wants to
// use 'replace_prgoram()'! -+ Doomdark 14-feb-96 +-
// if (stringp(cmd_func)) cmd_func = symbol_function(cmd_func, this_object());
	if (!mappingp(commands)) commands = ([ cmd_name : cmd_func ]);	
	else commands[cmd_name] = cmd_func;
	return 1;
}

// Add a 'permanent' object, ie. something players can't take, a statue
// or something like that.
status
add_permanent(string perm_name)
{
object o;
	if (!stringp(perm_name)) return 0;
	if (!pointerp(permanent_names)) {
		permanent_names = ({});
		permanent_objects = ({});
	}
	move_object((o = clone_object(perm_name)), this_object());
	permanent_objects += ({ o });
// Order changed. Might prevent few odd errors.
	permanent_names += ({ perm_name });
	return 1;
}

/****************************************************************
*								*
*		Remove-functions.				*
*								*
****************************************************************/

status
remove_exit(string exit_dir)
// Removes an exit, returns 1 if there was such an exit, otherwise 0.
{
int i, size;
	if (!Exits || !Exits[exit_dir]) return 0;
	Flags &= (~F_ROOM_EXIT_LISTS_UPDATED);
	Exits = m_delete(Exits, exit_dir);
	return 1;
}

int
remove_item(string it)	// Returns amount of items removed.
			// Note; synonyms are removed as well.
{
int i, j;
string *tmp, *tmp2;

	if (!Items) return 0;

	i = sizeof(Items);

	Items = m_delete(Items, it);
	tmp = m_values(Items);
	tmp2 = m_indices(Items);

	it = "=" + it;

	while ((j = member(it, tmp)) >= 0) {
		tmp[j] = 0;
		Items = m_delete(Items, tmp2[j++]);
	}

	return i - sizeof(Items);
}

/****************************************************************
*								*
*	Lfuns; functions that are called by gamedriver.		*
*								*
****************************************************************/

varargs status
id(string str, object who)
{
int i;
	if (Items && str && Items[str]) return 1;
	return 0;
}

#if 0
void create_room() { }
#endif

void
create()
{
int s, i, k;
	Flags = F_ROOM_PREVENT_EMOTE;
// Default value for flags.
	if (function_exists("reset_room")) Flags |= F_ROOM_RESET_USED;
	if (function_exists("catch_tell")) Flags |= F_ROOM_CATCH_TELL_USED;

	this_object()->
	create_room();

// Let's get pointer to init_room/extra_look-function(s), if such exist(s)!
	if (function_exists("init_room")) Flags |= F_ROOM_INIT_USED;
	ExtraLookFunc = symbol_function("extra_look", this_object());
// (but only after caling create_room, in case it wants to use
// 'replace_program'!

// Now let's clone permanent objects and move them to the room.

	if (pointerp(permanent_names) && (s = sizeof(permanent_names))) {
		i = 0;
		while (i < s) {
		  if (!permanent_objects[i]) {
			permanent_objects[i] = clone_object(permanent_names[i]);
			k++;
		  }
		  i++;
		}
		if (k || first_inventory() != permanent_objects[0]) {
			i = s;
			while ((--i) >= 0) move_object(permanent_objects[i],
				this_object());
		}
	}
}

void
reset()
{
int i, j, k, s, s2, type;
object ob;
mixed *tmp;
int t;
mixed *filename;

	if (Flags & F_ROOM_RESET_USED)
		this_object()->reset_room();

// Next, let's reset room resident objects and npcs. Quite identical to
// code in add_monster/object.

// Used to reset room-resident objects and monsters.

	if (pointerp(room_residents) && (s2 = sizeof(room_residents))) {
	 for (j = 0; j < s2; j++) {
	  if (!pointerp(tmp = room_residents[j])) continue;
// Let's check whether this is a resettable thing:
	  if (!(t = tmp[INDEX_RESETTING]) || t < random(100))
		continue;
	  if (!(ob = tmp[INDEX_OBJECT]) || (environment(ob) != this_object()
		&& (type = tmp[INDEX_TYPE]) == TYPE_OBJECT)) {

	    if (!tmp[INDEX_FNAME]) {
		room_residents[i] = 0;	// We'll remove this from list later!
		continue;
	    } else if (objectp(tmp[INDEX_FNAME]))
		continue;		// We aren't interested of these...

	    else if (pointerp(tmp[INDEX_FNAME])) {
		filename = tmp[INDEX_FNAME];
		if (!stringp(filename[0])) {
			ob = clone_object(tmp[INDEX_TYPE] == TYPE_OBJECT ?
			  TREASURE_FILE : NPC_FILE);  // Use std base object.
			i = 0;
		} else {


			ob = clone_object(filename[0]); // Use custom monster.



			i = 1;
		}

		if (!objectp(ob)) continue;

// Now, we can customize our monster with call_others. Each element of
// the array consists of function name (1st arg), and optional arguments
// (from 0 to 4 args currently; if you need more, contact (co)admins!).

		for (s = sizeof(filename); i < s; i++) {
		  if (!pointerp(filename[i])) continue;
		  if (sizeof(filename[i]) < 2)
			call_other(ob, filename[i][0], 0);
		  else


		  apply(#'call_other, ob, filename[i][0], filename[i][1..<1]);


                }

		} else ob = clone_object(tmp[INDEX_FNAME]);

		if (ob) {
		  move_object(ob, this_object());
		  room_residents[j][INDEX_OBJECT] = ob;
		  if (stringp(tmp[INDEX_MSG]))
			tell_here(tmp[INDEX_MSG], ob);
			
		  if (type == TYPE_MONSTER)
			this_object()->extra_add_monster();
		}
	  }
	 }
	 room_residents -= ({ 0 });	// Let's get rid of these...
	}

// Next, let's update permanent objects in case some of them have been
// destructed.

	if (pointerp(permanent_names) && (s = sizeof(permanent_names))) {
		i = 0;
		while (i < s) {
		  if (!permanent_objects[i]) {
			permanent_objects[i] = clone_object(permanent_names[i]);
			k++;
		  }
		  i++;
		}
		if (k || first_inventory() != permanent_objects[0]) {
			i = s;
			while ((--i) >= 0) move_object(permanent_objects[i],
				this_object());
		}
	}

}

public void
init()
{
	add_action("command_driver","",1);

	if (Flags & F_ROOM_INIT_USED)
		this_object()->init_room();
	if (!this_player()) return;
// Let's check if we need to start hb:
	if (interactive(this_player())) {
	if ((Flags & (F_ROOM_HAS_EFFECTS | F_ROOM_HB_ON)) ==
	    (F_ROOM_HAS_EFFECTS)) {
		Flags |= F_ROOM_HB_ON;
		configure_object(this_object(), OC_HEART_BEAT, 1);
	  }
	}
}

public void
heart_beat()
{
object x;
int i, r;
// Check occasionally if we should turn off the heart_beat
// (makes mud run faster and such) (check 25% of hbs)
	if (!((r = random(100)) / 4)) {
		x = first_inventory();
		while (x) {
			if (interactive(x)) break;
			x = next_inventory(x);
		}
		if (!x) {
			Flags &= (~F_ROOM_HB_ON);
			configure_object(this_object(), OC_HEART_BEAT, 1);
			return;
		}
	}
// Check if we'll do the effect
	if (pointerp(room_effects) && (r = random(100)) < room_effects[0]) {
		i = 1;
		while (i < sizeof(room_effects)) {
			if (r < room_effects[i]) {
				tell_here(room_effects[i + 1]);
				break;
			}
			i += 2;
		}
	}

}

/****************************************************************
*								*
*			Other functions				*
*								*
****************************************************************/

/* To be redefined if needed. Called when items are handled with
   'get', 'put' or 'drop' commands in the room. //Graah */
void extra_move_object(object ob, object to) { }

varargs status // has to be varargs ++Tron.
move(string dir, string room)
{
	if (!this_player()) return 0;

	if (!dir) dir = query_verb();

	if (!room && (!Exits || !(room = Exits[dir]))) return 0;

// Cheap kludge for compatibility...

	if (this_player() -> query_rest()) {
	  if (!this_player() -> query_invis())
	    tell_here((string) this_player() -> query_name()+" stands up.", this_player());
		this_player() -> set_rest(0, 1);
	} else if (this_player() -> query_sit()) {
	  if (!this_player() -> query_invis())
	    tell_here((string) this_player() -> query_name()+" stands up.", this_player());
		this_player()->set_sit(0, 1);
	}
	this_player() -> move_player(dir, room);
	return 1;
}

status
weather_cmd(string s)
{ 
	if (!(query(ROOM_WD) & WD_OUTDOORS)) {
		this_player() -> tell_me("You have no sense of weather from here!");
		return 1;
	}
	this_player()->tell_me(NATURE_D->query_long_weather());
	return 1;
}

status
search_cmd(string str)
{
  this_player() -> tell_me(
"You search around for a while, but don't find anything interesting.");
  return 1;
}

status
listen_cmd(string str)
{
  this_player() -> tell_me(
"You listen carefully for a while, but don't hear anything interesting.");
  return 1;
}

status
smell_cmd(string str)
{
  this_player() -> tell_me(
"You smell around for a while, but don't detect any interesting scents.");
  return 1;
}

status
command_driver(string s)
{
  string v;
  mixed f;
  int i;

  v = query_verb();

  // Let's check commands first even though it's more probable that it's
  // a move; this way we can both define the visible exit, and then block
  // it with a command (and if command-func returns 0, move will succeed
  // normally).

  if (commands && (f = commands[v])) {
    // If command is not yet a closure, let's convert it to one!
    // (ie. most of the time, we first add command function; now it's time
    // to make it into a closure...)
    // We _could_ have converted it to a closure earlier, but let's not bother
    // to do that; this way room can use replace_program!!!
    if (stringp(f)) {
      f = symbol_function(f, this_object());
      commands[v] = f;
    }
    i = funcall(f, s, this_player());
    // Let's check if it was a moving command. If so, we'll inform
    // party daemon, so that party follow is possible!
    if (i && this_player() && !present(this_player(), this_object())) {
      PARTY_D->player_moved(this_player(), v, s, this_object());
    }
    if (i) return i;
  }

  if (Exits && (f = Exits[v])) {
    if (closurep(f)) {
      f = funcall(f, s, this_player());
      if (!stringp(f) && !objectp(f)) return f;
    }
    i = move(v, f);
    if (i && this_player() && !present(this_player(), this_object())) {
      // Same for "normal exits"; let's inform party daemon.
      PARTY_D->player_moved(this_player(), v, s, this_object());
    }
    if (i) return i;
  }

  if (v == "weather") return weather_cmd(s);
  if (v == "search") return search_cmd(s);
  if (v == "listen" || v == "hear") return listen_cmd(s);
  if (v == "smell") return smell_cmd(s);
  if (v=="l" || v=="look" || v=="exa" || v=="exam" || v=="examine")
    if (s == "up" || s == "sky" || s == "at sky")
      return weather_cmd(0);
  return 0;
}

// rec_destr(ob) recursively destructs items in ob 1.5.1992 //Frobozz
void
rec_destr(object ob)
{
object item;
	while(item = first_inventory(ob)) {
		rec_destr(item);
		destruct(item);
	}
}

// Temporary version. Hope this saves memory a bit.
// Idea is to check whether there are other objects than those added with
// "add_monster" and "add_objects". If yes, we won't destruct this room.
// Otherwise, it's safe to destruct those items and this room; things will
// get back to this same situation after new "create".

int
clean_up(int clones)
{
object *stuff;
int count, i;

#if 0
	return 0;
#endif
	if (Flags & F_ROOM_NO_CLEANUP) return 0;	// Won't call again!
	if (clones > 1 || environment()) return 1;	// May be called again

// First, let's check how many objects are present in this room.
// These are all thought as 'unknown objects'; we have to check if they are
// added with "add_monster/object" or "add_permanent". These objects do not
// prevent destruction of this room; others do.

	if ((count = sizeof(stuff = all_inventory(this_object()))) &&
		pointerp(room_residents)) {
	  for (i = sizeof(room_residents) - 1; i >= 0; i--) {
		if (!room_residents[i][INDEX_OBJECT])
			continue;
		else if (environment(room_residents[i][INDEX_OBJECT]) == this_object())
			count--;

// If room-resident object was here, let's subtract one from count of
// 'unknown' items.

		else if (room_residents[i][INDEX_TYPE] == TYPE_MONSTER)
			return 1;

// We won't destruct, if some room-resident monster is somewhere else.
// Otherwise we might have n+1 of those monsters...

	  }
	}

	if (count && pointerp(permanent_objects) &&
	  (i = sizeof(permanent_objects) - 1)) {
	  for (; i >= 0; i--)
	  if (permanent_objects[i] && environment(permanent_objects[i]) == this_object())
		count--;
	}

	if (count) return 1;
// If there were some other objects than those added with add_object/monster
// or add_permanent, let's just return.

	rec_destr(this_object());
	destruct(this_object());

// As 0 means we won't call clean_up again, it's pretty much same what we
// return, but let's use 0 here.... -+ Doomdark +-
	return 0;
}

/****************************************************************
*								*
*	Compatibility section - Not to be used any more!!!	*
*								*
****************************************************************/

// Bit of a hassle, this function...
// Use "query_exits"

string *
query_dest_dir()
{
string *dir, *room, *tmp;
int i;
	if (!mappingp(Exits) || !(i = sizeof(Exits))) return ({ });
	room = m_values(Exits);
	dir = m_indices(Exits);
	tmp = allocate(2 * i);

	while (i-- > 0) {
		tmp[2 * i] = room[i];
		tmp[2 * i + 1] = dir[i];
	}
	return tmp;
}

// No type casts, for compatibility...
// Shouldn't be needed; but will as long as we have old rooms...
long(what, who)
{
	if (who || (who = this_player()))
		return (int) who -> tell_me(query_long(what, who), 0,
			 TELL_TYPE_ROOM_DESC | TELL_BEGIN, 0, 0, 0);
}
short(mode, who) { return query_short(mode, who); }
status is_room() { return 1; }

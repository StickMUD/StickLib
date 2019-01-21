/**
 * @name Virtual
 */

//@{

/**
 * NAME:    virtual_room.c
 *
 * PURPOSE: This room code is used in virtual room
 *          system. Room code is compatible with
 *          /lib/room.c with some exeptions. Study
 *          files in /doc/virtual directory for more
 *          info about virtual rooms and setting up
 *          your own virtual environment.
 *
 * @author Tron, Copyright (c) StickMUD.com, 1992 - 2009
 *
 * NOTES:  Fafnir@StickMUD added significant enhancements
 *         to this server, including the ANSI support, and
 *         also documented!
 *
 */

#include "/sys/configuration.h"

#include <nroom.h>
#include <player_defs.h>
#include <daemons.h>
#include <virtual_defs.h>

#define F_VIRTUAL_INIT          1
#define F_VIRTUAL_HB_ON         2
#define F_VIRTUAL_HB_ENABLED    4
#define F_CREATE_CALLED         8

// Virtual look mode
private static int m_virtual_look;
private static int m_look_x_range, m_look_y_range;

private string *room_map;        // Map of environment.
private string objects;          // Temporary object list.
private mixed  obj_list;         // Active object creation list.
private string rooms_home;       // Home for your virtual rooms.
private string descs_home;       // Home for specific descriptions.
private string server_ob;        // Our server.
private string *random_objects;  // Temporary random object list.
private string beat_function;    // Your own beat function in HB.
private string moving_command;   // Special moving command.
private string moving_msgout;    // Special leaving message.
private string moving_msgin;     // Special arriving message.
private status moving_flag;      // 1 if players moves with special command.

private status read;             // 1 if special description has been read.

int X, Y, Z;                     // Coordinates of this room.
static int virtualFlags;         // Used to hold some useful info

/**
 * Get a map for the room. This includes the set padding and so on, with
 * current range.
 *
 * @return An array describing the map near the current coordinates
 **/
string *
query_virtual_map ();

/**
 * Gets long description based on virtual look short flag
 *
 * @param arg Look argument if any given
 * @param who Who is doing the look
 *
 * @return The long desc
 **/
private nomask string
query_virtual_look_short (string arg, object who)
{
    string tmp, full_desc;
    object ob;

    // We are able to see in other rooms.
    if (arg) {
	if (tmp = query_exit (arg)) {
	    full_desc = "You look " + arg + "...\n" +
	    call_other (tmp, "query_short", 2, who);
	    ob = first_inventory (tmp);
	    if (!ob) return full_desc;
	    while (ob) {
		if (random(2)) { //Can't always see what's in there.
		    if (ob->short()) {
			if (living(ob))
			    full_desc += "\nSomeone or something is "
			    "moving there.";
			else
			    full_desc += "\nYou can see there is "
			    "something there but you are not "
			    "quite sure what it is.";
			break;
		    }
		}
		ob = next_inventory (ob);
	    } // while
	}
    } else {
	full_desc = query_short (2, who);
    }

    return full_desc;
}

/**
 * Gets long description based on virtual look long flag. This function
 * is rather heavy, but on the other hand, much of the overhead comes from
 * player object's tell_me() function. If we wanted to skip the map system,
 * we would have much more room for various improvements in movement code.
 *
 * On the other hand the movement code could receive a little bit love so
 * there's more room to fancy effects.
 *
 * @param who  Who is looking around
 * @param desc Room's long desc passed in as a string array
 *
 * @return The full desc
 **/
private nomask string
query_virtual_look_long (string desc, object who)
{
    int columns, map_size, map_width, desc_size, final_size, i;
    string wrapped_desc;
    string *map, *broken_desc;
    string *room_desc;

    map = query_virtual_map ();
    if (!map) return desc;
    map_width = sizeof (map[0]);

    columns = who->query_env ("columns");
    if (!columns) columns = 80;

    /* The map contains the frame in it. We're adding additional padding of
     * one space on both sides, tell_me substracts columns by 1, so we need
     * to fix in that space. So, we need to substract at least 4 chars from
     * desc size to be sure we fit
     */
    wrapped_desc = sprintf ("%-=*s", columns - (map_width + 4), desc);
    broken_desc = explode (wrapped_desc, "\n");

    /* Decide the size of the desc */
    desc_size = sizeof (broken_desc);
    map_size = sizeof (map);
    if (desc_size < map_size) final_size = map_size;
    else final_size = desc_size;

    /* Let's build the room desc with the map frame. Looks horrible
     * but don't know if there's more efficient way to do this...
     */
    room_desc = allocate (final_size);
    for (i = 0; i < final_size; i++) {
	if (i < map_size) {
	    if (i < desc_size) {
		room_desc[i] = " " + map[i] + " " + broken_desc[i];
	    } else {
		room_desc[i] = " " + map[i];
	    }
	} else {
	    room_desc[i] = sprintf ("%' '*s", sizeof(map[0]) + 2, " ") +
	    broken_desc[i];
	}
    }

    return implode (room_desc, "\n\r");
}

/**
 * Inform caller that this is a virtual room.
 *
 * @return 1 for virtual room
 **/
nomask status
query_virtual_room ()
{
    return 1;
}

/**
 * Sets coordinates for the room, needed for special rooms inside the
 * areas... to allow mapping and virtual exits and such
 *
 * @see query_coordinates()
 *
 * @param x_coordinate The X-coordinate on map
 * @param y_coordinate The Y-coordinate on map
 **/
nomask void
set_coordinates (int x_coordinate, int y_coordinate)
{
    X = x_coordinate; Y = y_coordinate;
}

/**
 * Returns array of coordinates, X in 0, Y in 1
 *
 * @return Array of coordinates
 **/
nomask int*
query_coordinates ()
{
    return ({ X, Y });
}

/**
 * Reset objects set for the room. Travels through internal lists fetch
 * from the virtual server and clones the objects.
 *
 * FIXME: The virtual rooms really would benefit from using the standard
 *        room code more. This rather complex code is already present in
 *        rooms, which this object could, and should use.
 *        This is rather scary function :P Not so badly broken though,
 *        so will leave it be for the time being. --Fafnir
 *
 * @param st If set to 1, decodes object information from the internal
 *           string list. If not set or set to 0, try to use previously
 *           decoded object list.
 **/
nomask void
reset_objects (int st)
{
    string *list;
    int i, n;
    object ob;
    string objec;
    string fun, arg;
    string tail;
    mixed a;
    mixed b;

    if(st) obj_list = 0;

    if(!obj_list) {
	if(!objects) return;

	obj_list = ({});
	list = explode(objects, "@");
	objects = 0;

	for (i = sizeof (list) - 1; i > -1; i--) {
	    if (sscanf (list[i], "%d:%s", n, tail) != 2) continue;

	    while (n) {
		obj_list += ({ 0, tail });
		n--;
	    }
	}
    }

    while ((n = member (obj_list, 0)) != -1) {
	tail = obj_list[n+1];

	if (sscanf (tail, "%s:%s:%s", objec, fun, arg) != 3)
	    if (sscanf (tail, "%s:%s", objec, fun) != 2)
		if (sscanf (tail,"%s", objec) != 1) continue;

	ob = clone_object (objec);
	move_object (ob, this_object());
	if(fun) call_other (ob, fun, arg);
	obj_list[n] = ob;
    }
}

/**
 * Resets random objects when room is created, or resetted.
 **/
nomask void
reset_random_objects()
{
    object ob;
    int i, n, chance, siz;
    string tail, objec, tmp;
    string fun, arg;

    if(!random_objects) return;
    siz = sizeof(random_objects);

    for(i=0; i < siz; i++) {
	if(sscanf(random_objects[i], "%d:%s", n, tail) != 2) continue;

	if(sscanf(tail, "%d:%s:%s:%s", chance, objec, fun, arg)!=4)
	    if(sscanf(tail, "%d:%s:%s", chance, objec, fun) !=3)
		if(sscanf(tail, "%d:%s", chance, objec)!=2) continue;

	if(random(100) < chance) {
	    while(n--) {
		ob = clone_object(objec);
		move_object(ob, this_object());
		if(fun) call_other(ob, fun, arg);
	    }
	}
    } // FOR

    random_objects = 0;
}

/**
 * Gets server filename from according to the room file name.
 *
 * @return The server file name
 **/
nomask string
get_server()
{
    string name;
    string tmp;

    if (server_ob) return server_ob;

    if (sscanf(object_name(this_object()), "%s/virtual/%s", name, tmp) == 2) {
	server_ob = name + "/virtual/server";
	return server_ob;
    }
    else return 0;
}

/**
 * Creates the room. Loads objects and random objects and calls coder's
 * create function.
 **/
nomask static void
create_room() 
{
    mixed  *tmp;

    m_look_x_range = m_look_y_range = 2;

    if (function_exists("init_virtual"))
	virtualFlags |= F_VIRTUAL_INIT;
    if (server_ob = get_server()) {
	if(tmp = (mixed *)server_ob->query_virtual_room_server()) {
	    X = tmp[0];
	    Y = tmp[1];
	    objects = tmp[2];
	    descs_home = tmp[3];
	    rooms_home = tmp[4];
	} else {
	    server_ob = 0;
	}
    }

    if (!(virtualFlags & F_CREATE_CALLED)) {
	virtualFlags |= F_CREATE_CALLED;
	this_object()->create_virtual();
    }
    add_command("terrain", "show_terrain");

    obj_list = 0;
    reset_objects(1);
    reset_random_objects();
}

/**
 * Check if the room is a virtual room by asking it's coordinates and
 * comparing them to it's object name. This is to prevent blueprints
 * to contain monsters (that might carry uniques).
 *
 * Checking if the room is a clone, or asking server's is_virtual_room
 * does not work in all cases. Elena's park has a fake virtual room, that
 * is there, but does not have a terrain file, and is using a virtual room
 * code, so server check and clone check alone will not work.
 *
 * @param filename an array of function calls and parameters or just a
 *    file name of the monster to clone
 * @param appearmsg The message to print when mob is reseted
 * @param reset_chance Chance for the mob to reset
 *
 * @return 1 if the call was a - ok, 0 if something is wrong. Typically,
 *    this is ignored value
 **/
int
add_monster (mixed filename, string appearmsg, int reset_chance)
{
    object room = this_object();

    if (!room) return 0;

    if (!clonep (room)) {
	string room_name;
	string *path;
	int *coords;
	int x, y;

	/* Strip path from room's object name */
	room_name = object_name (room);
	path = explode (room_name, "/");

	if (!path || !sizeof (path)) return 0;
	room_name = path[<1];

	if (!room_name) return 0;
	if (sscanf (room_name, "%d_%d", x, y) != 2) return 0;

	coords = query_coordinates();

	if (!coords || sizeof (coords) != 2) return 0;
	if (x != coords[0]) return 0;
	if (y != coords[1]) return 0;
    }

    return ::add_monster (filename, appearmsg, reset_chance);
}

/***********************************************
 DESCRIPTION: Perform init.
 ***********************************************/

nomask static void
init_room()
{
    if (virtualFlags & F_VIRTUAL_INIT)
	this_object()->init_virtual();

    if (moving_command) {
	add_action("virtual_move", moving_command);
    }

    if (!(virtualFlags & F_VIRTUAL_HB_ENABLED) ||
      (virtualFlags & F_VIRTUAL_HB_ON)) return;

    virtualFlags |= F_VIRTUAL_HB_ON;
    configure_object(this_object(), OC_HEART_BEAT, 1);
}

/***********************************************
 DESCRIPTION: Show short desc of terrain.

 ARGUMENT:    str -terrain type.

 RETURN:      1 if success, 0 for failure.
 ***********************************************/

nomask status
show_terrain(string str) {
    string s;
    string server;

    if(!str) {
	notify_fail("Usage: terrain <terraintype>\n");
	return 0;
    }

    server = get_server();
    if(server) {
	s = (string) server->query_terrain_short(str);
	if(!s) {
	    write("No such terrain.\n");
	    return 1;
	}
	write(str+": "+s+"\n");
    }
    else { write("No server!!!\n"); }
    return 1;
}

/******************************************

 DESC: Resets room. Loads objects and calls
       coder's reset function.

*******************************************/

nomask static void
reset_room()
{
    reset_objects(0);
    if(!first_inventory(this_object()))
	destruct(this_object());
    else this_object()->reset_virtual();
}

/******************************************

 DESC: Sets random objects for the room.

 RET:  Returns 1 if success, 0 for failure.

*******************************************/

nomask status
set_random_objects(string *objec)
{

    if(!pointerp(objec)) return 0;
    random_objects = objec;
    return 1;
}

/*********************************************

 DESC: Adds new random object to object list.

 RET:  Returns 1 for success, 0 for failure.

**********************************************/

nomask status
add_random_object(string objec) {

    if(!stringp(objec)) return 0;

    if(!random_objects) random_objects = ({});
    random_objects += ({objec});

    return 1;
}

/**
 * Applies virtual room specific path resolving and calls the rooms
 * add_exit() code. This used to mask add_exit() function to private,
 * but that's a bad idea... There's no reason why virtual rooms
 * couldn't have a physical exit.
 *
 * @param dir direction description
 * @param file filename for direction
 **/
nomask private void
virtual_room_add_exit(string dir, string file)
{
    string str;

    if (server_ob) {
	if (server_ob->is_virtual_room(file)) {
	    if (moving_command)
		dir = moving_command+" "+dir;
	    add_exit(dir, rooms_home+file);
	}
    }
}

/******************************************

 DESC: Sets all virtual exits.

 ARG:  Mapping of exit names and directions
       type of ([ exit:({x, y}), ... ])

*******************************************/

nomask void
set_virtual_exits(mapping dir)
{
    mixed dir_names;
    mixed *dir_dest;
    mixed x, y;
    int i, n;

    if (!mappingp(dir)) return;

    dir_names = m_indices(dir);
    dir_dest  = m_values(dir);

    n = sizeof(dir);
    for(i=0; i<n; i++) {
	x = X + dir_dest[i][0];
	y = Y + dir_dest[i][1];
	virtual_room_add_exit(dir_names[i], x+"_"+y);
    }
}

/******************************************

 DESC: Add new virtual exit to exit list.

 ARG: dir - direction description
      x   - relative x coordinate
      y   - relative y coordinate

*******************************************/

nomask void
add_virtual_exit(string dir, int x, int y) {

    mixed xx, yy;

    xx = X + x;
    yy = Y + y;
    virtual_room_add_exit(dir, xx+"_"+yy);
}

/******************************************

 DESC: Pick one description from given ones

 ARG:  list - array of descriptions

 RET:  Returns 1 for success, 0 for failure.

*******************************************/

nomask status
set_random_desc(string *list)
{
    if(!pointerp(list)) return 0;

    set_long(list[random(sizeof(list))]);

    return 1;
}

/******************************************

 DESC: Enables/disables heart beat.

 ARG:  i - 1 enables, 0 disables.

 RET:  1 for success, 0 for failure.

*******************************************/

nomask status
set_hb(int i)
{
    if(i < 0 || i > 2) return 0;

    if (i) {
	virtualFlags |= (F_VIRTUAL_HB_ENABLED | F_VIRTUAL_HB_ON);
    } else {
	virtualFlags &= (~(F_VIRTUAL_HB_ENABLED | F_VIRTUAL_HB_ON));
	;
    }

    return 1;
}

/**
 * Set 'visual' or 'sound' effects. This function is practically
 * pointless, one should use room's add (ROOM_EFFECT, 10, "message");
 * instead.
 *
 * @param s      Array of effect messages
 * @param chance Chance to display the message
 *
 * @return 0 for failure, 1 for success
 **/
nomask varargs status
set_effects (string *s, int chance)
{
    if (pointerp (s)) {
	int i, effect_size;

	effect_size = sizeof (s);
	if (!chance || chance < 0 || chance > 100) chance = 5;

	for (i = 0; i < effect_size; i++) {
	    add (ROOM_EFFECT, chance, s[i]);
	}
    }
    else return 0;
    return 1;
}

/******************************************

 DESC: Sets special moving description for
       the terrain type instead of 'leaves'
       or 'arrives'. Also sets the command
       that enables moving to other rooms.

       Example: swim east

       where "swim" is the moving command.

 ARG:  s - moving command
       o - description instead of 'leaves'
       i - description instead of 'arrives'

 RET:  Returns 1 for success, 0 for failure.

*******************************************/

nomask status
set_moving_command(string s, string o, string i)
{
    if(stringp(s)) {
	moving_command = s;
	if(stringp(o)) moving_msgout = o;
	if(stringp(i)) moving_msgin = i;
    }
    else return 0;
    return 1;
}

/******************************************

 DESC: Sets function called from heart
       beat.

 ARG:  s - function name

 RET:  Returns 1 for success, 0 for failure.

*******************************************/

nomask static status
set_beat_function(string s)
{
    if(stringp(s)) {
	beat_function = s;
    }
    else return 0;

    return 1;
}

/*****************************************

 DESC: Check if we can see in different
       directions here.

 ARG:  Id (from look etc.)

 RET:  Id matches when 1, 0 when failed.

******************************************/

varargs status
id(string s, object me)
{
    if (s && m_virtual_look && query_exit(s)) return 1;

    return ::id(s, me); //check items.
}

/**
 * Enable/disable virtual look mode.  This mode enables players to see
 * to the other rooms. Also allows the desc formatting to be tuned a little
 * bit. With VIRTUAL_F_LOOK_SHORT set, the old short desc map is
 * printed. With VIRTUAL_F_LOOK_LONG set, the map is printed to long
 * desc.
 *
 * @param i A bitwise OR of settings
 *
 * @return 1 on success
 **/
nomask status
set_virtual_look (int i)
{
    m_virtual_look = i;

    return 1;
}

/**
 * Get distant look mode from the room. This can be used in external commands
 * or in other objects to do something.
 *
 * @return The distant look set in set_virtual_look()
 **/
int
query_virtual_look ()
{
    return m_virtual_look;
}

/**
 * Set virtual look range, in X-range and Y-range. This determines how many
 * rooms ahead one is able to look. The server's get_environment() function
 * is used to get the actual environment.
 *
 * Minimum range to set is 2. If you want to disable the map, set the distant
 * look to 0 instead.
 *
 * @see query_virtual_map()
 * @see set_virtual_look()
 *
 * @param x_range The X coordinate range, note that this is in array values,
 *                so value of 2 yields total of 3 rooms of map
 * @param y_range The Y coordinate range, see the odditiy in configuration from
 *                y_range
 **/
void
set_virtual_look_range (int x_range, int y_range)
{
    if (x_range < 2) x_range = 2;
    if (y_range < 2) y_range = 2;

    m_look_x_range = x_range;
    m_look_y_range = y_range;

    /* Reset the room map, so it's fetched again */
    room_map = 0;
}

/**
 * Queries virtual look ranges. These can be used to determine how big the
 * map is supposed to be in this room.
 *
 * @return Array containing the ranges, ({ x_range, y_range })
 **/
int *
query_virtual_look_range ()
{
    return ({ m_look_x_range, m_look_y_range });
}

/**
 * Get a map for the room. This includes the set padding and so on, with
 * current range.
 *
 * @return An array describing the map near the current coordinates
 **/
string *
query_virtual_map ()
{
    string server;

    if (room_map) return room_map;

    server = get_server ();
    if (!server) return 0;

    room_map = server->query_virtual_map (
      X, Y, m_look_x_range, m_look_y_range);

    return room_map;
}

/**
 * Shows a small map and a short desc, if the terrain is
 * allowed to show this and user can see.
 *
 * @return Map and short desc.
 **/
varargs string
query_short (int a, object b)
{
    string *map;
    string s;

    if (!(m_virtual_look & VIRTUAL_F_LOOK_SHORT))
	return ::query_short (a, b);

    if (!b) b = this_player();
    if (b && b->test_dark ()) return ::query_short (a, b);

    map = query_virtual_map ();

    if (!map) return ::query_short (a, b);

    s = implode (map, "\n") + "\n" + ::query_short (a, b);
    return s;
}

/**
 * Shows long description to a room. If distant look is enabled, it shows
 * a small "map" similar to short desc. As a matter of fact it prints the
 * short desc without exits. If an argument is given, and distant look is
 * set, an exit can be used to look at into another room. In this case,
 * short description is printed for the user.
 *
 * @param n   Argument for look command
 * @param who Object who is querying the description
 **/
varargs string
query_long (string n, object who)
{
    string full_desc = "";
    object ob;

    if (!who && !(who = this_player())) return ::query_long (n, who);
    if (who && who->test_dark ()) return ::query_long (n, who);
    if (who->query_coder_level()) full_desc = "("+X+", "+Y+")\n";

    if (!read) {
	string tmp;

	read = 1;
	tmp = read_file ("/" + descs_home + X + "_" + Y);
	if (tmp) set_long(tmp);
    }

    /* Distant look when short flag is set, shows both long and short desc */
    if (m_virtual_look & VIRTUAL_F_LOOK_SHORT) {
	string short_desc;

	short_desc = query_virtual_look_short (n, who);

	if (!n) {
	    full_desc += short_desc + "\n" + ::query_long (n, who);
	} else if (short_desc) {
	    full_desc += short_desc;
	} else {
	    full_desc += ::query_long (n, who);
	}
    } else if ((m_virtual_look & VIRTUAL_F_LOOK_LONG) && !n) {
	string temp_desc;
	int skip_exits = 0;

	/* Skip the exits. This is done in an inconvenient manner in order
	 * to allow basic room code to be called. We will query for the
	 * exits later on
	 */
	if (query (SHOW_EXITS)) {
	    set (SKIP_EXITS);
	    temp_desc = ::query_long (n, who);
	    set (SHOW_EXITS);
	} else {
	    skip_exits = 1;
	    temp_desc = ::query_long (n, who);
	}
	if (!temp_desc) return ::query_long (n, who);

	temp_desc = query_virtual_look_long (temp_desc, who);

	if (skip_exits) {
	    full_desc += temp_desc;
	} else {
	    full_desc += temp_desc + "\n" + query_exit_list (1);
	}
    } else {
	full_desc += ::query_long (n, who);
    }

    return full_desc;
}

/**
 * Heart beat function, this is only here anymore because of the heart
 * beat function. If the heart beat function would be supported by
 * generic room, the need for this function would be gone. This was defined
 * as nomask, but still masks room's heart_beat()
 **/
public void
heart_beat()
{
    object ob;
    object ob2;

    ob = first_inventory(this_object());
    while(ob) {
	if (virtualFlags & F_VIRTUAL_HB_ENABLED) {
	    if(interactive(ob)) break;
	}
	else if(living(ob)) break;

	ob = next_inventory(ob);
    }
    if (!ob) {
	virtualFlags &= (~F_VIRTUAL_HB_ON);
	configure_object(this_object(), OC_HEART_BEAT, 0);
	return;
    }

    if (beat_function) call_other(this_object(), beat_function, 0);

    /* Added compatability to plain rooms */
    ::heart_beat();
}

status
virtual_move(string s)
{
    if(!s) {
	this_player()->tell_me(capitalize(moving_command)+" where?");
	return 1;
    }
    command(s, this_player()); //move() needs verb, let's change it.
    if (this_player() && environment(this_player()) != this_object()) {
	PARTY_D->player_moved(this_player(), moving_command, s, this_object());
    }

    return 1;
}

varargs string
query_msgout(object x) { return moving_msgout; }

varargs string
query_msgin(object x) { return moving_msgin; }

//@}

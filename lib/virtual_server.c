/**
 * @name Virtual
 */

//@{

/**
 * The generic purpose virtual server. Creates virtual world rooms.
 * This object is to be inherited in an implementing virtual area server
 *
 * @author Tron, Copyright (c) StickMUD.com, 1992 - 2009
 * @see    virtual_room
 *
 * NOTES:  Fafnir@StickMUD added significant enhancements
 *         to this server, including the ANSI support, and
 *         also documented!
 */

#include <generic_rooms.h>
#include <virtual_defs.h>

/// Map indexes
#define X1 0
#define Y1 1
#define X2 2
#define Y2 3
#define MP 4

/// Configuration modes to read different configs
#define R_OBJS   0
#define R_MAPS   1
#define R_TERR   2
#define R_ALL    3

/// Commands for checking what to call in virtual_users()
#define REBOOT   0
#define TELL     1

/// Maximum array size to handle for room cache
#ifndef MAX_ARRAY
#  define MAX_ARRAY 3000
#endif

/// Terrain configuration indexes
#define TERRA_FILE 0 // The terrain object file to clone
#define TERRA_ANSI 1 // The terrain ansi code to use, in terminal_colour() mode
#define TERRA_DESC 2 // The replacement for terrain descriptor

#if 0
private:
#endif

/// Private non saveable variables

/// Reboot counter
private nosave int reboot;

/// Local room memory, aka room cache
private nosave object *m_room_cache;

/// Virtual look mode
private nosave int m_virtual_look;
private nosave int m_look_x_range, m_look_y_range;

/// Private saveable variables

/// Terrain configuration from terra_cfg
private mapping m_terrains;

#if 0
public:
#endif

/// Public variables

/// Saveable variables

/// Object configuration from obj_cfg
mapping objects;

/// Map configuration in form: ({ x1, y1, x2, y2, mapname })
mixed   *maps;

/// Non saveable variables

/// Last used coordinates
static int X, Y;

/// Maps in memory
static mapping map_memory;

/// Configuration directories
static string virtual_home;
static string terra_home;
static string maps_home;
static string rooms_home;
static string descs_home;

/// Configuration files to read certain configs
static string terra_cfg;
static string obj_cfg;
static string map_cfg;

/// Object save file to cache some information. Not very useful thing
static string virtual_save;

/**
 * Environment for the virtual rooms. Useless, as driver inherits
 * the light level to everyhing in same environment
 */
static string universum;

/// Amount of maps to keep in memory
static int    memory;

/// Width of a terrain descriptor.
static int    tdesc_x;

/// Name of the optional set up function for virtual rooms
static string create_func;

/**
 * Gets server's full configuration data.
 *
 * @return Array containing the full configuration data for the server
 */
mixed *
query_setup()
{
    return ({ m_terrains,
      objects,
      maps,
      virtual_home,
      terra_home,
      maps_home,
      rooms_home,
      descs_home,
      terra_cfg,
      obj_cfg,
      map_cfg,
      virtual_save,
      universum,
      tdesc_x,
      create_func });
}

/**
 * Function identifies this object as a server. Also returns a mixed set
 * of some configuration values. This is as a matter of fact used by the
 * virtual room to query it's own setup during the creation time
 *
 * @return Mixed array is returned. Array contains configuration
 *         information and currently processed virtual room
 *         coordinates and its objects.
 **/
nomask mixed *
query_virtual_room_server()
{
    return ({ X, Y,
      (objects ? objects[X+"_"+Y] : 0),
      descs_home,
      rooms_home, });
}

/**
 * Read configuration settings from server's configuration files.
 * This function is capable of (re-)reading small portions of configuration,
 * or everything at once.
 *
 * @param map Integer describing which configuration to read. R_ALL for
 *            reading all, R_OBJS to read just the obj_cfg file, R_MAPS
 *            to read only the map_cfg and R_TERR to read the terra_cfg.
 **/
nomask static void
read_data (int map)
{
    string buf, *file;
    string crd, obj;
    int i, a, b, c, d, f;

    file = allocate(3);
    file = ({0, 0, 0});

    switch(map) {

    case R_ALL:
	m_terrains   = ([:3]);
	objects      = ([]);
	maps         = ({});
	file[R_TERR] = terra_cfg;
	file[R_OBJS] = obj_cfg;
	file[R_MAPS] = map_cfg;
	break;

    case R_OBJS:
	objects      = ([]);
	file[R_OBJS] = obj_cfg;
	break;

    case R_MAPS:
	maps         = ({});
	file[R_MAPS] = map_cfg;
	break;

    case R_TERR:
	m_terrains   = ([:3]);
	file[R_TERR] = terra_cfg;
	break;

    default: return;
    }

    for(i=1, f=0; f<3; f++, i=1) {

	if(!file[f]) continue;

	while(buf = read_file("/"+file[f], i++, 1)) {

	    if(buf[0..0] == "!") continue; /* Commented line */

	    switch(f) {

	    case R_OBJS:
		if(sscanf(buf,"%s:%s\n", crd, obj)==2) {
		    if(objects[crd])
			objects[crd] += ("@"+obj);
		    else
			objects += ([ crd:obj ]);
		}
		break;

	    case R_MAPS:
		if(sscanf(buf,"%s:%d,%d,%d,%d", crd, a, b, c, d)==5)
		    maps += ({ ({a, b, c, d, crd}) });
		break;

	    case R_TERR:
		if (sscanf (buf, "%s:%s:%s:%s\n", crd, obj, a, b) == 4)
		    m_terrains += ([ crd:obj;a;b ]);
		else if (sscanf (buf, "%s:%s:%s\n", crd, obj, a) == 3)
		    m_terrains += ([ crd:obj;a;0 ]);
		else if (sscanf (buf, "%s:%s\n", crd, obj) == 2)
		    m_terrains += ([ crd:obj;0;0 ]);
		if (m_terrains[crd, TERRA_FILE] == "")
		    m_terrains[crd, TERRA_FILE] = 0;
		if (m_terrains[crd, TERRA_ANSI] == "")
		    m_terrains[crd, TERRA_ANSI] = 0;
		if (m_terrains[crd, TERRA_DESC] == "")
		    m_terrains[crd, TERRA_DESC] = 0;
		break;

	    default: break;
	    }

	} //WHILE

    } //FOR
}

/**
 * Sets a virtual look mode for the server. If the setting is set
 * to the server, then it's passed all cloned rooms.
 *
 * So, if one wants to override it, one needs to take this into account
 * in the terrain code, or not do it here.
 *
 * NOTE: Setting this on the fly on the server will not affect existing
 *       rooms right now. It could, but it does not right now.
 *
 * @param mode    The virtual look mode as defined in the virtual defs
 * @param x_range The virtual look x range to use in virtual look mode
 * @param y_range The virtual look y range to use in virtual look mode
 **/
nomask void
set_virtual_look (int mode, int x_range, int y_range)
{
    m_virtual_look = mode;
    if (x_range < 2) x_range = 2;
    if (y_range < 2) y_range = 2;

    m_look_x_range = x_range;
    m_look_y_range = y_range;
}

/**
 * Reads all object configuration again, by calling read_data()
 *
 * @see read_data()
 **/
nomask void
read_objects()
{
    read_data(R_OBJS);
}

/**
 * Reads all map configurations again, by calling read_data()
 *
 * @see read_data()
 **/
nomask void
read_maps()
{
    read_data(R_MAPS);
}

/**
 * Reads all terrain configurations again, by calling read_data(). The terrain
 * types are not recompiled
 *
 * @see read_data()
 **/
nomask void
read_terrains()
{
    read_data(R_TERR);
}

/**
 * Inform users about actions made on the virtual area. Using the REBOOT flag,
 * the server will inform users about server reboot and move them to the
 * church.
 *
 * Using the TELL flag, the server sends a message defined by the second
 * parameter to the users in the virtual area.
 *
 * @param st  Flag telling the function which action to perform
 * @param str Message to be sent to all the players on the virtual area, if a
 *            TELL flag is being used
 **/
nomask varargs static void
virtual_users (int st, string str)
{
    object *obs;
    object env;
    string tmp;
    int i, n;

    obs = users();
    n = sizeof(obs);
    for(i=0; i<n; i++) {
	env = environment(obs[i]);
	if(!env) continue;
	if(sscanf("/"+object_name(env), rooms_home+"%s", tmp))
	{
	    if(st == REBOOT) {
		tell_object(obs[i], "VIRTUAL WORLD: Rebooting world NOW!!!\n"+
		  "VIRTUAL WORLD: Moving to church...\n");
		move_object(obs[i], GENERIC_ROOM_CHURCH);
	    }
	    else if(st == TELL)
		tell_object(obs[i], str);
	}
    }
}

/**
 * Resets the server. To be overridden in own custom server code if there
 * is something that the server would want to do periodically
 **/
nomask void
reset_server ()
{
}

/**
 * Starts the virtual server. Call this function in your own virtual
 * server code to start server. This should be called after you've set
 * all configuration variables.
 *
 * FIXME: This starts a useless call_out chain for checking stale rooms.
 *        This at the moment does nothing else but cause useless overhead
 *        in the server. --Fafnir
 **/
nomask void
start_server()
{
    map_memory = ([]);
    m_room_cache = ({});

    if(!virtual_save || !restore_object(virtual_save)) {
	read_data (R_ALL);

	if (virtual_save) {
	    save_object (virtual_save);
	}
    }
    if (!m_terrains || !maps) {
	read_data (R_ALL);

	if (virtual_save) {
	    save_object (virtual_save);
	}
    }
}

/**
 * Stops the server. In essence this means it will unload all rooms
 * from internal memory and move users to safety
 **/
void
stop_server ()
{
    /* There's no rooms to destroy, so probably noone to move to
     * safety either. So, don't bother doing anything
     */
    if (!m_room_cache || sizeof (m_room_cache) < 1) return;

    /* Inform users about reloading and move them to safety */
    virtual_users (REBOOT);

    /* Destruct the rooms and their inventory */
    foreach (object room : m_room_cache) {
	if (!room) continue;

	destruct (room);
    }

    m_room_cache = ({});
}

/**
 * Create a server. This can also be overridden, but also the support
 * for configuring in inheriting server is supported.
 *
 * For this, just implement create_server() that sets up all configuration.
 * The server will handle everything else.
 **/
void
create ()
{
    this_object()->create_server();
    start_server ();
}

/**
 * Reset function implementation. This is called by the game driver
 * periodically. This clears up deleted rooms from cache.
 **/
void
reset ()
{
    /* Remove deleted rooms from internal cache */
    if (m_room_cache) {
	m_room_cache -= ({ 0 });
    }

    this_object()->reset_server();
}

/**
 * Cleans up the virtual server. If there's no more clones or no more
 * tracked rooms, the server will destruct itself.
 *
 * @param arg Reference count. If the reference count is greater than
 *            one, it's not recommended to self destruct.
 *
 * @return 0 when the object has been destructed. Non-zero if we're
 *           still tracking rooms.
 **/
int
clean_up (int arg)
{
    /* We still got clones or are inherited, check later */
    if (arg > 1) return 1;

    /* Still got rooms being tracked, check again later */
    if (m_room_cache && sizeof (m_room_cache) > 0) return 1;

    destruct (this_object ());

    return 0;
}

/**
 * Server is about to get destructed. Clean up all tracked rooms. This will
 * be helpful in coding, the server is able to recompile it's own rooms,
 * no more the need for a coder to dest rooms to check if the changes work
 **/
void
prepare_destruct ()
{
    catch (stop_server ());
}

/**
 * Manage and read maps into the memory. If the sizeof of the internal
 * map memory is bigger than what's the configured size, then the first
 * item is removed from the memory.
 *
 * Be very careful to keep the map memory big enough, else it is very
 * likely that the server will keep needlessly reading maps into the
 * memory, on every access.
 *
 * @param dir  The directory for map "database" files
 * @param file The map file to read
 *
 * @return The whole map contents. Or 0 if the map wasn't found
 **/
nomask static mixed
map_manager(string dir, string file)
{
    string s;
    mixed m;

#ifdef DEBUG
    write("Map_manager: /"+dir+file+"\n");
#endif

    if (sizeof (map_memory) > memory)
	map_memory = m_delete (map_memory, m_indices (map_memory)[0]);

    if (!(s = read_file ("/"+dir+file))) return 0;

    m = explode (s, "\n");

    map_memory += ([ file: m ]);

    return m;
}

/**
 * Read a terrain descriptor for a given map in given coordinates. Note that
 * this function does not take into account the map coordinate setup. For
 * that, one should use the get_terrain() function.
 *
 * @see get_terrain()
 *
 * @param x  The relative X coordinate to read
 * @param y  The relative Y coordinate to read
 * @param mp The map descriptor to read from
 *
 * @return String representing the possible terrain descriptor on the
 *         given map and coordinates. Zero is returned if map can't be
 *         read or coordinates are off the range.
 **/
nomask static string
read_map (int x, int y, string mp)
{
    mixed m;
    int xx;

#ifdef DEBUG
    write("Read_map: "+x+", "+y+" : "+mp+"\n");
#endif

    m = map_memory[mp];

    if (!m) {
	m = map_manager (maps_home, mp);
    }

    if (!m) {
	return 0;
    }

    /* Y coordinate out of range */
    if (sizeof (m) <= y) {
	return 0;
    }

    xx = x * tdesc_x;

    /* X coordinate out of range */
    if (sizeof (m[y]) <= xx) {
	return 0;
    }

    return m[y][ xx..(xx + tdesc_x - 1) ];
}

/**
* Find a correct map desc with given coordinates.
*
* @param x Absolute x coordinate on a map
* @param y Absolute y coordinate on a map
*
* @return Map descriptor or zero if no such map exists for given coordinates.
**/
nomask static mixed
find_map (int x, int y)
{
    int i;
    int n;

#ifdef DEBUG
    write("Find_map: "+x+", "+y+"\n");
#endif

    n = sizeof(maps);
    for (i=0; i < n; i++) {
	if ((x >= maps[i][X1] && y >= maps[i][Y1]) &&
	  (x <= maps[i][X2] && y <= maps[i][Y2])) {
	    return maps[i];
	}
    }

    return 0;
}

/**
 * Find a terrain type on a map. This uses absolute coordinates for the map,
 * seeking out first the suitable map to query, then calculates the relative
 * coordinates, that match the arrays... and uses read_map() to get the
 * actual terrain description.
 *
 * @see read_map()
 *
 * @param x Absolute X coordinate on the map
 * @param y Absolute Y coordinate on the map
 *
 * @return Terrain descriptor string is returned or zero if no descriptor
 *         was present in given coordinates.
 **/
nomask string
get_terrain (int x, int y)
{
    mixed crd;
    int rx, ry;

#ifdef DEBUG
    write("Get_terrain: "+x+", "+y+"\n");
#endif

    crd = find_map (x, y);

    if (!crd) {
	return 0;
    }

    /* Get the relative coordinates */
    rx = x - crd[X1];
    ry = y - crd[Y1];

    /* Then read the contents of the map */
    return read_map (rx, ry, crd[MP]);
}

/**
 * Clones a new room object based on the configuration. The result is
 * returned for object renaming.
 *
 * This function is called by the master object every time when there is a
 * virtual object request on the virtual area. This does not limit just
 * to the rooms, but currently it's all that this function is capable
 * of handling.
 *
 * @todo It probably would make more sense to push the config to the
 *       virtual room at this function, than to make it call back to the
 *       server to get the configuration. If nothing else, at least the
 *       coordinates should be pushed. --Fafnir
 *
 * @param path The file name of the requested virtual object
 *
 * @return The newly created virtual object. Or zero if failing to clone
 *         the object.
 **/
/* nomask */
object
compile_object (string path)
{
    int x, y;
    string terrain;
    string file;
    object ob;

    if (!path) return 0;
#ifdef DEBUG
    write("Compile_object:"+path+"\n");
#endif

    if (sscanf (path, rooms_home+"%d_%d", x, y) != 2)
	return 0;

    if (!(terrain = get_terrain(x, y))) return 0;
    if (!(file = m_terrains[terrain, TERRA_FILE])) return 0;

    /* Set up current coordinates */
    X = x; Y = y;

    ob = clone_object (terra_home+file);

    /* Track the room internally */
    if (ob) {
	if (!m_room_cache || sizeof (m_room_cache) < (MAX_ARRAY - 1)) {
	    m_room_cache += ({ ob });
	}
	if (m_virtual_look) {
	    ob->set_virtual_look (m_virtual_look);
	    ob->set_virtual_look_range (m_look_x_range, m_look_y_range);
	}
    }

    /* If it was cloned, call custom create func */
    if (ob) {
	if (create_func) call_other(ob, create_func);
    }

    return ob;
}

/**
 * Query last used coordinates to create a room. Rooms might ask their
 * coordinates through this function at creation time. Using this function is
 * a bit unsafe, as the last used coordinates are updated every time a new
 * room is created.
 *
 * @return Array of last used coordinates, 0 index contains X coordinate and 1
 *         index contains Y coordinate. These are absolute, unmodified
 *         coordinates.
 **/
nomask mixed
query_coordinates()
{
    return ({ X, Y });
}

/**
 * Query for configured objects in obj_cfg for coordinates. The coordinates
 * have to be absolute, real room coordinates.
 *
 * @param x The absolute X coordinate
 * @param y The absolute Y coordinate
 *
 * @return String array of configured object files for coordinates
 **/
nomask mixed
query_objects (int x, int y)
{
    return objects[x+"_"+y];
}

/**
 * Query for configured rooms home directory from the server.
 *
 * @return The configured rooms home directory
 **/
nomask string
query_rooms_home()
{
    return rooms_home;
}

/**
 * Query for configured home dir of descriptions. This directory is used
 * by virtual rooms to check if there are custom descriptions set for them,
 * based on the absolute room coordinates.
 *
 * @return The configured dir for descriptions
 **/
nomask string
query_descs_home()
{
    return descs_home;
}

/**
 * Checks if a passed file is a virtual room. Essentially just checks
 * the coordinates directly from the file name and then checks if a terrain
 * can be found for the coordinates. Noone should really need this function
 * in it's current form.
 *
 * This is used to check if the destination room in virtual room is loadable,
 * to create an actual exit for the room. This is one way to do it...
 *
 * @return 1 if the room is "virtual", 0 if it's not.
 **/
nomask status
is_virtual_room (string file)
{
    int x, y;
    string t;

    if(!file || sscanf(file, "%d_%d", x, y)!=2) return 0;

    if ((t = get_terrain (x, y))) {
	string obj;
	obj = m_terrains [t, TERRA_FILE];

	if (obj) return 1;
    }

    return 0;
}

/**
 * Gets an environment for absolute coordinates, with certain range
 * defined in xrange and yrange.
 *
 * @param x      The Absolute X coordinate on the map, this is used as
 *               the beginning point in X axis
 * @param y      The Absolute Y coordinate on the map, this is used as
 *               the beginning point in Y axis
 * @param xrange X coordinate range to check, starting from x
 * @param yrange Y coordinate range to check, starting from y
 *
 * @return Array of strings describing the environment asked.
 **/
nomask varargs string *
get_environment (int x, int y, int xrange, int yrange)
{
    mixed mapd;
    string *tmpx, *tmpy;
    string *map;
    string *map2;
    int y2;
    int x2;
    int n;

    mapd = find_map(x, y);
    if(!mapd) return 0;

    map = map_memory[mapd[MP]];
    if (!map) map = map_manager(maps_home, mapd[MP]);
    if (!map) return 0;

    y2 = y+yrange;
    x2 = x+xrange;

    if(x2 > mapd[X2]) {
	n = x2 - mapd[X2];
	tmpx = get_environment(mapd[X2]+1, y, n, yrange);
	xrange -= n;
	x2 = mapd[X2];
    }
    if(y2 > mapd[Y2]) {
	tmpy = get_environment(x, mapd[Y2]+1, xrange, y2 - mapd[Y2]);
	y2 = mapd[Y2];
    }

    x -= mapd[X1];
    y -= mapd[Y1];

    x2 -= mapd[X1];
    y2 -= mapd[Y1];

    map2 = ({});
    for(n = y; n <= y2; n++) {
	if (tmpx) {
	    map2 += ({map[n][x..x2] + tmpx[n-y]});
	} else {
	    map2 += ({map[n][x..x2]});
	}
    }
    if (tmpy) {
	map2 += tmpy;
    }

    return map2;
}

/**
 * Queries a terrain owner for the short description.
 *
 * @param s The terrain descriptor to ask for the desc
 *
 * @return Short desc currently set in the terrain's implementing room.
 *         On several occasions this probably doesn't return the actual
 *         short desc to the terrain, as it's quite likely for the terrain
 *         to generate descs when it's cloned into certain coordinates
 **/
nomask string
query_terrain_short (string s)
{
    if(!s) return 0;

    s = m_terrains[s, TERRA_FILE];
    if(!s) return 0;
    else return (string)(terra_home+s)->query_short();
}

/// Some utility functions

/**
 * Query a full map at given coordinates. This is will use absolute
 * coordinates for the room. First a corresponding configured map
 * matching the coordinates is found, after which the full map is
 * returned.
 *
 * @return Array containing the full map. Each row contains all terrains
 *         in x-axis. If map is not found, 0 is returned.
 **/
nomask string *
query_map (int x, int y)
{
    mixed tmp;
    string *map;

    tmp = find_map (x, y);
    if (!tmp) return 0;

    map = map_memory[tmp[MP]];
    if (!map) map = map_manager (maps_home, tmp[MP]);
    if (!map) return 0;

    return map;
}

/**
 * Queries a visual virtual map at given coordinates with given range.
 * Uses get_environment() internally, but in addition to that, applies
 * the user's own point into the proper place, applies the frame and
 * takes into account other potential decorations, as potential replacement
 * for terrain descriptor in visual map, and the ansi colors.
 *
 * @see get_environment()
 *
 * @param x_coordinate The begin X coordinate to start digging the map
 * @param y_coordinate The begin Y coordinate to start digging the map
 * @param x_range      How many terrains in x-axis we want to include
 * @param y_range      How many terrains in y-axis we want to include
 *
 * @return Array of strings describing the map, with it's frame and with
 *         a position for the player is to be expected to be when querying
 *         the map
 **/
nomask string *
query_virtual_map (
  int x_coordinate,
  int y_coordinate,
  int x_range,
  int y_range)
{
    string *visual_map;
    string *map;
    int x, y, i, j;

    x = x_range / 2;
    y = y_range / 2;

    /* Probably not the most effective way to do this, but... */
    for (i = 0; i < x_range && !map; i++) {
	map = get_environment (
	  x_coordinate - (x - i), y_coordinate - y, x_range - i, y_range);

	for (j = 1; j < y_range && !map; j++) {
	    map = get_environment (
	      x_coordinate - (x - i), y_coordinate - (y - j),
	      x_range - i, y_range - j);
	}
    }

    x = abs (x - (i - 1));
    y = abs (y - (j - 1));

    /* Build a frame around the map */
    if (map) {
	int k, map_size, l, length;
	string t_desc, last_desc, code, new_desc;

	map[y][x] = '@';

	map_size = sizeof (map);
	visual_map = allocate (map_size + 2);
	for (k = 0; k < (map_size + 2); k++) {
	    if (!k || k == map_size + 1) {
		visual_map[k] = sprintf ("+%'-'*s+", k ?
		  sizeof (map[map_size - 1]) :
		  sizeof (map[0]), "-");
	    } else {
		visual_map[k] = "|";
		length = sizeof (map[k - 1]);
		last_desc = 0;

		for (l = 0; l < length; l++) {
		    t_desc = map[k - 1][l..l];
		    new_desc = m_terrains[t_desc, TERRA_DESC];

		    if (t_desc != last_desc) {
			if (l) {
			    visual_map[k] += "%^reset%^";
			}

			code = m_terrains[t_desc, TERRA_ANSI];
			if (code) {
			    visual_map[k] += "%^" + code + "%^";
			}
		    }
		    if (new_desc) {
			visual_map[k] += new_desc;
		    } else {
			visual_map[k] += t_desc;
		    }
		    last_desc = t_desc;
		}
		visual_map[k] += "%^reset%^|";
	    }
	}
    }

    return visual_map;
}

/// The Reboot feature

/**
 * Inform the user that the area (server) is about to reboot the world
 *
 * @param time Time in seconds the area is going to be rebooted
 **/
nomask static void
inform_reboot (int time)
{
    int minutes;

    minutes = time / 60;

    virtual_users (TELL, "VIRTUAL WORLD: Reboot in "+minutes+" minutes!!!\n"+
      "VIRTUAL WORLD: You will be transfered to church.\n");
}

/**
 * Starts a reboot sequence. The sequence should be given in minutes. If
 * there already was a reboot sequence, it is cancelled. If there was no
 * reboot sequence running, and time is not given, the reboot will be
 * performed instantly.
 *
 * @param time Reboot time in minutes, or 0 to reboot instantly, as long
 *             as there's no reboot sequence already going.
 **/
nomask void
reboot_world (int time)
{
    if(time < 0) time = 0;

    if(!time) {
	if(reboot) {
	    reboot = 0;
	    if(find_call_out("count_reboot")) remove_call_out("count_reboot");
	    write("Reboot aborted!!!\n");
	    virtual_users(TELL, "VIRTUAL WORLD: Reboot aborted!!!\n");
	}
	else {
	    stop_server ();
	    start_server ();
	}

	return;
    }

    reboot = time * 60;

    write("Reboot in "+time+" minutes.\n");
    inform_reboot(reboot);

    call_out("count_reboot", 60);

    return;
}

/**
 * Periodically called function to count down to reboot. Once time is up,
 * area is supposed to reboot. The reboot counter is zeroed and configs
 * are re-read
 **/
nomask void
count_reboot()
{
    reboot -= 60;
    if(reboot <= 0) {
	stop_server ();
	start_server ();
	reboot = 0;
	return;
    }

    inform_reboot(reboot);

    call_out("count_reboot", 60);
    return;
}

//@}

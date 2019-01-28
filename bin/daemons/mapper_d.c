/*
** /bin/daemons/mapper_d.c
**
** Author: Tamarindo
**   Date: August 2018
**
** Purpose:
**
**	The Mapper Daemon registers information about rooms which
**	is communicated through the Telopt Daemon to clients that are
**	interested in consuming GMCP (Generic Mud Communication
**	Protocol) messages to enhance the playing experience.  The
**	primary use of the information the Mapper Daemon processes
**	is to drive mapping modules within the clients that conform
**	to MMP (Mud Mapping Protocol) XML script.
**
** Notes:
**
**	The Mapper Daemon was originally written to store the room
**	meta data inside of a serializable mapping, however it was
**	discovered that the number of rooms would out grow the maximum
**	size of a mapping. The current iteration (October 2018) uses
**	mappings within a mapping to hold the meta data such as:
**
**	map_rooms ([
**	    "/room": ([
**		"/room/church": ([
**		    "env": 12,
**		    "num": 1,
**		    "area": 2,
**		    "name": "Temple yard",
**		    "exits": ([
**			"east": 2,
**			"west": 3,
**			"north": 4,
**			"south": 5,
**			"northwest": 6
**		      ]),
**		    "coords": ({
**			0,
**			0,
**			0
**		      })
**		  ]),
**		"/room/path": ([ // Lazy loading rooms players have not visited yet
**		    "num": 7,    // or do not have required meta data set like env,
**		  ]),		 // area, etc.
**	      ]),
**	    "/areas/tristeza/virtual": ([
**		"/areas/tristeza/virtual/S7_7": ([
**		    "num": 13,
**		    "name": "Market Square, at Well and Gallows",
**		    "env": 12,
**		    "area": 1,
**		    "exits": ([
**			"up": 17,
**			"east": 10,
**			"west": 12,
**			"down": 16,
**			"north": 5,
**			"south": 18,
**			"northwest": 11,
**			"northeast": 9,
**			"southwest": 14,
**			"southeast": 15,
**		      ]),
**		    "coords": ({
**			0,
**			0,
**			0
**		      })
**		  ]),
**	      ]),
**	  ])
**
**	Further improvements are certainly possible if needed, such
**	as storing in a database. The necessity for the map is so that
**	the MMP maps/map.xml file could be generated which is necessary
**	for the mapper - however it has also been useful for calculating
**	coordinates of adjacent rooms.
**
** See Also:
**
**	https://wiki.mudlet.org/w/Manual:Mapper_Functions
**	https://github.com/IRE-Mudlet-Mapping/
**
*/
#include <map_environment_defs.h>
#include <map_area_defs.h>

#define CONFIGFILE "/data/d/mapper_d/config"
#define SAVEFILE "/data/d/mapper_d/mapper_d"
#define MAPXML "/data/d/mapper_d/maps/map.xml"
#define TIME_STEP 2
#define CHUNK 500

private static mapping map_areas;
private static string *xml_areas;
private static string *xml_rooms;
private static string *xml_environments;
private static int xml_generating = 0;

private int map_room_sequence;
private mapping map_rooms;

public varargs mapping register_map_room(mixed room, int update);
public void generate_xml_areas();
public void generate_xml_rooms(string area_key, string *map_room_keys);
public void generate_xml_environments();
public void write_xml_file();

private string get_area_key(string arg);

nomask void save_me() {
    save_object(SAVEFILE);
}

nomask void create() {
    int area_id, area_coord_x, area_coord_y, area_coord_z;
    string area_name, area_newbie;

    // Load the area names from a config file.
    map_areas = ([ ]);

    foreach (string line : explode(read_file(CONFIGFILE), "\n")) {
	// Example -> 1:City of Tristeza:0:0:0:N
	if (sscanf(line, "%d:%s:%d:%d:%d:%s",
	    area_id, area_name, area_coord_x, area_coord_y, area_coord_z,
	    area_newbie) == 6) {
	    map_areas[area_id] = ([
	      "name": area_name,
	      "x": area_coord_x,
	      "y": area_coord_y,
	      "z": area_coord_z,
	      "newbie": area_newbie,
	    ]);
	}
    }

    restore_object(SAVEFILE);

    if (map_room_sequence == 0) map_room_sequence = 1;

    if (map_rooms == 0) {
	map_rooms = ([]);
    }

    save_me();
}

/*
** Function: query_map_environments
**
**  Returns: mapping, environment information such as id, name, etc.
*/
public mapping query_map_environments() {
    return copy(MAP_ENVIRONMENTS);
}

#if 0
/*
** Function: query_map_colors
**
**  Returns: mapping, color information such as color number and htmlcolor hex code.
*/
public mapping query_map_colors() {
    return copy(MAP_COLORS);
}
#endif

/*
** Function: query_map_areas
**
**  Returns: mapping, area information such as id, name, etc.
*/
public mapping query_map_areas() {
    return copy(map_areas);
}

/*
** Function: query_map_area_name
**
** Parameters: int area_id, the key used to identify the area
**
**    Returns: string, area name
*/
public mapping query_map_area_name(int area_id) {
    if (member(map_areas, area_id) < 1 || member(map_areas[area_id], "name") < 1)
	return 0;

    return map_areas[area_id]["name"];
}

/*
** Function: query_map_rooms
**
**  Returns: mapping, keys are object_name's of rooms and values
**           are mappings containing the room num, area id,
**	     environment id, short desc, exits, doors, details
**           such as if it is a bank, shop, etc. and also a
**           map url representing the location of an MMP XML
**           file for the area which may not be implemented
**           for this MUD.
*/
public mapping query_map_rooms() {
    return copy(map_rooms);
}

/*
**   Function: get_area_key
**
**    Returns: string, everything before the final slash in the file
**             path of a room.
**
** Parameters: string arg, expecting the string file path (object_name)
**             of a room.
**
**    Purpose: Due to the max size of mappings being just over 10,000
**             it was necessary to create a number of smaller mappings
**             to store the rooms for the map. Meta data from the room
**             environment of the player is loaded in addition to some
**             lazy loading of information for the room's exits without
**             loading that exit's room object. This is to avoid
**             creating a ripple of room objects loading and experiencing
**             too long evaluation errors.
*/
private string get_area_key(string arg) {
    return arg[0..strrstr(arg, "/") - 1];
}

/*
**   Function: query_map_room
**
**    Returns: mapping, contains a 0 if the room information is
**             not present in the Mapper Daemon or a mapping
**             of room attributes like room num, area, environment,
**             short desc, exits, doors and map.
**
** Parameters: mixed arg, expecting either a room object, a
**             string file path (object_name) of a room or
**             an int which is the query_map_key() of the
**             room.
*/
public mapping query_map_room(mixed arg) {
    mapping map_room;
    string area_key;

    if (!arg)
	return 0;

    if (objectp(arg)) {
	if (!arg->is_room())
	    return 0;

	arg = object_name(arg);
    }

    if (stringp(arg)) {
	if (arg[0..0] != "/")
	    arg = "/" + arg;

	area_key = get_area_key(arg);

	if (member(map_rooms, area_key) > 0 &&
	  member(map_rooms[area_key], arg) > 0 &&
	  member(map_rooms[area_key][arg], "coords") > 0)
	    return copy(map_rooms[area_key][arg]);
    } else if (intp(arg)) {
	foreach (area_key, mapping area_rooms : map_rooms) {
	    foreach (string room_key, mapping room_info : area_rooms) {
		if (room_info["num"] == arg && member(room_info, "coords") > 0) {
		    map_room = copy(room_info);
		    break;
		}
	    }

	    // If found, escape this loop.
	    if (map_room) break;
	}
    } else return 0;

    return map_room ? map_room : register_map_room(arg);
}

/*
**   Function: set_map_coords
**
** Parameters: mixed arg, the room object or the file path of the room object
**
**             int x, y & z, the x, y and z coordinates, respectively
**
**    Returns: int, 1 for success and 0 for fail
**
**    Purpose: Sometimes one just needs to manually set the coordinates of a
**             room. This is useful for forcing coordinates to a room that may
**             be on the other side of a hidden exit or portal so once that is
**             done one could continue letting the daemon peform the auto
**             mapping.  Use this function rarely!
*/
public int set_map_coords(mixed arg, int x, int y, int z) {
    mapping map_room;
    string area_key;

    if (!arg)
	return 0;

    if (objectp(arg)) {
	if (!arg->is_room())
	    return 0;

	arg = object_name(arg);
    }

    if (stringp(arg)) {
	if (arg[0..0] != "/")
	    arg = "/" + arg;

	area_key = get_area_key(arg);

	if (member(map_rooms, area_key) > 0 &&
	  member(map_rooms[area_key], arg) > 0)
	    map_room = map_rooms[area_key][arg];
    } else if (intp(arg)) {
	foreach (area_key, mapping area_rooms : map_rooms) {
	    foreach (string room_key, mapping room_info : area_rooms) {
		if (room_info["num"] == arg) {
		    map_room = room_info;
		    break;
		}
	    }
	}
    }

    if (map_room) {
	map_room["coords"] = ({ x, y, z });
	save_me();
	return 1;
    }

    return 0;
}

/*
**   Function: query_map_coords
**
**    Returns: int *, contains a 0 if no coordinates could
**             be found or an integer array of ({X, Y, Z})
**             map coordinates for the given room.
**
** Parameters: mixed arg, expecting either a room object, a
**             string file path (object_name) of a room or
**             an int which is the query_map_key() of the
**             room.
*/
public int *query_map_coords(mixed arg) {
    int *map_coords;
    string area_key;

    if (!arg)
	return 0;

    if (objectp(arg)) {
	if (!arg->is_room())
	    return 0;

	arg = object_name(arg);
    }

    if (stringp(arg)) {
	if (arg[0..0] != "/")
	    arg = "/" + arg;

	area_key = get_area_key(arg);

	if (member(map_rooms, area_key) > 0 &&
	  member(map_rooms[area_key], arg) > 0 &&
	  member(map_rooms[area_key][arg], "coords") > 0)
	    return copy(map_rooms[area_key][arg]["coords"]);
    } else if (intp(arg)) {
	foreach (area_key, mapping area_rooms : map_rooms) {
	    foreach (string room_key, mapping room_info : area_rooms) {
		if (room_info["num"] == arg && member(room_info, "coords") > 0) {
		    map_coords = copy(room_info["coords"]);
		    break;
		}
	    }

	    // If found, escape this loop.
	    if (map_coords) break;
	}
    } else return 0;

    return map_coords;
}

/*
**   Function: query_exit_coords
**
**    Returns: int *, the ({X, Y, Z}) coordinates of an exit got a given
**             exit's direction.  For instance, going northeast would add
**             1 to the X and 1 to the Y but 0 to the Z because we did not
**             go up or down.
**
** Parameters: string exit, the direction these coordinates will lead us to
**
**             int *room_coords, the coordinates of the room we are starting
**             from.
**
**      Notes: MMP encourages the use of in and out on the map and typically
**             these would show up on the map near the base room likely in a
**             coordinate not taken up by the cardinal compass directions.
**             It is encouraged to figure out a way to do this effectively
**             with coordinates too.
*/
private int *query_exit_coords(string exit, int *room_coords) {
    int *exit_coords;

    switch (exit) {
    case "east": case "e":
	exit_coords = ({ room_coords[0] + 1, room_coords[1], room_coords[2] });
	break;
    case "northeast": case "ne":
	exit_coords = ({ room_coords[0] + 1, room_coords[1] + 1, room_coords[2] });
	break;
    case "north": case "n":
	exit_coords = ({ room_coords[0], room_coords[1] + 1, room_coords[2] });
	break;
    case "northwest": case "nw":
	exit_coords = ({ room_coords[0] - 1, room_coords[1] + 1, room_coords[2] });
	break;
    case "west": case "w":
	exit_coords = ({ room_coords[0] - 1, room_coords[1], room_coords[2] });
	break;
    case "southwest": case "sw":
	exit_coords = ({ room_coords[0] - 1, room_coords[1] - 1, room_coords[2] });
	break;
    case "south": case "s":
	exit_coords = ({ room_coords[0], room_coords[1] - 1, room_coords[2] });
	break;
    case "southeast": case "se":
	exit_coords = ({ room_coords[0] + 1, room_coords[1] - 1, room_coords[2] });
	break;
    case "up": case "u":
	exit_coords = ({ room_coords[0], room_coords[1], room_coords[2] + 1 });
	break;
    case "down": case "d":
	exit_coords = ({ room_coords[0], room_coords[1], room_coords[2] - 1 });
	break;
	/* TODO: Figure out how to place these
	    case "in":
		exit_coords = ({ room_coords[0], room_coords[1], room_coords[2]  });
		break;
	    case "out":
		exit_coords = ({ room_coords[0], room_coords[1], room_coords[2]  });
		break;
	*/
    }

    return exit_coords;
}

/*
**   Function: register_map_room
**
**    Returns: mapping, contains a 0 if the room information could
**             not be registered in the Mapper Daemon or a mapping
**             of room attributes like room num, area, environment,
**             short desc, exits, doors and map.
**
** Parameters: mixed arg, expecting either a room object, a
**             string file path (object_name) of a room.
**
**             int update, optional.  When > 0, the function will
**             attempt to re-register the room information from
**             scratch other than the room num, which is preserved
**             indefinitely.
*/
public varargs mapping register_map_room(mixed room, int update) {
    string key, area_key, proper_exit;
    int area, env;
    string short_desc, *details;
    mapping registry, exits;
    object room_ob, exit_room;

    if (!objectp(room)) {
	if (stringp(room))
	    room_ob = load_object(room);
	else return 0;
    } else room_ob = room;

    if (!objectp(room_ob) || !room_ob->is_room())
	return 0;
    else {
	key = object_name(room_ob);

	if (key[0..0] != "/")
	    key = "/" + key;

	area_key = get_area_key(key);

	if (!update) {
	    if (member(map_rooms, area_key) > 0 &&
	      member(map_rooms[area_key], key) > 0 &&
	      member(map_rooms[area_key][key], "coords") > 0 &&
	      member(map_rooms[area_key][key], "area") > 0 &&
	      member(map_rooms[area_key][key], "env") > 0) {
		room_ob->set_map_key(map_rooms[area_key][key]["num"]);
		return copy(map_rooms[area_key][key]);
	    }
	}

	if (member(map_rooms, area_key) > 0 && member(map_rooms[area_key], key) > 0) {
	    registry = ([ "num": map_rooms[area_key][key]["num"], ]);
	} else {
	    registry = ([ "num": map_room_sequence++, ]);
	}

	room_ob->set_map_key(registry["num"]);

	env = room_ob->query_map_environment();

	if (member(MAP_ENVIRONMENTS, env) < 1) registry["env"] = 0;
	else registry["env"] = env;

	area = room_ob->query_map_area();

	if (member(map_areas, area) < 1) registry["area"] = 0;
	else registry["area"] = area;

	if (room_ob->query_map_center()) {
	    registry["coords"] = ({0, 0, 0});
	} else if (area && member(registry, "coords") < 1) {
	    // Let's try to fetch our coordinates based on an adjacent room in the same area.
	    foreach (string coord_area_key, mapping area_rooms : map_rooms) {
		if (area_rooms && mappingp(area_rooms)) {
		    foreach (string room_key, mapping room_info : area_rooms) {
			if (room_info && mappingp(room_info) &&
			  member(room_info, "area") > 0 &&
			  room_info["area"] == area &&
			  member(room_info, "coords") > 0 &&
			  member(room_info, "exits") > 0) {
			    foreach (string exit_key, int exit_value: room_info["exits"]) {
				if (registry["num"] == exit_value) {
				    int *tmp_coords = query_exit_coords(exit_key, room_info["coords"]);

				    if (tmp_coords) {
					registry["coords"] = tmp_coords;
				    }
				}
			    }
			}

			// If we found something, we're done looping.
			if (member(registry, "coords") > 0)
			    break;
		    }
		}

		// If we found something, we're done looping.
		if (member(registry, "coords") > 0)
		    break;
	    }
	} // Otherwise we claim no "coords" for this room until it has a valid area!

	short_desc = room_ob->query_short(2);

	// This handles wacky mini-map short descs such as priest gardens -- Copied from Germ
	short_desc = stringp(short_desc) ? explode(short_desc,"\n")[<1] : "<UNKNOWN PLACE>";

	registry["name"] = short_desc;

	// Doors!
	if (area != MAP_AREA_PLAYER_HOMES_OF_TRISTEZA && sizeof(all_inventory(room_ob))) { // Skip the complex door for now
	    foreach (object door : filter(all_inventory(room_ob), (: $1->is_door() :))) {
		string exit_key, exit_value;

		if ((exit_key = door->query_dir()) && (exit_value = object_name(environment(door->query_partner_door())))) {
		    if (exit_value[0..0] != "/")
			proper_exit = "/" + exit_value;
		    else proper_exit = exit_value;

		    string exit_area_key = get_area_key(proper_exit);

		    if (member(map_rooms, exit_area_key) < 1)
			map_rooms[exit_area_key] = ([ ]);

		    if (member(map_rooms[exit_area_key], proper_exit) < 1)
			map_rooms[exit_area_key][proper_exit] = ([ "num": map_room_sequence++, ]);

		    if (!registry["exits"] || !mappingp(registry["exits"]))
			registry["exits"] = ([ exit_key: map_rooms[exit_area_key][proper_exit]["num"], ]);
		    else registry["exits"][exit_key] = map_rooms[exit_area_key][proper_exit]["num"];

		    if (!registry["doors"] || !pointerp(registry["doors"]))
			registry["doors"] = ({ exit_key });
		    else registry["doors"] += ({ exit_key });
		}
	    }
	}

	// Exits!
	if (exits = room_ob->query_exits()) {
	    foreach (string exit_key, mixed exit_value : exits) {
		if (!stringp(exit_value)) // Consider handling closures in future
		    continue;

		if (exit_value[0..0] != "/")
		    proper_exit = "/" + exit_value;
		else proper_exit = exit_value;

		string exit_area_key = get_area_key(proper_exit);

		if (member(map_rooms, exit_area_key) < 1)
		    map_rooms[exit_area_key] = ([ ]);

		if (member(map_rooms[exit_area_key], proper_exit) < 1)
		    map_rooms[exit_area_key][proper_exit] = ([ "num": map_room_sequence++, ]);

		if (!registry["exits"] || !mappingp(registry["exits"]))
		    registry["exits"] = ([ exit_key: map_rooms[exit_area_key][proper_exit]["num"], ]);
		else registry["exits"][exit_key] = map_rooms[exit_area_key][proper_exit]["num"];
	    }
	}

	if (room_ob->query_guild())
	    if (!registry["details"] || !pointerp(registry["details"]))
		registry["details"] = ({ "guild" });
	    else if (member(registry["details"], "guild") < 0)
		registry["details"] += ({ "guild" });

	if (function_exists("create_shop", room_ob))
	    if (!registry["details"] || !pointerp(registry["details"]))
		registry["details"] = ({ "shop" });
	    else if (member(registry["details"], "shop") < 0)
		registry["details"] += ({ "shop" });

	if (function_exists("create_pub", room_ob))
	    if (!registry["details"] || !pointerp(registry["details"]))
		registry["details"] = ({ "pub" });
	    else if (member(registry["details"], "pub") < 0)
		registry["details"] += ({ "pub" });

	if (present("mailbox", room_ob))
	    if (!registry["details"] || !pointerp(registry["details"]))
		registry["details"] = ({ "post" });
	    else if (member(registry["details"], "post") < 0)
		registry["details"] += ({ "post" });

	if (room_ob->query_bank_name())
	    if (!registry["details"] || !pointerp(registry["details"]))
		registry["details"] = ({ "bank" });
	    else if (member(registry["details"], "bank") < 0)
		registry["details"] += ({ "bank" });

	if (member(map_rooms, area_key) < 1)
	    map_rooms[area_key] = ([ key: registry ]);
	else map_rooms[area_key][key] = registry;

	save_me();

	return copy(registry);
    }
}

/*
** Function: generate_xml
**
**  Returns: void
**
**  Purpose: Transforms the Mapper Daemon data into an array of XML lines.
*/
public void generate_xml() {
    int i = 1, j = 0, k, size;

    xml_areas = ({});
    xml_rooms = ({});
    xml_environments = ({});
    xml_generating = 1;

    call_out("generate_xml_areas", i += TIME_STEP);

    foreach (string area_key, mapping area_rooms : map_rooms) {
	j = 0;
	size = sizeof(area_rooms);

	while (j < size) {
	    k = j + CHUNK;
	    call_out( "generate_xml_rooms", i += TIME_STEP, area_key, m_indices(area_rooms)[j..k] );
	    j = k + 1;
	}
    }

    call_out("generate_xml_environments", i += TIME_STEP);

    call_out("write_xml_file", i += TIME_STEP);
}

/*
** Function: generate_xml_generating
**
**  Returns: int, 1 for generating, 0 for not generating
**
**  Purpose: Indicator to explain if the XML is currently being generated for the map.
*/
public int query_xml_generating() {
    return xml_generating;
}

/*
** Function: generate_xml_areas
**
**  Returns: void
**
**  Purpose: Transforms the map_areas mapping into an array of XML lines.
*/
public void generate_xml_areas() {
    foreach (int key : map_areas) {
	xml_areas += ({ sprintf("   <area id=\"%d\" name=\"%s\" x=\"0\" y=\"0\" />", key,
	    (map_areas[key]["newbie"] == "Y" ? sprintf("*%s", map_areas[key]["name"])
	      : map_areas[key]["name"])) });
    }

    if (find_player("tamarindo")) find_player("tamarindo")->tell_me("Finished generating areas xml.");
}

/*
**   Function: generate_xml_rooms
**
** Parameters: string area_key, the area (file path) container for the rooms 
**
**             string * map_room_keys, select keys from the map_rooms mapping
**
**    Returns: void
**
**    Purpose: Transforms select entities within the map_rooms mapping into an array of XML lines.
*/
public void generate_xml_rooms(string area_key, string *map_room_keys) {
    string xml;

    foreach (string key : map_room_keys) {
	mapping room_info = map_rooms[area_key][key];

	if (room_info && mappingp(room_info)) {
	    int has_doors = member(room_info, "doors") > 0 ? 1 : 0;

	    if (member(room_info, "name") > 0 && member(room_info, "coords") > 0) {
		if (!xml) {
		    xml = sprintf("   <room id=\"%d\" area=\"%d\" title=\"%s\" environment=\"%d\"",
		      room_info["num"], room_info["area"], room_info["name"], room_info["env"]);
		} else {
		    xml += sprintf("   <room id=\"%d\" area=\"%d\" title=\"%s\" environment=\"%d\"",
		      room_info["num"], room_info["area"], room_info["name"], room_info["env"]);
		}

		if (member(room_info, "details")) {
		    foreach (string detail : room_info["details"]) {
			if (detail == "bank") {
			    xml += " marker=\"Bank\" markerdir=\"north\"";
			} else if (detail == "important") {
			    xml += " important=\"1\" color=\"#ff0000\"";
			}
		    }
		}

		xml += ">\n";
		xml += sprintf("      <coord x=\"%d\" y=\"%d\" z=\"%d\" />\n",
		  room_info["coords"][0], room_info["coords"][1], room_info["coords"][2]);

		if (member(room_info, "exits") > 0) {
		    foreach (string direction : room_info["exits"]) {
			xml += sprintf("      <exit direction=\"%s\" target=\"%d\"", direction, room_info["exits"][direction]);

			if (has_doors && member(room_info["doors"], direction) != -1) {
			    xml += " door=\"1\"";
			}

			xml += "/>\n";
		    }
		}

		if (key != map_room_keys[sizeof(map_room_keys) - 1])
		    xml += "   </room>\n";
		else xml += "   </room>";
	    }
	}
    }

    if (xml) xml_rooms += ({ xml });

    if (find_player("tamarindo")) find_player("tamarindo")->tell_me(sprintf("Generated rooms xml for %d rooms.", sizeof(map_room_keys)));
}

/*
** Function: generate_xml_environments
**
**  Returns: void
**
**  Purpose: Transforms the MAP_ENVIRONMENTS mapping into an array of XML lines.
*/
public void generate_xml_environments() {
    foreach (int key : MAP_ENVIRONMENTS) {
	string color_key = MAP_ENVIRONMENTS[key]["color"];
	string color = MAP_COLORS[color_key][0];
	string htmlcolor = MAP_COLORS[color_key][1];

	xml_environments += ({ sprintf("   <environment id=\"%d\" name=\"%s\" color=\"%d\" htmlcolor=\"#%s\" />",
	    key, MAP_ENVIRONMENTS[key]["name"], color, htmlcolor) });
    }

    if (find_player("tamarindo")) find_player("tamarindo")->tell_me("Finished generating environments xml.");
}

/*
** Function: write_xml_file
**
**  Returns: void
**
**  Purpose: Writes xml in the MMP (MUD Mapping Protocol) format to a file stored on the
**           MUD's website that could be loaded by the MUD clients like Mudlet.
*/
public void write_xml_file() {
    string xml = "<?xml version=\"1.0\"?>\n";

    xml += "<map>\n";

    xml += "<areas>\n";
    xml += implode(xml_areas, "\n");
    xml += "\n</areas>\n";

    xml += "<rooms>\n";
    xml += implode(xml_rooms, "\n");
    xml += "\n</rooms>\n";

    xml += "<environments>\n";
    xml += implode(xml_environments, "\n");
    xml += "\n</environments>\n";

    xml += "</map>\n";

    xml_areas = 0;
    xml_rooms = 0;
    xml_environments = 0;
    xml_generating = 0;

    if (find_player("tamarindo")) find_player("tamarindo")->tell_me("Finished generating map file.");

    write_file(MAPXML, xml, 1);
}

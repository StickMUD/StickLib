/*
** /basic/player/telopt.c
**
** Author: Tamarindo
**
**   Date: August 2018
**
** This module performs some of the core handling of telnet negotiation
** driven from a driver hook in the master object.  These messages are passed
** to the telopt_negotiate function.  This file process Mud Server Status
** Protocol (MSSP) and Generic Mud Communications Protocol (GMCP) messages.
**
** If we establish that a client could process GMCP messages, we set an
** environmental variable in the player object like set_env("gmcp", 1).
** Thereafter, there's a good bit of code spread about the mudlib that will
** perform a query_env("gmcp") to determine whether to send a GMCP event to
** the Telopt Daemon.  The Telopt Daemon does some preprocessing and sends
** messages to the gmcp_message function in this module.  In many cases, the
** information is stored in the nosave gmcp_cache mapping available in this
** module.  If the value was not yet in the cache, the GMCP will be sent to
** the player's client (i.e. Mudlet).  If the value was already in the cache,
** the message will not be sent to the player's client.  There is also capability
** for the game to receive messages from the player's client to request that the
** game send certain GMCP messages.  For example, if there was a window in the
** player's client tracking the skills of a player and the player wanted to
** dig further into the details of that skills description the client window
** could send a request to the game to send that information on to the client.
** Messages transferred between the game and the client are in JSON format.
** The mapping for the cache management is fairly close to the format, but is
** optimized as best as the author could manage to enable the game to quickly
** parse the cache where needed.
**
** References:
**
**	https://tintin.sourceforge.io/protocols/mssp/
**	http://mudflinging.tumblr.com/post/37634529575/gmcp-negotiation-in-ldmud
**	https://www.gammon.com.au/gmcp
**	http://www.aardwolf.com/wiki/index.php/Clients/GMCP
**	http://nexus.ironrealms.com/GMCP
**
** See Also:
**
**	/include/mssp_defs.h
**	/include/gmcp_defs.h
**	/bin/daemons/telopt_d.c
*/
#ifndef TELNET_H__
#include "/sys/telnet.h"
#endif /* TELNET_H__ */

#ifndef LPC_LPCTYPES_H
#include "/sys/lpctypes.h"
#endif /* LPC_LPCTYPES_H */

#ifndef LPC_TLS_H
#include "/sys/tls.h"
#endif /* LPC_TLS_H */

#ifndef GMCP_DEFS_H
#include <gmcp_defs.h>
#endif /* GMCP_DEFS_H */

#ifndef MSSP_DEFS_H
#include <mssp_defs.h>
#endif /* MSSP_DEFS_H */

#ifndef PLAYER_C
int set_env(mixed e, mixed val);
mixed query_env(mixed e);
#endif /* PLAYER_C */

#ifndef _DAEMONS_H
#include <daemons.h>
#endif /* _DAEMONS_H */

#ifndef LEVELS_H
#include <levels.h>
#endif /* LEVELS_H */

#ifndef _MUD_NAME_H
#include <mud_name.h>
#endif /* _MUD_NAME_H */

static mapping gmcp_cache;

// Prototypes
public varargs int gmcp_message(string package, mixed value, int refresh);

/*
** Function: query_gmcp_cache
**
**  Returns: mapping, all of the GMCP data that should sensibly be cached
**	so not to continually send it on to the player's client when they
**	should already have the information.
**
**    Note: This function is here for debugging purposes.
*/
public mapping query_gmcp_cache() {
    return copy(gmcp_cache);
}

/*
**   Function: parse_cache
**
**    Purpose:
**
**	Accepts data matching up to the GMCP spec (or this game's customizations)
**	and stores it in a nosave mapping within the player object if there is
**	no value in the cache or if the cache needs updated because the data has
**	has changed.
**
** Parameters:
**
**	string package, one of the GMCP_PKG defines from /include/gmcp_defs.h
**		(i.e. GMCP_PKG_ROOM_INFO is "Room.Info").
**
**	mixed gmcp, often a string, array or mapping containing various GMCP_KEY
**		keys from /include/gmcp_defs.h and data values (i.e. a mapping
**		related to "Room.Info" like ([ "num": 1, "name": "Temple Yard" ]).
**
**	string *keys, GMCP_KEY keys that relate with a single GMCP_PKG as defined
**		in /include/gmcp_defs.h (i.e. related to "Room.Info" ({ "num",
**		"name", etc }) although these will actually be defines like ({
**		GMCP_KEY_ROOM_INFO_NUM, GMCP_KEY_ROOM_INFO_NAME, etc. })
**
**    Returns: mixed, if the cache was updated, this will contain the results
**	in the format similar to what was originally sent in to this function
**	that aligns with the GMCP specification.  If the cache was not updated
**	then this function will return 0, prompting the function that called
**	this to send no data to the intended recipient, because they should
**	already have it. 
*/
private mixed
parse_cache(string package, mixed gmcp, string *keys) {
    mixed result;
    int updated = 0;

    if (pointerp(gmcp)) {
	foreach (mixed element : gmcp) {
	    if (stringp(element)) {
		if (member(gmcp_cache, package) < 1) {
		    gmcp_cache[package] = ({ element });
		} else {
		    gmcp_cache[package] += element;
		}
	    } else if (mappingp(element)) {
		string id = element[keys[0]];

		if (member(gmcp_cache, package) < 1) {
		    gmcp_cache[package] = ([ ]);
		}

		for (int key = 1; key < sizeof(keys); key++) {
		    if (member(gmcp_cache[package], id) < 1) {
			updated = 1;

			gmcp_cache[package][id] = ([ keys[key]: element[keys[key]] ]);
		} else if (member(gmcp_cache[package][id], keys[key]) < 1) {
		    updated = 1;

		    gmcp_cache[package][id][keys[key]] = element[keys[key]];
		} else {
		    mixed sent = element[keys[key]];
		    mixed cached = gmcp_cache[package][id][keys[key]];

		    if (sent != cached) {
			updated = 1;
		    }

		    gmcp_cache[package][id][keys[key]] = element[keys[key]];
		}
	    }
	}

	if (!result || !pointerp(result)) {
	    result = ({ element });
	} else result += ({ element });
    }
} else if (mappingp(gmcp)) {
    if (package == GMCP_PKG_CHAR_ITEMS) {
	string location = gmcp[GMCP_KEY_CHAR_ITEMS_LOCATION];
	int dummy;

	if (member(({ GMCP_VALUE_CHAR_ITEMS_LOCATION_INV,
	      GMCP_VALUE_CHAR_ITEMS_LOCATION_ROOM }), location) < 0
	  && sscanf(location, GMCP_VALUE_CHAR_ITEMS_LOCATION_CONTAINER, dummy) != 1) {
	    return 0; // Shouldn't happen
	}

	if (member(gmcp_cache, package) < 1) {
	    gmcp_cache[package] = ([ ]);
	}

	if (member(gmcp_cache[package], location) < 1) {
	    updated = 1;
	    gmcp_cache[package][location] = ([ ]);

	    // List package uses GMCP_KEY_CHAR_ITEMS_ITEMS, while
	    // Add/Update packages use GMCP_KEY_CHAR_ITEMS_ITEM per the spec
	    string items_key = member(gmcp, GMCP_KEY_CHAR_ITEMS_ITEMS) > 0
	    ? GMCP_KEY_CHAR_ITEMS_ITEMS
	    : GMCP_KEY_CHAR_ITEMS_ITEM;

	    foreach (mapping map : gmcp[items_key]) {
		string id = map[keys[0]];

		for (int key = 1; key < sizeof(keys); key++) {
		    if (member(gmcp_cache[package][location], id) < 1) {
			gmcp_cache[package][location][id] = ([ keys[key]: map[keys[key]] ]);
#if 0
		    {
#endif
		    } else {
			gmcp_cache[package][location][id][keys[key]] = map[keys[key]];
		    }
		}

		if (!result || !pointerp(result)) {
		    result = ([
		      GMCP_KEY_CHAR_ITEMS_LOCATION: location,
		      items_key: ({ map })
		    ]);
		} else result[items_key] += ({ map });
	    }
	} else {
	    // List package uses GMCP_KEY_CHAR_ITEMS_ITEMS, while
	    // Add/Update packages use GMCP_KEY_CHAR_ITEMS_ITEM per the spec
	    string items_key = member(gmcp, GMCP_KEY_CHAR_ITEMS_ITEMS) > 0
	    ? GMCP_KEY_CHAR_ITEMS_ITEMS
	    : GMCP_KEY_CHAR_ITEMS_ITEM;

	    foreach (mapping map : gmcp[items_key]) {
		string id = map[keys[0]];

		for (int key = 1; key < sizeof(keys); key++) {
		    if (member(gmcp_cache[package][location], id) < 1) {
			gmcp_cache[package][location][id] = ([ keys[key]: map[keys[key]] ]);
#if 0
		    {
#endif
			updated = 1;
		    } else if (member(gmcp_cache[package][location][id], keys[key]) < 1) {
			gmcp_cache[package][location][id][keys[key]] = map[keys[key]];
			updated = 1;
		    } else {
			mixed sent = map[keys[key]];
			mixed cached = gmcp_cache[package][location][id][keys[key]];

			if (sent != cached) {
			    gmcp_cache[package][location][id][keys[key]] = map[keys[key]];
			    updated = 1;
			}
		    }
		}

		if (!result || !pointerp(result)) {
		    result = ([
		      GMCP_KEY_CHAR_ITEMS_LOCATION: location,
		      items_key: ({ map })
		    ]);
		} else result[items_key] += ({ map });
	    }
	}
    } else if (package == GMCP_PKG_GROUP) {
	foreach (string key : keys) {
	    if (member(gmcp, key) > 0) {
		if (member(gmcp_cache, package) < 1) {
		    gmcp_cache[package] = ([ ]);
		}

		if (key == GMCP_KEY_GROUP_MEMBERS && pointerp(gmcp[key])) {
		    string *names;

		    // Let's first fetch a list of player names in our party from the cache,
		    // if they exist, so we could determine if someone had dropped out of the
		    // party and we need to update the cache and send to the client.
		    if (member(gmcp_cache[package], key) > 0 && sizeof(gmcp_cache[package][key])) {
			names = m_indices(gmcp_cache[package][key]);
		    }

		    foreach (mapping player : gmcp[key]) {
			if (member(player, GMCP_KEY_GROUP_MEMBERS_NAME) > 0) {
			    // Remove this player from the balance of the names we were tracking.
			    // We'll check if any names are left at the end in our balance and if
			    // so we'll trigger an update to the client.
			    if (names && sizeof(names) && member(names, player[GMCP_KEY_GROUP_MEMBERS_NAME]) != -1) {
				names -= ({ player[GMCP_KEY_GROUP_MEMBERS_NAME] });
			    }

			    // If none of the players in the party exist, add this to the cache!
			    if (member(gmcp_cache[package], key) < 1) {
				updated = 1;

				gmcp_cache[package][key] = ([
				  player[GMCP_KEY_GROUP_MEMBERS_NAME] : ([
				    GMCP_KEY_GROUP_MEMBERS_INFO_HP: player[GMCP_KEY_GROUP_MEMBERS_INFO][GMCP_KEY_GROUP_MEMBERS_INFO_HP],
				    GMCP_KEY_GROUP_MEMBERS_INFO_MAXHP: player[GMCP_KEY_GROUP_MEMBERS_INFO][GMCP_KEY_GROUP_MEMBERS_INFO_MAXHP],
				    GMCP_KEY_GROUP_MEMBERS_INFO_SP: player[GMCP_KEY_GROUP_MEMBERS_INFO][GMCP_KEY_GROUP_MEMBERS_INFO_SP],
				    GMCP_KEY_GROUP_MEMBERS_INFO_MAXSP: player[GMCP_KEY_GROUP_MEMBERS_INFO][GMCP_KEY_GROUP_MEMBERS_INFO_MAXSP],
				    GMCP_KEY_GROUP_MEMBERS_INFO_FP: player[GMCP_KEY_GROUP_MEMBERS_INFO][GMCP_KEY_GROUP_MEMBERS_INFO_FP],
				    GMCP_KEY_GROUP_MEMBERS_INFO_MAXFP: player[GMCP_KEY_GROUP_MEMBERS_INFO][GMCP_KEY_GROUP_MEMBERS_INFO_MAXFP],
				    GMCP_KEY_GROUP_MEMBERS_INFO_LEVEL: player[GMCP_KEY_GROUP_MEMBERS_INFO][GMCP_KEY_GROUP_MEMBERS_INFO_LEVEL],
				    GMCP_KEY_GROUP_MEMBERS_INFO_HERE: player[GMCP_KEY_GROUP_MEMBERS_INFO][GMCP_KEY_GROUP_MEMBERS_INFO_HERE],
				  ])
				]);
			    } else if (member(gmcp_cache[package][key], player[GMCP_KEY_GROUP_MEMBERS_NAME]) < 1) {
				// If the player does not exist in the cache, add the player!
				updated = 1;

				gmcp_cache[package][key][player[GMCP_KEY_GROUP_MEMBERS_NAME]] = ([
				  GMCP_KEY_GROUP_MEMBERS_INFO_HP: player[GMCP_KEY_GROUP_MEMBERS_INFO][GMCP_KEY_GROUP_MEMBERS_INFO_HP],
				  GMCP_KEY_GROUP_MEMBERS_INFO_MAXHP: player[GMCP_KEY_GROUP_MEMBERS_INFO][GMCP_KEY_GROUP_MEMBERS_INFO_MAXHP],
				  GMCP_KEY_GROUP_MEMBERS_INFO_SP: player[GMCP_KEY_GROUP_MEMBERS_INFO][GMCP_KEY_GROUP_MEMBERS_INFO_SP],
				  GMCP_KEY_GROUP_MEMBERS_INFO_MAXSP: player[GMCP_KEY_GROUP_MEMBERS_INFO][GMCP_KEY_GROUP_MEMBERS_INFO_MAXSP],
				  GMCP_KEY_GROUP_MEMBERS_INFO_FP: player[GMCP_KEY_GROUP_MEMBERS_INFO][GMCP_KEY_GROUP_MEMBERS_INFO_FP],
				  GMCP_KEY_GROUP_MEMBERS_INFO_MAXFP: player[GMCP_KEY_GROUP_MEMBERS_INFO][GMCP_KEY_GROUP_MEMBERS_INFO_MAXFP],
				  GMCP_KEY_GROUP_MEMBERS_INFO_LEVEL: player[GMCP_KEY_GROUP_MEMBERS_INFO][GMCP_KEY_GROUP_MEMBERS_INFO_LEVEL],
				  GMCP_KEY_GROUP_MEMBERS_INFO_HERE: player[GMCP_KEY_GROUP_MEMBERS_INFO][GMCP_KEY_GROUP_MEMBERS_INFO_HERE],
				]);
			    } else {
				// Let's see if anything changed with this player
				foreach (string info : ({
				    GMCP_KEY_GROUP_MEMBERS_INFO_HP, GMCP_KEY_GROUP_MEMBERS_INFO_MAXHP,
				    GMCP_KEY_GROUP_MEMBERS_INFO_SP, GMCP_KEY_GROUP_MEMBERS_INFO_MAXSP,
				    GMCP_KEY_GROUP_MEMBERS_INFO_FP, GMCP_KEY_GROUP_MEMBERS_INFO_MAXFP,
				    GMCP_KEY_GROUP_MEMBERS_INFO_LEVEL,
				    GMCP_KEY_GROUP_MEMBERS_INFO_HERE })) {
				    mixed sent = player[GMCP_KEY_GROUP_MEMBERS_INFO][info];
				    mixed cached = gmcp_cache[package][key][player[GMCP_KEY_GROUP_MEMBERS_NAME]][info];

				    if (sent != cached) {
					updated = 1;
					gmcp_cache[package][key][player[GMCP_KEY_GROUP_MEMBERS_NAME]][info] = sent;
				    }
				}
			    }
			}
		    }

		    if (sizeof(names)) {
			updated = 1;

			// Delete names of party members who are no longer in the party from the cache.
			foreach (string name : names) {
			    m_delete(gmcp_cache[package][key], name);
			}
		    }

		    // This ends the block of code for the special handling of GMCP_KEY_GROUP_MEMBERS information 
		} else if (member(gmcp_cache[package], key) < 1) {
		    updated = 1;
		    gmcp_cache[package][key] = gmcp[key];
		} else {
		    mixed sent = gmcp[key];
		    mixed cached = gmcp_cache[package][key];

		    if (sent != cached) {
			updated = 1;
			gmcp_cache[package][key] = sent;
		    }
		}

		if (!result && !mappingp(result)) {
		    result = ([ key: gmcp[key] ]);
#if 0
		{
#endif
		} else result[key] = gmcp[key];
	    }
	}
    } else {
	foreach (string key : keys) {
	    if (member(gmcp, key) > 0) {
		if (pointerp(gmcp[key]) || mappingp(gmcp[key])) {
		    updated = 1;
		    gmcp_cache[package][key] = gmcp[key];
		} else {
		    mixed sent = gmcp[key];
		    mixed cached = gmcp_cache[package][key];

		    if (sent != cached) {
			updated = 1;
			gmcp_cache[package][key] = sent;
		    }
		}

		if (!result && !mappingp(result)) {
		    result = ([ key: gmcp[key] ]);
#if 0
		{
#endif
		} else result[key] = gmcp[key];
	    }
	}
    }
}

return updated ? result : 0;
}

/*
**   Function: gmcp_cache_filter
**
**    Purpose:
**
**	A method that controls whether GMCP data will be cached within a nosave
**	mapping (gmcp_cache) within the player object to reduce the amount of
**	duplicate data sent to the player's client.
**
** Parameters:
**
**	string package, a GMCP package name, such as those found in /include/gmcp_defs.h
**		that start with GMCP_PKG and resolve to examples like "Char.Status" and
**		"Room.Info".
**
**	mixed gmcp_value, a string, array or mapping containing GMCP data.
**
**    Returns: mixed, a string, array or mapping containing GMCP data or 0 if the
**	information in the cache matched with the input data and nothing needs sent
**	back to the player's client.
*/
private varargs mixed
gmcp_cache_filter(string package, mixed gmcp_value) {
    mixed filtered_result;

    if (!gmcp_cache || !mappingp(gmcp_cache)) {
	gmcp_cache = ([ ]);
    }

    if (gmcp_value) {
	switch (package) {
	case GMCP_PKG_CLIENT_GUI:
	    filtered_result = gmcp_value;
	    break;
	case GMCP_PKG_CLIENT_MAP:
	    if (member(gmcp_cache, package) < 1) {
		gmcp_cache[package] = filtered_result = gmcp_value;
	    } else filtered_result = parse_cache(package, gmcp_value, ({
		    GMCP_KEY_CLIENT_MAP_URL,
		  }));
	    break;
	case GMCP_PKG_CORE_HELLO:
	    if (member(gmcp_cache, package) < 1) {
		gmcp_cache[package] = filtered_result = gmcp_value;
	    } else filtered_result = parse_cache(package, gmcp_value, ({
		    GMCP_KEY_CORE_HELLO_CLIENT,
		    GMCP_KEY_CORE_HELLO_VERSION
		  }));
	    break;
	case GMCP_PKG_CORE_GOODBYE:
	    filtered_result = gmcp_value;
	    break;
	case GMCP_PKG_CORE_SUPPORTS_SET:
	case GMCP_PKG_CORE_SUPPORTS_ADD:
	    foreach (string element : gmcp_value) {
		string pkg, version;

		if (sscanf(element, "%s %s", pkg, version) == 2) {
		    if (member(gmcp_cache, GMCP_PKG_CORE_SUPPORTS) < 1) {
			gmcp_cache[GMCP_PKG_CORE_SUPPORTS] = ([ pkg: version ]);
		    } else { // Per the standard, we overwrite existing values
			gmcp_cache[GMCP_PKG_CORE_SUPPORTS][pkg] = version;
		    }
		}
	    }

	    // Likely will not be sent back to Clients and if Mudlib needs, then
	    // a mapping will be simpler to parse.
	    filtered_result = gmcp_cache[GMCP_PKG_CORE_SUPPORTS];
	    break;
	case GMCP_PKG_CORE_SUPPORTS_REMOVE:
	    // Cache cleanup
	    foreach (string pkg : gmcp_value) {
		if (member(gmcp_cache, GMCP_PKG_CORE_SUPPORTS) > 0) {
		    m_delete(gmcp_cache[GMCP_PKG_CORE_SUPPORTS], pkg);
		}
	    }

	    // Likely will not be sent back to Clients and if Mudlib needs, then
	    // a mapping will be simpler to parse.
	    filtered_result = gmcp_cache[GMCP_PKG_CORE_SUPPORTS];
	    break;
	case GMCP_PKG_EXTERNAL_DISCORD_HELLO:
	    filtered_result = gmcp_value;
	    break;
	case GMCP_PKG_EXTERNAL_DISCORD_INFO:
	    if (member(gmcp_cache, package) < 1) {
		gmcp_cache[package] = filtered_result = gmcp_value;
	    } else filtered_result = parse_cache(package, gmcp_value, ({
		    GMCP_KEY_EXTERNAL_DISCORD_INFO_INVITEURL,
		    GMCP_KEY_EXTERNAL_DISCORD_INFO_APPLICATIONID
		  }));
	    break;
	case GMCP_PKG_EXTERNAL_DISCORD_STATUS:
	    if (member(gmcp_cache, package) < 1) {
		gmcp_cache[package] = filtered_result = gmcp_value;
	    } else filtered_result = parse_cache(package, gmcp_value, ({
		    GMCP_KEY_EXTERNAL_DISCORD_STATUS_GAME,
		    GMCP_KEY_EXTERNAL_DISCORD_STATUS_STARTTIME
		  }));
	    break;
	case GMCP_PKG_CORE_KEEPALIVE:
	    break;
	case GMCP_PKG_CORE_PING:
	    if (intp(gmcp_value)) {
		gmcp_cache[GMCP_PKG_CORE_PING] = gmcp_value;
	    }

	    filtered_result = gmcp_value;
	    break;
	case GMCP_PKG_CHAR_NAME:
	    if (member(gmcp_cache, package) < 1) {
		gmcp_cache[package] = filtered_result = gmcp_value;
	    } else filtered_result = parse_cache(package, gmcp_value, ({
		    GMCP_KEY_CHAR_NAME_NAME,
		    GMCP_KEY_CHAR_NAME_FULLNAME
		  }));
	    break;
	case GMCP_PKG_CHAR_VITALS:
	    if (member(gmcp_cache, package) < 1) {
		gmcp_cache[package] = gmcp_value;
		filtered_result = gmcp_value;
	    } else filtered_result = parse_cache(package, gmcp_value, ({
		    GMCP_KEY_CHAR_VITALS_HP,
		    GMCP_KEY_CHAR_VITALS_MAXHP,
		    GMCP_KEY_CHAR_VITALS_SP,
		    GMCP_KEY_CHAR_VITALS_MAXSP,
		    GMCP_KEY_CHAR_VITALS_FP,
		    GMCP_KEY_CHAR_VITALS_MAXFP,
		    GMCP_KEY_CHAR_VITALS_NL,
		    GMCP_KEY_CHAR_VITALS_STRING
		  }));
	    break;
	case GMCP_PKG_CHAR_MAXSTATS:
	    if (member(gmcp_cache, package) < 1) {
		gmcp_cache[package] = gmcp_value;
		filtered_result = gmcp_value;
	    } else filtered_result = parse_cache(package, gmcp_value, ({
		    GMCP_KEY_CHAR_MAXSTATS_MAXCON,
		    GMCP_KEY_CHAR_MAXSTATS_MAXINT,
		    GMCP_KEY_CHAR_MAXSTATS_MAXDEX,
		    GMCP_KEY_CHAR_MAXSTATS_MAXSTR
		    //GMCP_KEY_CHAR_MAXSTATS_MAXFAT
		  }));
	    break;
	case GMCP_PKG_CHAR_STATS:
	    if (member(gmcp_cache, package) < 1) {
		gmcp_cache[package] = gmcp_value;
		filtered_result = gmcp_value;
	    } else filtered_result = parse_cache(package, gmcp_value, ({
		    GMCP_KEY_CHAR_STATS_CON,
		    GMCP_KEY_CHAR_STATS_INT,
		    GMCP_KEY_CHAR_STATS_DEX,
		    GMCP_KEY_CHAR_STATS_STR
		    //GMCP_KEY_CHAR_STATS_FAT
		  }));
	    break;
	case GMCP_PKG_CHAR_TRAINING_LIST:
	    filtered_result = parse_cache(GMCP_PKG_CHAR_TRAINING_LIST, gmcp_value, ({
		GMCP_KEY_CHAR_TRAINING_LIST_SKILL,
		GMCP_KEY_CHAR_TRAINING_LIST_NAME,
		GMCP_KEY_CHAR_TRAINING_LIST_RANK,
		GMCP_KEY_CHAR_TRAINING_LIST_PERCENT
	      }));
	    break;
	case GMCP_PKG_CHAR_SESSION_TRAINING:
	    filtered_result = parse_cache(GMCP_PKG_CHAR_SESSION_TRAINING, gmcp_value, ({
		GMCP_KEY_CHAR_SESSION_TRAINING_SKILL,
		GMCP_KEY_CHAR_SESSION_TRAINING_NAME,
		GMCP_KEY_CHAR_SESSION_TRAINING_PERCENT
	      }));
	    break;
	case GMCP_PKG_CHAR_STATUS:
	case GMCP_PKG_CHAR_STATUSVARS:
	    if (member(gmcp_cache, package) < 1) {
		gmcp_cache[package] = gmcp_value;
		filtered_result = gmcp_value;
	    } else filtered_result = parse_cache(package, gmcp_value, ({
		    GMCP_KEY_CHAR_STATUS_NAME,
		    GMCP_KEY_CHAR_STATUS_FULLNAME,
		    //GMCP_KEY_CHAR_STATUS_AGE,
		    GMCP_KEY_CHAR_STATUS_SESSION_LOGIN,
		    //GMCP_KEY_CHAR_STATUS_SESSION_CONNECTED,
		    //GMCP_KEY_CHAR_STATUS_SESSION_XP,
		    //GMCP_KEY_CHAR_STATUS_SESSION_KILLS,
		    GMCP_KEY_CHAR_STATUS_SESSION_ROOMS,
		    GMCP_KEY_CHAR_STATUS_RACE,
		    GMCP_KEY_CHAR_STATUS_LEVEL,
		    GMCP_KEY_CHAR_STATUS_XP,
		    GMCP_KEY_CHAR_STATUS_CLASS,
		    GMCP_KEY_CHAR_STATUS_GOLD,
		    GMCP_KEY_CHAR_STATUS_QP,
		    GMCP_KEY_CHAR_STATUS_BANK,
		    GMCP_KEY_CHAR_STATUS_ENEMY,
		    GMCP_KEY_CHAR_STATUS_ENEMY_HEALTH,
		    GMCP_KEY_CHAR_STATUS_GENDER,
		    GMCP_KEY_CHAR_STATUS_TITLE,
		    GMCP_KEY_CHAR_STATUS_NL,
		    GMCP_KEY_CHAR_STATUS_DEAD,
		    GMCP_KEY_CHAR_STATUS_BRAVE,
		    GMCP_KEY_CHAR_STATUS_DRUNK,
		    GMCP_KEY_CHAR_STATUS_HUNGER,
		    GMCP_KEY_CHAR_STATUS_THIRST,
		    GMCP_KEY_CHAR_STATUS_DISGUISE,
		    GMCP_KEY_CHAR_STATUS_ALIGN,
		    //GMCP_KEY_CHAR_STATUS_STATE,
		    GMCP_KEY_CHAR_STATUS_SIT,
		    GMCP_KEY_CHAR_STATUS_REST,
		    GMCP_KEY_CHAR_STATUS_FROG,
		    GMCP_KEY_CHAR_STATUS_INVIS,
		    GMCP_KEY_CHAR_STATUS_SUMMONABLE,
		    GMCP_KEY_CHAR_STATUS_PKABLE,
		    GMCP_KEY_CHAR_STATUS_DRAINCON,
		    GMCP_KEY_CHAR_STATUS_DRAININT,
		    GMCP_KEY_CHAR_STATUS_DRAINDEX,
		    GMCP_KEY_CHAR_STATUS_DRAINSTR,
		    GMCP_KEY_CHAR_STATUS_AMULET,
		    GMCP_KEY_CHAR_STATUS_ARMOUR,
		    GMCP_KEY_CHAR_STATUS_BELT,
		    GMCP_KEY_CHAR_STATUS_BOOTS,
		    GMCP_KEY_CHAR_STATUS_BRACERS,
		    GMCP_KEY_CHAR_STATUS_CLOAK,
		    GMCP_KEY_CHAR_STATUS_GIRDLE,
		    GMCP_KEY_CHAR_STATUS_GLOVES,
		    GMCP_KEY_CHAR_STATUS_HELMET,
		    GMCP_KEY_CHAR_STATUS_LEGGINGS,
		    GMCP_KEY_CHAR_STATUS_MASK,
		    GMCP_KEY_CHAR_STATUS_NECKLACE,
		    GMCP_KEY_CHAR_STATUS_RING,
		    GMCP_KEY_CHAR_STATUS_RING2,
		    GMCP_KEY_CHAR_STATUS_SHIELD
		  }));
	    break;
	case GMCP_PKG_CHAR_WORTH:
	    if (member(gmcp_cache, package) < 1) {
		gmcp_cache[package] = gmcp_value;
		filtered_result = gmcp_value;
	    } else filtered_result = parse_cache(package, gmcp_value, ({
		    GMCP_KEY_CHAR_STATUS_GOLD,
		    GMCP_KEY_CHAR_STATUS_BANK,
		    GMCP_KEY_CHAR_STATUS_QP,
		  }));
	    break;
	case GMCP_PKG_CHAR_CONDITIONS_LIST:
	    if (member(gmcp_cache, GMCP_PKG_CHAR_CONDITIONS) < 1) {
		gmcp_cache[GMCP_PKG_CHAR_CONDITIONS] = gmcp_value;
		filtered_result = gmcp_value;
	    } else filtered_result = parse_cache(GMCP_PKG_CHAR_CONDITIONS, gmcp_value, ({
		    GMCP_KEY_CHAR_CONDITIONS_NAME,
		    GMCP_KEY_CHAR_CONDITIONS_DESC
		  }));
	    break;
	case GMCP_PKG_CHAR_CONDITIONS_ADD:
	    if (member(gmcp_cache, GMCP_PKG_CHAR_CONDITIONS) < 1) {
		gmcp_cache[GMCP_PKG_CHAR_CONDITIONS] = ([ gmcp_value[0]: gmcp_value[1] ]);
#if 0
	    {
#endif
		filtered_result = gmcp_value;
	    } else filtered_result = parse_cache(GMCP_PKG_CHAR_CONDITIONS, gmcp_value, ({
		    GMCP_KEY_CHAR_CONDITIONS_NAME,
		    GMCP_KEY_CHAR_CONDITIONS_DESC
		  }));
	    break;
	case GMCP_PKG_CHAR_CONDITIONS_REMOVE:
	    // Cache cleanup
	    if (member(gmcp_cache, GMCP_PKG_CHAR_CONDITIONS) > 0) {
		m_delete(gmcp_cache[GMCP_PKG_CHAR_CONDITIONS], gmcp_value);
	    }

	    filtered_result = gmcp_value;
	    break;
	case GMCP_PKG_CHAR_RESISTANCES_LIST:
	    if (member(gmcp_cache, GMCP_PKG_CHAR_RESISTANCES) < 1) {
		gmcp_cache[GMCP_PKG_CHAR_RESISTANCES] = gmcp_value;
		filtered_result = gmcp_value;
	    } else filtered_result = parse_cache(GMCP_PKG_CHAR_RESISTANCES, gmcp_value, ({
		    GMCP_KEY_CHAR_RESISTANCES_NAME,
		    GMCP_KEY_CHAR_RESISTANCES_DESC
		  }));
	    break;
	case GMCP_PKG_CHAR_RESISTANCES_ADD:
	    if (member(gmcp_cache, GMCP_PKG_CHAR_RESISTANCES) < 1) {
		gmcp_cache[GMCP_PKG_CHAR_RESISTANCES] = ([ gmcp_value[0]: gmcp_value[1] ]);
#if 0
	    {
#endif
		filtered_result = gmcp_value;
	    } else filtered_result = parse_cache(GMCP_PKG_CHAR_RESISTANCES, gmcp_value, ({
		    GMCP_KEY_CHAR_RESISTANCES_NAME,
		    GMCP_KEY_CHAR_RESISTANCES_DESC
		  }));
	    break;
	case GMCP_PKG_CHAR_RESISTANCES_REMOVE:
	    // Cache cleanup
	    if (member(gmcp_cache, GMCP_PKG_CHAR_RESISTANCES) > 0) {
		m_delete(gmcp_cache[GMCP_PKG_CHAR_RESISTANCES], gmcp_value);
	    }

	    filtered_result = gmcp_value;
	    break;
	case GMCP_PKG_CHAR_AFFLICTIONS_LIST:
	    if (member(gmcp_cache, GMCP_PKG_CHAR_AFFLICTIONS) < 1) {
		gmcp_cache[GMCP_PKG_CHAR_AFFLICTIONS] = gmcp_value;
		filtered_result = gmcp_value;
	    } else filtered_result = parse_cache(GMCP_PKG_CHAR_AFFLICTIONS, gmcp_value, ({
		    GMCP_KEY_CHAR_AFFLICTIONS_NAME,
		    GMCP_KEY_CHAR_AFFLICTIONS_CURE,
		    GMCP_KEY_CHAR_AFFLICTIONS_DESC
		  }));
	    break;
	case GMCP_PKG_CHAR_AFFLICTIONS_ADD:
	    if (member(gmcp_cache, GMCP_PKG_CHAR_AFFLICTIONS) < 1) {
		gmcp_cache[GMCP_PKG_CHAR_AFFLICTIONS] = ([ gmcp_value[0]: gmcp_value[1] ]);
#if 0
	    {
#endif
		filtered_result = gmcp_value;
	    } else filtered_result = parse_cache(GMCP_PKG_CHAR_AFFLICTIONS, gmcp_value, ({
		    GMCP_KEY_CHAR_AFFLICTIONS_NAME,
		    GMCP_KEY_CHAR_AFFLICTIONS_CURE,
		    GMCP_KEY_CHAR_AFFLICTIONS_DESC
		  }));
	    break;
	case GMCP_PKG_CHAR_AFFLICTIONS_REMOVE:
	    // Cache cleanup
	    if (member(gmcp_cache, GMCP_PKG_CHAR_AFFLICTIONS) > 0) {
		m_delete(gmcp_cache[GMCP_PKG_CHAR_AFFLICTIONS], gmcp_value);
	    }

	    filtered_result = gmcp_value;
	    break;
	case GMCP_PKG_CHAR_DEFENCES_LIST:
	    if (member(gmcp_cache, GMCP_PKG_CHAR_DEFENCES) < 1) {
		gmcp_cache[GMCP_PKG_CHAR_DEFENCES] = gmcp_value;
		filtered_result = gmcp_value;
	    } else filtered_result = parse_cache(GMCP_PKG_CHAR_DEFENCES, gmcp_value, ({
		    GMCP_KEY_CHAR_DEFENCES_NAME,
		    GMCP_KEY_CHAR_DEFENCES_DESC
		  }));
	    break;
	case GMCP_PKG_CHAR_DEFENCES_ADD:
	    if (member(gmcp_cache, GMCP_PKG_CHAR_DEFENCES) < 1) {
		gmcp_cache[GMCP_PKG_CHAR_DEFENCES] = ([ gmcp_value[0]: gmcp_value[1] ]);
#if 0
	    {
#endif
		filtered_result = gmcp_value;
	    } else filtered_result = parse_cache(GMCP_PKG_CHAR_DEFENCES, gmcp_value, ({
		    GMCP_KEY_CHAR_DEFENCES_NAME,
		    GMCP_KEY_CHAR_DEFENCES_DESC
		  }));
	    break;
	case GMCP_PKG_CHAR_DEFENCES_REMOVE:
	    // Cache cleanup
	    if (member(gmcp_cache, GMCP_PKG_CHAR_DEFENCES) > 0) {
		m_delete(gmcp_cache[GMCP_PKG_CHAR_DEFENCES], gmcp_value);
	    }

	    filtered_result = gmcp_value;
	    break;
	case GMCP_PKG_CHAR_ITEMS_LIST:
	case GMCP_PKG_CHAR_ITEMS_UPDATE:
	case GMCP_PKG_CHAR_ITEMS_ADD:
	    filtered_result = gmcp_value;
	    break;
	case GMCP_PKG_CHAR_ITEMS_REMOVE:
	    filtered_result = gmcp_value;
	    break;
	    /*
		    case GMCP_PKG_CHAR_SKILLS_GROUPS:
			if (member(gmcp_cache, GMCP_PKG_CHAR_SKILLS_GROUPS) < 1) {
			    gmcp_cache[GMCP_PKG_CHAR_SKILLS_GROUPS] = gmcp_value;
			    filtered_result = gmcp_value;
			} else filtered_result = parse_cache(GMCP_PKG_CHAR_SKILLS_GROUPS, gmcp_value, ({
				GMCP_KEY_CHAR_SKILLS_NAME,
				GMCP_KEY_CHAR_SKILLS_RANK
			      }));
			break;
		    case GMCP_PKG_CHAR_SKILLS_LIST:
			if (member(gmcp_cache, GMCP_PKG_CHAR_SKILLS_LIST) < 1) {
			    gmcp_cache[GMCP_PKG_CHAR_SKILLS_LIST] = gmcp_value;
			    filtered_result = gmcp_value;
			} else filtered_result = parse_cache(GMCP_PKG_CHAR_SKILLS_LIST, gmcp_value, ({
				GMCP_KEY_CHAR_SKILLS_GROUP,
				GMCP_KEY_CHAR_SKILLS_LIST,
				GMCP_KEY_CHAR_SKILLS_DESC
			      }));
			break;
		    case GMCP_PKG_CHAR_SKILLS_INFO:
			if (member(gmcp_cache, GMCP_PKG_CHAR_SKILLS_INFO) < 1) {
			    gmcp_cache[GMCP_PKG_CHAR_SKILLS_INFO] = gmcp_value;
			    filtered_result = gmcp_value;
			} else filtered_result = parse_cache(GMCP_PKG_CHAR_SKILLS_INFO, gmcp_value, ({
				GMCP_KEY_CHAR_SKILLS_GROUP,
				GMCP_KEY_CHAR_SKILLS_SKILL,
				GMCP_KEY_CHAR_SKILLS_INFO
			      }));
			break;
	    */
	case GMCP_PKG_COMM_CHANNEL_PLAYERS:
	    if (member(gmcp_cache, GMCP_PKG_COMM_CHANNEL_PLAYERS) < 1) {
		gmcp_cache[GMCP_PKG_COMM_CHANNEL_PLAYERS] = gmcp_value;
		filtered_result = gmcp_value;
	    } else filtered_result = parse_cache(GMCP_PKG_COMM_CHANNEL_PLAYERS, gmcp_value, ({
		    GMCP_KEY_COMM_CHANNEL_NAME,
		    GMCP_KEY_COMM_CHANNEL_CHANNELS
		  }));
	    break;
	case GMCP_PKG_COMM_CHANNEL_LIST:
	    if (member(gmcp_cache, GMCP_PKG_COMM_CHANNEL_LIST) < 1) {
		gmcp_cache[GMCP_PKG_COMM_CHANNEL_LIST] = gmcp_value;
		filtered_result = gmcp_value;
	    } else filtered_result = parse_cache(GMCP_PKG_COMM_CHANNEL_LIST, gmcp_value, ({
		    GMCP_KEY_COMM_CHANNEL_NAME,
		    GMCP_KEY_COMM_CHANNEL_CAPTION,
		    GMCP_KEY_COMM_CHANNEL_COMMAND
		  }));
	    break;
	case GMCP_PKG_COMM_CHANNEL_TEXT:
	    // No caching
	    filtered_result = gmcp_value;
	    break;
	case GMCP_PKG_GROUP:
	    if (mappingp(gmcp_value)) {
		filtered_result = parse_cache(GMCP_PKG_GROUP, gmcp_value, ({
		    GMCP_KEY_GROUP_GROUPNAME,
		    GMCP_KEY_GROUP_LEADER,
		    //GMCP_KEY_GROUP_CREATED,
		    //GMCP_KEY_GROUP_STATUS,
		    GMCP_KEY_GROUP_COUNT,
		    //GMCP_KEY_GROUP_KILLS,
		    GMCP_KEY_GROUP_EXP,
		    GMCP_KEY_GROUP_MEMBERS
		  }));
	    } else {
		// If gmcp_value is "" and we had a party in our cache let's one time send
		// gmcp to the client to wipe gmcp.Group a.k.a GMCP_PKG_GROUP and we'll
		// also clean out the cache here so we don't continually send the wipe
		// message back to the client.
		if (member(gmcp_cache, GMCP_PKG_GROUP) && mappingp(gmcp_cache[GMCP_PKG_GROUP])) {
		    m_delete(gmcp_cache, GMCP_PKG_GROUP);
		    filtered_result = "";
		}
	    }
	    break;
	case GMCP_PKG_ROOM_INFO:
	    if (member(gmcp_cache, GMCP_PKG_ROOM_INFO) < 1) {
		gmcp_cache[GMCP_PKG_ROOM_INFO] = gmcp_value;
		filtered_result = gmcp_value;
	    } else filtered_result = parse_cache(GMCP_PKG_ROOM_INFO, gmcp_value, ({
		    GMCP_KEY_ROOM_INFO_NUM,
		    GMCP_KEY_ROOM_INFO_NAME,
		    GMCP_KEY_ROOM_INFO_AREA,
		    GMCP_KEY_ROOM_INFO_ENVIRONMENT,
		    GMCP_KEY_ROOM_INFO_COORDS,
		    GMCP_KEY_ROOM_INFO_MAP,
		    GMCP_KEY_ROOM_INFO_DETAILS,
		    GMCP_KEY_ROOM_INFO_EXITS
		  }));
	    break;
	case GMCP_PKG_ROOM_PLAYERS:
	case GMCP_PKG_ROOM_ADDPLAYER:
	case GMCP_PKG_ROOM_REMOVEPLAYER:
	case GMCP_PKG_ROOM_WRONGDIR:
	    filtered_result = gmcp_value;
	    break;
	}
    }

    return filtered_result;
}

private void
gmcp_input(int *optdata) {
    string package;
    string value;
    int i, space;

    for (i = 0; i < sizeof(optdata); i++) {
	if (!space && optdata[i] == ' ') {
	    space = i;
	} else if (!space) {
	    package = sprintf("%s%c", package ? package : "", optdata[i]);
	} else {
	    value = sprintf("%s%c", value ? value : "", optdata[i]);
	}
    }

    //if (find_player("tamarindo")) find_player("tamarindo")->tell_me("Package: " + package);
    //if (find_player("tamarindo")) find_player("tamarindo")->tell_me("Value: " + value);

    switch (package) {
    case GMCP_PKG_CORE_HELLO:
	gmcp_cache_filter(package, json_decode(value));
	TELOPT_D->send_client_map(this_player());
	break;
    case GMCP_PKG_CORE_SUPPORTS_SET:
	gmcp_cache_filter(package, json_decode(value));
	break;
    case GMCP_PKG_CORE_SUPPORTS_ADD:
	gmcp_cache_filter(package, json_decode(value));
	break;
    case GMCP_PKG_CORE_SUPPORTS_REMOVE:
	gmcp_cache_filter(package, json_decode(value));
	break;
    case GMCP_PKG_CORE_KEEPALIVE:
	// Not yet supported
	break;
    case GMCP_PKG_CORE_PING:
	gmcp_cache_filter(package, json_decode(value ? value : "null"));
	gmcp_message(package, "");
	break;
    case GMCP_PKG_EXTERNAL_DISCORD_GET:
	TELOPT_D->send_external_discord_status(this_player());
	break;
    case GMCP_PKG_EXTERNAL_DISCORD_HELLO:
	gmcp_cache_filter(package, value ? json_decode(value) : value);
	TELOPT_D->send_external_discord_info(this_player());
	TELOPT_D->send_external_discord_status(this_player());
	break;
    case GMCP_PKG_CHAR_LOGIN:
	// Not yet supported
	break;
    case GMCP_PKG_CHAR_ITEMS_CONTENTS:
	// Must be a container and for now only obeject we can hold.  Perhaps in the future
	// this could be expanded to objects present in our room that are open. -Tamarindo
	foreach (object container_ob : filter(all_inventory(this_player()), (: $1->can_put_and_get() :))) {
	    if (value == hash(TLS_HASH_SHA1, object_name(container_ob))) {
		TELOPT_D->send_char_items_list(this_player(), value, container_ob);
		break;
	    }
	}
	break;
    case GMCP_PKG_CHAR_ITEMS_INV:
	TELOPT_D->send_char_items_list(this_player(), "inv");
	break;
    case GMCP_PKG_CHAR_ITEMS_ROOM:
	TELOPT_D->send_char_items_list(this_player(), "room");
	break;
    case GMCP_PKG_COMM_CHANNEL_PLAYERS:
	TELOPT_D->send_comm_channel_players(this_player(), 1);
	break;
    case GMCP_PKG_COMM_CHANNEL_ENABLE:
	// Not yet supported
	// TODO: Decode the json value and turn on a valid channel
	break;
    }
}

private string
mssp_message() {
    string mssp_message = "";

    mapping mssp = ([
      MSSP_VAR_NAME: MSSP_VAL_NAME,        // Required
      MSSP_VAR_PLAYERS: MSSP_VAL_PLAYERS,  // Required
      MSSP_VAR_UPTIME: MSSP_VAL_UPTIME,    // Required
#ifdef MSSP_VAR_HOSTNAME
      MSSP_VAR_HOSTNAME: MSSP_VAL_HOSTNAME,
#endif
#ifdef MSSP_VAR_PORT
      MSSP_VAR_PORT: MSSP_VAL_PORT,
#endif
#ifdef MSSP_VAR_CODEBASE
      MSSP_VAR_CODEBASE: MSSP_VAL_CODEBASE,
#endif
#ifdef MSSP_VAR_CONTACT
      MSSP_VAR_CONTACT: MSSP_VAL_CONTACT,
#endif
#ifdef MSSP_VAR_CREATED
      MSSP_VAR_CREATED: MSSP_VAL_CREATED,
#endif
#ifdef MSSP_VAR_ICON
      MSSP_VAR_ICON: MSSP_VAL_ICON,
#endif
#ifdef MSSP_VAR_IP
      MSSP_VAR_IP: MSSP_VAL_IP,
#endif
#ifdef MSSP_VAR_LANGUAGE
      MSSP_VAR_LANGUAGE: MSSP_VAL_LANGUAGE,
#endif
#ifdef MSSP_VAR_LOCATION
      MSSP_VAR_LOCATION: MSSP_VAL_LOCATION,
#endif
#ifdef MSSP_VAR_MINIMUM_AGE
      MSSP_VAR_MINIMUM_AGE: MSSP_VAL_MINIMUM_AGE,
#endif
#ifdef MSSP_VAR_WEBSITE
      MSSP_VAR_WEBSITE: MSSP_VAL_WEBSITE,
#endif
#ifdef MSSP_VAR_FAMILY
      MSSP_VAR_FAMILY: MSSP_VAL_FAMILY,
#endif
#ifdef MSSP_VAR_GENRE
      MSSP_VAR_GENRE: MSSP_VAL_GENRE,
#endif
#ifdef MSSP_VAR_GAMEPLAY
      MSSP_VAR_GAMEPLAY: MSSP_VAL_GAMEPLAY,
#endif
#ifdef MSSP_VAR_GAMESYSTEM
      MSSP_VAR_GAMESYSTEM: MSSP_VAL_GAMESYSTEM,
#endif
#ifdef MSSP_VAR_INTERMUD
      MSSP_VAR_INTERMUD: MSSP_VAL_INTERMUD,
#endif
#ifdef MSSP_VAR_STATUS
      MSSP_VAR_STATUS: MSSP_VAL_STATUS,
#endif
#ifdef MSSP_VAR_SUBGENRE
      MSSP_VAR_SUBGENRE: MSSP_VAL_SUBGENRE,
#endif
#ifdef MSSP_VAR_AREAS
      MSSP_VAR_AREAS: MSSP_VAL_AREAS,
#endif
#ifdef MSSP_VAR_HELPFILES
      MSSP_VAR_HELPFILES: MSSP_VAL_HELPFILES,
#endif
#ifdef MSSP_VAR_MOBILES
      MSSP_VAR_MOBILES: MSSP_VAL_MOBILES,
#endif
#ifdef MSSP_VAR_OBJECTS
      MSSP_VAR_OBJECTS: MSSP_VAL_OBJECTS,
#endif
#ifdef MSSP_VAR_ROOMS
      MSSP_VAR_ROOMS: MSSP_VAL_ROOMS,
#endif
#ifdef MSSP_VAR_CLASSES
      MSSP_VAR_CLASSES: MSSP_VAL_CLASSES,
#endif
#ifdef MSSP_VAR_LEVELS
      MSSP_VAR_LEVELS: MSSP_VAL_LEVELS,
#endif
#ifdef MSSP_VAR_RACES
      MSSP_VAR_RACES: MSSP_VAL_RACES,
#endif
#ifdef MSSP_VAR_SKILLS
      MSSP_VAR_SKILLS: MSSP_VAL_SKILLS,
#endif
#ifdef MSSP_VAR_ANSI
      MSSP_VAR_ANSI: MSSP_VAL_ANSI,
#endif
#ifdef MSSP_VAR_GMCP
      MSSP_VAR_GMCP: MSSP_VAL_GMCP,
#endif
#ifdef MSSP_VAR_MSDP
      MSSP_VAR_MSDP: MSSP_VAL_MSDP,
#endif
#ifdef MSSP_VAR_MCCP
      MSSP_VAR_MCCP: MSSP_VAL_MCCP,
#endif
#ifdef MSSP_VAR_MCP
      MSSP_VAR_MCP: MSSP_VAL_MCP,
#endif
#ifdef MSSP_VAR_MSP
      MSSP_VAR_MSP: MSSP_VAL_MSP,
#endif
#ifdef MSSP_VAR_MXP
      MSSP_VAR_MXP: MSSP_VAL_MXP,
#endif
#ifdef MSSP_VAR_PUEBLO
      MSSP_VAR_PUEBLO: MSSP_VAL_PUEBLO,
#endif
#ifdef MSSP_VAR_UTF_8
      MSSP_VAR_UTF_8: MSSP_VAL_UTF_8,
#endif
#ifdef MSSP_VAR_VT100
      MSSP_VAR_VT100: MSSP_VAL_VT100,
#endif
#ifdef MSSP_VAR_XTERM_256_COLORS
      MSSP_VAR_XTERM_256_COLORS: MSSP_VAL_XTERM_256_COLORS,
#endif
#ifdef MSSP_VAR_SSL
      MSSP_VAR_SSL: MSSP_VAL_SSL,
#endif
#ifdef MSSP_VAR_PAY_TO_PLAY
      MSSP_VAR_PAY_TO_PLAY: MSSP_VAL_PAY_TO_PLAY,
#endif
#ifdef MSSP_VAR_PAY_FOR_PERKS
      MSSP_VAR_PAY_FOR_PERKS: MSSP_VAL_PAY_FOR_PERKS,
#endif
#ifdef MSSP_VAR_HIRING_BUILDERS
      MSSP_VAR_HIRING_BUILDERS: MSSP_VAL_HIRING_BUILDERS,
#endif
#ifdef MSSP_VAR_HIRING_CODERS
      MSSP_VAR_HIRING_CODERS: MSSP_VAL_HIRING_CODERS,
#endif
    ]);

    // IAC SB MSSP MSSP_VAR "PLAYERS" MSSP_VAL "52" MSSP_VAR "UPTIME" MSSP_VAL "1234567890" IAC SE

    foreach (string var, string val : mssp) {
	mssp_message += sprintf("%c%s%c%s", MSSP_VAR, var, MSSP_VAL, val);
    }

    return mssp_message;
}

public void
telopt_negotiate(int action, int option, int *optdata) {
    string type;
    int n;

    switch (option)
    {
#ifdef LOGIN_C
    case TELOPT_MSSP:

	if (action == DO) {
	    binary_message(({IAC, SB, TELOPT_MSSP}));
	    binary_message(mssp_message());
	    binary_message(({IAC, SE}));
	}
	break;
#endif /* LOGIN_C */

    case TELOPT_GMCP:

	if (action == SB) {
	    //may want to perform some actions
	    gmcp_input(optdata);

#ifndef LOGIN_C
	    if (query_env("gmcp")) return;
	    set_env("gmcp", 1);
#endif /* LOGIN_C */

	}
	break;
#if 1
	// Do we need this? Maybe not.
    default:

	if (action == WILL) {
#ifndef LOGIN_C
	    if (query_env("gmcp")) return;
	    set_env("gmcp", 1);
#endif /* LOGIN_C */

	    binary_message(sprintf("%c%c%c", IAC, DONT, option), 3);
	} else if (action == DO) {
#ifndef LOGIN_C
	    if (query_env("gmcp")) return;
	    set_env("gmcp", 1);
#endif /* LOGIN_C */

	    binary_message(sprintf("%c%c%c", IAC, WONT, option), 3);
	}
#endif
    }
}

public varargs int
gmcp_message(string package, mixed value, int refresh) {
    mixed cache_value;
#ifndef LOGIN_C
    string gmcp_enabled = query_env("gmcp");

    // make sure it makes sense to send to this client
    if (interactive() && package && gmcp_enabled) {
#else
	// in the case of External.Discord.Hello we need to reply while we are
	// inside of the login object, so we're being more liberal with this
	// check.  A smart way may be to include the set/query Env within
	// the login object -Tamarindo (coding at 1am)
	if (interactive() && package) {
#endif /* LOGIN_C */
	    // validate value
	    if (package != GMCP_PKG_CLIENT_GUI) {
		switch (typeof(value))
		{
		case T_MAPPING:
		    /* only string keys are valid */
		    if (sizeof(filter(value, (:!stringp($1):)))) {
			return 0;
		    }
		case T_POINTER:
		case T_STRING:
		case T_NUMBER:
		case T_FLOAT:
		    // Only send what has changed
		    cache_value = gmcp_cache_filter(package, value);
		    break;
		    /* Invalid types */
		case T_OBJECT:
		case T_CLOSURE:
		    return 0;
		}

		// Send only if the cache changed
		if (refresh ? value : cache_value) {
		    string json_serialized_value = json_encode(refresh ? value : cache_value);

		    // send the client the value
		    binary_message(({IAC, SB, TELOPT_GMCP}));
		    binary_message(sprintf("%s %s", package, json_serialized_value));
		    binary_message(({IAC, SE}));
		}
	    } else { // GMCP_PKG_CLIENT_GUI needs sent raw
		binary_message(({IAC, SB, TELOPT_GMCP}));
		binary_message(sprintf("%s %s", package, value));
		binary_message(({IAC, SE}));
	    }

	    return 1;
	}

	return 0;
    }

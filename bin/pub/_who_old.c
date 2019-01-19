/*
 * 04-Nov-92 Starbyt   who_cmd()
 * 4.1-93    Val       show_who_special()
 * 23-Oct-93 Graah     moved to /bin/pub
 * 16-Sep-94 Doomdark	now uses "query_guild()"
 * 31-Aug-95 Doomdark  Now _much_ more efficient
 * 3-Dec-95 Tron    Fixed a bug in filter_who (guest could see invis GOD :)
 * 28-Apr-96 Graah  Dark Elves into troll-who-list
 * 17-Jul-96 Graah  Isolated rooms
 * 17-Dec-96 Macleod    Warriors guild to who g
 * 14-Oct-97 Graah	Show % at who g and who r (only players counted)
 */

#include "/sys/interactive_info.h"

#include <cmd.h>
#include <driver.h>
#include <guild.h>
#include <invis_levels.h>
#include <room_defs.h>

#define PURGATORY	"/room/purgatory"
/* Now the limit is 25. 2.5.1993 //Frobozz */
#define CAN_SEE_INVIS 25

/* undef to disable troll/dark elf-who //Graah */
#define TROLLS

private static int who_mode;

#define WHO_C 1
#define WHO_R 2
#define WHO_F 3

/* Coders: who diseases, who quests, who guilds */
#define WHO_D 4
#define WHO_QU 5
#define WHO_G 6

#define COUNTRY_USA ({"gov","edu","com","mil","org"})
#define COUNTRY_MAP ([\
"":"Unknown",\
"at": "Austria",\
"au": "Australia",\
"be": "Belgium",\
"ca": "Canada",\
"ch": "Switzerland",\
"cl": "Chile",\
"de": "Germany",\
"dk": "Denmark",\
"ee" : "Estonia",\
"es": "Spain",\
"fi": "Finland",\
"fr": "France",\
"kr": "Korea",\
"ie": "Ireland",\
"il": "Israel",\
"is": "Iceland",\
"it": "Italy",\
"lv": "Latvia",\
"net":"Internet organization",\
"nl":"Netherlands",\
"no":"Norway",\
"nz":"New Zealand",\
"mx":"Mexico",\
"pt":"Portugal",\
"se":"Sweden",\
"sg":"Singapore",\
"tw":"Taiwan",\
"uk":"United Kingdom",\
"us":"United States",\
"za":"South Africa",\
])
#define RACE_MAP (["dwarf":"Dwarves", "elf":"Elves", "troll":"Trolls",\
"dark elf":"Dark Elves"])


#ifdef TROLLS
static status TP_is_troll;
#endif

static status TP_isolated;

/*
 * Filter non-real players from list. (logins and other)
 * Filter invisible players from list. (not from coders nor lords)
 */
object *
who_filter(object *x, object me, int npc_test)
{
    int i, level, coder_level, il;
    object ob;
#ifdef TROLLS
    string obr;
#endif

    level = (int) me->query_level();
    coder_level = (int) me->query_coder_level();

    for (i = sizeof(x) - 1; i >= 0; i--)
    {
	ob = x[i];

	if (!living(ob) || (npc_test && !interactive(ob))) {
	    // Was "query_npc". -+ Doomdark +-
	    x[i] = 0;
	    continue;
	}

	/* Coders see all */
	if (coder_level)
	    continue;

	// Isolated rooms: self only.
	if (TP_isolated && ob != me)
	{
	    x[i] = 0;
	    continue;
	}

	if (!TP_isolated && environment(ob) && environment(ob)->
	  query(ROOM_ISOLATED))
	{
	    x[i] = 0;
	    continue;
	}

#ifdef TROLLS
	// Trolls only see trolls, non-trolls never see trolls (even coders)!
	// Dark elves are now on their channel too.  /Graah
	// Can now see coders. 190397 Haw
	obr = (string)ob->query_race();

	if (TP_is_troll)
	{
	    if(obr != "troll" && obr != "dark elf" && !ob->query_coder_level()) {
		x[i] = 0;
		continue;
	    }
	}
	else
	{
	    if (obr == "troll" || obr == "dark elf") {
		x[i] = 0;
		continue;
	    }
	}
#endif

	il = (int)ob->query_invis();
	if (il) {
	    if (level < CAN_SEE_INVIS || il >= IL_TOTALLY_INVISIBLE)
		x[i] = 0;
	}
    }

    x -= ({ 0 });
    return x;
}


/*
 * Map object pointer to name.
 */
nomask string
who_mapper(object ob) {
    return capitalize((string)ob->query_real_name());
}

/*
 * Sort user names into 'map' with a key.
 */
nomask void
data_mapper(object ob, mapping map) {
    string *tmp, key, blah;
    object z;

    /* Global selector because we can't send more data to function */
    switch (who_mode) {
    case WHO_C:
	tmp = explode(interactive_info(ob, II_IP_NAME), ".");
	key = lower_case(tmp[sizeof(tmp) - 1]);
	if (key[0] < 'a' || key[0] > 'z') {
	    if (tmp[0] == "130" && tmp[1] == "238" && tmp[2] == "197")
		key = "se";
	    else
		key = "";
	} else {
	    if (member(COUNTRY_USA, key) != -1)
		key = "us";
	}
	break;
    case WHO_F:
	if (key = (string)ob->query_liege()) break;
	return;
    case WHO_R:
	if (!(key = (string)ob->query_race()))
	    key = "human";
	break;
    case WHO_D:
	if ((z = present("disease", ob))) {
	    if (!(key = capitalize((string)z->query_name())))
		key = "Unknown";
	    blah = (string)z->query_nickname();
	    if (blah) key += " (\"" + blah + "\")";
	    break;
	}
	return;
    case WHO_G:
	switch ((string) ob -> query_guild()) {
#ifdef GN_FIGHTER
	case GN_FIGHTER: key = "Fighters"; break;
#endif
#ifdef GN_MAGE
	case GN_MAGE: key = "Mages"; break;
#endif
#ifdef GN_NINJA
	case GN_NINJA: key = "Ninjas"; break;
#endif
#ifdef GN_PRIEST
	case GN_PRIEST: key = "Priests"; break;
#endif
#ifdef GN_THIEF
	case GN_THIEF: key = "Thieves"; break;
#endif
#ifdef GN_NECROMANCER
	case GN_NECROMANCER: key = "Necromancers"; break;
#endif
#ifdef GN_HEALER
	case GN_HEALER: key = "Healers"; break;
#endif
#ifdef GN_WARRIOR
	case GN_WARRIOR: key = "Warriors"; break;
#endif
#ifdef GN_MONK
	case GN_MONK: key = "Monks"; break;
#endif
	case "munchkin": key = "Munchkins"; break;
	default: key = "Unknown guild";
	}
	break;
    }

    if (map[key]) {
	map[key] += ({ capitalize((string)ob->query_real_name()) });
    } else {
	map[key] =  ({ capitalize((string)ob->query_real_name()) });
    }
}


nomask void
show_percents(mapping map, object me)
{
    int s, x, i, j;
    string *keys, *tmp;
    object ob;

    keys = m_indices(map);

    for (s = 0, i = sizeof(keys) - 1; i >= 0; i--)
	if (pointerp(map[keys[i]]))
	{
	    for (x = 0, j = sizeof(map[keys[i]]) - 1; j >= 0; j--)
		if (stringp(map[keys[i]][j])
		  && (ob = find_player(lower_case(map[keys[i]][j])))
		  && (!ob->query_coder_level())
		  && (!ob->query_testplayer()))
		{
		    x++;
		    s++;
		}

	    map[keys[i]] = x;
	}
	else { m_delete(map, keys[i]); keys -= ({ keys[i] }); }

    if (!s) return;

    tmp = ({});
    for (i = sizeof(keys) - 1; i >= 0; i--)
	if (map[keys[i]] > 0)
	    tmp += ({ capitalize(keys[i]) + ": " + ((100 * map[keys[i]]) / s) +
	      "%" });

    TP->tell_me("Players: " + implode(tmp, ", ", " and ") + ".");
}


/*
 * Show who lists for: c(ountries), f(ollowers), r(ace), d(isease), g(uilds)
 */
nomask void
show_who_special(int mode, object me)
{
    mapping _map;
    string  *keys;
    int     i;
    string key, header;

    who_mode = mode;
    _map = ([]);
    map(who_filter(users(), me, 0),
      "data_mapper", this_object(), _map);

    if (!sizeof(_map)) {
	if (mode == WHO_F)
	    TP->tell_me("No followers in the game currently.");
	else if (mode == WHO_D)
	    TP->tell_me("No diseases in the game currently.");
	else TP->tell_me("No guild members in the game currently.");
	return;
    }

    keys = sort_array(m_indices(_map), #'>);

    for (i = 0; i < sizeof(keys); i++) {

	key = keys[i];

	switch (who_mode) {
	case WHO_C:
	    if (header = COUNTRY_MAP[key]) break;
	    header = "Unknown (" + key + ")";
	    break;
	case WHO_R:
	    if (header = RACE_MAP[key]) break;
	    header = capitalize(key) + "s";
	    break;
	default:
	    header = capitalize(key);
	}

	write(sprintf("%-24s%-=54s\n", header + ":",
	    implode(sort_array(_map[key], #'>), ", ", " and ")
	    + "."
	  ));
    }

    if (who_mode == WHO_G) show_percents(_map, me);
}


nomask void
show_who(object me)
{
    string *s;
    int    i, il;

    s = sort_array(
      map(who_filter(users(), me, 0),
	"who_mapper", this_object()),
      #'>);

    for (i = 0; i < sizeof (s); i++) {
	string str, stmp;
	object bng;

	if(!(bng = find_player(lower_case(s[i])))) continue;

	if((int)bng->query_frog())
	    str = "                  " +
	    capitalize((string)bng->query_real_name()) + " the Frog";
	else {
	    if ((int)bng->query_ghost())
		str = "         Ghost of ";
	    else 
	    if (stmp = (string)bng->query_pretitle())
		str = sprintf("%18s",stmp) + "";
	    else
		str = "                   ";

	    str += capitalize((string)bng->query_real_name()) + " " +
	    (string)bng->query_title();
	}
	if(!interactive(bng)) str += "(link dead)";
	else if (interactive_info(bng, II_IDLE) > 300)
	    str += " (idle)";

	if ((il = (int)bng->query_invis()))
	    str += (il <= IL_SNEAKING ? " (sneaking)" :
	      il <= IL_HIDDEN ? " (hiding)" :
	      " (invisible)");
	s[i] = str;
    }

    this_player()->more(
      ({ sprintf(
	  "==== Total of %d players visible to you ====================================",
	  sizeof(s)) }) + s + 
      ({
	"============================================================================"
      }) );
}

// The command itself
nomask int
who_old_cmd(string arg, object me)
{
    string *s, r;
    int i;

    if (!me && !(me = this_player())) return 0;

#ifdef TROLLS
    r = (string)TP->query_race();
    if (r == "troll" || r == "dark elf") TP_is_troll = 1;
    else TP_is_troll = 0;
#endif

    TP_isolated = environment(TP)->query(ROOM_ISOLATED);

    if (!stringp(arg)) {
	show_who(me);
	return 1;
    }

    switch (arg[0]) {
    case 'c':
	show_who_special(WHO_C, me);
	return 1;
    case 'f':
	show_who_special(WHO_F, me);
	return 1;
    case 'd':
	if ((int)TP->query_coder_level() < 1) {
	    TP->tell_me("This option is available to coders only.");
	    return 1;
	}
	show_who_special(WHO_D, me);
	return 1;
    case 'g':
	if ((int)TP->query_coder_level() < 1) {
	    TP->tell_me("This option is available to coders only.");
	    return 1;
	}
	show_who_special(WHO_G, me);
	return 1;
    case 'l':
	call_other(PURGATORY, "??");
	s = map(
	  who_filter(
	    all_inventory(find_object(PURGATORY)),
	    me, 0), "who_mapper", this_object());
	// Can't use npc test as it relies on checking if it's interactive...
	// And ld'ers sure aren't... :-( But, there won't be any NPCs in
	// purgatory anyway, will there?

	if (sizeof(s))
	    TP->tell_me("Total of "+sizeof(s)+" linkdead players visible to you:\n"
	      +implode(s, ", ", " and "));
	else TP -> tell_me("No linkdead players.");
	return 1;
    case 'p':
	this_player()->tell_me("Try 'party' to get\
 list of all parties.");
	return 1;
#if 0
	return (int) "/areas/tristeza/virtual/S8_6"->party_list();
#endif
    case 'q':
	s = sort_array(
	  map(
	    who_filter(users(), me, 0),
	    "who_mapper", this_object()),
	  #'>);

	TP -> tell_me("Total of "+sizeof(s)+" players visible to you:\n\n"
	  +implode(s, ", ", " and "));
	return 1;
    case 'r':
	show_who_special(WHO_R, me);
	return 1;
    default:    
	me->tell_me(
	  "Usage: who c/f/l/p/q/r/d/g\n"+
	  "(countries/followers/linkdead/party/quick/race/diseases/guilds)");
	return 1;
    }
}

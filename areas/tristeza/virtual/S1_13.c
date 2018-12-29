#include "../DEFS.h"
#include <areas.h>
#include <driver.h>
#include <guild.h>

#ifdef GN_MAGE
#include "/guilds/mage/mage_defs.h"
#endif

#include <generic_rooms.h>

inherit TRISTEZA_ROOM_C;

/* There was a teleport device for mage guild members
 * in StickMud. As StickLib has no guilds (or at least
 * not mages), we need to think of something else...
 */

// Teleport to Mage Guild for members   //Elena
// Now _only_ mages _REALLY_ can create the portal...  //Bull
// Added portal access to mage white list people. //Bull
// Added fumble possibility to portal. //Bull

static string *FUMBLE_PLACES = ({
	GENERIC_ROOM_SHOP,
/* Old fumble rooms: */
#ifdef AREA_EAST_FOREST
	AREA_EAST_FOREST "virtual/M8x0"
#endif
#ifdef AREA_ISLANDS

	AREA_ISLANDS "r/sea1",
	AREA_ISLANDS "r/frog1",
#endif
#ifdef AREA_SEWERS
        AREA_SEWERS+"lvl1/narrow6",
#endif
	});

#ifdef GN_MAGE
#define	ROOM_TO_TELEPORT_TO	GUILD_PATH_MAGE "rooms/inforoom"
#else
#define	ROOM_TO_TELEPORT_TO	GENERIC_ROOM_SHOP
#endif

string *ld_id_list;

void
create_room()
{
	::create_room();
	ld_id_list = ({ });
	set_short("Mages' Square");
	set_long("You are on the Mages' Square. The Godshire runs east and\
 the Wall Street to the north. In the southwest corner stands a tall tower.\n\
In the middle of the square stands a HUGE ice cube and in front of it stands\
 a statue.");
	set_items(([
({"statue","mage statue","statue of val","val"}) :
"The statue depicts Val. Somehow it is always in front of the ice cube, no\
 matter how much you try to walk around it. There is a plaque on the foot\
 of the statue.",
"plaque" : "On the plaque you can read:\n'The old stupid wizard who got\
 himself nearly killed. Retired from all magical activities.\
 Thinking how to create gun powder'."
 	]));
	set_exits(([
		"north"	: CITY_DIR + "S1_12",
		"east"	: CITY_DIR + "S2_13",
		"southwest" : CITY_DIR + "tower_0x14"
	]));
	set_commands(([
		"cast" : "teleport"
	]));
	add_monster(AREA_TRISTEZA+"npc/leper",
		    "Leper arrives and cries: Unclean!", 1);

	add_monster(AREA_TRISTEZA+"npc/beaver",
		    "Beaver sneaks in and giggles pervertly.",1);
}

int
teleport(string str)
{
        string *fumbles;

// No mage guild, no need to teleport...
	return 0;

#ifdef GN_MAGE
        if(((string)this_player()->query_guild() != GN_MAGE) &&
           (member(LIST_DAEMON->read_whitelist(),
             (string)this_player()->query_real_name()) == -1 )) return 0;
#endif
	if (str!="portal") return notify_fail("Create what?\n"), 0;
	this_player()->tell_me("You open a large portal and boldly step\
 through it. It seems exceptionally stable and clear.");

        if(!random(41)) {
          fumbles = FUMBLE_PLACES;
          this_player()->move_player("Vcreates a portal and steps through it\
 to some other dimension", fumbles[random(sizeof(fumbles))] );
        }
        else
	  this_player()->move_player("Vcreates a portal and steps through it\
 to some other dimension", ROOM_TO_TELEPORT_TO);

	return 1;
}

// The following is ugly but it's the best way before we get VBFC
// items.. //Starbyt

#define ICECUBE_ID ({"cube","ice cube","ice","huge ice cube"})

varargs status
id(string arg, object who)
{
    return ::id(arg, who) ||
	member_array(arg, ICECUBE_ID) >= 0 ||
        member_array(arg, ld_id_list) >= 0;
}

varargs string
query_long(string arg, object who)
{
object *u, o;
string *tmp_list;
int    i;
	if (arg && member_array(arg, ld_id_list) >= 0) {
	  if (o = find_player(arg)) {
          	return (string) o -> query_long(0, this_player()) +
		"\nThe ice makes it bit hard to see the details.";
	  } else return ::query_long(arg, who);
	}
	if (member_array(arg, ICECUBE_ID) < 0)
		return ::query_long(arg, who);

// Now we assume we have who_filter() in player object which
// checks if we should see certain persons... this might change so
// be aware.

	call_other(GENERIC_ROOM_PURGATORY, "???");
	u = filter_array(all_inventory(find_object(GENERIC_ROOM_PURGATORY)),
		"who_filter", this_player());
	ld_id_list = allocate(i = sizeof(u));
	if (!i) return "A strange, huge ice cube. Wonder what it does here.";
	tmp_list = allocate(i);
	while (i--) {
		tmp_list[i] = capitalize(ld_id_list[i] = (string)
			u[i]->query_real_name() );
	}
	return sprintf("A strange, huge ice cube. Inside it you see a frozen\
 figure%s of %s, with blue cheeks and a petrified look in the%s eyes.",
		(sizeof(tmp_list)==1) ? "" : "s",
		implode(tmp_list, ", ", " and "),
		(sizeof(tmp_list)==1) ? "" : "ir");
}

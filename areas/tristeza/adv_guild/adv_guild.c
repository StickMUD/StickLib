inherit "/lib/guild";

#include <areas.h>
#include <room_defs.h>
#include <generic_rooms.h>
#include <generic_items.h>

void make_noise();

void
create_guild()
{
    set_short("Adventurers' Guild");
    set_long("Adventurers' Guild! This is usually the place where the "
      "players with a clue go as soon as they can. The doorway in west "
      "leads out, and there is a stairway going up. Some people love to "
      "type 'quests' and try to make themselves feel more powerful.\nThe "
      "descriptions are blatantly boring here, so why not customize "
      "them?\nIn addition, you probably want to have more bulletin "
      "boards, some of them perhaps coder-only... like the one to the "
      "north.");
    set(NO_PK);
    set(IN_CITY);
    set(ROOM_WD,WD_INDOORS);
    set_light_level(LT_LIGHTS);
    set_exits(([
	"west"  : AREA_TRISTEZA+"virtual/S8_6",
	/* moved the board upstairs
			"south" : AREA_TRISTEZA+"adv_guild/ad_ed",
	*/
	/* Was:
			"up"	: AREA_TRISTEZA "tower/tower1.c",
	Is: */
	"up"	: AREA_TRISTEZA "adv_guild/skill_room",
	"north" : GENERIC_ROOM_IMMORTAL,
	"fire" : GENERIC_ROOM_FIRE,
	"wind" : GENERIC_ROOM_WIND,
	"earth" : GENERIC_ROOM_EARTH,
	"water" : GENERIC_ROOM_WATER
      ]));
    add_command("north", "do_north");

    /* Bulletin board */
    add_object(({ GENERIC_BBOARD,
	({ "set_board", 300, 0, "adv_guild_board", 0 }),
      }), 0, 1);

    /* User graph */
    call_other(GENERIC_USER_GRAPH, "????");

    // So I'm a little theatrical.....
    // Starks
    call_out(#'make_noise, 500);
}

status do_north(string arg)
{
    if(!this_player()->query_coder_level())
    {
	this_player()->tell_me("Coders only!");
	return 1;
    }
    return 0;
}

void make_noise() {
    remove_call_out(#'make_noise);
    if(random(2))
	tell_here("You hear a banging sound from above.");
    else
	tell_here("You hear a sawing noise from above.");
    call_out(#'make_noise, 450 + random(50));
}

#include <nroom.h>
#include <areas.h>
#include <coder_levels.h>
#include <generic_npcs.h>

void
create_room()
{
    set_short("The hall of the wizards");
    set_long("You are in the hall of the wizards. There is an elevator \
door to the west, with a bell attached to it.");
    set(ROOM_WD, WD_INDOORS);
    set_light_level(LT_LIGHTS);
    set(NO_PK);
    set(IN_CITY);
    set(PREVENT_TELEPORT);
    set_items(([
	"bell" : "You can ring it.",
	"field" : "It shimmers a lot!",
	"door" :  "What a crazy door!"
      ]));
    set_commands(([
	"west"	: "west",
	"ring"	: "ring",
	"use"	: "ring",
	//		"north"	: "north",
	"steal"	: "steal"
      ]));
    add_monster(GENERIC_NPC_LEO, "Leo arrives.", 1);
}

status
steal()
{
    this_player() -> tell_me("Not now.");
    return 1;
}

varargs string
query_long(string str, object who)
{
    if (str == "door") {
	if (!(AREA_TRISTEZA+"rooms/elevator")->query_closed_door()
	  && (int) (AREA_TRISTEZA+"rooms/elevator") -> query_level() == 1)
	    return "The door is open.";
	else return "The door is closed.";
    }
    return ::query_long(str, who);
}

status
west()
{
    if ((AREA_TRISTEZA+"rooms/elevator")->query_closed_door()
      ||(int)(AREA_TRISTEZA+"rooms/elevator")->query_level() != 1) {
	this_player()->tell_me("The door is closed. Ring bell to \
call the boy.");
	return 1;
    }

    if ((string)this_player()->query_real_name() == "Leo") return 1;

    this_player()->move_player("west", (AREA_TRISTEZA+"rooms/elevator"));
    return 1;
}

status
ring(string str)
{
    if (str != "bell")
	return notify_fail(capitalize(query_verb()) + " what?\n") , 0;
    if ((AREA_TRISTEZA+"rooms/elevator") -> call_elevator(1))
	(AREA_TRISTEZA+"rooms/elevator")->tell_here(
	  "Bell rings.\nBoy makes a note in his notebook.");
    this_player() -> tell_me("Ok.");
    return 1;
}

// Quest room isnt needed anymore - Quest daemon handles the quests -=Germ=-
#if 0
status
north()
{
    if ((int) this_player()->query_coder_level() < LVL_CODER) {
	this_player() -> tell_me("A strong magic force stops you.");
	return 1;
    }
    this_player() -> tell_me("You wriggle through the force field...");
    this_player()->move_player("north through shimmering field",
      "/room/quest_room");
    return 1;
}
#endif

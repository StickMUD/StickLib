/* Death's rooms */
#include <nroom.h>
#include <areas.h>

void
create_room()
{
    set_short("Bottom of stairs");
    set_long(
      "You are standing on a top of spiral staircase. To the east goes a dark\
 winding corridor, from which you can sense terrible hopelessness.");
    set_light_level(LT_DARKNESS);
    set(ROOM_WD, WD_NONE);
    set(PREVENT_SHOUT);
    set(ROOM_ISOLATED);
    set_exits(([
	"east"	: AREA_TRISTEZA+"death/chellar1",
	"up"	: AREA_TRISTEZA+"death/top_stairs"
      ]));
    replace_program(ROOM_FILE);
}

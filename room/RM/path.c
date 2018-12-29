#include <nroom.h>
void create_room() {
    set_short("A path leading behind the church");
    set_long(
      "You are walking along a path leading behind the church. "
    );
    set_exits(([
	"south" : "/areas/tristeza/rooms/race_guild",
	"west" : "/areas/tristeza/temple/temple_ruins",
      ]));
}

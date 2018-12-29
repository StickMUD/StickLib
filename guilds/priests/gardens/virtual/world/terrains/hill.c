/* Small hills */

#include "../gardens.h"
#include <virtual_room.h>

string *
do_desc() {
    string *tmp1;
    string *tmp2;

    tmp1= ({"You are following an animal trail up and over a hill.\n"});

    tmp2= ({
      "This small hill rises up out of the basin, giving a clear \
view of the surrounding areas. You feel that this surely must have \
been where Clerics of old came to meditate and worship Boccob."
    });

    return ({tmp1[random(sizeof(tmp1))]+tmp2[random(sizeof(tmp2))]});
}

void
create_virtual(){
    set(ROOM_WD, WD_OUTDOORS);
    set_light_level(LT_LIGHTS);
    set_short("Small hill");
    if(strstr(file_name(), "#") < 0) return;

    set_virtual_exits(([
	"north":({0,-1}),
	"northeast":({1,-1}),
	"east":({1,0}),
	"southeast":({1,1}),
	"south":({0,1}),
	"southwest":({-1,1}),
	"west":({-1,0}),
	"northwest":({-1,-1}),
      ]));

    set_distant_look(1);
    set(SKIP_EXITS);

    set_random_desc(do_desc());

    add_random_object(sprintf("%d:%d:%s",
	1+random(2), 75, GARDEN_NPC "mini_dragon"));
    add_random_object(sprintf("%d:%d:%s",
	2+random(3), 100, GARDEN_NPC "mini_troll"));
}

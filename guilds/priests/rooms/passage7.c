#include "priest_room.h"
#include <guild.h>

void create_room()
{
    set_light(LT_LIGHTS);
    if(random(3)) set(PREVENT_TELEPORT);
    set(ROOM_GUILD);
    set_outdoors(WD_INDOORS);
    set_short("A small marble passageway");
    set_long(
      "This passageway is extremely spartan, with cold unadorned marble "
      "floors, and an extremely tall ceiling that vanishes into mist. ");
    add_exit("northwest",PRIEST_ROOM_DIR+"garden_entry");
    add_exit("south",PRIEST_ROOM_DIR+"passage5");
    set(IN_CITY);
    replace_program(ROOM_FILE);
}

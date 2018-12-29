#include "d_room.h"

void
create_room ()
{
    set_short ("Cave opening");
    set_long (
"Dusty opening in the crossing of two tunnels, other going east-west\n" +
"and disappearing into distance just in few meters, other leading\n" +
"south towards something bright.\n");
    add_exit ("east", PATH + "pathway1");
    add_exit ("west", PATH + "sidehall");
    add_exit ("south", PATH + "entrance");
    SET_LIGHTS();
}

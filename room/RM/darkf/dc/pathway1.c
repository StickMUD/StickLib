#include "d_room.h"

void
create_room ()
{
   set_short ("Dusty tunnel");
    set_long (
"Dusty tunnel leading east-west inside dark, damp cave. You can't\n" +
"see longer than just few feets.\n");
    add_exit ("west", PATH + "crossing");
    add_exit ("east", PATH + "pathway2");
    SET_LIGHTS();
}

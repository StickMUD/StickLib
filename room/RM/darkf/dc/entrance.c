#include "d_room.h"

void
create_room ()
{
    set_short ("Entrance to a cave");
    set_long (
"Dark tunnel crawls downwards, deeper into the rock, darker, towards\n" +
"something what the smell of sulphur tickling your nose makes you make\n" +
"scary assumptations on. Back to south you see some forest and bright sky.\n");
    add_exit ("north", PATH + "crossing");
    add_exit ("south", PATH + "../forest99");
    SET_LIGHTS();
}

#include "tower.h"

int destination;

void
create_room ()
{
    destination = 0;
    set_short ("Top of the tower");
    set_long (
"The top level of the Wizard's Tower. Ridge ceiling limits the " +
"space here. A huge, very technical looking device is here, half-way " +
"out from an opening in the ceiling. The winding stairs lead down into " +
"shadows.");
    SET_LIGHTS();
    add_exit ("down", PATH + "tower5");
    add_item (({"ceiling", "roof"}),
        "Well, it's a normal tower ridge ceiling, with an opening in it.");
    add_item (({"device","machine","telescope","scope"}),
"Thinking for a while what it might be, it occurs to you that it is a tele" +
"scope, a device what with you can reach the sun and touch the stars. May"+
"be you could point it at some specific star and peer through the eyehole.");
    add_item (({"opening","hole"}), "You see the sky through it!");
}

void
init_room ()
{
    add_action ("peer_", "peer");
    add_action ("point_", "point");
}

int
peer_ (string s)
{
    if (!this_player()->query_invis()) {
        say((string)this_player()->query_name() + " peers through the " +
        "telescope.\n");
    }

    if (destination); /* do the stuff here */
    /* maybe a switch for different stars.. see the library */
    /* check for the time of day and the cloud situation */

    return 1;
}


int
point_ (string s)
{
    notify_fail ("Point telescope at what?\n");
    if (!s) return 0;
    sscanf (s, "the %s", s);
    sscanf (s, "telescope %s", s);
    if(!sscanf (s, "at %s", s))
        return 0;

    /* handle starnames with switch here.. see library */

return 1;
}

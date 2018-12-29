#include "tower.h"

void
create_room ()
{
    set_short ("Second level of the tower");
    set_long (
"The second level of the tower consists of a cozy living-room set with a "+
"nice dusty and comfortable looking armchair and a fine wooden table "+
"standing over a very oriental and very expensive looking thick rug. "+
"Flames crack brown wooden logs in a fireplace leaning on one corner. "+
"Winding stairs lead up towards darkness.");
    SET_LIGHTS();
    add_exit ("up", PATH + "tower3");

    add_item (({"armchair","chair","arm chair"}),
"It's a soft and very comfortable looking, covered with fine red silk, "+
"and has a small dust topping on it - looks like no-one has sat on it "+
"for a while.");
    add_item (({"table","wooden table","carvings"}),
"A wooden antique table with nice carvings. Nothing more.");
    add_item (({"rug", "thick rug", "oriental rug"}),
"You could swear this is THE oriental rug you heard about when you were "+
"a kid, the flying one. Maybe it's the mystical looking patterns on it "+
"that make the image.");
    add_item (({"pattern","patterns"}),
"The rug sure has some very strange looking cloth patterns... Deeper and deeper, "+
"stronger and stronger, over and over again, your mind feels a bit like it "+
"was twisting. But then, nothing happens.");

    add_item (({"log","logs","wood","wooden log","wooden logs"}),
"They crack nicely in the fire.");
    add_item (({"fire","flames"}), "Flames dance in the fireplace.");
    add_item ("fireplace",
"Just a normal fireplace with a fire in it. No, you won't fit into there, " +
"and no, you shouldn't put your hand into there.");

    add_object (PATH + "cat");

}

void
init_room()
{
    add_action("put_", "put");
}

int
put_ (string arg)
{
    if (arg != "hand into fireplace" && arg != "hand into fire")
        return 0;
    if (!this_player()->query_invis())
        say((string)this_player()->query_name() + " foolishly put " +
         (string)this_player()->query_possessive() + " hand into the " +
         "fireplace.");
    write ("OUCH! THAT HURT!\n");
    this_player()->hit_player(random(10)); /* punish them, a little, for */
    return 1;                              /* stupidity. */
}

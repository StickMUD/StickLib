#include "tower.h"

void
create_room ()
{
    set_short ("Bottom of the tower");
    set_long (
"The bottom level of the tower shows no way up. Bare stonen walls and " +
"ceiling make this room look cold and unfriendly. Small opening on the " +
"south wall gives an escape to the outside.");
    SET_LIGHTS();
    add_exit ("out", PATH + "outside");
    add_item (({ "wall", "walls" }), "Stonen walls, nothing else.");
    add_item ("opening", "It leads out from this tower.");
    add_item (({ "ceiling", "roof" }), "The stonen ceiling shows no possible way up.");
}

void
init_room ()
{
    if (find_call_out ("gnome_incarnation") == -1)
        call_out ("gnome_incarnation", 45 + random(15));
    add_action ("say_", "say");
    add_action ("say_", "'", 1);
    add_action ("jump_", "jump");
    add_action ("climb_", "climb");
    add_action ("up_", "up");
    add_action ("up_", "u");
}

int
up_ ()
{
    write ("Try to be more creative.\n");
    return 1;
}

int
jump_ (string arg)
{
    write ("The ceiling is far out of reach by jumping, besides the ceiling\n"+
           "itself does not show any way up.\n");
    return 1;
}

int
climb_ (string arg)
{
    write ("You try to climb up the walls but have no success.\n");
    return 1;
}

void
gnome_incarnation ()
{
    while (remove_call_out ("gnome_incarnation") != -1);
    tell_room (this_object(), "A gnome arrives.\n");
    call_out ("gnome_postincarnation", 1);
}

void
gnome_postincarnation ()
{
    tell_room (this_object(), "Gnome says: beam me up, scottie\n");
    tell_room (this_object(), "Gnome disappears in a blizzing flash of light.\n");
}


int
say_ (string arg)
{
    if (query_verb() != "say") {
        if (!arg) return 0;
        arg = query_verb()[1..1000] + " " + arg;
    }
    if (arg) arg = lower_case (arg);

    if (arg[0..18] == "beam me up, scottie" ||
       arg[0..17] == "beam me up scottie") {
        call_out ("wake_up_scottie", 1, this_player());
    }
    return 0;
}


void
wake_up_scottie (object pl)
{
    if (!pl || environment(pl) != this_object()) return;
    if (!pl->query_invis())
        say ((string)pl->query_name() + " disappears in a blizzing flash of light.\n");
    write ("Bright light blinds your eyes you for a moment, and you feel your stomach\n");
    write ("turn upside down. When your vision retains, you find out to be elsewhere.\n");
    move_object (this_player(), (PATH + "tower2"));
}


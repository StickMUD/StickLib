#include "priest_room.h"
#include PRIEST_GARDENS "push_cmd.c"
#include <guild.h>

#define GUILD_HALL PRIEST_ROOM_DIR "hall"
#define GARDEN_ROOM PRIEST_GARDENS "virtual/world/rooms/3_7"

int gates_shut;

void create_room()
{
    set(PREVENT_TELEPORT);
    set_light_level(LT_LIGHTS);
    set(ROOM_GUILD);
    set(ROOM_WD,WD_INDOORS);
    set(IN_CITY);
    set_short("The antechamber to the guild conservatory");
    set_long(
"You stand in an entirely glassed-in room, as breathtaking in it's \
beauty as the passageways leading here were stark. There is some kind \
of a magic field shimmering around the door leading north, and the scenes \
beyond the glass are beautiful, but somehow distorted.");

    set_items(([
    "glass": "Most of the glass is freshly gleaned, and sparkles in \
the strange light that comes through from the outside. However, there \
is one pane of glass that has a smudged handprint on it.",
    "field": "You get a strong sense that the magic field will change \
something about you, when you go through it. The feeling passes.",
    "door": "The door is magnificent cut glass, beveled and arranged \
in an extremely pleasing fashion. You notice a glyph 'K' interwoven \
into the motif.",
    "scenes": "You watch the scenes through the glass for a while, \
and while there is definately something strange about it, you cannot \
put your finger on it. It is as if you are looking at a garden through \
the bottom of an extremely thick drinking glass.",
    ]));

    set_exits(([
    "southwest" : PRIEST_ROOM_DIR "passage6",
    "southeast" : PRIEST_ROOM_DIR "passage7",
    "north" : GARDEN_ROOM,
    ]));

    add_command("north", "go_north");
    add_command("push", "push");

    gates_shut = GUILD_HALL->query_shut();
}

int go_north() {
    if(gates_shut) {
        this_player()->tell_me("The magic field prevents you from going \
through the door.");
        return 1;
    }
    this_player()->tell_me("You feel strange as you pass through the field. \
The feeling quickly passes.");
    return 0;
}

void
shut_gates(int shut_em) {
    gates_shut = shut_em;
    if(!find_object(GARDEN_ROOM))
        call_other(GARDEN_ROOM, "???", 0);
    GARDEN_ROOM->shut_gates(shut_em);
}


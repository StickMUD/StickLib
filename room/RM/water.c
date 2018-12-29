#include <nroom.h>
#include <bboard_defs.h>
#include <generic_rooms.h>
#include <generic_items.h>
#include <player_defs.h>

int valid_board_write_access(object me);
int valid_board_read_access(object me);

void create_room()
{
    set_short("Guild of the Water Kingdom");
    set_long("The walls shimmer so brightly no other light source is "
             "needed to light the room. The walls are made of flawless "
             "sapphire diamond.  Theres a beautiful symbol on the wall "
             "to your left.");
 
    set_items( ([
       ({ "wall", "walls" }) :
          "The walls glitter almost makes you blind.",
       ({ "water symbol", "symbol" }) :
          "Its the symbol of water, a Killer Whale leaping from the water. "
          "It represents Power and Survival!",] ) );


    set_exits( ([
        "out"  : GENERIC_ROOM_ADV_GUILD,
    ]) );
 
    set_outdoors(WD_INDOORS);
    set_light_level(LT_LIGHTS);
 
    set(NO_PK);
    set(IN_CITY);
    set(PREVENT_TELEPORT);
    set(PLR_PREVENT_SUMMON); 


    add_object( ({ GENERIC_BBOARD,
        ({ "set_board", 100, ([ BB_WRITE_CLOSURE : #'valid_board_write_access,
          BB_READ_CLOSURE : #'valid_board_read_access, ]), "water_guild_board",
        0 }),
        }), 0, 1);
}

int valid_board_write_access(object me)
{
    return (this_player()->query_kingdom("water") ||
this_player()->query_coder_level());
}

int valid_board_read_access(object me)
{
    return 1;
}



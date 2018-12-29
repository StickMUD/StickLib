#include <nroom.h>
#include <bboard_defs.h>
#include <generic_rooms.h>
#include <generic_items.h>
#include <player_defs.h>

int valid_board_write_access(object me);
int valid_board_read_access(object me);

void create_room()
{
    set_short("Guild of the Earth Kingdom");
    set_long("The solid granite walls and marble pillars make this room as "
             "elegant as any ever seen. Bright light emits from torches "
             "mounted in the walls.  Theres an Earth symbol on one wall.");
 
    set_items( ([
       ({ "wall", "walls" }) :
          "The walls are solid granite. No magic could penetrate this room.",
       ({ "earth symbol", "symbol" }) :
          "Its the symbol of earth, an immense snow covered mountain peak. "
          "The symbol of strength and solidarity.", 
       ({ "pillar", "pillars" }) :
          "Two solid marble pillars stand on either side of the exit.",
       ({ "torch", "torches" }) :
          "The torches are magically mounted into the wall.",
           ] ) );


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
          BB_READ_CLOSURE : #'valid_board_read_access, ]), "earth_guild_board",
        0 }),
        }), 0, 1);
}

int valid_board_write_access(object me)
{
    return (this_player()->query_kingdom("earth") ||
this_player()->query_coder_level());
}

int valid_board_read_access(object me)
{
    return 1;
}



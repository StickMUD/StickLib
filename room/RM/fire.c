#include <nroom.h>
#include <bboard_defs.h>
#include <generic_rooms.h>
#include <generic_items.h>
#include <player_defs.h>

int valid_board_write_access(object me);
int valid_board_read_access(object me);

void create_room()
{
    set_short("Guild of the Fire Kingdom");
    set_long("The walls of the room emit a deep red glow. They seem to give "
             "off some kind of magical heat.  Torches mounted into the walls "
             "add to rooms comfortable ambiance.  You feel totally safe here. "
             "You see a large symbol on one wall.");
 
    set_items( ([
       ({ "wall", "walls" }) :
          "The walls are made of pure Brimstone which is also buffed and "
          "polished to a glass-like appearance.",
       ({ "fire symbol", "symbol" }) :
          "Its the symbol of fire, a large reddish orange flame. The symbol "+
          "of ferocity and total domination."] ) );


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
          BB_READ_CLOSURE : #'valid_board_read_access, ]), "fire_guild_board",
        0 }),
        }), 0, 1);
}

int valid_board_write_access(object me)
{
    return (this_player()->query_kingdom("fire") ||
this_player()->query_coder_level());
}

int valid_board_read_access(object me)
{
    return 1;
}



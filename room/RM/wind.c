#include <nroom.h>
#include <bboard_defs.h>
#include <generic_rooms.h>
#include <generic_items.h>
#include <player_defs.h>

int valid_board_write_access(object me);
int valid_board_read_access(object me);

void create_room()
{
    set_short("Guild of the Wind Kingdom");
    set_long("You enter this room and you get the sensation of floating "
             "in the clouds. The walls are soft white in color but dont "
             "seem to have substance. A bright golden symbol seems to "
             "hover to your left.");
 
    set_items( ([
       ({ "wall", "walls" }) :
          "You touch the walls but your hands seems to go through them.",
       ({ "wind symbol", "symbol" }) :
          "Its the symbol of wind, a beautiful golden cloud, the symbol "
          "of Nature and its magnificant powers. ",] ) );


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
          BB_READ_CLOSURE : #'valid_board_read_access, ]), "wind_guild_board",
        0 }),
        }), 0, 1);
}

int valid_board_write_access(object me)
{
    return (this_player()->query_kingdom("wind") ||
this_player()->query_coder_level());
}

int valid_board_read_access(object me)
{
    return 1;
}



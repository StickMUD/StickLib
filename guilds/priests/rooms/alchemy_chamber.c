#include "priest_room.h"

#define POT_HEAL_MIN PRIEST_ALC_DIR "heal_minor_wounds"
#define POT_HEAL_MAJ PRIEST_ALC_DIR "heal_major_wounds"

#define COEF_FILE PRIEST_ALC_DIR "get_coef"
#define BOTTLE 30   /* costs... */
#define SCALPEL 1000
#define HMINP 300
#define HMAJP 1600

#define _DESC 0
#define _FUN 1
#define _COST 2
#define _N 3

#include PRIEST_DIR "lib/fill_bag.c"
#include <guild.h>
#include <macros.h>

void reset_room();

mapping shop_list;


string get_list() {
    return sprintf("\
List of items available:\n\
----------------------------------------------------------\n\
Sample bottle........................................."+BOTTLE+" gc\n\
A fine scalpel (saveable)..........................."+SCALPEL+" gc\n\
A potion of heal minor wounds (%d pcs) (buy minor)...."+HMINP+" gc\n\
A potion of heal major wounds (%d pcs) (buy major)..."+HMAJP+" gc\n\n\
Fillbag [bag number], cost: bottles fit * "+BOTTLE+" gc."
      ,shop_list["minor", _N],shop_list["major", _N]);
}


void
create_room() {
    set(ROOM_WD,WD_INDOORS);
    set(ROOM_GUILD);
    set(NO_CLEANUP);
    set(PREVENT_TELEPORT);
    set_light_level(LT_LIGHTS);
    set_short("The alchemist's chamber");
    set_long("This is the chamber of the guild alchemist. Innumerous \
wondrous scents and fumes fill the air, making you feel a bit sick. The room \
is filled with many kinds of bottles and tubes, some of them empty, some \
filled with multi-coloured liquids. A collection of strange herbs and other \
weird-looking ingredients lie in their containers on a shelf in the back of \
of the room.");

    set_items(([
	({"shelf","herb","herbs"}) : "A collection of strange herbs and other \
weird-looking ingredients lie in their containers on a shelf in the back \
of the room",
	({"bottles","tubes"}) : "The room is filled with many kinds of bottles \
and tubes, some of them empty, and some filled with multi-coloured liquids.",
	"list": #'get_list,
      ]));

    set_exits(([
	"northeast" : PRIEST_ROOM_DIR "nave",
	"south" : PRIEST_ROOM_DIR "alchemy_lab"
      ]));
    reset_room();
    set_commands(([
	"list" : "list_items",
	"buy"  : "buy_item",
	"fillbag" : "fill_bag"
      ]));

    add_monster(PRIEST_MON_DIR "alchemist", "The alchemist arrives.", 1);
    add_permanent(PRIEST_OBJ_DIR "chest");
    add_permanent(PRIEST_OBJ_DIR "top_alc_board");
}


status
list_items() {
    TP->tell_me(get_list());
    return 1;
}

status
buy_item(string str) {
    string desc;
    object item;
    closure clone_fun;
    int cost,n;

    if(!str)
	return notify_fail("Buy what?\n"),0;

    if(!present("alchemist",this_object()))
	return notify_fail("The shop seems to be closed.\n"),0;

    if (!IS_PRIEST(TP))
	return notify_fail("The alchemist says: We don't do business with heathens.\n"),0;

    if(!mapping_contains(&desc,&clone_fun,&cost,&n,shop_list,str))
	return notify_fail("The alchemist says: Excuse me? We don't sell those.\n"),0;

    if(n==0)
	return notify_fail("The alchemist says: Sorry, we are sold out.\n"),0;

    if(TP->query_money()<cost)
	return notify_fail("You can't afford that.\n"),0;

    item = funcall(clone_fun);
    if(!item)
	return notify_fail("Error when cloning object. Report this to guild coders.\n"),0;

    if(transfer(item,TP)) {
	destruct(item);
	return notify_fail("You can't carry that much.\n"),0;
    }
    if(n>0)
	shop_list[str,_N]--;
    TP->add_money(-cost);
    TP->tell_me(sprintf("You pay %d coins for %s.",cost,desc));
    tell_here(sprintf("%s buys %s", TP->query_name(), desc), TP);
    return 1;
}

object clone_minor() {
    return POT_HEAL_MIN->clone_potion(this_player(), 100);
}

object clone_major() {
    return POT_HEAL_MAJ->clone_potion(this_player(), 100);
}

object clone_bottle() {
    return clone_object(PRIEST_OBJ_DIR "priest_bottle");
}

object clone_scalpel() {
    return clone_object(PRIEST_OBJ_DIR "scalpel");
}

void
reset_room()
{
    shop_list = ([
      "minor": "a potion of heal minor wounds"; #'clone_minor; HMINP; 1+random(5),
      "major": "a potion of heal major wounds"; #'clone_major; HMAJP; 1+random(3),
      "bottle": "a sample bottle"; #'clone_bottle; BOTTLE; -1,
      "scalpel": "a fine scalpel"; #'clone_scalpel; SCALPEL; -1,
    ]);
}

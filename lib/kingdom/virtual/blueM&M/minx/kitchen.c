

//Minx's Kitchen
#include "minx.h"
#include <nroom.h>

inherit "/lib/kingdom/food_shop";

#define TP this_player()


void
init_foods(){

/* ids, short, strength, weight, cost, #per, long desc, eat msg, said msg */
    foods = ({
        ({ "hare","rabbit" }), "Rabbit pie",
	40, 1, 75,10,
	"A well roasted rabbit pie.",
	"Wonderfully flavorfull!",
	"munches on a rabbit pie." });
}


void
create_shop(){

	set_keeper_name("cook"); //optional
	set_hours(6,20);  // optional
	set_castle(KINGDOM,CASTLE); //MUST BE HERE
        set_light_level(LT_LIGHTS);
        set(ROOM_WD,WD_INDOORS);
        set_short("Kitchen"); 
        set_long("A cheesy little kitchen in Minx's castle. There's \
a menu on the wall.");
	
	set_exits(([
		
		]));

	add_monster(LOCAL+"cook","Cook comes back.",1);
}
status
read_cmd(string xx){

	if(!xx || xx != "menu"){
		this_player()->tell_me("Are you trying to read the menu?");
		return 1;
	}

	list_foods(1);
	return 1;
}



//Minxie's Estate
#include <nroom.h>
#include "minx.h"
string name;

void
castle_setup(string kingdom, string owner, string rooms){
name = rooms;
   switch(rooms){

      case "gate":

	set_short("the gate of Minx's Estate");
	set_long("There is a white marble building. The extra large \
windows	are framed by grey slate shudders. There is a flowerbed \
snuggled up against the estate. Several vines of ivy, dotted with \
small red flowerettes, twist and turn coating the marble, reaching up \
toward the copper roof. There is a sign.");
	set_light_level(LT_LIGHTS);
	set(ROOM_WD,WD_OUTDOORS);
	set_items((["sign":"Why don't you try reading this?"]));
	set_exits((["south":LOCAL+"room1"])); 
	break;

     case "room1":
	
	set_short("Minx's Estate");
	set_long("You are standing in the middle of a circular foyer \
made entirely out of the finest white marble. The stained glass ceiling \
is about twenty feet above your head and is supported by six white \
marble pillars. The centerpiece of the room is a crystal table and a \
large floral arrangement. A winding carpeted staircase leads to the \
second floor. You can feel the heat of a fire sweeping in from the south.");
	set_light_level(LT_LIGHTS);
	set(ROOM_WD,WD_INDOORS);
	set_exits((["north":LOCAL+"gate",
		"south":LOCAL+"room3",
		"up":LOCAL+"room6"]));		
	break;

    case "room2":

	set_short("Minx's Estate");
        set_long("The dining room is decorated in royal blues and \
accented with a touch of silver. The huge crystal dining table must \
seat twelve comfortably, yet is set with six place settings. There are \
four candles flickering slightly. You can almost hear the laughter \
and the merriment of one of Minx's infamous dinner parties.");
	set_light_level(LT_LIGHTS);
	set(ROOM_WD,WD_INDOORS);
	set_exits((["east":LOCAL+"room3", 
		"west":LOCAL+"kitchen"]));
	break;

     case "room3":

	set_short("Minx's Estate");
        set_long("This is the great hall of the estate. A massive \
white marble fireplace and floor to ceiling windows cover the back \
wall. There are numerous overstuffed couches and one overstuffed \
recliner. There are dozens of flickering candles scattered all about \
the room, providing a warm and cozy feeling. You can see a dining room \
to the west and a piano room to the east.");
	set_light_level(LT_LIGHTS);
	set(ROOM_WD,WD_INDOORS);
        set_exits((["east":LOCAL+"room4", 
		"north":LOCAL+"room1",
		"west":LOCAL+"room2"]));
	break;

     case "room4":

	set_short("Minx's Estate");
        set_long("The showpiece of this room is an enourmous black grand \
piano. There are plenty of comfortable chairs carefully arranged around \
the piano. There are plenty of flickering candles burning in here as \
well. You are enchanted by your surroundings and the capitvating aroma \
of roses and jasmine.");
	set_light_level(LT_LIGHTS);
	set(ROOM_WD,WD_INDOORS);
	set_exits((["west":LOCAL+"room3"]));
	break;

     case "room5":

	set_short("Minx's Estate");
	set_long("You are standing in the middle of Minx's Master \
bedroom. The antique kingsized canopy bed is draped with white sheer \
curtains. The bed is covered with a white quilt, finely crafted out of \
slightly different white patterned material. Arranged on the bed are \
dozens of colorful petite pillows. Along the back wall, underneath a \
large bay window, is a sitting area. The loveseat and chair are \
upholstered in a deep red, blue, green plaid fabric. There is a large \
portrait hanging over the white marble fireplace. On a large wooden \
table is a model of a small town, made completely out of sugar and sweets.");
	set_light_level(LT_LIGHTS);
	set(ROOM_WD,WD_INDOORS);
	set_items((["portrait":"Kelgath and Minx are smiling happily in \
the portrait hanging over the fireplace."]));
	set_exits((["west":LOCAL+"room6"]));
	break;

     case "room6":

	set_short("Minx's Estate");
	set_long("This is the second floor landing. Running along the \
back wall is a massive bookshelf. There must be hundreds of leather \
bound, gold trimmed books. A small table and a few wicker chairs are \
tucked over to one side. To the east is the master bedroom and to the \
west is the guest bedroom.");
	set_light_level(LT_LIGHTS);
	set(ROOM_WD,WD_INDOORS);
	set_exits((["west":LOCAL+"room7",
		"down":LOCAL+"room1",
		"east":LOCAL+"room5"]));
	break;

      case "room7":

	set_short("Minx's Estate");
	set_long("This is the guest bedroom of the estate. The room is \
decorated in deep reds and rich pinks. The queen sized bed looks \
comfortable and the over stuffed pillows look very welcoming. There is a \
tall wooden wardrobe and a long chester drawers. The windows look out \
over Minx's botantical gardens.");
	set_light_level(LT_LIGHTS);
	set(ROOM_WD,WD_INDOORS); 
        set_exits((["east":LOCAL+"room6"]));
	break;
	}
}

status
read_cmd(string what){
	if (what != "sign") return 0;
	if (what != "gate") return 0;
	TP->tell_me("Minx's Estate\n\
Home of Coder Minx the Blue M&M");
	return 1;
}

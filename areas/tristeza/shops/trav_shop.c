#include <nroom.h>
#include <areas.h>
#include <daemons.h>
#include <pipe_defs.h>
#include <generic_items.h>
#include "/areas/tristeza/DEFS.h"
 
#define PHASE_IN_TIME   90
#define PHASE_OUT_TIME  30
 
#define S_ITEMS ({ \
          150,  "Pipe (saveable)", "pipe", PIPE_FILE, \
          300,  "Mild Tobacco", "mild tobacco", OBJ_DIR+"lily", \
          900,  "Strong Tobacco", "strong tobacco", OBJ_DIR+"dung", \
          2700, "Altairian Sentient Hop Weed", "weed", OBJ_DIR+"weed", \
          500,  "Chewing Gum", "gum", OBJ_DIR+"gum", \
          50,   "Bottle collector", "collector", OBJ_DIR+"collector", \
        })
 
phase_in();
status deal(int i);
 
object door, door_place, chimp;
status in_phase;
static mixed *item;
static string *place;
 
void
create_room()
{
    item = S_ITEMS;
 
 
    place = ({
#ifdef CITY_DIR
        CITY_DIR+"S2_7",
        CITY_DIR+"S7_10",
        CITY_DIR+"S13_3",
        CITY_DIR+"S6_7",
        CITY_DIR+"S7_7",
        CITY_DIR+"S7_8",
#endif
#ifdef AREA_EAST
        AREA_EAST+"east/vill_shore",
#endif
#ifdef AREA_WEST
        AREA_WEST+"mines/tunnel2",
        AREA_WEST+"forest/clearing",
#endif
#ifdef GENERIC_ROOM_CHURCH
        GENERIC_ROOM_CHURCH,
#endif
#ifdef AREA_TRISTEZA
        AREA_TRISTEZA+"adv_guild/adv_guild",
        AREA_TRISTEZA+"shops/shop",
        AREA_TRISTEZA+"well_maze/maze1",
#endif
    });
 
 
    set(PREVENT_TELEPORT);
    set_light_level(LT_LIGHTS);
    set_short("In the travelling shop");
    set_long("Oh, oh! This is one of THOSE shops! You are in an "+
        "enormous room that seems to stretch into infinity. The walls "+
        "are lined with shelves upon shelves, all filled with wondrous "+
        "and exotic items.");
    set_outdoors(WD_INDOORS);
    add_permanent(GENERIC_TRASHCAN);
    add_monster(NPC_DIR+"chimpanzee", "Chimpanzee arrives.", 1);
    phase_in();
}
 
varargs string query_long(string s, object w)
{
    if (s) return ::query_long(s, w);
 
    return sprintf("%s\n%sType 'list to display the wares.\n\t'buy "+
        "<item>' to buy something.",
        ::query_long(s, w),
        (door && in_phase) ? 
        "There is a shimmering door to the south.\n" : "");
}
 
phase_in()
{
    object us;
    int i;
    string tmp;
    call_out("phase_out", PHASE_IN_TIME);
    if(!(us = users()) || !sizeof(us)) return;
    door_place = environment(us[(i = random(sizeof(us)))]);
    if(random(100) < 50 || !door_place || !sscanf(file_name(door_place),
        "room/%s", tmp) || door_place->query(PREVENT_TELEPORT) ||
        us[i]->query_coder_level()) {
        call_other(place[(i = random(sizeof(place)))], "???");
        door_place = find_object(place[i]);
    }
    if(door) destruct(door);
    door = clone_object(TRAV_SHOP_DOOR);
    move_object(door, door_place);
    in_phase = 1;
    tell_here("The sign winks off. Stability returns...");
}
 
phase_out()
{
    call_out("phase_in", PHASE_OUT_TIME);
    if(!in_phase) return;
    in_phase = 0;
    tell_here("A sign lights up: FASTEN SAFETY BELTS!\n"+
        "A low thrumming reaches your ears. You feel... not exactly motion\n"+
        "but rather a lack of stability.");
    if(door) door->fade();
}
 
init()
{
    add_action("list", "list");
    add_action("buy", "buy");
    add_action("south", "south");
    add_action("sell", "sell");
 
    // Rincewind, the original coder -- left in for posterity --Kieve
    if(this_player()->query_real_name() != "rincewind")
        return;
    add_action("off", "off");
}
 
status sell(string args)
{
    return notify_fail("The shop-keeper isn't buying.\n"), 0;
}
 
status off(string args)
{
    if(door) destruct(door);
    remove_call_out("phase_in");
    remove_call_out("phase_out");
    this_player()->tell_me("Ok.");
    return 1;
}
 
status list(string args)
{
    int i;
    this_player()->tell_me("\nItems for sale:\n");
    i = 0;
    while(i < sizeof(item))
    {
        this_player()->tell_me(item[i] +"\t"+ item[i+1]);
        i += 4;
    }
    this_player()->tell_me("\n");
    return 1;
}
 
status buy(string str)
{
    int i;
    object ob;
    string tmp;
    status bought;
 
    if(!str)
        return notify_fail("Buy what?\n"), 0;
 
    bought = 0;
    i = 0;
    while(i < sizeof(item))
    {
        if(sscanf(str, "%s"+ item[i+2] +"%s", tmp, tmp))
        {
            bought = 1;
            if(deal(i/4 + 1))
            {
                if(this_player()->query_money() >= item[i])
                {
#if 0
                    log_file("magic_shop", ctime(time())+"   "+
                        capitalize(this_player()->query_real_name())+
                        " bought '"+item[i+1]+"'\n");
#endif
                    this_player()->tell_me("You buy: "+ item[i+1] +"");
                    tell_here(this_player()->query_name() +" buys "+
                        item[i+1] +".", this_player());
                    ob = clone_object(item[i+3]);
                    if (!i) SMOKE_D->add_smoker(this_player());
                    this_player()->add_money(-item[i]);
                    if(!this_player()->add_weight(ob->query_weight()))
                    {
                        this_player()->tell_me("You have to drop it. It "+
                            "weighs too much!\n");
                        tell_here(this_player()->query_name() +" drops "+
                            item[i+1] +".", this_player());
                        move_object(ob, this_object());
                    }
                    else
                        move_object(ob, this_player());
                } // enough moeny
                else
                    this_player()->tell_me("You don't have enough money "+
                        "to buy "+item[i+1] +"!");
            } // if deal
        } // sscanf
        i += 4;
    }
    if(!bought)
        this_player()->tell_me("We do not have that in stock.");
    return 1;
}
 
status south(string args) {
    if(!door || !in_phase)
        this_player()->tell_me("The door is no longer there!");
    else
    {
        tell_here(this_player()->query_name() +" leaves south.",
            this_player());
        move_object(this_player(), door_place);
        tell_here(this_player()->query_name() +
            " arrives through the shimmering door.\n", this_player());
        command("look",this_player());
    }
    return 1;
}
 
status deal(int i) {
    if(i == 1 && present("smoking pipe", this_player()))
    {
        this_player()->tell_me("You already have a perfectly good pipe.");
        return 0;
    }
#if 0
    if(i == 8 && present("dictionary", this_player()))
    {
        write("You already have one!\n");
        return 0;
    }
#endif
    return 1;
}

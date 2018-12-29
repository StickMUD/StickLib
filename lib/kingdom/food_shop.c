/*
 * /lib/kingdom/food_shop.c   
 *
 * Modified 05-dec-94 by Doomdark.
 * Revised 19-Nov-95 by Kelgath
 * For Kingdoms 02-Jan-96 by Kel and Minx
 * - Won't automatically call "reset_shop" from "create_room" any more.
 * - Writes changed to "tell_me"s.
 * - now uses mapping and has limit to items to be sold per reset.
 * - checks if player is valid to use shop.
 * - allows autosetting of shop open hours.
 */
#include <nroom.h>

#pragma strict_types

#include "kingdoms.h"

#include <criminal.h>
#include <city_name.h>
#include <food_defs.h>

mixed *foods;
status kn_set, t_set; //Is name and time set?
int *remaining, open_time, close_time;
int open_time, close_time;
string keeper_name; // Name by which s/he goes... (baker,butcher,etc)
string castle, kingdom; // Kingdom values.

private static int functions_used;

#define	RESET_USED	1
#define	INIT_USED	2

#define DDATA_BLOCK	9

void set_keeper_name(string x){ keeper_name = x; kn_set = 1;}

void set_hours(int open, int close){ t_set = 1;
	open_time = open;
	close_time = close;
}

void
set_castle(string kgdm, string cstl){
	kingdom = kgdm;
	cstl = castle;
}

void
init_foods()
{
/* name: ids, short, strength, weight, 
	cost, num_per_reset, long desc, eat msg, said msg */
    foods =({ ({"bread", "rye", "loaf"}), "Loaf of rye bread",10, 2, 50,20,
	"This is a standardized " CITY_NAME " rye bread.",
	"Munch munch munch...quite tasty!",
	"eats a loaf of rye bread."}) ;
}

/* No need to change these in Castles */

int
shop_open(){
    	object ob;
	int now;
 
	if(!(ob=present(keeper_name,this_object())))
	return notify_fail("The "+keeper_name+" is not here.\n"), 0;
 
	now = (int)nature()->query_24_hour();

	if(now < open_time || now > close_time){
		notify_fail("The kitchen staff is off right now.\n");
		return 0;
	}  
 
    return 1;
}

string query_food_obj_file_name() { return FOOD_FILE; }
string query_default_food() { return "bread"; }
void extra_purchase_msg() { }

/* Modify drink/food cost for non-members etc. */

int
modify_cost(int c)
{
	return c;
}

status
valid_to_serve(object me){
//temp
return 1;
/*
	return KINGDOM_D->allowed_here(me,kingdom,castle);
*/
}

/* Option 1: write the list, otherwise just return it. */
string
list_foods(int option)
{
string l;
int i,j;

	l = sprintf("%-30s %6s %6s\n","Item","Cost","Remain");

	for(i=0;i<sizeof(foods);i+=DDATA_BLOCK){
		j = i / DDATA_BLOCK;
	l+=sprintf("%-30s %6d %3d\n", foods[i+1],
		modify_cost((int)foods[i + 4]),
		remaining[j]);
	}

	if (option != 1) return l;

	this_player()->tell_me(l);
	return 0;
}

void
reset_room()
{
	int i;

	remaining=({});
	for(i = 0; i < sizeof(foods); i += DDATA_BLOCK){
		remaining+=({foods[i+5]});
	}
 
	if (functions_used & RESET_USED)
		this_object()->reset_shop();
}

void
create_room()
{
	if (function_exists("reset_shop")) functions_used |= RESET_USED;
	if (function_exists("init_shop")) functions_used |= INIT_USED;

	init_foods();
	this_object()->create_shop();

// Let's add these commands after shops create_shop is called...

	if(!kn_set) keeper_name = "cook";
	if(!t_set){ open_time = 8; close_time = 22;}

	add_command("order","order");
	add_command("buy","order");
	add_command("fill","fill");

	reset_room();

}

void
init_room()
{
	if (functions_used & INIT_USED)
		this_object()->init_shop();
}

int
order(string str){
object food;
int i, dnum, value;

	if (!shop_open()) return 0;

	if(!valid_to_serve(this_player())){
		notify_fail(capitalize(keeper_name)+
" snarls: Like I'd serve your mangy carcass. Be gone you fool.\n");
		return 0;
	}

	if (!str) {
		notify_fail(capitalize(query_verb()) + " what?\n");
		return 0;
	}

	str = lower_case(str);

	dnum = -1;

	for (i = 0; i < sizeof(foods); i += DDATA_BLOCK)
	  if (member_array(str, foods[i]) >= 0) {
		dnum = i;
		break;
	  }

	if (dnum < 0) return notify_fail(
"They don't seem to have \"" + str + "\" for sale here.\n"), 0;

	if(remaining[dnum/DDATA_BLOCK] <= 0) return notify_fail(
"The shop is out of that item.\n"), 0;

	value = modify_cost((int)foods[dnum + 4]);

	if (this_player()->query_money() < value) {
		this_player()->tell_me("That costs " + value + " gold coins, which you don't have.");
		return 1;
	}

	food = clone_object(query_food_obj_file_name());

	if (!food) {
		this_player()->tell_me("Error in creating food!");
		return 1;
	}

	food->set_name(lower_case(foods[dnum + 1]));
	food->set_id(foods[dnum]);
	food->set_short(foods[dnum + 1]);
	food->set_long(foods[dnum + 5]);
	food->set_strength(foods[dnum + 2]);
	food->set_weight(foods[dnum + 3]);
	food->set_value(foods[dnum + 4]);
	food->set_eater_mess(foods[dnum + 6]);
	food->set_eating_mess(foods[dnum + 7]);

	remaining[dnum/DDATA_BLOCK]--;

	if (!this_player()->add_weight((int)food->query_weight())) {
	  this_player()->tell_me("Oops, the " + foods[dnum + 1] +
	   " slips through your fingers.");
	  move_object(food, environment(this_player()));
	} else move_object(food, this_player());

	this_player()->add_money(-value);
	this_player()->tell_me("You pay " + value + " for " + str + ".");
	tell_here((string)this_player()->query_name() + " orders " + str + ".", this_player());
	extra_purchase_msg();

	return 1;
}

int
fill(string str)
{
string tmp1, tmp2;
object bag, food, me;
int amount, dnum, i, value;

	if (!shop_open()) return 0;

	if (!str) return notify_fail("Fill what (and, perhaps, with what)?\n"), 0;

	str = lower_case(str);

	if (sscanf(str,"%s with %s",tmp1,tmp2) != 2) {
		tmp1 = str;
		tmp2 = query_default_food();
	}

	me = this_player();

	bag = present(tmp1, me);

	if (!bag || !bag->query_max_weight()) return notify_fail(
"Hmm...are you sure that " + tmp1 + " is a container?\n"), 0;

	if (living(bag)) return notify_fail(
"Please! Let " + tmp1 + " do that itself...\n"), 0;

	dnum = -1;

	for (i = 0; i < sizeof(foods); i += DDATA_BLOCK)
	  if (member_array(tmp2, foods[i]) >= 0) {
		dnum = i;
		break;
	  }

	if (dnum < 0) {
	  me->tell_me("They don't seem to have \"" + tmp2 + "\" for sale here.\n");
	  return 1;
	}
	
	if(remaining[dnum/DDATA_BLOCK]<=0){
		me->tell_me("Sorry, the shop is out of that item.");
		return 1;
	}

	if (((int)bag->query_max_weight() - (int)bag->query_inserted_weight())
	  < foods[dnum + 3]) {
		me->tell_me("I'm afraid that " + tmp1 + " is already full.");
		return 1;
	}

	amount = (((int)bag->query_max_weight() -
		(int)bag->query_inserted_weight()) / foods[dnum + 3]);

	if (amount < 1 || amount > 15) {
		me->tell_me("There's something really wrong with your container.\n\
You should tell this to some higher authority.");
		return 1;
	}

	if(amount > remaining[dnum/DDATA_BLOCK])
		amount = remaining[dnum/DDATA_BLOCK];

	me->tell_me("Your container can hold " + amount + " pieces of that.");

	value = modify_cost((int)foods[dnum + 4]);

	if ((int) me->query_money() < (value * amount)) {
		me->tell_me("Such an amount of that costs " + (value * amount) +
		   " gold coins, and you don't have that much cash.");
		return 1;
	}

	for (i = 0; i < amount; i++) {
		food = clone_object(query_food_obj_file_name());

		if (!food) {
			me->tell_me("Error in creating food!");
			return 1;
		}

		food->set_name(lower_case(foods[dnum + 1]));
		food->set_id(foods[dnum]);
		food->set_short(foods[dnum + 1]);
		food->set_long(foods[dnum + 5]);
		food->set_strength(foods[dnum + 2]);
		food->set_weight(foods[dnum + 3]);
		food->set_value(foods[dnum + 4]);
		food->set_eater_mess(foods[dnum + 6]);
		food->set_eating_mess(foods[dnum + 7]);
		remaining[dnum/DDATA_BLOCK]--;

		bag->add_weight((int)food->query_weight());
		move_object(food, bag);
	}

	me->add_money(- (value * amount));

	me->tell_me("You fill your " + tmp1 + " with " + tmp2 + ".\n" +
	  "That cost you " + (value * amount) + " coins.");

	tell_here((string)me->query_name() + " orders lots of food.", me);
	extra_purchase_msg();

	return 1;
}


int *query_remaining(){ return remaining; }


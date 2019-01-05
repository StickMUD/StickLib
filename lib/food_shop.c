/****************************************************************
*								*
* Module:							*
*	/lib/food_shop.c					*
* Description:							*
*	The standard food shop that should always be used when	*
*	building other shops that sell food (shops, bakeries).	*
*	Inherit it and configure to your taste. :-)		*
* Version:							*
*	1.0a (for StickLib), Aug-98.				*
* Last update:							*
*	25-Aug-98 by Doomdark					*
*								'
* Modifications:						*
*								*
****************************************************************/
/*
 * /lib/food_shop.c   - Standard inherited food shop or bakery
 *
 * NEW: Handles criminals. //Graah
 *
 * Additions 17-Jan-96 by Graah.
 * - Kludge: if "strength" is -1, weight defines file name to clone
 *   an item from, and long desc says what it is ("food", "drink",
 *   "tool" etc). Thus, shop can sell non-food things.
 */

#pragma strict_types

#include <nroom.h>

#include <criminal.h>
#include <city_name.h>
#include <food_defs.h>

mixed *foods;
object baker;	// Should _not_ be defined here, me thinks...

private static int functions_used;

#define	RESET_USED	1
#define	INIT_USED	2

#define DDATA_BLOCK	8

void
init_foods()
{
    /* ids, short, strength, weight, cost, long desc, eat msg, said msg */
    foods = ({
      ({"bread", "rye", "loaf"}), "Loaf of rye bread",
      10, 2, 50,
      "This is a standardized " CITY_NAME " rye bread.",
      "Munch munch munch...quite tasty!",
      "eats a loaf of rye bread.\n",
    });
}

/* Redefinable functions */
int
shop_open()
{
    object ob;

    if (!(ob = present("baker", this_object())))
	return notify_fail("The baker is not here.\n"), 0;

    if ((int)this_player()->query_crime_level() > CR_THEFT)
	return notify_fail((string)ob->query_name(0, this_player()) +
	  " tells you: We do not trade with criminals!\n"), 0;

    return 1;
}

string query_food_obj_object_name() { return FOOD_FILE; }
string query_default_food() { return "bread"; }
int check_open() { return shop_open(); }
void extra_purchase_msg() { }

/* Modify drink/food cost for non-members etc. */
int
modify_cost(int c)
{
    int cri;

    if (!(cri = (int)this_player()->query_crime_level()))
	return c;
    else
	return c * cri; /* Won't normally deal with crime > CR_THEFT */
}

/* Option 1: write the list, otherwise just return it. */
string
list_foods(int option)
{
    string l;
    int i;

    l = sprintf("%-50s %s\n","Item","Cost gc");
    l += "=================================================================\n";

    for (i = 0; i < sizeof(foods); i += DDATA_BLOCK)
    {
	l += sprintf("%-50s %6d", foods[i + 1],
	  modify_cost((int)foods[i + 4]));
	if (foods[i + 2] != -1 || !stringp(foods[i + 3]))
	    l += "  (food)";
	else
	    l += "  (" + foods[i + 5] + ")";

	l += "\n";
    }

    if (option != 1) return l;

    this_player()->tell_me(l);
    return 0;
}

void
reset_room()
{
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

    /* Let's add these commands after shops create_shop is called... */

    add_command("order","order");
    add_command("buy","order");
    add_command("fill","fill");
    add_command("list", "list_cmd");
    add_command("read", "list_cmd");
    add_command("exa", "list_cmd");
    add_command("examine", "list_cmd");
}

void
init_room()
{
    if (functions_used & INIT_USED)
	this_object()->init_shop();
}

status
list_cmd(string arg)
{
    if (query_verb() == "list") { list_foods(1); return 1; }
    if (!arg)
    {
	notify_fail(capitalize(query_verb()) + " what?\n");
	return 0;
    }
    if (lower_case(arg) != "list")
    {
	notify_fail("You can't see any \"" + arg + "\" here.\n");
	return 0;
    }

    list_foods(1);
    return 1;
}

int
order(string str)
{
    object food;
    int i, dnum, value;

    if (!check_open()) return 0;

    if (!str) {
	notify_fail(capitalize(query_verb()) + " what?\n");
	return 0;
    }

    str = lower_case(str);

    dnum = -1;

    for (i = 0; i < sizeof(foods); i += DDATA_BLOCK)
	if (member(foods[i], str) >= 0) {
	    dnum = i;
	    break;
	}

    if (dnum < 0) return notify_fail(
	  "They don't seem to have \"" + str + "\" for sale here.\n"), 0;

    value = modify_cost((int)foods[dnum + 4]);

    if (this_player()->query_money() < value) {
	this_player()->tell_me("That costs " + value + " gold coins, which you don't have.");
	return 1;
    }

    if (foods[dnum + 2] != -1 || !stringp(foods[dnum + 3]))
	food = clone_object(query_food_obj_object_name());
    else
	food = clone_object(foods[dnum + 3]);

    if (!food) {
	this_player()->tell_me("This product is faulty! Report this to a (co-)admin.");
	return 1;
    }

    if (foods[dnum + 2] != -1)
    {
	food->set_name(lower_case(foods[dnum + 1]));
	food->set_id(foods[dnum]);
	food->set_short(foods[dnum + 1]);
	food->set_long(foods[dnum + 5]);
	food->set_strength(foods[dnum + 2]);
	food->set_weight(foods[dnum + 3]);
	food->set_value(foods[dnum + 4]);
	food->set_eater_mess(foods[dnum + 6]);
	food->set_eating_mess(foods[dnum + 7]);
    }

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
    int amount, dnum, i, value, wt;

    if (!check_open()) return 0;

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
	if (member(foods[i], tmp2) >= 0) {
	    dnum = i;
	    break;
	}

    if (dnum < 0) {
	me->tell_me("They don't seem to have \"" + tmp2 + "\" for sale here.\n");
	return 1;
    }

    if (foods[dnum + 2] == -1 && stringp(foods[dnum + 3]))
    {
	if (!find_object(foods[dnum + 3])) call_other(foods[dnum + 3], "create");
	if (!find_object(foods[dnum + 3]))
	{
	    me->tell_me("Sorry, but there is something wrong with that \
product. Could you report this to a (co-)admin?");
	    return 1;
	}
	wt = (int)find_object(foods[dnum + 3])->query_weight();
    }
    else
	wt = foods[dnum + 3];

    if (((int)bag->query_max_weight() - (int)bag->query_inserted_weight()) < wt)
    {
	me->tell_me("I'm afraid that " + tmp1 + " is already full.");
	return 1;
    }

    amount = (((int)bag->query_max_weight() -
	(int)bag->query_inserted_weight()) / wt);

    if (amount < 1 || amount > 15) {
	me->tell_me("There's something really wrong with your container.\n\
You should tell about this to some higher authority.");
	return 1;
    }

    me->tell_me("Your container can hold " + amount + " pieces of that.");

    value = modify_cost((int)foods[dnum + 4]);

    if ((int) me->query_money() < (value * amount)) {
	me->tell_me("Such an amount of that costs " + (value * amount) +
	  " gold coins, and you don't have that much cash.");
	return 1;
    }

    for (i = 0; i < amount; i++) {

	if (foods[dnum + 2] != -1 || !stringp(foods[dnum + 3]))
	    food = clone_object(query_food_obj_object_name());
	else
	    food = clone_object(foods[dnum + 3]);

	if (!food) {
	    me->tell_me("There is something wrong with this product! \
Could you report this to a (co-)admin please?");
	    return 1;
	}

	if (foods[dnum + 2] != -1 || !stringp(foods[dnum + 3]))
	{
	    food->set_name(lower_case(foods[dnum + 1]));
	    food->set_id(foods[dnum]);
	    food->set_short(foods[dnum + 1]);
	    food->set_long(foods[dnum + 5]);
	    food->set_strength(foods[dnum + 2]);
	    food->set_weight(foods[dnum + 3]);
	    food->set_value(foods[dnum + 4]);
	    food->set_eater_mess(foods[dnum + 6]);
	    food->set_eating_mess(foods[dnum + 7]);
	}

	bag->add_weight((int)food->query_weight());
	move_object(food, bag);
    }

    me->add_money(- (value * amount));

    me->tell_me("You fill your " + tmp1 + " with " + tmp2 + ".\n" +
      "That cost you " + (value * amount) + " coins.");

    tell_here((string)me->query_name() + " orders lots of stuff.", me);
    extra_purchase_msg();

    return 1;
}

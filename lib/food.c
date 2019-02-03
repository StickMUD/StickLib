/****************************************************************
*								*
*		Standard eatable object.			*
*								*
*		Version 1.01					*
*								*
*		Last modifications:				*
*		26-oct-94 by Doomdark.				*
* 07-Dec-94: Now using /basic/object/dimensions //Graah		*
* 18-Dec-94: Added call to create_food //Graah			*
* 01-APR-95: Fixed to consider bulk of eater, and also all      *
*	     for posioned food.					*
*								*
****************************************************************/

/****************************************************************
*								*
* This object is a standard food object and works much like	*
* /lib/drink.c.							*
*								*
* To use this you can do:					*
*								*
* #include <food.h>						*
*								*
*	or							*
*								*
*#include <food_defs.h>						*
*								*
* object ob;							*
* ob = clone_object(FOOD_FILE);					*
* ob->set_name("apple pie");					*
* ...								*
*								*
* For more documentation look at /doc/dev/food			*
*								*
* These functions are defined:					*
*								*
* set_name(string)	To set the name of the item.		*
* set_id(*string)	To set the synonyms of this food.	*
* set_short(string)	To set the short description.		*
* set_long(string)	To set the long description. NO LINEFEEDS!
* set_value(int)	To set the value of _one_bite_ from this food.
* set_weight(int)	To set the permanent weight of the item.
* (For technical reasons, bites from the food don't weigh anything)
* set_strength(int)	To set the food value of one bite.	*
* set_no_heal(int)	If set, this food is normal food which	*
*			doesn't heal at all.			*
* set_max_full(int)	Set the maximum amount of bites we have.*
* set_full(int)		Set amount of bites remaining.		*
* set_eater_mess(string) To set the message that is written to	*
*			the player when he eats the item.	*
* set_eating_mess(string) To set the message given to the	*
*			surrounding players when this object	*
*			is eaten.				*
* set_poisoned_food(int ) To set if food is poison(ed) or not.   *
*								*
****************************************************************/

inherit "/basic/id";
inherit "/basic/object/dimensions";
inherit "/basic/misc/fstring";

#include <conditions.h>
#include <daemons.h>
#include <disease_defs.h>
#include <living_defs.h>
#include <race.h>

string	eating_mess, eater_mess;
int	value, strength, is_poisoned;
int	no_heal, full, max_full;

private static int functions_used;

#define TP this_player()
#define	RESET_USED	1
#define	INIT_USED	2

string how_much_left();
int min_cost();

/****************************************************************
*								*
*			Query-functions.			*
*								*
****************************************************************/

int
query_value() 
{
    if (value) return (value * full);
    else return (min_cost() * full);
}
int query_strength() { return strength; }
string query_eating_mess() { return eating_mess; }
string query_eater_mess() { return eater_mess; }
status query_no_heal() { return no_heal; }
int query_full() { return full; }
int query_max_full() { return max_full; }
int query_poisoned_food() { return is_poisoned; }

is_food() { return 1; }	/* Yup! */

varargs string
query_long(string s, object who)
{
    string t;
    return ::query_long(s, who) + "\n" + how_much_left() + ".";
}

/****************************************************************
*								*
*			Set-functions.				*
*								*
****************************************************************/

void set_value(int v) { value = v; }
void set_strength(int s) { strength = s; }
void set_eating_mess(string em)
{
    eating_mess = "%<me.name>%"+em;
    eating_mess = make_format_string(eating_mess);
}
void set_eater_mess(string em)
{
    eater_mess = make_format_string(em);
}
void set_no_heal(status x) { no_heal = x; }
void set_full(int x) { full = x; }
void set_max_full(int x) { max_full = x; }
void set_poisoned_food(int x) { is_poisoned = x; }

/****************************************************************
*								*
*		Lfuns (functions called by gamedriver).		*
*								*
****************************************************************/

status get() { return 1; }

/* No need for add_actions; command "eat" from /bin/pub handles
 * eat-commands; parses the input and calls the function
 * "eat_cmd" in appropriate object.
 */

void
init()
{
    if (functions_used & INIT_USED) this_object() -> init_food();
}

void
reset(int arg)
{
    if (arg) {
	if (functions_used & RESET_USED) this_object() -> reset_food();
	return;
    }
    if (function_exists("reset_food")) functions_used |= RESET_USED;
    if (function_exists("init_food")) functions_used |= INIT_USED;

    max_full = full = 1;
    set_weight(1);
    no_heal = 0;
    eater_mess = "Yum yum yum.";

    set_name("food");

    this_object()->create_food();
}

/****************************************************************
*								*
*		And then all the remaining functions.		*
*								*
****************************************************************/

string
how_much_left()
{
    int d;
    string str;

    if (full) {
	if (max_full > 0) d = ((full * 100) / max_full); else d = 100;

	str = "There is ";

	if (max_full == 1 || d > 80) str += "plenty";
	else if (d > 60) str += "more than a half";
	else if (d > 40) str += "about half";
	else if (d > 20) str += "less than a half";
	else str += "only a small piece";

	str += " of it remaining";
    } else str = "There is none of it left";

    return str;
}

/* Note: No need to check the argument string as it is already
 * been parsed by /bin/pub/_eat.c.
 */

varargs status
eat_cmd(string str)
{
    object ob;

    if (full < 1) {
	this_player() -> tell_me(
	  "The food was spoiled and rotten. You throw it away.");
	if (environment()) environment()->add_weight(-(query_weight()));

	if (environment() != this_player()) {
	    foreach (object you : filter(all_inventory(environment()), (: $1->query(LIV_IS_PLAYER) :))) {
		if (you->query_env("gmcp")) {
		    TELOPT_D->send_char_items_remove(you, "room", this_object());
		}
	    }
	} else {
	    if (this_player()->query_env("gmcp")) {
		TELOPT_D->send_char_items_remove(this_player(), "inv", this_object());
	    }
	}

	destruct(this_object());
	return 1;
    }

    if ((int) TP->query_level() * 8 < strength) {
	this_player() -> tell_me(
	  "You realize, even before trying, that you'll never be able to eat all this.");
	return 1;
    }

    if (!TP->eat_food(strength)) return 1;

    /* Food is food, not healing. Healing is reduced to one tenth of
     * the original, if there is any. //Graah
     * Not quite right...now is based on bulk of eater. Make it 
     * worthwhile for some races.-Kelgath
     */

    if (!no_heal) TP->heal_self(strength / ((int) TP->query_race_stat(RACE_EAT_VALUE)));

    TP->tell_me(eater_mess, 0,
      0, TP, this_object(), this_object());
    if (TP->query_invis())
	environment(TP)-> tell_here(
	  ({"You hear someone munching something delicious.",
	    "You hear: *munch* *rousk* *burb*\n"}) [random(2)], TP);
    else if (eating_mess) environment(TP) -> tell_here(
	  eating_mess, 0, 0, TP,
	  this_player(), this_object(), this_object());
    else environment(TP) -> tell_here(
	  (string) TP->query_name(0, TP) + " eats " + query_name(0, TP) + ".",TP);

    if(is_poisoned){
	TP->tell_me("ACK! It you shouldn't have eaten this. \
You feel very sick, as if you've been poisoned.");
	environment(TP)->tell_here(TP->query_name()+
	  " looks very ill all of the sudden.",TP);
	TP->set_condition(C_POISONED,(random(60)+1));
    }

    this_object()->extra_eat();

    if (--full < 1) {
	TP -> tell_me( ({
	    "You gormandized it all.",
	    "That was the last bit of it.",
	    "You devoured the whole thing.",
	    "You fat glutton, you ate all of it!"})[random(4)]);
	if (environment()) environment()->add_weight(-(query_weight()));
	if (environment() != this_player()) {
	    foreach (object you : filter(all_inventory(environment()), (: $1->query(LIV_IS_PLAYER) :))) {
		if (you->query_env("gmcp")) {
		    TELOPT_D->send_char_items_remove(you, "room", this_object());
		}
	    }
	} else {
	    if (this_player()->query_env("gmcp")) {
		TELOPT_D->send_char_items_remove(this_player(), "inv", this_object());
	    }
	}
	destruct(this_object());
	return 1;
    }

    /* Rye bread may VERY RARELY have enough of that fungi to cause
     * ergotism; the 'Fire of Holy Antonius'.
     */
    if (id("rye") && id("bread") && (random(100) < 2)) {
	if (!present("disease", TP)) {
	    if (ob = clone_object(DISEASE_FILE)) {
		move_object(ob, TP);
		ob->set_disease("ergotism");
	    }
	}
    }


    TP -> tell_me(how_much_left() + ".");

    return 1;
}

/* Redefine to make more things happen. */
void extra_eat() { }

int
min_cost()
{
    return 4 * strength + (strength * strength) / 10 -
    (no_heal * 2 * strength);
}

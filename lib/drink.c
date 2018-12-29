/****************************************************************
*								*
*	Standard drinkable object.				*
*								*
*	Version 1.01 29-sep-94.					*
*								*
*	Last modifications 08-jul-96 by Doomdark		*
* 07-Dec-94: Now using /basic/object/dimensions //Graah		*
* 08-jul-96: And now uses combat queue to prevent using n+1	*
*	potions when in combat... -+ Doomdark +-		*
* 21-Jul-96: Now the bottles again break. //Graah		*
*								*
****************************************************************/

/****************************************************************
*								*
* This object is a standard drink object for _both_ alcoholic	*
* and soft drinks. Now it can also hold many gulps of the	*
* stuff, the containers can be unbreakable (when dropped),	*
* and the stuff doesn't always need to heal the players. We	*
* want to have water barrels and such which just quench the	*
* thirst (which is going to be really installed to the game).	*
* Also, now you can set the name of the liquid in the drink;	*
* the name of the object doesn't have to be simple "beer"	*
* anymore; instead:						*
*	set_name("bottle of beer";				*
*	set_liquid("beer");					*
*								*
* Example:							*
*	#include <drink_defs.h>					*
*								*
*	ob = clone_object(DRINK_FILE);				*
*	ob->set_name("bottle of apple juice");			*
*	ob->set_id( ({ "juice", "apple juice" }) );		*
*	ob->set_liquid("apple juice");				*
*	ob->set_short("A bottle apple juice");			*
*								*
*  	// makes the drink a soft drink instead of alcoholic	*
*        ob->set_soft(1);					*
*	ob->set_strength(6); // Heals 6 hit- & spellpoints	*
*								*
*	// If we want a _drink_, not a potion, we do		*
*	ob->set_no_heal(1);					*
*	// If this would have been a barrel or something,	*
*	// we would have done					*
*	ob->set_unbreakable(1);					*
*	// If the container was a barrel, perhaps		*
*	ob->set_container_value(200);				*
*	// If it can contain many drinks, for example 4		*
*	ob->set_max_full(4);					*
*	ob->set_full(4);					*
*	// If we don't want it to become "An empty something"	*
*	ob->keep_short(1);					*
*								*
****************************************************************/

#include <player_defs.h>

inherit "/basic/id";
inherit "/basic/object/dimensions";
inherit "/basic/misc/fstring";

string drinking_mess, drinker_mess, container;
string liquid_name;
int value, container_value, strength, full, max_full;
status soft_drink, lords_only, unbreakable, no_heal, keep_short;
string info, read_msg;

static int drinking;
static closure *_tmp_cl;

private static int _flags;

#define	F_RESET_USED	1
#define	F_INIT_USED	2
#define	F_CALL_DONE	4
#define	F_EXTRA_DRINK	8

int begin_drink(object plr);
int end_drink(object plr);

/****************************************************************
*								*
*			Query-functions.			*
*								*
****************************************************************/

string query_info() { return info; }
int
query_value()
{
	if (full > 0)
		return container_value + value * full;
	return container_value;
}

int query_strength() { return strength; }
status query_soft() { return soft_drink; }
status query_unbreakable() { return unbreakable; }
status query_no_heal() { return no_heal; }
status query_type() { return lords_only; }
string query_container() { return container; }
int query_container_value() { return container_value; }
int  query_full() { return full; }
int query_max_full() { return max_full; }
status query_keep_short() { return keep_short; }
string query_liquid() { return liquid_name; }

/* For checks */
status is_drink() { return 1; }

/****************************************************************
*								*
*			Set-functions.				*
*								*
****************************************************************/

void set_info(string str) { info = str; }
void set_read(string str) { read_msg = str; }
void set_strength(int s) { strength = s; }
/* set_value() is already reserved :(  //Elena */
/* yep, but it does the same thing :)  //Graah */
void set_cost(int c) { value = c; }
void set_container_value(int v) { container_value = v; }
void set_drinking_mess(string dm)
{
    drinking_mess = "%<me.name>%"+dm;
    drinking_mess = make_format_string(drinking_mess);
}
void set_drinker_mess(string dm)
{
    drinker_mess = make_format_string(dm);
}
void set_empty_container(string ec) { container = ec; }
void set_soft(status s) { soft_drink = s; }
void set_type(status t) { lords_only = t; }
void set_unbreakable(status x) { unbreakable = x; }
void set_no_heal(status x) { no_heal = x; }
void set_full(int x) { if (x <= max_full) full = x; else full = max_full; }
void set_max_full(int x) { if (x > 0 && x < 12) max_full = x; else max_full = 1; }
void set_keep_short(status x) { keep_short = x; }
void set_liquid(string s) { liquid_name = s; }

/*
 * Some pretty awful code because we want to be compatible with the old code.
 */

int
set_value(int arg)
{
string name, kludge;
int    heal; /* no longer used, directly from strength */

/* See? */

	if (intp(arg)) {
		value = arg;
		return arg;
	}

	if (sscanf(arg, "%s#%s#%s#%d#%d#%d", name, kludge, drinker_mess,
		heal, value, strength) != 6) return 0;

	if (!container) container = name;
	set_name(name);
	set_short(kludge);
	set_long(capitalize(kludge) + ".");

/* Somehow beer, for example, sold at 19 gp, but was bought at 18. */
	value = ((value * 2) / 3);

	return 1;
}

/****************************************************************
*								*
*			Lfuns.					*
*								*
****************************************************************/

status get() { return 1; }

status
drop(status silent)
{
	if (full < 1 && !unbreakable && !(_flags & F_CALL_DONE)) {
		call_out("my_destruct", 1);
		_flags |= F_CALL_DONE;
	}

        return silent;  // Added to fix bug
}

varargs status
id(string s, object who)
{
	return s == container || ::id(s);
}

void
init()
{
	if (_flags & F_INIT_USED)
		this_object() -> init_drink();
}

void
reset(int arg)
{
	if (arg) {
		if (_flags & F_RESET_USED)
			this_object() -> reset_drink();
	} else {
		set_weight(full = max_full = 1);
		unbreakable = no_heal = keep_short = 0;
		drinker_mess = "Gloink Glurk Glburp.";
		container = "bottle";
		container_value = 10;
		if (function_exists("reset_drink"))
			_flags |= F_RESET_USED;
		if (function_exists("init_drink"))
			_flags |= F_INIT_USED;
		if (function_exists("extra_drink"))
			_flags |= F_EXTRA_DRINK;
		this_object() -> create_drink();
		_tmp_cl = ({ #'begin_drink, #'end_drink });
	}
}

/****************************************************************
*								*
* Now, let's modify some functions from /basic/id.c		*
*								*
****************************************************************/

varargs string
query_long(string s, object who)
{
int d;
mixed txt;
string n;

	txt = ::query_long(s, who);

	if (liquid_name) n = liquid_name;
	else n = query_name();

	if (full > 0) {
		if (full >= max_full) txt += "\nIt is full of " + n;
		else {
			d = ((100 * full) / max_full);
			if (d > 80) txt += "\nIt is almost full of " + n;
			else if (d > 60) txt += "\nIt is more than a half filled with " + n;
			else if (d > 40) txt += "\nIt is about half full of " + n;
			else if (d > 20) txt += "\nIt is less than a half filled with " + n;
			else txt += "\nThere's only a small amount of " + n + " left";
		}
        	txt += ".";
	}
	else txt += "\nIt's empty.";
	return txt;
}

/****************************************************************
*								*
*	 	And then all the rest functions.		*
*								*
****************************************************************/

status
prevent_insert()
{
	return 0; /* .... */

	if (full) {
		if (this_player()) this_player() -> tell_me(
"You don't want to ruin the " + query_name(0, this_player()) + ".");
		return 1;
	}
}


/*
 * Called from drink()
 */
int
make_empty(string msg_me, string msg_other, int heal, object me)
{
	if (!me && !(me = this_player()))
		return 0;
	me->tell_me(msg_me, 0, 0, me, this_object(), this_object());
	if (environment(me))
		environment(me)->tell_here(msg_other, 0, 0, me, me,
			this_object(), this_object());
	if (!no_heal)
		me -> heal_self(heal);

        if (--full > 0) return 1;

	if (!keep_short) {
/* a small change to save some memory */
		if (container == "bottle") {
			set_name("empty bottle");
			set_short("empty bottle"); /* Got rid of 'an' /Graah */
		} else {
			set_name("empty " + container);
			set_short("empty " + container); /* likewise */
		}
	}

	return 1;
}

int
begin_drink(object plr)
{
	if (!full || !plr) return 0;
	plr->tell_me(sprintf("You raise the %s to your lips...", container));
	environment(plr)->tell_here(sprintf("%s raises %s to %s lips...",
		(string) plr->query_name(0, plr), container,
		(string) plr->Possessive()), plr);
	return 1;
}

int
end_drink(object plr)
{
string p_name, str;
int p_lev;
	if (!plr)
		return 0;
	// AARGH! Endless drink with !full   -Graah
	if (full < 1) {
		plr->tell_me(sprintf("... and notice that the %s is empty!",
			container));
		return 0;
	}

	p_name = (string)plr->query_name(0, plr);
	p_lev = (int) plr->query_level();
	if (soft_drink) {
	  if (p_lev * 8 < strength)
	    return make_empty(
"This is much too much for you to drink! You drool most of it on the ground.",
		sprintf("%s tries to drink %s but drools most of it on the\
 ground.", p_name, query_name(0, plr)), p_lev, plr);
	} else {
	  if (strength >= 12 && p_lev < 10)
	    return make_empty("You sputter liquid all over the room.",
		sprintf("%s tries to drink %s but coughs and sputters it all\
 over you.", p_name, query_name(0, plr)), 4, plr);
	  else if (strength >= 8 && p_lev < 5)
	    return make_empty("You throw it all up.",
		sprintf("%s tries to drink %s but throws up.",
p_name, query_name(0, plr)), 2, plr);
	}

	if (!call_other(plr, (soft_drink ? "drink_soft" : "drink_alco"), strength))
	{
		return 1;
	}

	if (plr->query_invis()) str = "You hear some slurping.";
	else if (drinking_mess) str = drinking_mess;
	else str = sprintf("%s drinks %s.", p_name, query_name(0, plr));

	make_empty(drinker_mess, str, (strength>1) ? (strength * 2 - 4) : 0, plr);

	if (_flags & F_EXTRA_DRINK)
		this_object()->extra_drink();

	return 1;
}

varargs status
drink_cmd(string str, object me)
{
	if (!me && !(me = this_player())) return 0;

	if (!full) {
		me -> tell_me(sprintf(
"You try to sip from empty %s.", container));
		return 1;
	}

	if (lords_only && (int)me->query_level() < 20
	&& !this_player()->query_coder_level()) {
		me -> tell_me("This drink is for Lords only, you cannot drink it.");
		return 1;
	}

	if ((int) this_player()->set(PLR_COMBAT_QUEUE, _tmp_cl) == 1)
		begin_drink(me);
	return 1;
}

/* Don't bother to add_action, nor check the syntax; these have
 * already been taken care of by /bin/pub/_read.c.
 */
varargs status
read_cmd(string s, object me)
{
	if (!me && !(me = this_player())) return 0;
	if (!read_msg) return notify_fail("You can't see any writing on the "
		+ query_name(0, me)+"."), 0;
	me -> tell_me(read_msg);
	return 1;
}

int
min_cost() { return 4 * strength + (strength * strength) / 10; }

/*
 * There is too much money currently. Let's break the bottles. --Val
 *
 * They aren't always bottles, and may be unbreakable now //Graah
 */
void
my_destruct(status silent)
{
object ob;
int    i;
	_flags &= (~F_CALL_DONE);
	ob = this_object();
	if (!environment() || environment(environment())) return;
	while (ob = environment(ob))
		if (interactive(ob))
			return;
	if (!silent && (i = random(240)) > 120) {
		call_out("my_destruct", i, 1);
		return;
	}

	if (!silent)
		environment() -> tell_here(sprintf(
"The %s breaks as it is dropped down.", container));
	destruct(this_object());
}

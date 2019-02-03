/****************************************************************
*								*
*	Standard bag-file.					*
*								*
*	Version 1.00 28-sep-94.					*
*								*
*	Modifications:						*
*								*
*	By Graah 07-Dec-94: Added size.				*
*	By Doomdark 28-dec-94: Now overrides create of		*
*		/obj/container (and also reset). Reason:	*
*		more efficient (as /obj/container doesn't set	*
*		any values at create or reset).			*
*								*
****************************************************************/

#include <container_defs.h>
#include <daemons.h>
#include <living_defs.h>

inherit CONTAINER_FILE;

int usage;

//static int used_functions;

#define RESET_USED	1


void set_usage(int x) { usage = x; }

void
create()
{
    string tmp;

    tmp = ({ "leather", "cloth", "old", "dirty", "frayed", "mouldy",
      "damp" })[random(7)];

    set_max_weight(6);
    set_size(({ 50, 75, 75 }));
    set_weight(1);
    set_name(tmp + " bag");
    set_long("It's just a normal "+tmp+" bag.");
    set_id(({ "bag" }));
    set_value(12);
    set_usage(0);
    if (function_exists("reset_bag"))
	used_functions |= RESET_USED;
    this_object() -> create_bag();
}

void
reset()
{
    if (used_functions & RESET_USED)
	this_object()->reset_bag();
}


int query_usage() { return usage; }

/* Bags will eventually worn out and spill contents //Graah
 */
varargs status
can_put_and_get(string s)
{
    object *baginv, env, own;
    int i;

    if (random(20) > 3) return 1;

    if (++usage < (value * 8)) return 1;

    if (!(env = environment())) return 1;

    if (living(env)) {
	own = env;
	if (!(env = environment(own))) return 1;
    }

    if (!own) own = this_player();

    if (own)
	own->tell_me("Oops. "+capitalize(query_short())+" is so old that it is destroyed.");

    if ((baginv = all_inventory(this_object())) && (i = sizeof(baginv))) {
	for (i--; i >= 0; i--) move_object(baginv[i], env);
	if (own) {
	    own->tell_me("The contents are scattered into the room!");
	    env -> tell_here("The contents of "+(string)
	      own->query_name(0, own)+" worn-out " + query_name(0,own) +
	      " are scattered around the place.", own);
	}
    }

    if (living(environment())) environment() -> add_weight(
	  - (query_weight()));

    // If it is in our inventory and we are using a client that understands GMCP let's perform an update.
    if (own && own->query(LIV_IS_PLAYER) && own->query_env("gmcp")) {
        TELOPT_D->send_char_items_remove(own, "inv", this_object());
    }

    // And since items may have been scattered all over the room let's update clients interested in that change.
    foreach (object you : filter(all_inventory(environment()), (: $1->query(LIV_IS_PLAYER) :))) {
        if (you->query_env("gmcp")) {
            TELOPT_D->send_char_items_list(you, "room");
        }
    }

    destruct(this_object());
    return 0;
}

/* Wand tricks are not fun! */
status
refuse_item(object ob)
{
    if (!ob && !(ob = previous_object())) return 0;
    if (ob->id("wand")) {
	this_player() -> tell_me(capitalize((string)ob->query_name(0,this_player()))
	  + " is too long to fit in the "+query_name(0, this_player())+"!");
	return 1;
    }

    return 0;
}

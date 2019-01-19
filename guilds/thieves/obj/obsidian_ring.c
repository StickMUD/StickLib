/*
 * the Obsidian Ring
 *
 * This must be manufactured with a very difficult process. It will
 * never be found from any monster (if the owner doesn't throw it
 * away...) thus it is not really unique. DO NOT INSTALL UNIQUENESS
 * INTO THIS!
 *
 * Also this is now a bonded item, can't be worn by other than
 * the owner.
 *
 */
// Actually _has_ to be sort of semi-unique or something,
// to prevent it from being able to be put in locker.

#include <armour.h>
#include <conditions.h>
#include "../dirs.h"

/*
static object shadow;
static object owner;

object query_shadow() { return shadow; }
*/

// So, easiest way to prevent this from being put in locker:
int query_unique() { return -1; }

/* This prevents other guilds having the powerful thieven demon artifact
   ++skarda */

void init()
{
    object *o;
    int i;

    ::init();

    if (!this_player() || this_player()->query_dead() ||
      !environment() ||
      !environment(this_player())) return;

    // For now, it just slips from their greedy fingers.
    if (environment()==this_player() &&
      this_player()->query_guild()!="thief")
    {
	environment(this_player())->tell_here(
	  ":%<me.capname>% slips from the greedy fingers of %<him.capname>%.",
	  0, 0, ({ this_player() }), this_object(), this_player(), 0);
	this_player()->tell_me(
	  ":%<me.capname>% slips from your fingers.",
	  0, 0, this_object(), 0, 0);
	move_object(this_object(), environment(this_player()));
	return;
    }

#if 0
    if (environment()==this_player() && this_player()->query_guild()!="thief")
    {
	environment(this_player())->tell_here("The obsidian ring EXPLODES as "+
	  this_player()->query_name()+" takes it!",
	  ({this_player()}));
	this_player()->tell_me(
	  "The obsidian ring EXPLODES as you take it! WOAH!!");

	o = all_inventory(environment(this_player()));
	for (i = sizeof(o) - 1; i >= 0; i--)
	{
	    if (living(o[i])) {
		if (o[i] == this_player())
		{
		    o[i]->tell_me("That really HURT!");
		    o[i]->hit_player(random(100)+100);
		}
		else o[i]->hit_player(random(20)+10);
	    }
	}

	destruct(this_object());
	return;
    }
#endif

    // And this prevents from it being given for PK cheat.
    add_action("give_cmd", "give");
}


status
give_cmd(string str)
{
    if (id(str))
    {
	this_player()->tell_me("Why would you GIVE your ring away!?!");
	return 1;
    }

    return 0;
}


void
create_armour()
{
    set_name("obsidian ring");
    set_short("Obsidian Ring");
    set_long(
      "This is a smooth ring made of jet black obsidian. You can feel the \
ring pulsating with some ancient evil magic, perhaps of diabolical \
origin.");
    set_ac(3); // Gives a super high ac, instead.. - No, let's have some mercy
    set_weight(1);
    set_type("ring");
    set_id(({ "obsidian" }));
    set_info("It's the Obsidian Ring, a powerful thieven artifact.");
    set_value(4666);

    add_special_property(ARM_SP_RESIST_CONDITION, C_STUNNED, 1);
    // It could have this, but let's keep it rare for now... /Graah
    // add_special_property(ARM_SP_RESIST_CONDITION, C_UNCONSCIOUS, 1);
}

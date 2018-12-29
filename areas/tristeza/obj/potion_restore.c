/* A randomly defined potion of Restoration */

#include <drink.h>
#include <stats.h>

#define TP this_player()

static int rtype;

void
create_drink()
{
    rtype = random(5);

    /* Total Restoration is very rare. */
    if (rtype == 4 && random(100) > 30) rtype = random(4);

    set_name((({"brown", "blue", "green", "red", "brilliant white"})[rtype]) +
	" potion");
	set_liquid(({ "brown", "blue", "green", "red", "brilliant white" })
		[rtype] + " liquid");
    set_id(explode(query_name(), " ") + ({ "restoration" }));
    set_short(capitalize(query_name()));
    set_long(
        "This " + query_name() + " has a label saying: \" Ye Olde "+
	({ "Ambrosia", "Lotion", "Mixture", "Beverage", "Potion",
	   "Juice"})[random(6)]+
	" of " + ({ "Strength", "Dexterity", "Constitution", "Intelligence",
		    "Total"})[rtype] + " Restoration\".");

    set_info("It is just the stuff that the label says.\n");
    set_empty_container("bottle");
    set_drinker_mess("Glug-glug-glug...");

    if (rtype < 4)
       set_value(1200 + (8 - rtype) * (10 + random(10)));
    else
       set_value(2000);

    set_strength(1);
    set_soft(1);
}

void
extra_drink()
{
int a, b, z;

    z = 0;

    if (rtype == 0 || rtype == 4) {
	if ((a = (int)TP->query_stat(ST_BASE_STR)) > (b = (int)
		TP->query_stat(ST_STR | ST_DRAINED))) {
		TP->tell_me("You feel your strength returning!");
	    z++;
		TP->add_stat(ST_STR, a - b);
	}
    }

    if (rtype == 1 || rtype == 4) {
	if ((a = (int)TP->query_stat(ST_BASE_DEX)) > (b = (int)
	TP->query_stat(ST_DEX | ST_DRAINED))) {
		TP->tell_me("You feel your quickness and speed returning!");
	    z++;
		TP->add_stat(ST_DEX, a - b);
        }
    }

    if (rtype == 2 || rtype == 4) {
	if ((a = (int)TP->query_stat(ST_BASE_CON)) > (b = (int)
	 TP->query_stat(ST_CON | ST_DRAINED))) {
		TP->tell_me("You feel your health returning!");
	    z++;
		TP->add_stat(ST_CON, a - b);
        }
    }

    if (rtype == 3 || rtype == 4) {
	if ((a = (int)TP->query_stat(ST_BASE_INT)) > (b = (int)
	 TP->query_stat(ST_INT | ST_DRAINED))) {
		this_player()->tell_me("You feel your intelligence returning!");
	    z++;
		TP->add_stat(ST_INT, a- b);
        }
    }

	if (!z) this_player()->tell_me("Nothing unusual happens.");
}

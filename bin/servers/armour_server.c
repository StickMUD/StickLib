/* /lib/armour_server.c */

/*
 * Server which "sells" armour parts.
 * It could use Mud Economy if neccessary.
 *
 * Notes:
 *
 * 82 different pieces so far.
 *
 * Tower/Wall shield: can be done if really needed.
 *
 * Players must not simply find ac 5 stuff lying on the floor!
 * Read the armour documents!
 *
 */

#include <armour_defs.h>



mixed *armours;
static int array_size;

#define ARM_NAME		0
/* Tool slot of the armour */
#define ARM_TOOLSLOT 		1
/* Armour class */
#define ARM_AC			2
/* Armour weight */
#define ARM_WEIGHT		3
/* Armour value */
#define ARM_VALUE		4
/* Magic factor of armour (not used yet) */
#define ARM_MAGIC		5
/* Race(s) for armour */
#define ARM_RACE		6

/* Number of elements in one armour */
#define CELL_SIZE 7

public void
create()
{
    armours = ({

      /* Standard armour parts */

      "leather jacket",         ARMOUR_TYPE_BODY_ARMOUR,       2,4,200,0,0,
      "leather hood",           ARMOUR_TYPE_HELMET,            1,2,750,0,0,
      "leather sleeves",        ARMOUR_TYPE_BRACERS,           1,2,500,0,0,
      "leather gloves",         ARMOUR_TYPE_GLOVES,            1,2,500,0,0,
      "leather leggings",       ARMOUR_TYPE_LEGGINGS,          1,2,500,0,0,

      "padded jacket",          ARMOUR_TYPE_BODY_ARMOUR,       2,4,300,0,0,
      "padded cap",             ARMOUR_TYPE_HELMET,            1,2,750,0,0,
      "padded sleeves",         ARMOUR_TYPE_BRACERS,           1,2,500,0,0,
      "padded gloves",          ARMOUR_TYPE_GLOVES,            1,2,500,0,0,
      "padded leggings",        ARMOUR_TYPE_LEGGINGS,	       1,2,500,0,0,

      "studded jacket",         ARMOUR_TYPE_BODY_ARMOUR,       2,4,350,0,0,
      "studded cap",            ARMOUR_TYPE_HELMET,            1,2,800,0,0,
      "studded sleeves",        ARMOUR_TYPE_BRACERS,           1,2,550,0,0,
      "studded gloves",         ARMOUR_TYPE_GLOVES,            1,2,550,0,0,
      "studded leggings",       ARMOUR_TYPE_LEGGINGS,          1,2,550,0,0,

      "ring mail jacket",       ARMOUR_TYPE_BODY_ARMOUR,       3,6,500,0,0,
      "ring mail coif",         ARMOUR_TYPE_HELMET,            1,2,800,0,0,
      "ring mail sleeves",      ARMOUR_TYPE_BRACERS,           1,2,550,0,0,
      "ring mail gauntlets",    ARMOUR_TYPE_GLOVES,            1,2,550,0,0,
      "ring mail leggings",     ARMOUR_TYPE_LEGGINGS,          1,2,550,0,0,

      "splint mail jacket",     ARMOUR_TYPE_BODY_ARMOUR,       3,6,550,0,0,
      "splint mail coif",       ARMOUR_TYPE_HELMET,            1,2,570,0,0,
      "splint mail bracers",    ARMOUR_TYPE_BRACERS,           1,2,570,0,0,
      "splint mail gauntlets",  ARMOUR_TYPE_GLOVES,            1,2,570,0,0,
      "splint mail leggings",   ARMOUR_TYPE_LEGGINGS,          1,2,570,0,0,

      "chain mail jerkin",      ARMOUR_TYPE_BODY_ARMOUR,       3,6,600,0,0,
      "chain mail coif",        ARMOUR_TYPE_HELMET,            1,2,600,0,0,
      "chain mail sleeves",     ARMOUR_TYPE_BRACERS,           1,2,600,0,0,
      "chain mail gauntlets",   ARMOUR_TYPE_GLOVES,            1,2,600,0,0,
      "chain mail leggings",    ARMOUR_TYPE_LEGGINGS,          1,2,600,0,0,

      "scale mail jacket",      ARMOUR_TYPE_BODY_ARMOUR,       3,6,660,0,0,
      "scale mail coif",        ARMOUR_TYPE_HELMET,            1,2,620,0,0,
      "scale mail bracers",     ARMOUR_TYPE_BRACERS,           1,2,620,0,0,
      "scale mail gauntlets",   ARMOUR_TYPE_GLOVES,            1,2,620,0,0,
      "scale mail leggings",    ARMOUR_TYPE_LEGGINGS,          1,2,620,0,0,

      "banded breastplate",     ARMOUR_TYPE_BODY_ARMOUR,       4,9,940,0,0,
      "banded helmet",          ARMOUR_TYPE_HELMET,            1,2,800,0,0,
      "banded bracers",         ARMOUR_TYPE_BRACERS,           1,2,800,0,0,
      "banded gauntlets",       ARMOUR_TYPE_GLOVES,            1,2,800,0,0,
      "banded leggings",        ARMOUR_TYPE_LEGGINGS,          2,5,4500,0,0,

      "plated breastplate",     ARMOUR_TYPE_BODY_ARMOUR,       4,9,1500,0,0,
      "plated helmet",          ARMOUR_TYPE_HELMET,            2,5,2200,0,0,
      "plate mail bracers",     ARMOUR_TYPE_BRACERS,           1,3,1200,0,0,
      "plate mail gauntlets",   ARMOUR_TYPE_GLOVES,            1,3,900,0,0,
      "plate mail leggings",    ARMOUR_TYPE_LEGGINGS,          2,5,4750,0,0,

      "mithril breastplate",    ARMOUR_TYPE_BODY_ARMOUR,       5,10,7500,0,0,
      "mithril helmet",         ARMOUR_TYPE_HELMET,            2,4,6000,0,0,
      "mithril bracers",        ARMOUR_TYPE_BRACERS,           1,2,4500,0,0,
      "mithril gauntlets",      ARMOUR_TYPE_GLOVES,            1,2,2500,0,0,
      "mithril leggings",       ARMOUR_TYPE_LEGGINGS,          2,4,6500,0,0,

      "wooden buckler",         ARMOUR_TYPE_SHIELD,            1,2,500,0,0,
      "small wooden shield",    ARMOUR_TYPE_SHIELD,            1,2,550,0,0,
      "medium wooden shield",   ARMOUR_TYPE_SHIELD,            1,2,600,0,0,
      "large wooden shield",    ARMOUR_TYPE_SHIELD,            1,3,550,0,0,

      "leather buckler",        ARMOUR_TYPE_SHIELD,            1,2,500,0,0,
      "small leather shield",   ARMOUR_TYPE_SHIELD,            1,2,520,0,0,
      "medium leather shield",  ARMOUR_TYPE_SHIELD,            1,2,550,0,0,
      "large leather shield",   ARMOUR_TYPE_SHIELD,            1,3,500,0,0,

      "iron buckler",           ARMOUR_TYPE_SHIELD,            1,2,600,0,0,
      "small iron shield",      ARMOUR_TYPE_SHIELD,            1,2,700,0,0,
      "medium iron shield",     ARMOUR_TYPE_SHIELD,            1,3,800,0,0,
      "large iron shield",      ARMOUR_TYPE_SHIELD,            1,4,900,0,0,

      "steel buckler",          ARMOUR_TYPE_SHIELD,            1,2,1000,0,0,
      "small steel shield",     ARMOUR_TYPE_SHIELD,            1,2,1200,0,0,
      "medium steel shield",    ARMOUR_TYPE_SHIELD,            1,3,1600,0,0,
      "large steel shield",     ARMOUR_TYPE_SHIELD,            2,5,3000,0,0,

      "mithril buckler",        ARMOUR_TYPE_SHIELD,            1,2,2200,1,0,
      "small mithril shield",   ARMOUR_TYPE_SHIELD,            1,2,3400,1,0,
      "medium mithril shield",  ARMOUR_TYPE_SHIELD,            2,4,5500,1,0,
      "large mithril shield",   ARMOUR_TYPE_SHIELD,            2,4,7500,1,0,

      "chain mail jacket",      ARMOUR_TYPE_BODY_ARMOUR,       3,6,600,0,0,
      "mithril chain shirt",    ARMOUR_TYPE_BODY_ARMOUR,       4,8,6000,1,0,
      "visored helmet",         ARMOUR_TYPE_HELMET,            2,5,4500,0,0,
      "iron pot helmet",        ARMOUR_TYPE_HELMET,            1,2,800,0,0,

      /* Special/magical armour parts */

      "elven chain shirt",      ARMOUR_TYPE_BODY_ARMOUR,       5,10,9000,1,"elf",
      "elven chain coif",       ARMOUR_TYPE_HELMET,            2,4,8000,1,
      "elven chain sleeves",    ARMOUR_TYPE_BRACERS,           1,2,8500,1,
      "elven chain leggings",   ARMOUR_TYPE_LEGGINGS,          2,4,8500,1,

      "crystal breastplate",    ARMOUR_TYPE_BODY_ARMOUR,       5,12,7000,1,0,
      "crystal helmet",         ARMOUR_TYPE_HELMET,            2,6,6500,1,0,
      "crystal bracers",        ARMOUR_TYPE_BRACERS,           1,4,6800,1,0,
      "crystal leggings",       ARMOUR_TYPE_LEGGINGS,          2,6,6800,1,0,

      /* Special things */
      "chastity belt",          ARMOUR_TYPE_BELT,              1,1,2000,1,0,
    });

    array_size = sizeof(armours);
}

/*
 * Function name:  make_armour
 * Description  :  generate a standard armour part
 * Arguments    :  n = armour's full name
 * Returns      :  /lib/armour object,	or 0 if failed
 */
public object
make_armour(string n)
{
    object ar;
    int x, x2, size;
    string *id, *id2, s;

    if (!n || n == "") return 0;
    n = lower_case(n);

    x = member(armours, n);

    if (x < 0) {
	write("Tried to create armour \"" + n + "\" which doesn't exist.\n");
	return 0;
    }

    ar = clone_object(ARMOUR_FILE);
    ar->set_short(n);
    ar->set_name(n);
    ar->set_long("It's a quite normal " + n + ".");
    ar->set_type(armours[x + ARM_TOOLSLOT]);
    ar->set_ac(armours[x + ARM_AC]);
    ar->set_value((armours[x + ARM_VALUE] * 2) / 3);
    ar->set_weight(armours[x + ARM_WEIGHT]);
    if (armours[x + ARM_RACE]) ar->set_race(armours[x + ARM_RACE]);

    // Let's handle this as it should be. If we had "black long sword"
    // why should ids be "black", "long" and "sword". Duh. Instead,
    // they'll be "long sword" and "sword" (in both cases, name will
    // be "black long sword"). -+ Doomdark 07-oct-95 +-

    id = explode(n, " ");
    x = size = sizeof(id) - 1;
    x2 = 2;
    id2 = allocate(x * 2);
    id2[0] = s = id[size];
    id2[1] = id[0] + " " + id[size];

    while (x-- > 1) {
	s = id[x] + " " + s;
	id2[x2++] = s;
	id2[x2++] = id[x] + " " + id[size];
    }

    ar->set_id(id2);

    return ar;
}

/*
 * Function name: make_random_armour
 * Description  : generate any random armour part
 * Arguments    : optional max_ac, maximum ac generated (at least 1)
 *		  by default we make armours of ac 1 to 3
 * Returns      : /obj/armour object, 0 if failed
 * Notes        : Do not generate too good armours in wrong places.
 *		  This routine may return 0 and that _must_ be checked!
 */ 
public varargs object
make_random_armour(int max_ac)
{
    int x,tries;

    if (max_ac < 1) max_ac = 3;

    tries = 50;

    do {
	x = random(array_size / CELL_SIZE) * CELL_SIZE;
	tries--;
    } while(armours[x + ARM_AC] > max_ac && tries > 0);

    if (tries < 1) return 0;

    return make_armour(armours[x + ARM_NAME]);
}

/*
 * Function name: list_armours
 * Description  : List the possible armours of the server. Coders only!
 * Arguments    : none
 * Returns      : 0 or 1
 * Notes        : Can be used as a command
 */
public int
list_armours()
{
    int i;

    if (!this_player()->query_coder_level()) {
	notify_fail("You are not allowed to do this!\n");
	return 0;
    }

    write("The armours cannot be listed yet - read the source code.\n");
    return 1;
}

/*
 * Bodily functions, commands to practice LPsex
 *
 * Argh, has it to be inherited into player object?
 *
 */

#define TP this_player()

static mapping cmd_list;
/* Should be conditions :-)? */
static int wanking, dirty_ass;

void
create()
{
    cmd_list = ([
      /* Pissing */
      "piss": "piss",
      "pee": "piss",
      "urinate": "piss",
      /* Shitting */
      "shit": "shit",
      "defecate": "shit",
      "crap": "shit",
      /* Wipe ass (Balfour's idea) */
      "wipe": "wipe",
      /* Sex, addition to things in soul */
      "make": "makelove",
      "love": "makelove",
      "fuck": "makelove",
      "suck": "suck",
      "lick": "lick",
      /* Jerking off */
      "jerk": "jerk",
      "wank": "masturbate",
      "masturbate": "masturbate"
    ]);
}

/* "You got to take off your rubber pants and mask before doing that."
 * Flag 0: naughty bits got to be exposed
 * Flag 1: mouth...
 */
string *
what_clothes_to_strip(object ob, int flag)
{
    object *inv;
    int i, s;
    string *n, tmp;

    if (!ob) return 0;
    inv = all_inventory(ob);
    if (!inv || (s = sizeof(inv)) < 2) return 0;
    n = ({ });

    for (i = 0; i < s; i++) {
	if (inv[i]->query_worn()) {
	    tmp = (string)inv[i]->query_type();
	    if ((flag == 1 && (tmp == "helmet" || tmp == "mask")) ||
	      (flag == 0 && (tmp == "armour")))
		n += ({ (string)inv[i]->query_short() });
	}
    }

    return n;
}

int
piss(string arg)
{
    string *x;

    if ((x = what_clothes_to_strip(TP, 0)) &&
      sizeof(x) > 0) {
	notify_fail(sprintf("%-=78s\n", "You have to take off your " +
	    implode(x, ", ", " and ") + " before doing this."));
	return 0;
    }

    write("You " + query_verb() + ".\n");
    say("Yuk! " + (string)TP->query_name() + " " + query_verb() + "s!\n");

    return 1;
}

int
shit(string arg)
{
    string *x, isee, osee;

    if ((x = what_clothes_to_strip(TP, 0)) &&
      sizeof(x) > 0) {
	notify_fail(sprintf("%-=78s\n", "You have to take off your " +
	    implode(x, ", ", " and ") + " before doing this."));
	return 0;
    }

    if (query_verb() == "defecate") {
	isee = "bend and empty your bowels.";
	osee = "bends and empties " + (string)TP->query_possessive() +
	" bowels. Smells awful!";
    } else {
	isee = "bend and pass some turds on the ground.";
	osee = "bends and passes some turds on the ground. Yuk!";
    }

    write(sprintf("%-=78s\n", "You " + isee));
    say(sprintf("%-=78s\n", TP->query_name() + " " + osee));

    return 1;
}

int
masturbate(string arg)
{
    mixed x;
    if (!arg ||
      member(({ "off", "myself", "self", "me" }), 
	lower_case(arg)) != -1) {
	if ((x = what_clothes_to_strip(TP, 0)) &&
	  sizeof(x) > 0) {
	    notify_fail(sprintf("%-=78s\n", "You have to take off your " +
		implode(x, ", ", " and ") + " before doing this."));
	    return 0;
	}

	if (wanking) {
	    notify_fail("But you are already masturbating!\n");
	    return 0;
	}
    }
}

int
makelove(string arg)
{
    string tmp1, tmp2;

    if (!arg) {
	notify_fail(capitalize(query_verb()) + " what? How?\n");
	return 0;
    }

    if (query_verb() == "make") {
	if (sscanf(arg, "love to %s", tmp1) != 1 &&
	  sscanf(arg, "love with %s", tmp1) != 1) {
	    notify_fail("Make what?\n");
	    return 0;
	}

	notify_fail("How pervert.\n");
	return 0;
    }

    if (query_verb() == "fuck" && lower_case(arg) == "off")
	masturbate(0);

    notify_fail("How pervert.\n");
    return 0;
}

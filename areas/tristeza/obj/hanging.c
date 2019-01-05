#include <conditions.h>
#include "../DEFS.h"

#define tme(x) owner->tell_me((x))
#define	tenv(x)	environment(owner)->tell_here(x, owner)
#define tboth(x) tme(x); tenv(x)
#define NAME ((string)owner->query_name())
#define HE ((string)owner->query_pronoun())

int proceed;
static object owner;
static string *bad_cmds;

reset(arg)
{
    if (arg) return;

    bad_cmds = ({
	"n", "s", "e", "w", "ne", "nw", "se", "sw",
	"north", "south", "east", "west",
	"northeast", "southeast", "northwest", "southwest",
	"invis", "wor"
    });

    call_other(GALLOWS, "???");
}

int drop() { return 1; }

string
query_auto_load()
{
string s, tmp;
int i;
	tmp = file_name(this_object());
	if (sscanf(tmp, "%s#%d", s, i) == 2) tmp = s;
	return s + ":" + proceed;
}

status
check_owner()
{
    owner = environment();
    if ((!owner) ||
	(!living(owner))) return 0;

    return 1;
}

void
continue_hanging()
{
    if (owner) {
        if (proceed >= 4) move_object(owner, GALLOWS);
	move_object(this_object(), owner);
    }
    call_out("hang_me", 1);
}

void
begin_hanging(object who)
{
    if (!who || !living(who)) return;
    owner = who;
    move_object(this_object(), owner);
    proceed = 0;

    continue_hanging();
}

void
init_arg(string arg)
{
    proceed = sscanf(arg, "%d");
    owner = previous_object();
    continue_hanging();
}

void
init()
{
    add_action("filter", "", 1);

    if (!owner) owner = this_player();
}

/* Check, check and check. */
status
filter(string arg)
{
    string v;

    v = query_verb();

    if (v == "quit") {
	tme("Ok, if you insist...you won't get away from hanging!\n");
	tenv(NAME + " quits! Hanging will continue when " + HE +
	    " enters the game again.\n");
	return 0;
    }

    if (member(bad_cmds, v) != -1) {
	tme("You can't do that now!\n");
	return 1;
    }

    return 0;
}

void
hang_me()
{
    object ob;

    if (!check_owner()) return;

    /* Went LD, wait for return... */
    if (!interactive(owner)) {
        call_out("hang_me", 10);
	return;
    }

    /* Free action stuff needs to be removed somewhere... */
    owner->set_condition(C_STUNNED, 10);
	owner->add_sp(-100000);

    /* Invisible hanging is just too strange */
    if (owner->query_invis()) owner->vis();

    /* One can't fight while hanging up there */
    if ((ob = (object)owner->query_attack())) {
	owner->stop_fight();
	ob->stop_fight();
    }

    proceed++;

    switch(proceed) {
	case 1:
		tme(
 "You are stunned by terror. They are going to hang you!\n");
		tenv(NAME + " is stunned by terror.\n");
		break;
	case 3:
		tme("Guards start to carry you away.\n");
		tenv("Guards start to carry " + NAME + " away.\n");
		break;
	case 7:
		move_object(owner, GALLOWS);
		tme(
 "You arrive at the city gallows. You feel cold sweat on your forehead.\n");
		tenv(NAME + " arrives, dragged by the guards.\n");
		break;
	case 9:
		tme("Guards lead you up the ladder.\n");
		tenv("Guards lead " + NAME + " up the ladder.\n");
		break;
	case 11:
		tme("Your hands are tied behind your back.\n");
		tenv("Guards tie " + NAME + "'s hands.\n");
		break;
	case 13:
		tboth("Executioner asks: Any last wish or last words?\n");
		break;
	case 30:
		tboth("Executioner says: It is time.\n");
		break;
	case 32:
		tme("The Executioner puts your head into the noose.\n");
		tenv("The Executioner puts " + NAME +
			"'s head into the noose.\n");
		break;
	case 34:
		tboth("The Executioner catches the lever.\n");
		break;
	case 36:
		tboth("The Executioner pulls the lever!\n");
		break;
	case 37:
		tme("You drop...\n");
		tenv(NAME + " drops...\n");
		break;
	case 38:
		tme("HNGKKKSKKKK!!! Your neck didn't broke!!!\n");
		tenv(NAME + " begins to struggle, twitch and jerk!\n");
		break;
	case 40:
		tme("AGGGHHH! You feel like your head is being ripped off!\n");
		tenv(NAME + "'s limbs spasm into impossible positions!\n");
		break;
	case 42:
		tme("GGGHH! You try to puke, but it just fills your lungs!\n");
		tenv("Blood and pus spurt out of " + NAME +
			"'s nostrils!\n");
		break;
	case 44:
		tme("You empty your bladder and bowels!\n");
		tenv(NAME + "'s pants are filled with excrement!\n");
		break;
	case 46:
		tme("You suffocate!\n");
		tenv(NAME + "'s face turns black!\n");
		break;
	case 48..999:
		tme("\n\nEverything goes dark.\n\n");
		tenv(NAME + " dies.\n");
		owner->hit_player(100000);
		destruct(this_object());
		return;
		break;
	default:
		break;
    }

    call_out("hang_me", 1);
}	

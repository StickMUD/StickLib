// Got rid of unneccessary log file. // Graah

#define TOSS_VALUE	19	/* What's the max. value of 'tossable' items. */
#define	TOSS_RECEIVE	5	/* How much player gets for tossing. */

inherit "/basic/id";

void
reset(int x)
{
    if (x) return;
    set_name("trashcan");
    set_id(({"can","the trashcan","green trashcan"}));
    set_short("A trashcan");
    set_long(
      "It is a green trashcan. You can toss unneccessary items and trashes into it.\
 It is better to drop them in trashcan than on the ground! To waste something\
 into the can it you should type:\t toss <object>\n\
Only things with a value below "+(TOSS_VALUE+1)+" can be dropped in it.\
 For keeping the Mud clean, you will receive "+TOSS_RECEIVE+" gold coins\
 for each item you toss.");
}

void
init()
{
    add_action("toss","toss");
}

status
toss(string str)
{
    object item;
    string a,b;
    if (!str) return notify_fail("Toss what?\n"), 0;
    if (sscanf(str,"%scoin%s", a, b) == 2) {
	this_player() -> tell_me("Throw away money?");
	return 1;
    }
    if (!(item = present(str, this_player())))
    {
	this_player() -> tell_me("You don't have that.");
	return 1;
    }

    if((int) item -> query_value() >= TOSS_VALUE) {
	this_player() -> tell_me(
	  "That is worth "+(TOSS_VALUE + 1)+" coins or more. You'd better sell it in a shop.");
	return 1;
    }
    // Hmmm... Being living-object doesn't mean anything nowadays... *shrug*
    if(living(item)) {
	this_player() -> tell_me
	("But...but... you don't have the heart to toss that. It's alive!");
	return 1;
    }

    if(item->id("baby")) {
	this_player()->tell_me(
	  "You toss the whining brat in the trash. How cruel of you. "
	);
	destruct(item);
	this_player()->set_luck(-5+(-random(3)));
	return 1;
    }
    if(item -> drop() || !item->short()) {
	this_player() -> tell_me("You can't toss that!\n");
	return 1;
    }

    // Nice money cheat: getting lord level forms or others and tossing...
    move_object(item,"/room/void");
    if (!item->id("form"))
	this_player() -> add_money(TOSS_RECEIVE);
    destruct(item);
    this_player() -> tell_me("You tossed it.");
    environment(this_player()) -> tell_here(
      (string) this_player()->query_name(0, this_player())
      + " tossed something in the trashcan.", this_player());
    return 1;
}

status
get()
{
    notify_fail("You can't "+query_verb()+" the trashcan, it's way too heavy!\n");
    return 0;
}

#include <cmd.h>

nomask int
heal_cmd(string name)
{
    object ob;

    if (!CHECK) return 0;
    if (!name) return 0;

    ob = find_living(lower_case(name));

    if (!ob) {
        notify_fail("No such living being found.\n");
	return 0;
    }

    /* Silly...but heal as a coder command was silly all the time. */
    if (!ob->query_coder_level() && !ob->query_testplayer()) {
        notify_fail("Only coders and testplayers can be healed!\n");
	return 0;
    }

    call_other(ob, "heal_self", 100000);
    write("Ok.\n");
    tell_object(ob, "You are healed by " +
		capitalize((string)TP->query_real_name()) + ".\n");

#if 0
    /* Why wasn't this here before? 11/7/91 Frobozz */
    /* It isn't required any more. //Graah */
    if (!IS_CODER_OB(ob))
        log_file("HEAL", (string)ob->query_real_name() +
		 "(" + (int)ob->query_level() + ")" + " healed by " +
		 (string)TP->query_real_name() + "\n");
#endif

    return 1;
}

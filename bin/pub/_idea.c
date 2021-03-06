#include <cmd.h>
#include <config.h>

nomask varargs status
idea_cmd(string str)
{
    string who, room, myname;
    if(!str) return notify_fail("Give an argument.\n"), 0;
    room = object_name(environment(TP));
    myname = (string) TP -> query_real_name();
    log_file("IDEA", "\n" + myname + " (" + room + "):\n" + str + "\n");
    if (sscanf(room, PATH_USER_FNAME "%s/", who)) log_file(
	  who+".rep", "\n"+room + " Idea: " + myname + "\n" + str + "\n");
    TP -> tell_me("Ok.");
    return 1;
}

#include "../priest.h"

string *commands;


create() {
    string s, *temp;
    int i;

    temp = get_dir(PRIEST_DIR "bin/priest/");
    commands = ({});
    i = sizeof(temp);
    while(--i>=0)
	if(sscanf(temp[i], "_%s.c", s)) {
	    commands += ({ s });
	}
}


status
priest_cmd(object who, object sym, string str) {
    string cmd, args;

    if(!str) return
	notify_fail("It's easier for both of us if you specify a command.\n"),0;
    if(sscanf(str, "%s %s", cmd, args) != 2) cmd = str;
    if(member(commands, cmd) == -1) return
	notify_fail("No such command.\n"),0;
    return (status)call_other(PRIEST_DIR + "bin/priest/_" + cmd,
      cmd + "_cmd", who, sym, args);
}


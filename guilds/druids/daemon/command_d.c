// GUILD HEADERS **********************************************************
#include "../global.h"

string *cmd_list;

int is_cmd(string str) {
    return (str[0] == '_');
}

string strip_c(string str) {
    string tmp;

    if (sscanf(str, "_%s.c", tmp) != 1) {
	return "";
    }

    return implode(explode(tmp, "_"), " ");
}

void create() {
    cmd_list = filter(get_dir(GUILD_COMMAND_DIR), #'is_cmd);
    cmd_list = map(cmd_list, #'strip_c);
}

string *get_cmd_list() {
    return cmd_list;
}

int do_cmd(object me, object mark, string cmd, string args) {
    if (member(cmd_list, cmd) < 0) {
	return notify_fail("No such command.\n"), 0;
    }

    mark->inform_debugger(sprintf("Calling: %s->do_cmd(%O, %O, %s)",
	GUILD_COMMAND_DIR + "_" + cmd, me, mark, args));

    return call_other(GUILD_COMMAND_DIR + "_" + cmd,
	"do_cmd", me, mark, args);
}

// EOF *********************************************************************

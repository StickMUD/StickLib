/* Moved to /bin/pub 25-sep-94. */

#include <cmd.h>

#include "/bin/pub/_look.c"
/* Efficiency over elegancy (and memory)... */

varargs nomask status
examine_cmd(string s, object me)
{
    if (!me && !(me = this_player())) return 0;

    if (!s) return notify_fail("Examine what?\n"), 0;
    return look_cmd("at "+s, me);
}

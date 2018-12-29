/* (Starbyt's code) */
#include <cmd.h>
#include <mud_name.h>

nomask int
sysinfo_cmd()
{
    if (!CHECK) return 0;

    write(MUD_NAME+" at " + query_host_name() + "\n");
#ifdef COMPAT_FLAG
    write("LP " + version() + " COMPAT.\n");
#else
    write("LP " + version() + ".\n")
#endif
    write(ctime(time()) + "\n");
    write(query_load_average() + "\n");
    write(sizeof(users()) + " interactives.\n");
    write(sizeof(call_out_info()) + " callouts pending.\n");
    write((int)nature()->query_sizeof_req() + " timerequests pending.\n");

    return 1;
}

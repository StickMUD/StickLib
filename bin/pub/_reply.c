/*
 * _reply.c -- replies the person who told you by Alcal 12/95
 *
 */

#include <std_lib>

int
reply_cmd(string str)
{
/* Simple kludge to allow mailbox to work ok: */
    if(to_int(str)) return 0;
    return "/bin/pub/_tell.c"->do_reply(str);
}

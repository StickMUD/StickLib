/*
 * _retell.c -- retells the tell you told to someone else by Alcal 12/95
 *
 */

#include <std_lib>

int
retell_cmd(string str)
{
    return "/bin/pub/_tell.c"->do_retell(str);
}

/*
 * _away.c Sets, displays or clears your away message
 * Coded by Sumppen July-1996
 */

#include <std_lib>

int
away_cmd(string str)
{
    return "/bin/pub/_tell.c"->away(str);
}

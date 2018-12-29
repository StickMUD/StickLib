// City Daemon - various functions to make things happen
// in the City. //Graah

// V2.0 19-feb-95 by Doomdark.

#pragma strict_types

#define NROOM_NO_INHERIT
#include <nroom.h>

// Function:	shout_city
// Arguments:	msg - message to be shouted (only to players)
// Description:	shout a message to every player on the streets


public void
shout_city(string msg)
{
object *xob, ob;
int i;
string tmp;

    i = sizeof(xob = users());

    while (i--)
	if ((ob = environment(xob[i])) && ob->query(IN_CITY))
	  xob[i]->tell_me(msg);
}

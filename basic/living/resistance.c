/****************************************************************
*								*
* Module: resistances.c						*
* Description: Contains code handling resistances for different *
*	       damagetypes                                	*
*                                                               *
* Version: 1.0							*
* Date: 16-jul-96 Sumppen					*
*								*
* Changes:							*
*								*
****************************************************************/

static mapping Resistances;	// Mapping holding info about resistances:

#ifndef LIVING_C

#include "/basic/living/living_private.h"
#include <attack.h>

#endif

/* 
      set_resistance(res,value)

      res - the damage-type to resist
      value - resistance in 1/10th of a % 
*/

int
set_resistance(int res, int value)
{
    if(!res || !value)
	return 0;
    if(!Resistances) Resistances = ([]);
    if(value > MAX_RES) value = MAX_RES;
    if(res == DT_PHYSICAL)
    {
	Resistances[DT_CUT] = Resistances[DT_IMPALE] = Resistances[DT_CRUSH]=
	Resistances[DT_IMPACT] = Resistances[DT_BITE] = Resistances[DT_CLAW]=
	Resistances[DT_STAB] = Resistances[DT_PHYSICAL] = value;
    }
    else
	Resistances[res] = value;
    return 1;
}

mixed
query_resistance(int res)
{
    if(!Resistances) return 0;;
    return Resistances[res];
}

varargs int
add_resistance(int res, int delta)
{
    int value;

    if(!Resistances) Resistances = ([]);
    value = Resistances[res] + delta;
    if(value > MAX_RES) value = MAX_RES;

    if(res == DT_PHYSICAL)
    {
	Resistances[DT_CUT] = Resistances[DT_IMPALE] = Resistances[DT_CRUSH]=
	Resistances[DT_IMPACT] = Resistances[DT_BITE] = Resistances[DT_CLAW]=
	Resistances[DT_STAB] = Resistances[DT_PHYSICAL] = value;
    }
    else
	Resistances[res] = value;
    return 1;
}

mapping
query_resistances()
{
    return Resistances;
}

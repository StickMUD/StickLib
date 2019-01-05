/*
** /complex/_timerequester.c (c) Starbyt
**
** To be inheritted by /complex/nature
**
**
**
** The aim of this file is to give a simple way to launch time-based actions
** for different monsters / rooms.
**
**
** EXAMPLE OF USE:
**

void room_reset()
{
   set_short("my room");
   midnight();
}

void midnight()
{
    nature()->add_timerequest(0, 0, "midnight");
    tell_room(this_object(), "The clock booms twelve times for midnight!\n");
}

**
** So as you see, it works quite a lot like call_outs. Shouldn't be any
** troubles using, but the normal warning: DO NOT USE THIS FOR YOUR EVERY
** MONSTER AND ROOM.
**
**
** !I HATE ANSI-C-INDENTING!
**
*/
 
#include "gametime.h"
 
private static object *r_objects;
private static string *r_functions;
private static int *r_hours;
private static int *r_minutes;
 
int convert_hours_minutes(int h, int m);
int query_hour();
int query_minute();
 
/*
 * Function: add_timerequest(int hour, int minute, string func, object ob);
 * Description: Initialize a time-based function call request.
 * Arguments: hour   - hour when to call (game time)
 *            minute - minute when to call (game time)
 *            func   - function to call when launched
 *            ob     - in which object to call. if nothing given,
 *                     this_object() is default.
 * Returns: true(1) for success; false(0) if not possible.
 *
 */
 
varargs
status add_timerequest(int thour, int tminute, string tfunction, object ob)
{
    if(!objectp(ob)) if(!(ob = previous_object())) return 0;
    if(!stringp(tfunction)) return 0;
    if(!intp(thour) || !intp(tminute)) return 0;
    if(!function_exists(tfunction, ob)) return 0;
 
    if(thour < 0 || thour >= HOURS) return 0;
    if(tminute < 0 || tminute >= MINUTES) return 0;
 
    if(!pointerp(r_objects))
    {
        r_objects = ({ });
        r_functions = ({ });
        r_hours = ({ });
        r_minutes = ({ });
    }
 
    r_objects += ({ ob });
    r_functions += ({ tfunction });
    r_hours += ({ thour });
    r_minutes += ({ tminute });
 
    return 1;
}
 
 
int calculate_real_time(int h, int m)
{
     m += MINUTES * h;
     h = query_minute() + MINUTES * query_hour();
     m -= h;
     if(m < 0) m += MINUTES * HOURS;
     h = m / MINUTES;
     m = m % MINUTES;
     return convert_hours_minutes(h, m);
}
 
/*
 * Function: query_timerequest(string func, object ob);
 * Description: return amount of seconds (real time) before that request
 *              will be called.
 * Arguments: func - function to the call is aimed to.
 *            ob   - object where the call is aimed to. if none given,
 *                   this_object() is default.
 * Returns: -1 if no such request pending, otherwise the amount of seconds
 *          before that call to take place.
 *
 */
 
varargs
int query_timerequest(string tfunction, object ob)
{
    int x;
    if(!objectp(ob)) if(!(ob = previous_object())) return -1;
    if(!pointerp(r_functions)) return -1;
    if((x = member(r_functions, tfunction)) == -1) return -1;
    if(r_objects[x] != ob) return -1;
    return calculate_real_time(r_hours[x], r_minutes[x]);
}
 
/*
 * Function: clear_timerequest(string func, object ob);
 * Description: terminate possible timerequest to wanted function.
 *
 * Arguments: func - function to the call is aimed to.
 *            ob   - object where the call is aimed to. if none given,
 *                   this_object() is default.
 * Returns: -1 if no such request pending, otherwise the amount of seconds
 *          before that call would have been.
 *
 */
 
static
int delete_by_index(int x)
{
    if(!pointerp(r_objects)) return 0;
    if(x > sizeof(r_objects)) return 0;
 
    r_objects   = r_objects[0..x-1] + r_objects[x+1..sizeof(r_objects)-1];
    r_functions = r_functions[0..x-1] + r_functions[x+1..sizeof(r_functions)-1];
    r_hours     = r_hours[0..x-1] + r_hours[x+1..sizeof(r_hours)-1];
    r_minutes   = r_minutes[0..x-1] + r_minutes[x+1..sizeof(r_minutes)-1];
    return 1;
}
 
varargs
int clear_timerequest(string tfunction, object ob)
{
    int x, y;
    if(!objectp(ob)) if(!(ob = previous_object())) return -1;
    if(!pointerp(r_functions)) return -1;
    if((x = member(r_functions, tfunction)) == -1) return -1;
    if(r_objects[x] != ob) return -1;
 
    y = calculate_real_time(r_hours[x], r_minutes[x]);
 
    if(!delete_by_index(x)) return -1;
 
    return y;
}
 
/*
** This is called from nature each new minute
**
*/
 
private static int oldreqh, oldreqm;
 
void timerequest_clocktick(int h, int m)
{
    object *tr_objects;
    string *tr_functions;
 
    int x, found;
 
    if(oldreqh == h && oldreqm == m) return;
 
    oldreqh = h; oldreqm = m;
 
    if(!pointerp(r_functions)) return;
 
    tr_objects = ({ });
    tr_functions = ({ });
 
    for(x = 0; x < sizeof(r_objects); x++)
        if(r_hours[x] == h && r_minutes[x] == m)
        {
            tr_objects += ({ r_objects[x] });
            tr_functions += ({ r_functions[x] });
        }
 
    for(x = 0; x < sizeof(r_objects); x++)
        if(r_hours[x] == h && r_minutes[x] == m)
            delete_by_index(x--);
 
    for(x = 0; x < sizeof(tr_objects); x++)
        catch(call_other(tr_objects[x], tr_functions[x]));
 
}
 
/*
** Probably interested by Admins only
**
*/
 
#define SPACES "                                           "
 
void dump_timerequests()
{
    int x;
    if(!pointerp(r_objects) || !sizeof(r_objects))
    { 
         write("No requests pending.\n");
         return;
    }
    for(x = 0; x < sizeof(r_objects); x++)
        if(!objectp(r_objects[x]))
        {
             write("Object destructed for " + r_functions[x] + "\n");
             delete_by_index(x);
             if(x > 0) x--; /* won't work */
        }
        else
        write((object_name(r_objects[x]) + SPACES)[0..35] + " : " +
              (r_functions[x] + SPACES)[0..25] + " at " +
              r_hours[x] + ":" + r_minutes[x] + " (h:m)\n");
}
 
 
int query_sizeof_req() { if(pointerp(r_objects)) return sizeof(r_objects); }
 

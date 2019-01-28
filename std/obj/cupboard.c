#include <container.h>

#include	<door_defs.h>

/* A generic 'cupboard'-object, in which you can put items.
 * commands 'open', 'close', 'get <x> from cupboard' and
 * 'put <x> to cupboard' defined. It's also lockpickable;
 * thieves should be able to pick its lock.
 *
 * V3.0 / 06-oct-94 / Doomdark
 * ====
 *
 * - 2.0: /basic/id added so that standard id-funcs can be used.
 * - 2.1: If player has only one key, he/she can use 'lock cupboard'
 *	  without 'with x key' part.
 * - 2.2: Now you can also have other 'cupboard-like' objects;
 *	  just use 'set_cupboard_type'.
 * - 3.0: Updated it. Now uses 'tell_me' and 'tell_here'.
 *	  Also; removed get and put; standard commands are enough.
 *	  Now we inherit "/obj/container".
 */

void update_quantity();

//int weight;
status closed, locked;
string lock_type, lock_code;
string inside_long;
string cupboard_type;
int quantity, pick_level;
int quantity_now;
private static int functions_used;

#define	RESET_USED	1
#define	INIT_USED	2

void set_weight(int x) { weight = x; }
void set_type(string s) { lock_type = s; }
void set_code(string s) { lock_code = s; }
void set_pick_level(int x) { pick_level = x; }
void set_inside_long(string s)
{
    if(stringp(s)) inside_long = s;
}

void set_cupboard_type(string s) { set_name(cupboard_type = s); }

int query_value() { return 20; }	/* Shouldn't have any effect... */
int query_pick_level() { return pick_level; }
string query_cupboard_type() { return cupboard_type; }
status query_is_closed() { return closed; }
status query_is_locked() { return locked; }
status query_can_lock() { return 1; }

varargs string
query_short(int mode, object who)
{
    return ::query_short(mode, who) + " (" + (closed ? "closed":"open") + ")";
}

varargs string
query_long(string str, object who)
{
    object ob;
    string s, t, txt;
    s = query_name();
    if (closed) return ::query_long(str, who) + "\nThe "+s+" is closed.";
    if (inside_long) txt = inside_long;
    else txt = ::query_long(str, who);
    if (!(ob = first_inventory(this_object())))
	txt += "\nThe " + s + " seems to be empty.";
    else  {
	txt += "\nIn the " + s + " you see:";
	while(ob) {
	    if (t = (string) ob -> short()) txt += "\n" + t + ".";
	    ob = next_inventory(ob);
	}
	if (quantity_now < (quantity / 3))
	    txt += "\nThe " + s + " seems to be quite full.";
    }
    return txt;
}

status is_empty() { return (first_inventory(this_object()) ? 0 : 1); }

int
pick_door(status unpick)
{
    if(!unpick) {
	if(!locked) return -1;
	locked = 0;
	return 1;
    }
    if(locked) return -1;
    locked = 1;
    return 1;
}

void
create()
{
    locked = closed = 1;
    set_max_weight(20);
    set_weight(50);
    cupboard_type = "cupboard";
    pick_level = 10;	/* Not too hard to lockpick. */
    set_name("cupboard");
    set_short("A cupboard");
    set_long("It is a typical cupboard.");
    if (function_exists("reset_cupboard")) functions_used |= RESET_USED;
    if (function_exists("init_cupboard")) functions_used |= INIT_USED;
    this_object() -> create_cupboard();
}

void
init()
{
    if (functions_used & INIT_USED) this_object() -> init_cupboard();
}

void
reset()
{
    if (functions_used & RESET_USED) this_object() -> reset_cupboard();
}

status
open_cmd(string str, object me)
{
    string s, t;
    int tmp;
    if (!me && !(me = this_player())) return 0;
    s = query_verb();
    t = query_name(0, this_player());
    if (closed == (tmp = (s=="close" ? 1 : 0))) {
	me -> tell_me("But why? It's already " + (tmp ? "closed":"open") + "!");
	return 1;
    }
    if (locked) {
	me -> tell_me("You can't " + s + " the " + t + ", it's locked!");
	return 1;
    }
    me -> tell_me("You " + s + " the " + t +".");
    environment() -> tell_here((string)me->query_name(0, me)
      + " "+ s +"s the " + t + ".", me);
    closed = tmp;
    return 1;
}

status close_cmd(string s, object me) { return open_cmd(s, me); }

private int
count_keys()
{
    object ob;
    int x;
    for(ob = first_inventory(this_player()), x = 0; ob;
      ob = next_inventory(ob)) if(ob -> id("key")) x++;
    return x;
}

status
lock_cmd(string str, object me)
{
    object *keys;
    object key;
    string s, t, code, key2, u, v, x1, x2;
    if (!me && !(me = this_player())) return 0;
    s = query_verb();
    t = query_name();
    if (sscanf(str, "the %s", u)) str = u;
    while (sscanf(str, "%s the %s", u, v) == 2) str = u + " " + v;
    x1 = "lock of "+cupboard_type;
    x2 = "lock on "+cupboard_type;
    if(present(str, environment()) == this_object() || str == x1 || str == x2) {
	if(count_keys() == 1) key = present("key", me);
	else {
	    me -> tell_me(capitalize(s)+" the "+t+ " with what?");
	    return 1;
	}
    }
    if(!key) {
	if (sscanf(str, "%s with %s", code, key2) != 2) return 0;
	if (present(code, environment()) != this_object() ||
	  (str != x1 && str != x2)) return 0;
    }
    if(locked == (s=="lock" ? 1 : 0)) {
	me -> tell_me("But why? It's already " + s + "ed!");
	return 1;
    }
    if (!key && !(key = present(key2, me)) || !(key->id("key"))) {
	me -> tell_me("Unlock the lock of the " + t + " with what?");
	return 1;
    }
    key2 = (string) key -> query_name(0, me);
    code = (string) key -> query_code();
    if(lock_code && code != MASTER_KEY && code != lock_code) {
	me -> tell_me("The " + key2 + " doesn't fit!");
	return 1;
    }
    me -> tell_me("You " + s + " the "+t+" with the " + key2 +".");
    environment() -> tell_here((string)me->query_name(0, this_object())
      +" " + s +"s the " + t + " with the "+ key2 + ".", me);
    locked = (s == "lock" ? 1 : 0);
    return 1;
}

status unlock_cmd(string s, object me) { return lock_cmd(s, me); }

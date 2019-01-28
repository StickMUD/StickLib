/*
** -{----- Shadowchaser Mudlib -----}-
**
**    File: /lib/chest.c
**  Author: Tamarindo
**    Date: March 17, 1999
*/

#include <chest_defs.h>
#include <container_defs.h>

private status chest_closed;
private status chest_locked;
private status chest_lockable;

#define TRUE				1
#define FALSE				0

private int functions_used;

#define INIT_FUNCTION_USED		(1 << 0)
#define RESET_FUNCTION_USED 		(1 << 1)

private string chest_key;
private string chest_code;
private string chest_read;
private string chest_type;

private int pick_level;

void create_chest();
void init_chest();
void reset_chest();

inherit CONTAINER_FILE;

void create_container()
{
    set_name("chest");
    set_short("A chest");
    set_long("A chest.");
    set_size( ({75, 75, 75}) );
    set_max_weight(10);
    set_value(150);
    set_weight(6);

    chest_read = 0;
    chest_type = 0;
    chest_closed = FALSE;
    chest_locked = FALSE;
    chest_lockable = FALSE;

    this_object()->create_chest();

    functions_used = 0;

    if (function_exists("init_chest"))
	functions_used |= INIT_FUNCTION_USED;

    if (function_exists("reset_chest"))
	functions_used |= RESET_FUNCTION_USED;
}

void init_container()
{
    if (functions_used & INIT_FUNCTION_USED)
	this_object()->init_chest();
}

void reset_container()
{
    if (functions_used & RESET_FUNCTION_USED)
	this_object()->reset_chest();
}

varargs string query_short(mixed arg1, mixed arg2)
{
    return ::query_short() +
    (chest_closed > 0 ? " (closed)" : " (open)");
}

varargs string query_long(mixed arg1, mixed arg2)
{
    return ::query_long() + "\nIt is " + 
    (chest_closed > 0 ? "closed." : "open.");
}

void set_closed(status arg)
{
    chest_closed = arg;
}

status query_closed()
{
    return chest_closed;
}

void set_locked(status arg)
{
    chest_locked = arg;
}

status query_locked()
{
    return chest_locked;
}

status query_lockable()
{
    return chest_lockable;
}

varargs void set_key(string key, string code)
{
    chest_key = key;

    if (code)
	chest_code = code;

    chest_lockable = TRUE;
}

string query_key()
{
    return chest_key;
}

void set_code(string arg)
{
    chest_code = arg;
    chest_lockable = TRUE;
}

string query_code()
{
    return chest_code;
}

void set_pick_level(int arg)
{
    pick_level = arg;
}

int query_pick_level()
{
    return pick_level; // -1 = no pick, 0-100 %
}

void set_read_msg(string arg)
{
    chest_read = arg;
}

void set_type(string arg)
{
    chest_type = arg;
}

string query_type()
{
    return chest_type;
}

status can_put_and_get(string arg) 
{ 
    return !chest_closed;
}

status read_cmd(string str)
{
    this_player()->tell_me(chest_read);
    return 1;
}

status open_cmd(string arg)
{
    string name;

    name = this_object()->query_name();

    if (!chest_closed)
    {
	this_player()->tell_me("The " + name + " is already opened.");
	return 1;
    }

    if (chest_locked)
    {
	this_player()->tell_me("The " + name + " is locked.");
	return 1;
    }

    this_player()->tell_me("You open the " + name + ".");
    chest_closed = FALSE;
    return 1;
}

status close_cmd(string arg)
{
    string name;

    name = this_object()->query_name();

    if (chest_closed)
    {
	this_player()->tell_me("The " + name + " is already closed.");
	return 1;
    }

    this_player()->tell_me("You close the " + name + ".");
    chest_closed = TRUE;
    return 1;
}

status unlock_cmd(string arg)
{
    object key;
    string name;

    name = this_object()->query_name();

    if (!chest_lockable)
	return notify_fail("The " + name + " does not have a lock.\n"), 0;

    if (!chest_locked)
    {
	this_player()->tell_me("The " + name + " is already unlocked.");
	return 1;
    }

    if (!(key = present(chest_key, this_player())))
	return notify_fail("You don't have the key!\n"), 0;

    if ((string)key->query_code() != chest_code && 
      (string)key->query_code() != MASTER_KEY)
	return notify_fail("The key does not fit in this " + name + "!"), 0;

    this_player()->tell_me("Click!\nYou unlock the " + name + ".");
    chest_locked = FALSE;
    return 1;
}

status lock_cmd(string arg)
{
    object key;
    string name;

    name = this_object()->query_name();

    if (!chest_lockable)
	return notify_fail("The " + name + " does not have a lock.\n"), 0;

    if (chest_locked)
    {
	this_player()->tell_me("The " + name + " is already locked.");
	return 1;
    }

    if (!(key = present(chest_key, this_player())))
	return notify_fail("You don't have the key!\n"), 0;

    if ((string)key->query_code() != chest_code && 
      (string)key->query_code() != MASTER_KEY)
	return notify_fail("The key does not fit in this " + name + "!"), 0;

    if (!chest_closed)
	this_player()->tell_me("You close the " + name + ".");

    this_player()->tell_me("Clack!\nYou lock the " + name + ".");
    chest_locked = TRUE;
    return 1;
}

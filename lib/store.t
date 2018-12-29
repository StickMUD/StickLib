/*
** -{----- Shadowchaser Mudlib -----}-
**
**    File: /lib/store.c
**  Author: Tamarindo
**    Date: March 17, 1999
*/

#include <nroom.h>
#include <store_defs.h>
#include <chest_defs.h>

private int store_flags;

#define INIT_FUNCTION_USED	(1 << 0)
#define RESET_FUNCTION_USED	(1 << 1)

private int functions_used;

private mixed storage_room;

void create_store();
void init_store();
void reset_store();
void create_chests();
void set_store(int arg);
void set_storage(mixed arg);
int store_open();
int cmd_handler(string arg);
int read_cmd(string arg);

void create_room()
{
    set_short("A store");
    set_long("You are in a store.  There is a plaque on the wall.");

    add_item("plaque", "The plaque has some writing on it.");

    set_light_level(LT_LIGHTS);
    set_outdoors(WD_INDOORS);

    this_object()->create_store();

    if (stringp(storage_room))
	storage_room = do_find_object(storage_room);

    create_chests();

    storage_room->set_storage_flags(store_flags);

    functions_used = 0;

    if (function_exists("init_store"))
	functions_used |= INIT_FUNCTION_USED;

    if (function_exists("reset_store"))
	functions_used |= RESET_FUNCTION_USED;
}

void init_room()
{
    if (functions_used & INIT_FUNCTION_USED)
	this_object()->init_store();
}

void reset_room()
{
    if (functions_used & INIT_FUNCTION_USED)
	this_object()->reset_store();
}

void create_chests()
{
    object chest;

    add_command("list", "cmd_handler");

    if (store_flags ^ BUY_AND_SELL)
    {
        add_command("borrow", "cmd_handler");
	add_command("donate", "cmd_handler");
    }

    if (store_flags & BUY_AND_SELL)
    {
        add_command("buy", "cmd_handler");
	add_command("sell", "cmd_handler");
	add_command("value", "cmd_handler");
    }

    if (store_flags & (STORE_ARMOUR | STORE_WEAPON))
	add_command("compare", "cmd_handler");

    if (store_flags & STORE_ARMOUR)
    {
	chest = clone_object(ARMOUR_CHEST);
        if (store_flags & BUY_AND_SELL)
	{
	    chest->set_closed(CHEST_CLOSED);
	    chest->set_locked(CHEST_LOCKED);
	    chest->set_key("storage key", file_name(storage_room));
	}
	move_object(chest, storage_room);

	add_command("mend", "cmd_handler");
    }

    if (store_flags & STORE_WEAPON)
    {
	chest = clone_object(WEAPON_CHEST);
        if (store_flags & BUY_AND_SELL)
	{
	    chest->set_closed(CHEST_CLOSED);
	    chest->set_locked(CHEST_LOCKED);
	    chest->set_key("storage key", file_name(storage_room));
	}
	move_object(chest, storage_room);

	add_command("fix", "cmd_handler");
	add_command("sharpen", "cmd_handler");
    }

    if (store_flags & STORE_GENERAL)
    {
	chest = clone_object(GENERAL_CHEST);
        if (store_flags & BUY_AND_SELL)
	{
	    chest->set_closed(CHEST_CLOSED);
	    chest->set_locked(CHEST_LOCKED);
	    chest->set_key("storage key", file_name(storage_room));
	}
	move_object(chest, storage_room);
    }

    if (store_flags & STORE_SCROLL)
    {
	chest = clone_object(SCROLL_CHEST);
        if (store_flags & BUY_AND_SELL)
	{
	    chest->set_closed(CHEST_CLOSED);
	    chest->set_locked(CHEST_LOCKED);
	    chest->set_key("storage key", file_name(storage_room));
	}
	move_object(chest, storage_room);
    }

    if (store_flags & STORE_POTION)
    {
	chest = clone_object(POTION_CHEST);
        if (store_flags & BUY_AND_SELL)
	{
	    chest->set_closed(CHEST_CLOSED);
	    chest->set_locked(CHEST_LOCKED);
	    chest->set_key("storage key", file_name(storage_room));
	}
	move_object(chest, storage_room);
    }
}

void set_store(int arg)
{
    store_flags = arg;
}

void set_storage(mixed arg)
{
    storage_room = arg;
}

int store_open()
{
    return 1;
}

int cmd_handler(string arg)
{
    if (!(#'store_open))
	return notify_fail("The store does not seem to be open.\n"), 0;

    return (int)call_other(storage_room, query_verb() + "_cmd", arg);
}

int read_cmd(string arg)
{
    string *tmp;

    if (!arg || arg != "plaque")
        return 0;

    tmp = ({ });

    tmp += ({ "list" });

    if (store_flags ^ BUY_AND_SELL)
    {
	tmp += ({ "borrow" });
	tmp += ({ "donate" });
    }

    if (store_flags & BUY_AND_SELL)
    {
	tmp += ({ "buy" });
	tmp += ({ "sell" });
	tmp += ({ "value" });
    }

    if (store_flags & (STORE_ARMOUR | STORE_WEAPON))
	tmp += ({ "compare" });

    if (store_flags & STORE_ARMOUR)
	tmp += ({ "mend" });

    if (store_flags & STORE_WEAPON)
    {
	tmp += ({ "fix" });
	tmp += ({ "sharpen" });
    }

    return 1;
}

/*
** -{----- Shadowchaser Mudlib -----}-
**
**    File: /lib/storage.c
**  Author: Tamarindo
**    Date: March 17, 1999
*/

#include <nroom.h>
#include <storage_defs.h>
#include <chest_defs.h>

#define HEAD1 "\n ___________________________________________________________________________"
#define HEAD2 "/\\__________________________________________________________________________\\"
#define HEAD3 "\\                                                                          \\"
#define FOOT "\\/__________________________________________________________________________/"


private int storage_flags;

#define INIT_FUNCTION_USED	(1 << 0)
#define RESET_FUNCTION_USED	(1 << 1)

private int functions_used;

void create_storage();
void init_storage();
void reset_storage();
void set_storage_flags(int arg);
int query_storage_flags();
varargs string *create_list(object chest, string item);
status list_cmd(string arg);
status borrow_cmd(string arg);
status donate_cmd(string arg);
status buy_cmd(string arg);
status sell_cmd(string arg);
status value_cmd(string arg);
status compare_cmd(string arg);
status mend_cmd(string arg);
status fix_cmd(string arg);
status sharpen_cmd(string arg);

void create_room()
{
    set_short("Storage room");
    set_long("You are in a storage room with some chests.");

    set_light_level(LT_LIGHTS);
    set_outdoors(WD_INDOORS);

    this_object()->create_storage();

    functions_used = 0;

    if (function_exists("init_storage"))
	functions_used |= INIT_FUNCTION_USED;

    if (function_exists("reset_storage"))
	functions_used |= RESET_FUNCTION_USED;
}

void init_room()
{
    if (functions_used & INIT_FUNCTION_USED)
	this_object()->init_storage();
}

void reset_room()
{
    if (functions_used & INIT_FUNCTION_USED)
	this_object()->reset_storage();
}

void set_storage_flags(int arg)
{
    storage_flags = arg;
}

int query_storage_flags()
{
    return storage_flags;
}

varargs string *create_list(object chest, string item)
{
    int i;
    string *list, *graphic;

    if (item)
        list = chest->list_item(item, (storage_flags & BUY_AND_SELL));
    else
	list = chest->list_items(1);
/*
	list = chest->list_items(storage_flags & BUY_AND_SELL);
*/
    find_player("tamarindo")->tell_me("Gets here");

    if (!sizeof(list))
	return list;

    graphic = ({ HEAD1, HEAD2, HEAD3 });

    if (item)
	for (i = 0; i < sizeof(list); i += 3)
            graphic += ({ 
		sprintf(" | %3s -> %-48s %5s gold coins |", list[i],
			list[i + 1], list[i + 2])
	    });
    else
	for (i = 0; i < sizeof(list); i++)
            graphic += ({ 
		sprintf(" | %3d -> %-64s |", (i + 1), list[i])
	    });

    graphic += ({ sprintf(" |%73s|\n_\\ %-72s \\", " ", " ") });
    graphic += ({ FOOT });

    return graphic;
}

status list_cmd(string arg)
{
    mixed *inv, *chests;
    int index;
    string *list, line, item;

    inv = all_inventory();

    chests = list = ({ });

    for (index = 0; index < sizeof(inv); index++)
	if (inv[index]->id("chest"))
	{
	    chests += ({ inv[index] });
	    list += ({ (string)inv[index]->query_type() });
	}

    line = implode(list, ", ", " or ");

    if (!arg)
	return notify_fail("List what? (" + line + ")?\n"), 0;

    if (arg[<1] == 's')
	arg = lower_case(arg[0..<2]);

    if (sscanf(arg, "%s %s", arg, item) < 2)
    {
	if ((index = member(list, arg)) < 0)
	    return notify_fail("That type of item is not sold here.\n"), 0;

	if (!sizeof(list = create_list(chests[index])))
	    return notify_fail("Storage is empty!\n"), 0;
	else
	    this_player()->more(list);
    }
    else
    {
	if ((index = member(list, arg)) < 0)
	    return notify_fail("That type of item is not sold here.\n"), 0;

	if (!sizeof(list = create_list(chests[index], item)))
	    return notify_fail("That item is not in storage.\n"), 0;
	else
	    this_player()->more(list);
    }

    return 1;
}

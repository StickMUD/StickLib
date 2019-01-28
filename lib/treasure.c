/********************************************************************
*                                                                   *
*           Generic base object; not only for treasures             *
*           but also any other object that hasn't specific          *
*           standard object to be used as a base (ie. not           *
*           an armour, a weapon, a bag or a drink...)               *
*                                                                   *
* Name: treasure.c                                                  *
* Version: 1.1                                                      *
* Last changed: 17-jan-96 by Doomdark                               *
								    *
********************************************************************/

inherit "/basic/id";
inherit "/basic/object/dimensions";

int    value;
string read_msg, info;

int tr_Flags;
static closure InitFunc;

#define	F_RESET_USED	1

/************************************
*                                   *
*       Query-functions             *
*                                   *
************************************/

int query_value() { return value; }
string query_info() { return info; }

/************************************
*                                   *
*       Set-functions               *
*                                   *
************************************/

void set_value(int v) { value = v; }
void set_read(mixed str) { read_msg = str; }
void set_info(string i) { info = i; }

/************************************
*                                   *
* L-funs; functions gamedriver calls*
*                                   *
************************************/

varargs status
get(status silent) { return 1; }

void create_treasure() { }

// Ugh. Would love to use create(), but...
void
create()
{
    if (!query_weight()) set_weight(1);
    if (function_exists("reset_treasure")) tr_Flags |= F_RESET_USED;
    InitFunc = symbol_function("init_treasure", this_object());
    create_treasure();
}

void
reset()
{
    if (tr_Flags & F_RESET_USED)
	this_object()->reset_treasure();
}
void
init()
{
    funcall(InitFunc);
}

// Very simple clean_up. TODO: Add NO_CLEANUP flag. -+ Doomdark +-
int
clean_up(int refs)
{
    if (!refs) return 0;
    if (refs > 1 || environment() || first_inventory()) return 1;
    destruct(this_object());
    return 0;
}

/************************************
*                                   *
* And then the rest of functions... *
*                                   *
************************************/

// No need for id-checks and stuff. /bin/pub/_read.c handles all that.
// NEW! Closure's are ok now too. -+ Doomdark 17-jan-96 +-
status
read_cmd(string str)
{
    string t;
    if (read_msg) {
	if (closurep(read_msg)) {
	    if (t = funcall(read_msg, str, this_player()))
		this_player()->tell_me(read_msg);
	} else {
	    this_player() -> tell_me(read_msg);
	}
    }
    else this_player()->tell_me("You can't see any writing on it.");
    return 1;
}

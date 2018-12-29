/****************************************************************
*								*
*	Standard container-object; bag, sack etc.		*
*								*
*	Version: 1.00 28-sep-94.				*
*								*
*	Last modifications by Doomdark 28-sep-94.		*
* 07-Dec-94: Now using /basic/object/dimensions //Graah
* TODO: We could use size and check if an object will fit!
*								*
****************************************************************/

inherit "/basic/id";
inherit "/basic/object/dimensions";

int    value, max_weight, local_weight;
string read_msg;
string info;

static int used_functions;

#define	RESET_USED	1

/****************************************************************
*								*
*			Query-functions.			*
*								*
****************************************************************/

string query_info() { return info; }
int query_value() { return value; }
int query_inserted_weight() { return local_weight; }
int query_max_weight() { return max_weight; }
varargs string
query_long(string what, object who)
{
	if (!first_inventory()) return
		::query_long(what, who) + "\nYou can put things in it.";
	return ::query_long(what, who);
}

/****************************************************************
*								*
*			Set-functions.				*
*								*
****************************************************************/

void set_info(string str) { info = str; }
set_max_weight(w) { max_weight = w; }
set_value(v) { value = v; }
void set_read(string str) { read_msg = str; }

/****************************************************************
*								*
*		And then other functions.			*
*								*
****************************************************************/

status
add_weight(int w)
{
	if (local_weight + w > max_weight) return 0;
	local_weight += w;
	return 1;
}
// We _should_ check if all the stuff fits in here but... *shrug*
// -+ Doomdark (is feeling lazy) +-
status recount_weight()
{
object ob;

	local_weight = 0;
	ob = first_inventory();
	while (ob) {
		local_weight += (int) ob->query_weight();
		ob = next_inventory(ob);
	}
	return 1;
}

status can_put_and_get(string s) { return 1; }
status get() { return 1; }

status prevent_insert()
{
	if (local_weight > 0) {
	        if (this_player()) this_player() -> tell_me(
"You can't when there are things in the " + query_name(0, this_player()) + ".");
	return 1;
	}
	return 0;
}

status
read_cmd(string str)
{
	if (this_player()) this_player() -> tell_me(read_msg);
}


/* Standard create was missing */
void
create()
{
	if (function_exists("reset_container"))
		used_functions |= RESET_USED;
  this_object()->create_container();
}

// And so was reset...
void
reset()
{
  if (used_functions & RESET_USED)
	this_object()->reset_container();
}

inherit "/basic/id";

int dropped, grow_stage;
string owner;

string
set_owner(string n)
{
	owner = n;
	return owner;
}

void
create()
{
#ifdef UIDS

	seteuid(getuid());
	
#endif 
	dropped = 0;
	grow_stage = 5;
	set_name("portable castle");
	set_id(({ "castle" }));
	set_short("portable castle");
	set_long("It's a portable castle for coders to use.");
}

void
heart_beat()
{
string name;
object e;
	if (!dropped || !(e = environment())) return;
	if (grow_stage > 0) {
		e->tell_here("The castle grows...");
		grow_stage -= 1;
		return;
	}
	e -> tell_here("The portable castle has grown into a full castle!");
	shout("You feel something in the world has changed.\n");
	name = create_wizard(lower_case(owner));
	if (name)
		move_object(name, environment());
	destruct(this_object());
}

status
drop()
{
	if (environment(this_player()) -> query_drop_castle()) {
		this_player() -> tell_me("Not this close to the city!");
		return 1;
	}
	dropped = 1;
	shout("There is a mighty crash, and thunder.\n");
	set_heart_beat(1);
	return 0;
}

status
get()
{
	if (dropped) {
		this_player() -> tell_me("You can't take it anymore!");
		return 0;
	}
	return 1;
}

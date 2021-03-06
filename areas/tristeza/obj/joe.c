// Joe is not a monster to be killed...

inherit "/basic/id";

#include <areas.h>

static int m;

void
create()
{
    set_short("Bear bearing a plaque: \"EAT AT JOE'S\"");
    set_long(
      "It's a large, brown bear which carries a huge plaque on its shoulders. \
Both the front and back side of the plaque contain the same advertisement:\n\
\"EAT AT JOE'S\".");
    set_id(({ "bear", "joe", "joe the bear" }));
}

status get() { return 0; }
int query_weight() { return 50; }

void
start_move()
{
    if (!environment()
      || strstr(object_name(environment()), "rooms/joes") < 0)
    {
	call_other(AREA_TRISTEZA "rooms/joes", "???");
	move_object(this_object(), AREA_TRISTEZA "rooms/joes");
    }

    m = 0;
    call_out("moving", 5);
}


void
moving()
{
    string tmp, dr;
    object ob;
    int d;

    tmp = "swssseeennneeeeeesssssswwwnnennwsswnwwnnnwwn";

    if (m >= sizeof(tmp))
    {
	start_move();
	return;
    }

    d = tmp[m];
    dr = (d=='n'?"north":
      d=='e'?"east":
      d=='s'?"south":"west");
    tmp = (string)environment()->query_exit(dr);

    if (tmp)
    {
	ob = find_object(tmp);
	if (!ob) call_other(tmp, "???");
	ob = find_object(tmp);
    }

    if (!tmp || !ob)
    {
	destruct(this_object());
	return;
    }

    environment()->tell_here("Bear leaves " + dr + ".");
    move_object(this_object(), ob);
    environment()->tell_here("Bear arrives.");

    m++;

    call_out("moving", 5 + random(10));
}

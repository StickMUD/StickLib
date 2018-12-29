/*
 * V1.1
 *
 * Graah 25-Jun-96: Notes the player if Peace is on [at query_allow_enter()]
 *
 */

#include <nroom.h>
#include <daemons.h>
#include <areas.h>
#include <generic_npcs.h>
#include <generic_items.h>
#include <generic_rooms.h>
#include <mud_name.h>
#include <city_name.h>
#include <coder_levels.h>

#include "/areas/tristeza/DEFS.h"

#define	NATURE		nature()
#define	REBOOT_SEC	250000

/* Not enough at master. Bah...  //Graah */
#undef WORKING_HOURS

int reboot_time, time_from_reset, last_reset_cycle;
int list_length, wizlocked;
static status cmds_loaded;

status pray(mixed s);

status query_prevent_shadow() { return 1; }

void
reset_room()
{
  if (time_from_reset) last_reset_cycle = time() - time_from_reset;
  time_from_reset = time();
  if (!reboot_time) reboot_time = time_from_reset;
}

void
create_room()
{
  set_light_level(LT_LIGHTS);
  set_outdoors(WD_INDOORS);
  set(PREVENT_TELEPORT);
  set(NO_CLEANUP);
  set(IN_CITY);
  set(NO_PK);
  add_item("bell", "You can ring it.");
  add_item("tower", "A tall tower stands west from here");
  set_short("Temple yard");
	set_long(
"You are in the temple yard south of " CITY_NAME " temple.\
 There is a huge pit in the center and a door in the west wall. There is \
a bell beside the door. As you look up the wall you see that it is in fact a\
 tall tower. There is a clock on the wall.");
  set_exits(([
	      "north" : AREA_TRISTEZA+"temple/temple_ruins",
	      "east"	: RACE_GUILD,
	      "south" : AREA_TRISTEZA+"virtual/S7_6",
	      // a 'dummy' exit; command defined later on.
	      "west": GENERIC_ROOM_CHURCH,
	      "northeast" : AREA_TRISTEZA "wedding/chapel"
  ]));

  // Christmas time: Santa Claus (do not replace Tim, keep Tim too)
    //add_monster(GENERIC_NPC_SAINT_XMAS, 0, 1);

  add_monster(GENERIC_NPC_SAINT, 0, 1);

  add_object(GENERIC_FINGER_TOOL, 0, 1);
  add_monster(GENERIC_NPC_MUD_GUIDE, 0, 0);
  set_commands(([
		 "west"	: "west",
//		 "wtoggle" : "wtoggle",
		 "ring"	: "ring",
		 "enter"	: "enter"
  ]));
  set_items(([
"pit" : "In the middle of the yard is a deep pit. It was used for sacrifice\
 in the old times, but nowadays it is only left for tourists to look at.",
"door" : "It is open.", "clock" : "It's old clock."
  ]));
  //CODERLOCKED //F
  //wizlocked=1;
  reset_room();
}

int
query_wizlocked() {
  return MUDLOCK_D->query_mudlock();    //1 - wizlocked, 0 - not
}

int
query_allow_enter(object obj)
{
  int hours, mins;
  string d;

#ifdef WORKING_HOURS
  sscanf(ctime(time())[11 .. 15], "%d:%d", hours, mins);
  if (ctime(time())[0 .. 2] == "Sat" || ctime(time())[0 .. 2] == "Sun")
    return 1;
  if (hours < 10 || hours > 15) return 1;
  d = ctime(time())[4 .. 9];

  /* Dec 27 = temporary (1995) */
  if(member_array(d,({"May  1", "May 25", "Dec  6", "Dec 24", "Dec 25",
			"Dec 26", "Jan  1", "Jan  2", "Jan  6",
			"Dec 27" })) >= 0)
    return 1;

  if (obj -> query_coder_level()) {
    this_player() -> tell_me("NOTE: "+MUD_NAME+" is closed for mortal \
players right now.");
    return 1;
  }

  if (obj -> query_testplayer()) {
    this_player() -> tell_me("NOTE: "+MUD_NAME+" is closed for mortal \
unless you are a testplayer.");
    return 1;
  }

  mins = hours * 60 + mins;
  mins = 960 - mins;
  hours = mins / 60;
  mins = mins % 60;

  this_player() -> tell_me("  "+MUD_NAME+" is closed for mortal players duri\
ng the most working hours of  our local system:\n\n\
 10.00 - 16.00 East European Time Cairo, Helsinki, Tallinn\n\
 9.00 - 15.00 Central European T. Amsterdam, Berlin, Oslo, Rome, Stockholm\n\
 8.00 - 14.00 GMT                 London, Lisboa\n\
 3.00 -  9.00 Eastern Std Time    New York, Montreal\n\
 2.00 -  8.00 Central Std Time\n\
 1.00 -  7.00 Mountain Std Time\n\
 24.00 -  6.00 Pacific Std Time   Los Angeles, San Fransisco, Vancouver\n\
 18.00 - 24.00                    Melbourne\n\
 17.00 - 23.00                    Tokyo\n\
 16.00 - 22.00                    Beijing, Hong Kong\n\n \
You can enter the game again in " + (hours ? ((hours > 1) ? (hours + " hours ")
: "1 hour") : "") + ((mins != 1) ? (mins + " minutes.\n\n") :
		     "1 minute.\n\n"));

  destruct(obj);
  return 0;
#else
  /* No working hours. */

  /* Peace? */
  if (PEACE_D->query_peace() && obj)
    obj->tell_me(
"\nPeace is upon the World. No playerkilling is possible at the moment.\n");

  return 1;
#endif
}

int
check_is_open()
{
	return (int) this_object() -> query_allow_enter(this_player());
}

void
init_room()
{
object ghost;
  if (interactive(this_player())) check_is_open();

  if (MUDLOCK_D->query_mudlock()) {
    if (!this_player()->query_coder_level() &&
	interactive(this_player())) {
      this_player() -> tell_me("The game is currently coder\
locked. Please try again a little later.");
      /* Not working properly
	 destruct(this_player());
	 */
      this_player()->login_timeout();
      return;
    }
    this_player() -> tell_me("The game is currently Coderlocked.");
  }

  call_out("welcome_novice", 1, this_player());
  if (interactive(this_player()) && this_player() -> query_ghost()) {
    add_action("pray", "pray");
    call_out("check_if_dead", 1, this_player());
  }
  /* Am I hunted? */
  if(present4("police_pk_flag", this_player(), this_player(), 1000)) {
    this_player()->inform_us("Season is now OPEN"); 
  }
}

void
check_if_dead(object who)
{
  if (who && who -> query_ghost() && !present("death_mark", who))
    pray(who);
}

#if 0
void
welcome_novice(object ob)
{
int a;
	remove_call_out("welcome_novice");
	if (!present("guide", this_object())) {
// let's see if we can get this thing working...
	add_monster(GENERIC_MUD_GUIDE, 0, 0);
	}
	if (!ob || !interactive(ob)) return;
	a = (int) ob->query_age();
	if (a < 5 && a > 0)
		if (guide) guide -> new_novice(ob);
	return;
}
#endif

varargs string
query_long(string str, object who)
{
	if (str == "clock") return "An old archaic clock, showing about "
		+ (string) NATURE -> query_about_time() + ".";

	if (str == "door") {
		if (!(AREA_TRISTEZA+"rooms/elevator")-> query_closed_door() &&
		  (int)(AREA_TRISTEZA+"rooms/elevator")->query_level() == 2)
                 return "The door is open.";
		else return "The door is closed.";
	}

	return ::query_long(str, who);
}

void
xyzzy()
{
	this_player() -> tell_me("Everything shimmers.\nYou wake up elsewhere...");
	this_player() -> move_player("elsewhere", "/room/test");
}

status
west(string str)
{
	if ((AREA_TRISTEZA+"rooms/elevator")->query_closed_door()
       || (int)(AREA_TRISTEZA+"rooms/elevator")->query_level() != 2) {
		this_player() -> tell_me("The door is closed. Ring bell to call\
 the boy.");
		return 1;
	}
	this_player() -> move_player("west", AREA_TRISTEZA+"rooms/elevator");
	return 1;
}

status
enter(string str)
{
	if (!str) return notify_fail("Enter what?\n"), 0;
	if (str == "door")
		return west(0);
	return 0;
}

status
ring(string str)
{
	if (str != "bell") return notify_fail("Ring what?\n"), 0;
	this_player() -> tell_me("Ok.");
	(AREA_TRISTEZA+"rooms/elevator")->call_elevator(2);
	return 1;
}

void
pray(mixed who)
{
	if (!objectp(who))
		who = this_player();
	who -> remove_ghost();
	return;
}

status
prevent_look_at_inv(string str)
{
	return str != 0;
}

#ifndef LPC3

void
call_combine()
{
int tmp;

	tmp = time();
	list_length = combine_free_list();
	if (time() - tmp > 20) shout("Game driver shouts: Sorry for the delay.\
 I had some garbage collection to do.\n");
	if (list_length < 1000) call_out("call_combine", 10);
	else if (list_length < 5000) call_out("call_combine", 30);
		else call_out("call_combine", 60 * 2);
}


#endif 

int
query_last_reset_cycle()
{
	return last_reset_cycle;
}

void
show_next_reboot_time()
{
int t, st, x, z, h, m;

	t = time();
	st = (int) "/secure/master" -> query_start_time();
	x = (t / 86400) * 86400 + (9 * 3600) + 1800;
	if (x < t) x += 86400;
	for (; (x - st) < REBOOT_SEC;)
		x += 86400;
	z = x - t;
	h = z / 3600;
	m = (z / 60) % 60;
	if (h < 1 && m < 1) {
		this_player() -> tell_me("Auto-reboot should be going on now!");
		return;
	}
	this_player() -> tell_me("Next auto-reboot starts in about");
	if (h > 0) {
		this_player() -> tell_me(h + " hour");
		if (h > 1) this_player() -> tell_me("s");
		if (m) this_player() -> tell_me(" and");
	}
	if (m > 0) {
		this_player() -> tell_me(m + " minute");
		if (m > 1) this_player() -> tell_me("s");
	}
	this_player() -> tell_me(".");
}

int
query_is_start_location()
{
	return 1;
}

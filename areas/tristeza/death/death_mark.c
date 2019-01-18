#include "/sys/configuration.h"

#include <areas.h>

#include <generic_rooms.h>

inherit "/basic/id";

#include <player_defs.h>

int hb_count, hb_wait;
object victim;
string race;

// death_mark.c
// Different deaths for different races.


#define	ROOM_END	GENERIC_ROOM_CHURCH
#define ROOM_ELF	"death/workroom"
#define ROOM_DARKELF	"death/workroom"
#define ROOM_ORC	"death/workroom"
#define ROOM_HOBBIT	"death/workroom"
#define ROOM_DWARF	"death/workroom"
#define ROOM_HUMAN	"death/workroom"

#include "dead_orc.c"
#include "dead_elf.c"
#include "dead_dark_elf.c"
/*
#include "dead_hobbit.c"
#include "dead_dwarf.c"
*/
#include "dead_human.c"

// NEW:

void
heart_beat()
{
    if (!victim) {
	destruct(this_object());
	return;
    }
    if (environment() != victim)
	move_object(this_object(), victim);
    if (--hb_wait > 0) return;
    switch (race) {
    case "orc":
    case "half-orc": hb_wait = dead_orc(environment(), hb_count); break;
    case "elf":    hb_wait = dead_elf(environment(), hb_count); break;
    case "dark elf": hb_wait = dead_dark_elf(environment(), hb_count);
	break;
#if 0
    case "hobbit": hb_wait = dead_hobbit(environment(), hb_count); break;
    case "dwarf":  hb_wait = dead_dwarf(environment(), hb_count); break;
#endif
    default:       hb_wait = dead_human(environment(), hb_count); break;
    }
    hb_count++;
}

void
create()
{
    set_name("death");
    set_id(({ "death_mark" }));
    set_short(0);
    set_long(
      "You can't quite focuse your eyes on Death but you see him wearing\
 a black robe and a large, very sharp scythe.");
}

void
init()
{
    if (environment() != this_player()) return;
    victim = this_player();
    victim->tell_me("You notice that death has appeared.");
    victim->set(LIV_DEATH_MARK);
    add_action("no_cmd", "", 1);
    race = (string) victim->query_race();
    configure_object(this_object(), OC_HEART_BEAT, 1);
}

int
no_cmd()
{
    if (member( ({ "l", "look", "tell", "exa",
	  "channel", "flord",
	  "help", "lord", "score", "cost", "sc", "session" }), query_verb()) != -1)
	return 0;
    victim->tell_me("Not now.");
    return 1;
}

// Important deathmark definitions

int get()  { return 1; }
int drop() { return 1; }
string query_auto_load() { return AREA_TRISTEZA+"death/death_mark:"; }

#include <areas.h>

#define	FOOP	AREA_TRISTEZA "duel/npc/orc"

inherit FOOP;

void
reset(status arg) {
object spear;

  if(arg) return;

  ::reset(arg);

  level=10+random(8);

  set_hp(level*8+50);
  
  set_wander(15);
  set_wimpy(0);

	set_move_msg("msgout", "walks");

	move_object(spear = clone_object(AREA_TRISTEZA "duel/obj/spear.c"),
		this_object());

	wield(spear, 1);

  set_short("Orc warrior");
  set_name("orc warrior");
  set_race("orc");
  set_ac(level + 5 + random(5));
  set_wc(18 + random(level/2));  
  set_keep_hb(-1800);
// So as to prevent it from staying on forever...
  l_chat();
}

#include <treasure.h>

#define P20(X)  (oldsp - (X))/5     /* 20% , oldsp > X */

#if 0
#define LOG_CHEAT(x, y, z) \
     write_file("/log/focus_cheat", \
   capitalize(x)+" used "+y+" sp and got "+z+" sp back from it (20%) \n")
#else
#define	LOG_CHEAT(x, y, z)	;
#endif

private int oldsp, zz;
private object owner;

void
init_stone() {
    oldsp=zz=0;
    set_heart_beat(1);
}

void
create()
{
  set_short("Focus stone");
  set_long("It's a round magical stone with pink aura around it. \
Somehow you feel magically stronger.");
  set_weight(2);
  set_value(400);
  set_id(({ "stone", "focus_stone" }));
  init_stone();
}

void
heart_beat() {

    int sp;
int hp;

    if(environment())
	if(living(environment())) {

	    if(environment() != owner) {
		owner = environment();
		oldsp=zz=0; /* no one gets free sps */
	    }

	    sp = (int)owner->query_sp();
	    if(sp < oldsp) 
               hp = (int)owner->query_hp();
	    {
	      if(!random(10))
		tell_object(owner, "You feel your magical strength growing.\n");
		LOG_CHEAT(owner->query_real_name(),
		  (oldsp - sp), P20(sp));
		owner->add_sp(P20(sp));
               owner->add_hp(-P20(sp));

             // This item is a total cheat. /Graah
             if ((++zz > 600) && (random(100) == 66))
                 {
                   tell_object(owner, "The focus stone \
suddenly explodes!!!\n");
		   owner->death(3, 0);
                   destruct(this_object());
                 }
	    }
	    oldsp = sp;
	}

}

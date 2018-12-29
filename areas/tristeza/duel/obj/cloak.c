#include <armour.h>

#include <invis_levels.h>
#include <areas.h>

#define	SHADOW	AREA_DUEL_ARENA+"obj/cloak_shadow"

static object cshadow;

void
create_armour() {

  set_long("It's a long pitch black cloak. You immediately notice that"+
           " it is not like regular cloaks but you can't define what makes"+
           " it so different.");
  set_id(({ "cloak", "black cloak", "a black cloak"}));
  set_name("cloak");
  set_short("A black cloak");
  set_type("cloak");
  set_ac(1);
  set_weight(3);
  set_value(500);

   set_wear_func(this_object());
   set_unwear_func(this_object());
}

status
wear(object this_armour, object wearer)
{
	wearer -> tell_me("Aaaah, that feels so comfortable... hmm... and safe!");
          if(cshadow) destruct(cshadow); //Just in case if bugged.

          cshadow = clone_object(SHADOW);
	cshadow -> do_shadow(wearer);
	wearer -> set_invis(IL_INVISIBLE);
return 0;
}

varargs status
unwear(object o, object who) {

	who -> tell_me("What a pity, it is a nice and safe feeling cloak.");
          if(cshadow) destruct(cshadow);
	who -> set_invis(IL_VISIBLE);
return 0;
}

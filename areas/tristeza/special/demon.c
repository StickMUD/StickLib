// this version is modified not to attack nonlords
// so we can raid town with him :)
//
// This is Ba'alzamon, the ancient demon!
// He is REDICULOUSLY powerful. Luckily, he isn't aggro.
// I made him because I was watching Ramlin nuke
// the galactic dragons with little effort at all.
// THIS one might be more of a challenge. - Germ 
//
#include <conditions.h>
#include <equipped_npc.h>
#include <attack.h>
#include <armour_defs.h>
#include <areas.h>

#define SHOW environment()->tell_here
#define TELL ene[i]->tell_me

void
create_monster()
{
   set_name("ba\'alzamon");
   set_short("Ba\'alzamon, the Forsaken");
   set_id(({ "daemon", "demon", "forsaken" }));
   set_long(
"This is the ancient daemon Ba\'alzamon, trapped here ages ago\
 by powerful wizards when the city was young. Awesome and terrible\
 to behold, he waits, brooding, hating, awaiting the day of his escape.");
   set_alignment(AL_SATANIC);
   set_gender(G_MALE);
   set_level(100);
   set_ep(10000000);
   set_hp(6666);
   load_a_chat(30, ({
	"Ba\'alzamon screams with maniacal laughter!",
	"Ba\'alzamon bellows: Puny mortal! I will grind you to dust!",
	"Ba\'alzamon's eyes flash with fire!",
	"Ba\'alzamon roars: Let my revenge begin!"
   }));
   add_armour(AREA_TRISTEZA+"special/amulet_forsaken.c","amulet",0,2,
	"Ba'alzamon takes his amulet." );

}

void
heart_beat()
{
  int i;
  object *ene;

   ::heart_beat();
  if ( !ene=all_inventory(environment())) return;
  for(i=sizeof(ene)-1;i>=0;i--)
    /* Let's make little players (non-lords) and City Guards flee */
    if(living(ene[i]) && ene[i]->query_level() < 20 &&
       !ene[i]->query_coder_level() &&
       (!ene[i]->query_npc() || ene[i]->id("city_guard")))
          ene[i]->run_away();
}


int
attack() 
{
   int i,dam;
   object *ene;

   if (!random(3) && ene=all_inventory(environment()))
    {
      environment()->tell_here("Ba\'alzamon mutters an incantation.");
      switch(random(3))
	{
          case 0: case 1:
	    for ( i=0; i< sizeof(ene); i++ )
		if ( living(ene[i]) && ene[i]!=this_object())
		  if (( random(10)>6&&ene[i]->query_level()>19) || ene[i]->is_npc() )
		      {
			ene[i]->tell_me("You are engulfed in flames!");
	    		environment()->tell_here(ene[i]->query_name()
				+" is engulfed in flames!",ene[i]);
			if ( ene[i]->is_npc() ) dam=100+random(100);
			else dam=50+random(100);
	    		ene[i]->hit_player(dam, DT_FIRE);
		      }
	    break;
	 case 2:
            for ( i=0; i< sizeof(ene); i++ )
                if ( living(ene[i]) && ene[i]!=this_object())
                  if (( random(10)>4&&ene[i]->query_level()>19) || ene[i]->is_npc() )
                      {
                        ene[i]->tell_me("Your muscles stiffen! You cannot move!");
                        environment()->tell_here(ene[i]->query_name()
			+" stands rigid, staring in terror!",ene[i]);
			if ( ene[i]->is_npc()) dam = 5+random(7);
			else dam = 3+random(5);
			ene[i]->set_condition(C_STUNNED, dam);

                      }
	    break;
	}
     }
   return ::attack();
}

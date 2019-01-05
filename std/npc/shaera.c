/* A somewhat castrated version of Shaera from StickMud. No
 * kingdom-related stuff. Doomdark, 28-Aug-98.
 */

#include <npc.h>
#include <unique.h>
#include <areas.h>
#include <city_name.h>
#include <mud_name.h>
#include <config.h>

void
create_monster()
{
  set_name("sharea");
  set_id(({"shaera"}));
  set_dead_ob(this_object());

  set_short("Shaera, the evil brother of Leo");
  set_long("You look upon a dark creature going by the name of \
Shaera. By defeating his noble brother Leo, he was able to lay \
his evil shadow all over the " CITY_NAME " city and surroundings. \
Shaera is not an ugly man, one could even feel attracted to \
his looks of nobility were it not for those creaturelike eyes \
of his. Never will you be the same upon looking into them.");
//  set(NPC_FOLLOW_ATTACKER);
//  set(NPC_MOVE_AREA, CHIOS_BASE+"armnah");
  set_level(40);
  set_hp(5000);
  load_a_chat(40, ({
"Shaera vigorously tumbles your masterly attempt to hit him!",
"Shaera says: Why are you fighting me? It is within YOU I see the war!",
"Shaera says: Strike me down and I will be more powerful than you could \
ever conceive!",
"Shaera says: Good! Let the evil demons flow through your hands!",
"Shaera says: Enough whining. Now I will kill you." }));
}

status monster_died(object p)
{
  object sce; 
  object id;

/*  shout(capitalize(this_player()->query_real_name())+
" has conquered the evil Sh"); */

  this_player()->tell_me("You have an empty feeling about not getting\
 the power you should. Weird.");

#if 0
  sce = clone_object("/u/cigam/nk/sceptre_tristeza");
  sce->set_kingdom_name(CITY_NAME);

  move_object(sce, this_player());
  UNIQUE_D->create_unique(object_name(sce), 1);
  if (!UNIQUE_D->query_unique(object_name(sce))) {
    destruct(sce);
    return 0;
  }
  UNIQUE_D->add_unique(object_name(sce), sce);

    sce->set_castle_entrance(AREA_EAST "east/to_castle");
    sce->make_king(this_player()->query_real_name());
    this_player()->tell_me("Naima, the Goddess of Protection appears \
in front of you!\n\
Naima smiles gently at you.\n\
Naima says: I am pleased with you, mortal!\n\
Naima gives you the Sceptre of the King of " CITY_NAME "!\n\
Naima smiles sadly and disappears.");
  id = present("kingdom_id", this_player());
  if (id && id->is_king()) {
    shout("\n\nThe world of "+MUD_NAME+" has arisen to see the dawn of \
a new and noble age! Let it be known that from now on all of "+MUD_NAME+" \
is to hail "+
  capitalize(this_player()->query_real_name())+" as their true \
Emperor!");
  }
  else {
  shout("\n\nAfter a sorrow age of evil darkness in the once \
admirable " CITY_NAME " Kingdom, King "+
  capitalize(this_player()->query_real_name())+" will now be the ruler \
of these parts of the world! May he bring true and noble life \
back to us once again!\n\n"); 
  }
#endif

  return 0;
}


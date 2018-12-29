/* City mobiles should inherit this instead of /obj/monster */

#include <group_npc.h>

#include <criminal.h>

/* Define this to activate crimes */
/* #define CRIMES */

static object *assaulter; /* To check someone is truly a murderer */

void
extra_attacked_by(object ob)
{
  int i, s;

  if (!ob || !living(ob)) return;

  /* Fresh attack from a player? */
  if (!this_object()->query_attack() &&
      !ob->query_npc()
      /* This is not the way to find out that he attacked us first...how???
	&& ((object)ob->query_attack()) == this_object()
	*/
      ) {

#ifdef CRIMES
    CRIME_D->crime(ob, CR_ASSAULT);
#endif

    if (!assaulter) assaulter = ({ });
    if (member_array(ob, assaulter) == -1)
      assaulter += ({ ob });
  }
}

/* Using the new event function //Graah */
void
killed_by(object ob)
{
  object witness;
  int i, s;

  if (!ob || !living(ob)) return;

  tell_object(ob, (string)this_object()->query_name() +
	      " whispers " + (string)this_object()->query_possessive() +
	      " last words: You will pay for this...\n");

  /* Crime is set only if some other cityguard
     is at the location and witnesses the crime. */

  if (!environment() || ob->query_npc()) return;

  /* Only the one who first assaulted us can be a murderer */
  if (!assaulter || member_array(ob, assaulter) == -1) return;

#ifdef CRIMES
  CRIME_D->crime(ob, CR_MURDER);
#endif
}

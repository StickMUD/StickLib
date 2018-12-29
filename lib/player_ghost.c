// Ghost of a player

// 26-Jun-97 / Graah: Killer by real name now.
// 10-Jul-97 / Graah: Some lamer asshole always runs to someone
// else's ghost and takes all stuff. So, now tell the "owner" when
// someone else kills the ghost. Happens quite rarely, though.
// New AI: Player ghost wanders around, seeking its owner.
// 17-Jul-97 / Graah: Even more intelligent.

#include <npc.h>
#include <room_defs.h>

static string killer;
static string owner;
static object origenv;

void
create_monster()
{
  set_id(({ "player_ghost", "ghost" }));
  set_race("ghost");
  set(LIV_UNDEAD);
  set_gender(G_NEUTER);
  set_dead_ob(this_object());
  set_wander(0);
  set_keep_hb(1);
}

set_wc(arg) { weapon_class = arg; }
set_ac(arg) { armour_class = arg; }

query_killer() { return killer; }
set_killer(string x) { killer = x; }

query_owner() { return owner; }
set_owner(string x) { owner = x; }

is_player_ghost() { return 1; }

/* We remember our killer. We won't attack a NPC however. */
extra_aggressive(ob)
{
  if (killer && ob &&
      killer == (string)ob->query_real_name() && !ob->is_npc())
    {
      tell_room(environment(), query_name() + " " +
		({ "screams", "wails", "howls" })[random(3)] + " " +
		({ "shrilly", "loudly", "ghastly" })[random(3)] +
		" when noticing its " +
		({ "slayer", "killer" })[random(2)] + "!\n");
      return 1;
    }

  return 0;
}

int
monster_died(object this_npc, object killer)
{
  object ob;

  if (owner && killer &&
      (ob = find_player(owner)) &&
      (ob != killer) &&
      ((int)ob->query_level() < (random(101) + random(101))))
    ob->tell_me("Your Ghost tells you: " +
capitalize((string)killer->query_real_name()) + " killed me!");

  owner = 0;

  return 0;
}

object
hades()
{
  object oe;

  oe = find_object("/room/hades");
  if (!oe)
    {
      call_other("/room/hades", "???");
      oe = find_object("/room/hades");
    }

  return oe;
}

void
heart_beat()
{
  object oe, ob;
  mapping ex;
  string toroom, *exd;
  int s;

  ::heart_beat();

  if (!origenv) origenv = environment();

  // Fight on?
  if (attackers) return;

  ob = hades();

  // Owner? In game? Don't clear it - owner may return.
  if (!owner ||
      !(oe = find_player(owner)))
    {
/* And what on earth was this supposed to do? -+ Doomdark 26-Dec-97 +- */
/* (if (.. !oe) ... oe->xxx() ?????) */
#if 0
      if (oe->query_dead() &&
	  ob &&
	  (environment() != ob))
#else
	if (ob && environment() != ob)
#endif
	{
	  move_player("Vfades away#", ob);
	  return;
	}

      if (environment() != origenv) move_player("Vfades away#", origenv);
      set_heart_beat(0);
      if (!owner) set_keep_hb(-52);
      return;
    }

  if ((environment() == ob) && origenv)
    {
      move_player("Vfades away#", origenv);
      return;
    }

  if (random(100) < 97) return;

  // No env or already here?
  if (!(oe = environment(oe)) ||
      oe == environment()) return;

  // Owner in teleport-to proof room, or in guild?
  // Then don't go there.
  if (oe->query(PREVENT_TELEPORT_TO) ||
      oe->query(ROOM_GUILD)) return;

  // Find random exit of room - we never appear in the _same_ room
  // than the owner.
  if (!mappingp(ex = (mapping)oe->query_exits())) return;
  if ((s = sizeof(exd = m_indices(ex))) < 1) return;
  if (!stringp(toroom = (string)oe->query_exit(exd[random(s)]))) return;

  oe = find_object(toroom);
  if (!oe)
    {
      // Avoid stopping beat...
      catch(call_other(toroom, "???"));
      if (!(oe = find_object(toroom))) return;
    }

  // Everything is ok, now finally move!
  move_player("V" +
	      ({ "fades away", "shifts through the ground",
		   "slowly levitates away", "flies away" })[random(4)],
		   oe);
}

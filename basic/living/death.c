/****************************************************************
*								*
*	Module: death						*
*								*
*	Version: 1.01b						*
*								*
*	Included by: living.c					*
*								*
* 09-Dec-94: MK with controlled monster doesn't reduce		*
*	stats, but otherwise it is a MK.			*
* 20-Dec-94: death() function is not static any more //Graah	*
* 05-may-95: Death() now can have 2nd optional parameter; living*
*	that killed us. Usually player... Used by some guild	*
*	objects.						*
* 23-may-95: Exp formulas tuned. Should be fair to players	*
*	on higher levels too. Still, they won't get much exp	*
*	from low-level monsters...				*
* 05-Jun-95: Exp formulas changed to use players exp instead of *
* 26-jun-95: Changed calls to "(un)pack_conditions" to use	*
*	"cond_pack(int)" straight away. Could remove 2 funcs!	*
* 	level to calculate exp_for_kill. No more cheats.	*
* 05-oct-95: Changed death mark -> set/query(LIV_DEATH_MARK)	*
* 03-nov-95: 50% lower exp loss when in a guild room. -+ DD +-	*
* 17-dec-95: Dirty stamp is cleared when we die. -+ DD +-	*
* 18-dec-95: Few small bug fixes and alike. -+ DD +-		*
* 23-dec-95: Due to rewrite of combat, death.c needed patching	*
*	too. -+ DD +-						*
* 19-jan-96: Session-info system was recoded; thus changes here *
*       as well. -+ DD +-					*
* 24-jan-96: Now informs guild object about death; in case it	*
*	wants to know; have to set hook with "set_guild_hook"!	*
*  3-Oct-96 / Graah: Added exp_times handling (for players)	*
* 18-Jul-97 / Graah: Fixed weird bug with dirty_stamp.Hopefully.*
*  9-May-98 / Graah: NPC_NO_CORPSE is now checked.		*
*  26-Jul-98, Doomdark: Now sends a new argument to second_life;*
*	the corpse object...					*
****************************************************************/

#ifndef	LIVING_C

#include "/basic/living/living_private.h"
#include <living_defs.h>
#include <npc_defs.h>
#include <room_defs.h>
#include <party.h>
#include <daemons.h>
#include <guild.h>
#include <corpse_defs.h>

status dead, ghost, player_kills;
static int liv_Flags;
int hit_point, max_hp, experience, exp_times;
int Str, Dex, Con, Int;
int max_Str, max_Dex, max_Con, max_Int;
int alignment;
static object kill_ob;
static int attackers;
static object *attacker;
static object *hunted;
object last_attacker;
string name, cap_name;
int level, luck, age;
static int dirty_stamp;
static int GuildFlags;
static mapping GuildHooks;
int dirty_stamp;

string query_name(int a, object b);
string query_real_name();
varargs void tell_me(string s, status a, status b);
void set_crime_count(int x);
void set_crime_level(int x);
varargs void stop_fight(object x);
void set_temp_luck(int x);
int query_temp_luck();
void transfer_all_to(object x);
void set_conditions(mapping m);
int query_ac();
int query_wc();
string query_race();
string query_short(int a, object b);
int query_level();
int query_coder_level();
varargs int set(int x, int y);
int query(int x);
#endif

#define	SAVE_ME	this_object()->save_me(0)
// This defines what is max. exp killer can get; it's (exp / MIN_EXP_DENOM).
#define		MIN_EXP_DENOM		33

// And this defines what's standard amount killer can get, like above.
#define		NORMAL_EXP_DENOM	40

/*
 * (l = levelmax + (player_kills * (level/5)*level * 1000))) x = l;
 *
 * Some players have lost over 300 million experience due that,
 * over 10M in one death. Isn't that a _bit_ too much?
 *
 * The tough players lose about one hour's work per 10M. Still,
 * is this enough? //Graah
 *
 */
#define MAX_EXP_LOSS	3000000

#ifndef TIME_TO_STAY_DIRTY
#define TIME_TO_STAY_DIRTY (120 + 2 * level)
#endif


/* Return the experience loss or exp when divided by div */
// Also, let's set it to private static; no need to call outside
// living object. -+ Doomdark +-
// Alas, this doesn't prevent func from inheritance... So won't save memory.

private static int
exp_loss(int xp, int div)
{
  int x;
  int l;
  int levelmax;

  x = xp / div;

  levelmax = level * 380000 - (100 - level) * 96000;
  levelmax += 20 * level * level * level;
  if (levelmax < 350000) levelmax = 350000;

  if (!player_kills)
    {
      if (x > levelmax) x = levelmax;
    }
  else
    {
      if (x > (l = levelmax + (player_kills * (level/5)*level * 1000))) x = l;
      if (x > xp/div) x = xp/div;
    }

  if (x > MAX_EXP_LOSS) x = MAX_EXP_LOSS;

  x -= x / 4;

  if (environment()->query(ROOM_GUILD)) x /= 2;
  return x;
}

private static int
exp_gained(int exp, int kr_level, int kr_ep, int kr_expt,
	   int killed_level, int k_wc, int k_ac)
{
  int diff, max_exp, denom, killer_level;

  // Check level from exp, and punish those who don't raise levels!

  killer_level = (int)LEVELS_D->check_level(kr_ep, kr_expt);

  // Let's really screw the cheaters for a change!

  diff = killer_level - killed_level + (killer_level - kr_level);

  // Let's calculate max. exp killer can get, depending on his/her/its level.
  // -> me = 500 - lvl^3 + 250lvl^2 - 1200lvl

  max_exp = 500 - (killer_level * killer_level * killer_level)
    + 250 * ((killer_level + 1) * (killer_level + 1))
    - 1200 * killer_level;

  if (max_exp < 300) max_exp = 300;
  // Be sure ac & wc won't have much effect on lower levels...
  if ((k_wc -= 10) < 0) k_wc = 0;
  if ((k_ac -= 10) < 0) k_ac = 0;

  // Also, we won't let small differences on levels to make much difference...
  if (diff > 0)
    if (diff < 6) diff = 0;
    else diff -= 5;
  else diff /= 2;

  denom = NORMAL_EXP_DENOM + 20 * diff / killed_level - k_wc/10 - k_ac/10;

  // Let's be sure we won't punish low level killers
  // (as we use ratios, this might otherwise happen),
  // nor give too big amounts of exp...

  if (denom < MIN_EXP_DENOM) denom = MIN_EXP_DENOM;

  exp = exp / denom;

  if (exp > max_exp)
    return max_exp;

  // Weird exp drain somewhere. Too lazy to find it. /Graah
  if (exp < 0) return 0;

  return exp;
}

// mode 1: normal death (we have an attacker)
// mode 2: bleed to death
// mode 3: death trap

// NEW: This is not static any more! //Graah
// NEW: Optional second argument; living who killed us. -+ Doomdark +-

public varargs void
death(int mode, object killer)
{
  string what, s;
  object remains, *inv;
  int pk, nicemk, exp_reward, x, pid, diff;
  int deathDiv;

  if (!killer) {
    if (!(killer = this_player())) return 0;
  }

  if ((liv_Flags & F_LIV_IS_PLAYER) && query_coder_level()) {
    tell_me("Someone called 'death()' from you! You didn't die.");
    return 0;
  }

  hunted = ({ });

  // Die in some creative ways. //Graah
  DEATH_D->die(this_object());

  // check out if we allow dying here (for dueling) ++Tron
  if (environment()) {
    if (environment() -> object_died(this_object(), killer)) { 
      hit_point = 10;	// Let's not bleed.
      stop_fight(killer);
      killer->stop_fight(this_object());
      set_conditions(([]));
      return;
    }
  }

  // Clear all conditions
  set_conditions(([]));

  if (liv_Flags & F_LIV_IS_PLAYER) {
    ghost = dead = 1;

    // Crimes are forgotten (only players).
    set_crime_count(0);
    set_crime_level(0);

  }

  // Let's prevent continous PKing this player...
  // Unless the stamp was set by the police as a punishment!
  // If level dropped, they remained dirty? 2 *, just to be sure. /Graah
  if (dirty_stamp < (2 * (age + TIME_TO_STAY_DIRTY)))
    dirty_stamp = 0;

  // Restore abilities to normal
  Str = max_Str;
  Dex = max_Dex;
  Con = max_Con;
  Int = max_Int;

  // Find out who killed us.

  if (mode == 2) {
    // no more this_player() as killer if attacker here.
    if ((killer == this_object()) && last_attacker &&
	present(last_attacker, environment()))
      killer = last_attacker, last_attacker = 0;
  }
 
  if (!killer) {  // Death trap, ie not living   /Elena
    killer = environment();
    if (mode == 1) mode = 3;
  }

  if (mode == 1 && killer && living(killer) && killer->query_ctrl())
    mode = 3;

  // Call event function //Graah

  // Let's only call this for NPCs. -+ Doomdark 11-oct-95 +-
  if (liv_Flags & F_LIV_IS_NPC)
    this_object()->killed_by(killer);

  if (killer && !(liv_Flags & F_LIV_IS_NPC)) {
    if (!living(killer) || mode != 1 || killer == this_object()
	|| killer->query(LIV_IS_PLAYER)) {

      pk = 1;

      // Death traps: no experience loss
      // Bleed to death or pk: some experience lost but no stats

      if (mode != 3 || (living(killer) && killer != this_object()))
	{
	  deathDiv = (level * level);
	  deathDiv /= (level / 2 == 0) ? 1 : level/2;
	  experience -=
	    exp_loss((exp_times > 0 ? 2100000000 : experience), deathDiv);
	}
    } else {
      /* Cruel: lose one third of experience */
      experience -=
	exp_loss((exp_times > 0 ? 2100000000 : experience), 3);
    }
  }

  // Fix exp_times if...
  if (experience < 0 && exp_times > 0)
    {
      exp_times--;
      experience += 2100000000;
    }

  // Controlled monster? If so, make nice mk.
  hit_point = 30;

  // The killer gains experience and updates alignment.
  if (killer && killer != this_object() && living(killer))
    {
      killer -> add_alignment(alignment);

      // This isn't a clean way to do it, but...
      // Let's compare levels to adjust exp. ++Tron
      // But let's do all comparing etc. in dedicated function! -+ Doomdark +-
      exp_reward = exp_gained((exp_times > 0 ? 2100000000 : experience),
			      (int) killer->query_level(),
			      (int)killer->query_ep(),
			      (int)killer->query_exp_times(),
			      query_level(), query_wc(),
			      query_ac());

      if (pk) {
	if (level < 3) exp_reward = 0; /* Prevent cheating */
	else if (exp_reward > 100000) exp_reward = 100000; /* Max PK exp */

	// If one kills a lower level player, luck decreases!
	if (level < (int) killer->query_level()) {
	  killer->set_luck((int)killer->query_luck() - 1);

	  /* Poor killed one gains temp luck */
	  set_temp_luck(query_temp_luck() + 2 + random(3));

	  /* If you kill an innocent player you are a player killer */
	  if(!player_kills)
	    killer -> add_player_kills(1);
	}
      }

      /* Update killer's session kill list */
      if (interactive(killer)) {
	what = query_name(0, killer);
	if (!what && name) what = capitalize(name);
	if (!what) what = "Someone";
	// Was:  killer->add_session_kill(what);
	killer->session("add", "kill", what);
      }

      // Guest doesn't give any exp.
      if (name != "guest") {
	// Is there a party to share exp with?

	if(pid = (int)killer -> query_party(PARTY_ID))
	  PARTY_D->share_party_exp(killer, pid, exp_reward);
	else killer -> add_exp(exp_reward);
      }

    }

  /* Death traps always generate a ghost. Not anymore, since it could
   * be used in cheating //Frobozz
   * Only players can leave a Ghost.
   * Testplayers must be able to leave a ghost too. //Graah
   * No more Ghosts or Corpses of Guest //Frobozz
   * 26-Jun-97 / Graah: Player ghost is more clever about "killer".
   */
  if ((liv_Flags & F_LIV_IS_PLAYER) && name != "guest"
      && level > random(15) + 5) {
    remains = clone_object("/lib/player_ghost");
    remains->set_name(name); /* ++Tron */

    // Who's got the big idea for name like this??? Let's do it
    // the right way so we won't have corpses with orig name:
    // Ghost of something. ++Tron
    // remains->set_name("ghost of " + capitalize(query_real_name()));
    remains->set_name("ghost");
    remains->set_id(({"ghost of " + capitalize(query_real_name()) , 
			query_real_name() }));
    remains->set_short("Ghost of " + cap_name);
    remains->set_long(
"A terrible, but sad-looking ghost of poor " + cap_name + ". It wails\
 in agony and waits for someone to end its cursed unlife.");
    // remains->set_gender(G_NEUTER);	// Ghosts have no sexlife...
    if (level > 1) remains->set_level(level - 1);
    else remains->set_level(1);
    remains->set_ep(2 * ((int)remains->query_ep() / 3));
    remains->set_hp(max_hp);
    if ((x = query_wc()) > 3) remains->set_wc(x - 2);
    else remains->set_wc(3);
    remains->set_ac(query_ac());

    /* Ghost remembers who killed it */
    if (killer != this_object())
      remains->set_killer((string)killer->query_real_name());

    // Ghost also needs to remember owner / Graah
    remains->set_owner((string)this_object()->query_real_name());

  } else
    // If NPC_NO_CORPSE == 1, never make a corpse. Otherwise, if set,
    // is a percentage change of making a corpse!
    if ((name != "guest")
	&& (!(x = (int)this_object()->query(NPC_NO_CORPSE))
	    || ((x > 1) && (random(101) <= x))))
      {
	string tmp_name;

	if (!(s = query_short(0, this_object()))) s = "Someone";
	remains = clone_object(CORPSE_FILE);
	if (this_object()->is_player_ghost() && sscanf(s, "Ghost of %s", what))
	  {
	    tmp_name = (string)this_object()->query_owner();
	    remains->set_name(what);
	  }
	else {
	  tmp_name = name;
	  remains->set_name(name);
	}
	remains -> set_orig_attributes(
	  (tmp_name ? capitalize(tmp_name) : "Something"),
	  level, query_race(),
	  (((liv_Flags & F_LIV_IS_NPC) && !this_object() -> is_player_ghost())
	   ? 0 : 1));

	// Graah: the corpse fondly remembers its killer.
	// Useful if we make revenants etc
	if (!this_object()->is_player_ghost())
	  remains->set_killer(killer);
	// Player ghost corpse gets the original killer.
	else remains->set_killer(this_object()->query_killer());

	/* Some corpses rot quite quickly! Heh heh... //Graah
	 * And now even fewer, and less rotten...  //Elena
	 * Now there are different decay speeds. 100 (%) is the normal,
	 * below is slower and above is faster 2.1.19994 //Frobozz
	 * 20% chance of speeds from 1 to 5 times faster decaying
	 */
	if (random(100) < 20)
	  remains->set_decay_speed(100 + random(400));
      }

  if (environment())
    {
      if (inv = all_inventory(this_object())
	  && (x = sizeof(inv)))
	{
	  // Corpse mega-carry cheat solution: drop all but wielded and
	  // worn things outside.
	  for (--x; x >= 0; x--) {
	    if (inv[x] && !inv[x]->is_used() && !inv[x]->drop(3))
	      if (inv[x]) move_object(inv[x], environment());
	  }
	}

      if (remains) {
	transfer_all_to(remains);
	move_object(remains, environment());
      } else transfer_all_to(environment());
    }

  // Send pk as argument to second life to prevent stat loss. ++Tron
  // Controlled monster mk is nice: otherwise it's MK but it does
  // not reduce stats. //Graah

  if (!this_object() -> second_life(pk, nicemk, killer,remains))
    destruct(this_object());
  else {
    x = 0;
    while (x < attackers) {
      if (attacker[x] && present(attacker[x], environment()))
	attacker[x]->stop_fight(this_object());
      x++;
    }
    kill_ob = 0;
    attackers = 0;
    attacker = allocate(4);
  }

  if (liv_Flags & F_LIV_IS_PLAYER) {
    if (luck < 0) luck += (random(2) + 1);
    else if (luck > 0) luck -= (random(2) + 1);
    player_kills << 1;	// Player kills are forgotten, albeit slowly.
    SAVE_ME;
    // Let's inform the guild object, in case it wants to know!
    if (GuildFlags & (1 << G_HOOK_DEATH))
      funcall(GuildHooks[G_HOOK_DEATH], this_object(), killer);
  }

  if (remains && living(remains))
    remains->run_away();
}

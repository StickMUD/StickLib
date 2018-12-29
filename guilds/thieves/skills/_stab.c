// 28-Sep-97 / Graah: Can now keep stabbing from hiding, as long as there
// is no target. This way they can "stab someone" until such arrives.

#include <stats.h>
#include <weapon_defs.h>

#include "../tune.h"

inherit "/guilds/thieves/skills/criticals";

// Do not move this before inherits!!!!
#include "../thief.h"

void remove_blade(object tm,object me,object victim,int dam);

#define BASE_PERCENT 65
#define POISON_D "/guilds/thieves/poison_d"
#define BLEED_D "/guilds/thieves/bleed_d"
static int succ, dam, crit, zoppa, sk_stab, guildmaster, advantage;
static int wea_c, sharp, stabstr, poisoned, vac, val, max_dam;
static string myname, tname;
static int pk;

varargs nomask status
stab_cmd(string str, object me, status special_stab)
{
  object tm, *inv, victim, wep, ob;
  string how, hend;
  status cm; 
  int p_stabstr, v_mhp, il;

  if (!me && !(me = this_player())) return 0;
  if (!(tm = (object) me->query_guild_object())) return 0;
  if (!tm->still_thief(0)) return 0;
  if (tm->check_ban(0)) return 1;

  if (!me->query_can_move())
    {
      me->tell_me("You can't move!");
      return 1;
    }

  sk_stab = (int)tm->query_skills(SK_STAB);
  guildmaster = (int)tm->query_guildmaster();
  advantage = (int)tm->query_advantage();

  if (me->query_sp() < (STAB_COST/2) || me->query_fp() < (STAB_COST/2))
    {
      me->tell_me("You feel too tired to stab now...");
      return 1;
    }

  if (!str)
    {
      if (!(victim = (object)me->query_attack()))
	{
	  succ = -1;
	  inv = all_inventory(environment(me));
	  for (dam = sizeof(inv) - 1; dam >= 0; dam--)
	    {
	      if (living(inv[dam]) && inv[dam]->query(LIV_IS_NPC))
		{
		  if (succ != -1)
		    {
		      me->tell_me("Hmm, stab who?");
		      return 0;
		    } 
		  else 
		    succ = dam;
		}
	    }
	  if (succ == -1)
	    {
	      me->tell_me("You can't see any monsters here.");
	      return 0;
	    }

	  victim = inv[succ];

	  /* Autodetect invisible :-(... */
	  if (how = (string) victim->query_short(0, me))
	    me->tell_me(sprintf("(%s)", how));
	  else
	    me->tell_me("(The unseen monster)");
	}
    }
  else 
    victim = present(str, environment(me));

  if (!victim || victim == me)
    {
      me->tell_me("You cannot backstab THAT!");
      return 1;
    }

  if ((string)victim->query_real_name() == "guest")
    {
      me->tell_me("But that is a visitor! You can't stab guests.");
      return 1;
    }

  if (!is_valid_target(victim))
    return 0;

  if (!present(victim, environment(this_player())))
    {
      me->tell_me(sprintf("You can't see anyone called \"%s\" here.", str));
      return 1;
    }

  // Hiding? Now it will end.
  if (tm->query_hidden())
    {
      il = (int)me->query_invis();
      if ((il > 0) && (il < IL_TOTALLY_INVISIBLE)) me->set_invis(IL_VISIBLE);
      tm->set_hidden(0);
    }

  pk = (!victim->is_npc());

  // Let's test PK as it should! -+ Doomdark +-
  if (!victim->attacked_by(me, -1))
    {
      me->tell_me("Player killing is not allowed here.");
      return 1;
    }

  if ((int)tm->query_delay_stab() > 0)
    {
      me->tell_me("You are not that fast.");
      return 1;
    }

  if ((int)tm->query_tth())
    {
      me->tell_me(
	"You try to throw at the same time, but decide just to stab.");
      tm->remove_tth();
    }

  tname = (string)victim->query_name();
  myname = (string)me->query_name();

  if (tm->stab_delay(victim))
    {
      me->tell_me(
	"You can't do that now, " + tname + " knows you are there.");
      return 1;
    }

  me->add_sp(-STAB_COST/2);
  me->add_fp(-STAB_COST/2);

  wep = (object)me->query_weapon();
  if (!wep)
    me->tell_me("You start bashing monsters with your bare hands!");

  if (!wep || wep->query(WP_BLUNT))
    {
      how = "strike";
      sharp = 0;
    }
  else
    {
      how = "stab";
      sharp = 1;
    }

  tm->set_delay_stab(1);

  // Stab-proof livings and items!?
  if (victim->prevent_stab(me) ||
      (ob = present("stab-proof", victim)) &&
      ob->prevent_stab(me)) return 1;

  vac = (int)victim->query_ac();
  val = (int)victim->query_level();
  v_mhp = (int)victim->query_max_hp();

  /* A change: invisibility is now always a successful sneak */
  if (present("paralyzer",victim) ||
      !(cm = (status)victim->query_can_move()) ||
      me->query_invis()) 
    {
      wea_c = (int)me->query_wc();

      // Blunt is worse than sharp
      if (how == "strike") wea_c = (wea_c / 2);

      if (wea_c < 1) wea_c = 1;

      me->set_attacker(victim);

      if (victim->query_no_stab() || _STAB_FAILURE)
	succ = 0;
      else
	{
	  succ = (sk_stab / 90) + random(99);
	  succ -= (val / 2 + vac);
	  if (guildmaster) 
	    succ += 4;
	}

      if (succ > 0)
	{
	  /* Poisoned weapon? */
	  if (wep)
	    poisoned =
	      (wep->is_poisoned() ? (int)wep->query_poison_strength() : 0);
	  else
	    poisoned = 0;

	  // Base stab strength
	  stabstr = (wea_c *
		     (BALANCE_D->damage(sk_stab/10,
					GN_THIEF,
					3 + STAB_COST * 3/2,
					0))/DMG_DIV);

	  // High skill bonus is back, but same as in tth. /Graah
	  if (sk_stab > 10000) 
	    stabstr += ((sk_stab - 10000) / HIGH_SKILL_DIV);

	  me->add_exp(val);

	  // Criticals?
	  crit = critical(sk_stab, guildmaster, advantage, pk);

	  if (crit > 0)
	    {
	      if (crit >= 3)
		{
		  stabstr = (2 * stabstr);
		  while (random(100) < 3)
		    stabstr += (stabstr/4);
		} 
	      else if (crit >= 2)
		{
		  stabstr = (2 * stabstr);
		  crit = 2;
		  if (how == "stab")
		    BLEED_D->add_bleeder(victim,wea_c * 3);
		} 
	      else if (crit >= 1)
		{
		  stabstr = (3 * stabstr / 2);
		  crit = 1;
		  if (how == "stab" && _STAB_BLEED)
		    BLEED_D->add_bleeder(victim,wea_c*2);
		}
	    }

	  dam = stabstr;
	  zoppa = (dam / 30);
	  if (random(100) < 40) zoppa = (zoppa / 2) + random(zoppa);

	  if (zoppa > 12) zoppa = 12;

	  hend = ({ " just barely.", " weakly.", "!",
		    " nicely.",
		    " almost hard.",
		    " quite hard!",
		    " hard!", " very hard!",
		    " extremely hard!", " devastatingly!",
		    " into small fragments!",
		    " into tiny bits of quivering flesh!",
		    " completely into finely minced meat!"
	  })[zoppa];

	  // Blunt
	  if (how == "strike")
	    {
	      environment(me)->tell_here(
		"Someone skillfully smashes " + tname +
		" from behind" + hend, ({ victim, me }), 0);
	      me->tell_me("You smash " + tname +
			  " from behind" + hend);
	      victim->tell_me(
		"Suddenly someone skillfully smashes you on"+
		" the head from behind" +
		hend + " OUCH!!!");

	      // Let them still advance because of the free hit when leaving.
	      // Was just "23000" - this is better. /Graah
	      if (random((v_mhp * 40)) > (sk_stab * 2) &&
		  _STAB_S_RND &&
		  !victim->query(LIV_IS_PLAYER) &&
		  (advantage < 0 ? (advantage == -1 ? random(4) : random(3) ) : 1 )   )
		tm->set_skill(SK_STAB,
			      (int)tm->advance_skill(sk_stab, DIFF_STAB));
	    }
	  else
	    {
	      environment(me)->tell_here(
		"Someone skillfully stabs " + tname +
		" from behind" + hend, ({ victim, me }), 0);
	      me->tell_me("You stab " + tname +
			  " from behind" + hend);
	      victim->tell_me(
		"Suddenly someone skillfully stabs you on from behind" +
		hend + " OUCH!!!");

	      // Let them still advance because of the free hit when leaving
	      // Was just "23000" - this is better. /Graah
	      if (random((v_mhp * 40)) > (sk_stab * 2) &&
		  _STAB_S_RND &&
		  !victim->query(LIV_IS_PLAYER) &&
		  (advantage < 0 ? (advantage == -1 ? random(4) : random(3) ) : 1 )   )
		tm->set_skill(SK_STAB,
			      (int)tm->advance_skill(sk_stab, DIFF_STAB));
	    }

	  if (crit >= 3)
	    {
	      me->tell_me(
	"Your weapon goes right through your opponent, spreading guts and "+
	"blood everywhere! You have done a MIGHTY CRITICAL!"+
	" Your enemy falls down UNCONSCIOUS!");
	      victim->tell_me("Ouch! That really hurt!");
	      victim->set_condition(C_UNCONSCIOUS, 2 + random(8));
	    } 
	  else 
	    if (crit == 2)
	      {
		me->tell_me("A CRITICAL HIT! Total massacre!");
		victim->tell_me("Ouch! That really hurt!");
		victim->set_condition(C_BLIND, 1 + random(6));
	      }
	    else 
	      if (crit == 1)
		{
		  me->tell_me(
	"Wow, that was GREAT hit! You cause superb damage!");
		  victim->tell_me("Ouch! It hurt!");
		  victim->set_condition(C_STUNNED, 1 + random(5));
		}

	  if (val < random(80) &&
	      random(200) < (35 + crit * 6) &&
	      (how == "strike" || random(75) < 35))
	    {
	      if (how != "strike")
		me->tell_me("Ultimate hit!");
	      me->tell_me(tname +
			  " falls down STUNNED!");
	      victim->tell_me("You feel dizzy and are STUNNED!");
	      victim->set_condition(C_STUNNED, 1 + random(5));

	      /* Hope this helps? */
	      me->attack_object(victim,1);
	    }

	  if (dam > 0)
	    {
	      if (_POISON_SAVE((int)victim->query_stat(ST_CON)))
		{
		  if (POISON_D->add_poisoned_living(victim,wep))
		    {
		      me->tell_me("Your poor victim is POISONED!");
		      victim->tell_me("You are poisoned!");
		    }
		}
	      /* Max damages and other fun stuff -Kel*/
	    }
	  if (me->query_coder_level() && !victim->is_npc())
	    me->tell_me("Stab Damage: " + stabstr +"% of " +
			victim->query_max_hp());

	  if (sharp) {
	    dam = dam / 2;
	    tm->stab_victim(({ victim, dam }));
	    remove_blade(tm, me, victim, dam);
	    dam = dam * 2;
	  } else {
	    tm->stab_victim(({ victim, dam }));
	  }
          if(dam > 0  &&  !me->query_coder_level()
              &&  !me->query_testplayer())
            call_other(TOP_TEN,"check_top_damage",0,dam,me);

	  /* NEW: Visibility might strike if non-coder */
	  if (me->query_invis() &&
	      !me->query_coder_level() &&
	      (random(100) > (40 + (int)me->query_level())) &&
	      (random(100) < 10))
	    {
	      if (tm->query_sneak() > 0) 
		tm->stop_sneak();
	    }

	  /* Died? */
	  if (!victim)
	    {
	      if ((int)tm->query_sneak() > 0) 
		tm->stop_sneak();
	      return 1;
	    }
	}
      else
	{
	  switch(random(2))
	    {
	    case 0:
	      me->tell_me("You try to " + how + " " +
			  tname +
			  ", but " + (string)victim->query_pronoun() +
			  " moves and is just scratched.");
	      if (how == "strike")
		victim->tell_me("Something scratches your head.");
	      else
		victim->tell_me("Something scratches your back.");
	      break;
	    default:
	      if (_STAB_FUMBLE)
		FUMBLE_D->fumble(me, victim, wep);
	      else
		{
		  me->tell_me("You stumble and completely miss " +
			      tname + ".");
		  environment(me)->tell_here(
					     myname + " stumbles noisily.",
					     ({ me }), 0);
		}
	      if (tm->query_sneak() > 0) 
		tm->stop_sneak();
	      break;
	    }

	  /* Notice? */
	  if (((me->query_invis() ? random(40) : 0) + random(40))
	      < (val - advantage))
	    {
	      victim->tell_me(
	"You notice that " + myname + " tried to stab you!");
	      me->tell_me("Uh-oh..." + (string)victim->query_pronoun() +
			  " noticed your attempt!");
	      victim->attack_object(me);
	    }

	  // Doesn't train on very small monsters. /Graah
	  if (!victim->query_no_stab() &&
	      (v_mhp * 40 > sk_stab) &&
	      _STAB_F_RND &&
	      (advantage < 0 ? (advantage == -1 ? random(4) : random(3) ) : 1 )   )
	    tm->set_skill(SK_STAB,
			  (int)tm->advance_skill(sk_stab, DIFF_STAB / 2));
	}
    }
  else
    {  
      me->tell_me("Your victim was aware of you! You missed!");
      me->set_attacker(victim);
    }

  if (tm->query_sneak() > 0) 
    tm->stop_sneak();

  if (!victim) 
    return 1; /* We killed it already */

  if (victim)
    me->attack_object(victim, 1);

  /* The victim strikes back unless paralyzed or immobile */
  if (cm)
    {
      victim->attack_object(me);
    }

  return 1;
}

void
remove_blade(object tm, object me, object victim, int dam)
{
  string tmp;

  if (!me || !tm) return;

  if (!victim || (int)victim->query_dead() ) {
    me->tell_me("You remove your weapon from the corpse.");
    return;
  } else {
    me->tell_me(sprintf(
			"You pull your weapon out of %s's back.",
			(string)victim->query_name() ));
  }

  victim->tell_me(sprintf(
			  "%s pulls %s weapon out of your back.",
			  (string)me->query_name(),
			  (string)me->query_possessive()));
  tm->stab_victim(({ victim, dam }));

  return;
}

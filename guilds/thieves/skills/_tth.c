#define BLEED_D "/guilds/thieves/bleed_d"
#define POISON_D "/guilds/thieves/poison_d"

#include "../tune.h"

inherit "/guilds/thieves/skills/criticals";

#ifndef _THIEF_H
#include "../thief.h"
#endif
static object tm, item, target;
static string tmp1, tmp2, hde;
static string msg1, msg2, msg3;
static int dam, succ, fiery, sk_throw, guildmaster, crit;
static string myname;
static int pk;

//Let's check if we are able to throw //F 2.8.1995
status
check_free_hands(object weapon, object owner)
{
  object wielded_weapon;
   
  if ((wielded_weapon=(object)owner->query_weapon())==weapon)
    return 1;

  if (wielded_weapon) 
    {
      if ((status)wielded_weapon->is_two_handed()
	  && !(wielded_weapon->id("demon_2H_sword")
	       &&
	 (int)(owner->query_guild_object())->query_guildmaster() == TR_DEMON)
	  && !(wielded_weapon->id("slayer") &&
	       present("gargamel",environment(owner))))
	{
	  owner->tell_me("You cannot throw it while wielding a two-handed weapon!");
	  return 0;
	}

      if ((object)owner->is_wearing("shield")) 
	{
	  owner->tell_me("You cannot throw it while wielding another weapon and wearing a shield!");
	  return 0;
	}
    }

  return 1;
}

status
tth_cmd(string arg, object owner)
{
  int race_bon,tthcost;

  if (!(tm = previous_object()) || !tm->id("tmark"))
    {
      notify_fail("You don't know how to do that.\n");
      return 0;
    }

  tm->set_tth(0);
  arg = lower_case(arg);

  if (sscanf(arg, "%s at %s", tmp1, tmp2) != 2)
    {
      tmp1 = arg; tmp2 = 0;
    }
    
  item = present(tmp1, owner);

  if (!item)
    {
      owner->tell_me("You do not carry any \"" +
		     capitalize(tmp1) + "\".");
      return 1;
    }

  if (item->short())
    tmp1 = (string)item->short();
  else
    tmp1 = capitalize(tmp1);

  if (!can_throw(owner, item)) 
    return 1;

  if (!check_free_hands(item,owner)) 
    return 1;

  if (!owner->query_can_move())
    {
      owner->tell_me("You can't move!");
      return 1;
    }

  // Can throw any shit you want, but only weapons train. /Graah
#if 0
  // Can only throw weapons from now. / Graah
  if (!item->is_weapon())
    {
      owner->tell_me(capitalize(tmp1) +
		     " does not resemble a weapon enough to be thrown.");
      return 1;
    }
#endif

  if (item->query_wielded())
    owner->unwield(item, 0);
   
  if (item->drop())
    {
      owner->tell_me("You try to throw " + tmp1 +
		     ", but find it impossible.");
      return 1;
    }

  if (!tmp2)
    {
      target = (object)owner->query_attack();
    }
  else
    target = present(tmp2, environment(owner));

  if (!target || (!present(target, environment(owner))))
    {
      if (!tmp2) 
	owner->tell_me("Your opponent isn't here.");
      else 
	owner->tell_me("Who's that \"" + capitalize(tmp2) + "\"?");

      return 1;
    }

  if (!is_valid_target(target))
    return 0;

  tmp1 = (string)item->query_name();
  if (!tmp1) 
    tmp1 = (string)item->short();

  if (!tmp1) 
    tmp1 = "Something";

  tmp2 = (string)target->query_name();
  if (!tmp2) tmp2 = (string)target->short();
  if (!tmp2) tmp2 = "Someone";

  item->throw();

  sk_throw = (int)tm->query_skills(SK_TTH);
  tthcost = THROW_COST + sk_throw/1000;

  if ((int)owner->query_fp() < tthcost)
    {
      owner->tell_me("You are too exhausted.");
      return 1;
    }

  if (target == owner)
    {
      owner->tell_me("Just before you throw, you remember that \
suicide is not the answer.");
      return 1;
    }

  if (living(target) &&
      !owner->query_testplayer() &&
      !owner->is_npc() &&
      !target->is_npc() &&
      !target->attacked_by(owner,-1))
    {
      owner->tell_me(
"Before you throw it, a small warning nymph arrives in a puff of smoke and \
says: \"Player killing is not allowed in this area. Have a nice day.\" \
The nymph then disappears, leaving you to curse the stupid pk-rules.");
      return 1;
    }

  if ((string)target->query_guild() == "healer") return 1;

  owner->add_fp(-tthcost);

  if (item->id("flask of oil")) 
    fiery = 1;
  else 
    fiery = 0;

  guildmaster = (int)tm->query_guildmaster();
  race_bon = (int) tm->query_adv();
  race_bon += race_bon < 0 ? -2 : 1;
  if ((int)tm->query_adv() <= 0)
    race_bon *= 2;

  pk = (!target->is_npc());

  myname = (string)owner->query_name();

  hde = " " + ({ "hurl", "throw", "sling", "cast", "fling" })[random(5)];

  owner->tell_me("Ok. You" + hde + " " + tmp1 + " at " + tmp2 + ".");
  environment(owner)->tell_here(myname + hde + "s " +
		    tmp1 + " at " + tmp2 + ".", ({ target, owner }));
  target->tell_me(myname +
		  hde + "s " + tmp1 + " at you!\n");

  /* Now based on thief dex... considers mobility to throw. -Kel */
  succ = ((sk_throw / _TTH_SK_DIV) + 5 +
	  ((int)tm->thief_dex() / 2) +
	  (int)tm->query_advantage() * 2 +
	  random((sk_throw / _TTH_SK_RND_DIV) + 9));

   if (target->query_can_move())
     {
       succ -= (((int)target->query_ac() / 5) +
		random(((int)target->query_dex() / 2) + 10));
     }

   if (guildmaster)
     succ += 2;

   if (succ <= 0 || random(100) < 5)
     {
       switch(random(4))
      {
      case 0:
	 msg1 = "It misses.";
	 msg2 = "It misses you.";
	 break;
      case 1:
	 msg1 = ({ "A good", "A remarkable", "An amazing",
		      "An accurate" })[random(4)] + " throw, but ";
	 msg2 = msg1 + "you dodge it.";
	 msg1 = msg1 + tmp2 + " dodges it.";
	 break;
      case 2:
	 if (random(100) < 20)
	 {
	      /* Fumble daemon - slowly all fumbles should be there */
	    FUMBLE_D->fumble(owner, target, item);
	    msg1 = "";
	    break;
	 }
	  /* Zorlock's suggestions */
	 msg1 = ({ "Maybe next time eh?",
		     "The object is to hit the opponent!",
		     "Lets try again shall we?",
		     "Lets recap what we want to accomplish here.",
		     "Not even close!",
		     "HOW HARD CAN THIS BE?!?!",
		     "We're obviously not getting the point here.",
		     "And it might hit! But it doesn't.",
		     "Just a bit outside." })[random(9)];
	 msg2 = myname +
	    ({
	      " blushes. What a spectacular failure!",
	      "'s ridiculous fumble lands somewhere.",
	      "'s futile attempt doesn't go anywhere near the foe.",
	      "'s throw is a total failure.",
	      "'s offense goes 180 degrees wrong." })[random(5)];
	 break;
      default:
	 msg1 = msg2 = "It misses by " +
	    ({ "a mile.", "an inch.", "a foot.", "two miles!",
		 "six leagues!!!",
		 "a league!" })[random(6)];
	 break;
      }

      if (msg1 != "")
      {
	 environment(owner)->tell_here(msg1, ({ target }), 0);
	 target->tell_me(msg2);
      }
      target->attack_object(owner);

      /* We slowly learn from our mistakes */
      if (item->is_weapon() &&
	  ((random(10) < 9) || (random(40000) > (sk_throw * 2))) &&
	  ( race_bon < 0 ? random(4) : 1 )   )
	 tm->set_skill(SK_TTH,(int)tm->advance_skill(sk_throw, DIFF_THROW));
   }
   else
   {
      if (fiery)
	 dam = 40 + random(15);
      else
      {   
// Nope. Balance daemon checks it and it might actually require it,
// if we start using the official criticals rules. /Graah
//      if(sk_throw > 10000) sk_throw = 10000;

	if (!(dam = (int)item->query_wc()))
	  {
	    dam = ((int)item->query_weight() * 4 + 2);
	    if (dam < 7) dam = 7;

	    // blunt does less dmg -colt
	    if (item->query_blunt())
	      dam = dam / 2;
	  }
	else
	  {
	  // Damage balancing: Thieves must first succeed in hit roll,
	  // which depends on opponent's ac and dex. Thus, double used
	  // points. / Graah
	  dam = (dam * BALANCE->damage(sk_throw / 10,
				       GN_THIEF,
				       tthcost * 2,
                                      0)) / DMG_DIV;

	  // Less with sucky skills (down to 75% with real suckers)
	  if (sk_throw < 10000)
	    dam = (dam * (100 - ((10000 - sk_throw) / 400))) / 100;
	  }
      }

   if (dam < 0) dam = 0;

   // High skill bonus
   if (sk_throw > 10000)
     dam += random(15) + random((sk_throw - 10000) / HIGH_SKILL_DIV);

   // Hits snake into left arm... but who cares...
   msg3 = capitalize(tmp1) + " " +
     ({
       "hits", "strikes", "bashes", "scratches", "smashes",
	 "punches", "pokes" })
     [random(7)] +
       " " + tmp2 +
       ({ "", "",
	    " painfully", " hard", " straight",
	    " directly", " badly",
	    " gruesomely", " brutally",
	    " ferociously", " ruthlessly",
	    " wickedly", " heavily",
	    " cruelly", " diabolically",
	    " dreadfully", " ghastly",
	    " awfully", " nicely",
	    " mortally woundingly",
	    " devastatingly" })
       [random(21)] +
	 " to " +
	 target->query_possessive() + " " +
	 ({ "head",
	      "ribs", "torso", "stomach", "left arm", "right arm", "left leg",
	      "right leg", "family jewels", "left foot", "right foot",
	      "left cheek", "right cheek", "left eye", "right eye",
	      "left knee", "right knee", "jaw", "teeth", "nose",
	      "left side", "right side", "groin", "forehead" })[random(24)] +
								 "!";

     environment(owner)->tell_here(msg3, 0, 0);

     if (fiery)
       {
	 environment(owner)->tell_here(capitalize(tmp1) +
	   " bursts into flames!", ({ target }), 0);
	 target->tell_me(capitalize(tmp1) +
			 " bursts into flames and burns you!");
       }
     else 
       target->tell_me(({
	 "Ouch! It hits you Hard!",
	   "You are hit!" })[random(2)]);

     crit = critical(sk_throw, guildmaster, race_bon, pk);

     // Criticals
     if (living(target) && (crit > 0))
       {
	 if (!fiery)
	   {
	     if (random(100) < (16 + race_bon))
	       {
		 owner->tell_me(tmp2 +
				" is hit on the head and STUNNED!");
		 target->tell_me("OOUCH! It hit your head! You are STUNNED!");
		 target->add_condition(C_STUNNED, 3 + crit + random(4));
		 owner->attack_object(target);
	       }
	     else 
	       {
		 if (crit <= 2)
		   {
		     owner->tell_me(
			"Wow, that was a GREAT hit! Superb damage!");
		     dam = dam * 3/2;
		     call_other(GDIR + "top_ten", "check_top_damage",
				1, dam, owner);
		   }
		 else
		   {
		     /* Rare! */
		     owner->tell_me("What a shot! It's a CRITICAL hit!");
		     target->tell_me("OOOOUUUUCHHH! It HURTS! AARGH!");
		     dam = dam * 3;
		     call_other(GDIR + "top_ten", "check_top_damage",
				1, dam, owner);
		   }
	       }
	   }
	 else
	   {
	     /* Fiery critical! */
	     /* Nothing for now...later it'll ignite things? */
	     /* ...in global throw skill at least. */
	   }
       }

     /* Possibility weapon doesn't return..if blunt it bounces off
	enemy, if sharp it sticks.  Sharp's can poison and/or cause
	bleeding, blunts can stun -Colt */
     if ((crit < 1) && _TTH_WPN_STUCK)
       {
	 if (!item->query_wc() || item->query_blunt())
	   {
	     owner->tell_me(
	       "ARGH!! Your blunt weapon bounces off your target at \
a weird angle and doesn't return to you!");
	     environment(owner)->tell_here(myname + "'s \
weapon bounces of " + owner->query_possessive() + " enemy at an unusual \
angle and doesn't return.", ({ owner, target }));
	     target->tell_me(myname + "'s weapon bounces off \
you at an unusual angle and doesn't return to " + myname);
	    
	     if (sk_throw > 6200 && random(sk_throw) > 5000 + random(8000))
	       {
		 owner->tell_me("Luckily you put your heart into that throw \
and " + tmp2 + " is stunned!");
		 environment(owner)->tell_here(tmp2 + 
	       " appears to have been hit hard by this errant throw and \
is quite stunned!", ({ target, owner }));
		 target->tell_me("WOW! " + myname + " had some \
good stuff on that throw! You are definitely stunned!");
		 target->set_condition(C_STUNNED, 4 + random(4));
		 owner->attack_object(target,1);
	       }
	     owner->add_weight(-(item->query_weight()));
	     move_object(item,environment(owner));
	   }
	 else
	   {
	     owner->tell_me(
"ACK!! To your surprise, the weapon you just threw \
stuck in " + tmp2 + " and didn't return!");
	     environment(owner)->tell_here(myname + "'s weapon \
amazingly sticks in " + tmp2 + " and doesn't return to " +
	       myname, ({ owner, target }));
	     target->tell_me("OUCH! " + myname + "'s weapon \
just stuck right into you instead of returning.");
	     if (POISON_D->add_poisoned_living(target,item))
	       {
		 owner->tell_me(
		   "It looks like " + tmp2 + " is \
poisoned!");
		 target->tell_me("ACK! You feel poisoned!");
	       }
	     if (sk_throw > 5200 && random(sk_throw) > 5000 + random(8000))
	       {
		 owner->tell_me("It looks like " + tmp2 +
				" is bleeding profusely from his wound!");
		 environment(owner)->tell_here(tmp2 + " is \
bleeding terribly from the wound!", ({ target, owner }));
		 target->tell_me("UH OH!  The wound is bleeding horribly!");
		 BLEED_D->add_bleeder(target,(int)item->query_wc() * 2);
	       }
	     owner->add_weight(-(item->query_weight()));
	     move_object(item, target);
	   }
       }

     // Ignores _some_ of foe's armour with thief throw
     tm->hit_victim(({target, ((int)target->query_ac() / 2) + dam }));

     /* Fiery thing is destroyed if it did hit */
     if (fiery)
       {
	 owner->add_weight(-((int)item->query_weight()));
	 destruct(item);
       }

     // Learn while doing it...but very sloowwlllyyyyyyy...
     if (item && item->is_weapon() &&
	 (random(12000) > (sk_throw * 2)) &&
	 _TTH_T_RND &&
	 ( race_bon < 0 ? random(4) : 1 )   )
       {
	 tm->set_skill(SK_TTH,(int)tm->advance_skill(sk_throw, 10000));
       }

   }

   return 1;
}

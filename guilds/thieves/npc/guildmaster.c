/* The Guildmaster */
/* REALLY tough! */
#include <npc.h>
#include <key_defs.h>

#define LOG_GM_STEAL

object key;

/* To do: collect taxes from thieves and place them in the temple
 * treasury.
 * use query_create_room to get him back to master's room... */

void
create_monster()
{
    set_heart_beat(1);
    set_name("guildmaster");
	set_short("The Guildmaster");
	set_long("He looks very skilled. I suggest you hold your things.");
    set_id("master");
    set_race("thief");
    set_level(17);
    set_hp(530);
	set_money(60 + random(100) + random(50));
	set_alignment(AL_NASTY);
//	set(NPC_NOT_AGGRESSIVE);
// set_wimpy();
    set_wc(15);
    set_ac(18);
/*    set_dead_ob(this_object()); */

    /* Create the Guild Key */
	key = clone_object(KEY_FILE);
	key -> set_type("metal");
	key -> set_code("thgld");
    move_object(key,this_object());
}

heart_beat()
{
    ::heart_beat();
    if(random(3) == 0) steal();
}

steal()
{
   object ob, who;
   int weight;

   who = this_player();

   /* Doesn't succeed always, and not against wizards */
   if(random(60) < who->query_level() ||
      who->query_coder_level()) return;

   /* But when he does it, he does it well. */
   while(who) {
      if(who != this_object() && living(who)) {
         ob = first_inventory(who);

       while(ob) {
          if(!ob->id("guild_mark") && !ob->query_auto_load() &&
             ob->query_value() && !ob->living() &&
             !ob->can_put_and_get() && !ob->drop())
          {
            weight = call_other(ob, "query_weight");
            if(add_weight(weight) && random(40) > who->query_level())
            {
               call_other(who, "add_weight", -weight);
               move_object(ob, this_object());

#ifdef LOG_GM_STEAL
          log_file("thief_guild","Guildmaster got " + ob->short() + " from " +
                   who->query_real_name() + " at " + ctime(time()) + "\n");
            }
#endif
          }
          ob = next_inventory(ob);
       }
      }
      who = next_inventory(who);
   }
}

#if 0
monster_died(ob) {
   if(attacker_ob)
      log_file("THIEF_ENEMIES",attacker_ob->query_real_name() +
         " (prim) at " + ctime(time()) + "\n");
   if(alt_attacker_ob)
      log_file("THIEF_ENEMIES",alt_attacker_ob->query_real_name() +
         " (alt) at " + ctime(time()) + "\n");
   if(attacker_ob)
   if(!attacker_ob->query_quests("M20")) {
      tell_object(attacker_ob,"You have solved a tough minor quest!\n"+
         "You feel more experienced.\n");
      attacker_ob->add_exp(5000);
      attacker_ob->set_quest("M20");
   }
   if(alt_attacker_ob)
   if(!alt_attacker_ob->query_quests("M20")) {
      tell_object(alt_attacker_ob,"You have solved a tough minor quest!\n"+
         "You feel more experienced.\n");
      alt_attacker_ob->add_exp(5000);
      alt_attacker_ob->set_quest("M20");
   }
   return 0;
}
#endif

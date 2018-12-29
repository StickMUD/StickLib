#include <npc.h>
#include <guild.h>

int have_gold, have_rope;

void create_monster()
{
  set_name("thief");
  set_race("hobbit");
  set_id(({ "old thief" }));
  set_short("An old thief is here, taking a shower");
  set_long("From the looks of him, this guy's been around \
a long time.  I bet he knows a secret or two!");
  set_level(15);
  set_guild("thief");
  set_ep(1);
  experience = 1;
  set_gender(G_MALE);
  load_chat(8, ({
    "The Old Thief exclaims: Ahh, the good old days, I could \
tell you a story or two!",
      "The Old Thief asks: Hand me the soap would ya?",
      "The Old Thief taunts: I know something you dont know!  \
NEENER NEENER!",
      "The Old Thief asks: Got any Geritol buddy?",
      "The Old Thief slips and falls.  OUCH!!",
      }));
  load_a_chat(10, ({
    "Old Thief says: ACK!  Can't a guy take a shower in peace!?",
      "The Thief hits you in the face with a bar of soap!",
      "The Old Thief says: I'll kill you!!  Soon as I find my glasses!",
      "The Old Thief says: Go ahead, kill me!  You'll never learn the \
secret!",
      }));
}

status
extra_aggressive(object who)
{
  if(((string)who->query_guild() != GN_THIEF) && (!who->query_coder_level()))
    {
      environment()->tell_here("The Old Thief shouts: AHHH, \
INTRUDER IN THE GUILD!! KILL KILL KILL!!");
      return 1;
    }
  return 0;
}

status
catch_ask(object who, string what)
{
  if (what != "secret")
    {
    environment()->tell_here("The Old Thief replies: Have you lost \
your marbles!  I don't know anything about that.\nThe Old Thief \
mumbles something about being old and still having his marbles.");
   }
   else 
      {
    environment()->tell_here("The Old Thief says: Ahhh yes, I bet you'd \
like to know my secret!  I've promised never to tell, but bring \
me a bar of gold and a rope and I will make a very valuable \
item for you!");
   }
   return 1;
}
void give_object_by(object who, string what)
{
  object *inv, oba;
  int i;

  if ((!what->id("goldbar")) && (!what->id("rope")))
    {
      environment()->tell_here("Hmm, I didn't ask for that!  \
This is useless!");
      move_object(what, environment(this_object()));
      return;
    }
   
  if ((string) what->query_name() == "goldbar")
      {
      have_gold = 1;
     environment()->tell_here("The Old Thief exclaims: Aha, the \
gold!");
   }
  if (what->id("rope"))
      {
     have_rope = 1;
    environment()->tell_here("The Old Thief exclaims: Great! A \
rope!");
   }
  if ((have_gold == 1) && (have_rope == 0))
      {
   environment()->tell_here("The Old Thief says: Now get me a rope!");
   }
  else if ((have_gold == 0) && (have_rope == 1))
      {
      environment()->tell_here("The Old Thief says: Now find me a \
gold bar!");
   }
  else if ((have_gold == 1) && (have_rope == 1))
      {
    environment()->tell_here("The Old Thief says: Woo hoo!  I have \
the gold bar and the rope.  Now don't peek, I'll make you \
something neat!");
  environment()->tell_here("The Old Thief turns his back to you and \
fiddles with the rope and gold bar.  After a second or two he \
turns around and hands you an odd golden dagger.");
    oba = clone_object("/guilds/thieves/colt/dagger.c");
     move_object(oba, who);
 inv = all_inventory(this_object());
  for(i=0;i < sizeof(inv);i++)
  {
   destruct(inv[i]);
  }
  have_rope = 0;
  have_gold = 0;
    environment()->tell_here("The Old Thief says: This will help \
you in your battles!");
   }
}

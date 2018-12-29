// Ancient old stupid item.

#include <treasure.h>

int drunk;

void
create_treasure()
{
	set_long("Quaff potion if you want your battle scars removed.");
   set_name ("potion");
   set_id("potion of remove scars");

   set_value(35000);
   set_weight(1);
}
id(str) { return (!drunk && ::id(str)); }
short() {
  if(!drunk) return "Potion of Remove Scars";
  else return;
}
init() {
  add_action("quaff","quaff");
  add_action("quaff","drink");
}
quaff(str) {
  if(!id(str) || drunk) return;
  write("Are you ABSOLUTELY sure you want to quaff it (y/n)? ");
  input_to("quaff2");
  return 1;
}
quaff2(str) {
  write("\n");
  if(!str || (str!="y" && str!="Y")) return 1;
  drunk=1;
  write("You quaff the potion...\n");
  say(this_player()->query_name()+" quaffs the potion.\n"+this_player()->query_name()+" looks nervous.\n",this_player());
  call_out("quaff3",10,this_player());
  return 1;
}
quaff3(ob) {
  int n,i;
  remove_call_out("quaff3");
  n=random(101);
  if(n>=25) {
    ob->clear_scar();
    tell_object(ob,"And you feel your scars removed!\n");
    tell_object(ob,"You feel a sudden urge to shout it aloud!\n");
    ob->force_us("shout IT WORKS!");
  }
  else if(n>=10) {
    tell_object(ob,"And you feel your scars removed!\n");
    tell_object(ob,"You feel a sudden urge to shout it aloud!\n");
    ob->force_us("shout NOTHING HAPPENED!");
  }
  else {
    n=random(5)+1;
    for(i=n;i;i--)
      ob->make_scar();
    tell_object(ob,"Something went wrong!\n");
    tell_object(ob,"You feel a sudden urge to shout it aloud!\n");
    ob->force_us("shout IT FAILED!");
  }
  destruct(this_object());
  return 1;
}

// _send.c
// 110497 -- Haw

#include <cmd.h>
//#include "/players/hawthorn/bin/find.c"

status
send_cmd(string str)
{
   object ob1, ob2;
   string src, dst;

   if(!CHECK)
      return 0;
   if(!str || sscanf(str, "%s %s", src, dst) != 2)
      return notify_fail("Send what where?\n"),0;
   if(!(ob1 = find_object(src)))
      return notify_fail("Object 1 not found.\n"),0;
   if(!(ob2 = find_object(dst)))
      return notify_fail("Object 2 not found.\n"),0;
   this_player()->tell_me(sprintf("Send: OBJ(%s) -> OBJ(%s).",
	object_name(ob1), file_name(ob2)));
   move_object(ob1, ob2);
   this_player()->tell_me("Ok.");
   return 1;
}

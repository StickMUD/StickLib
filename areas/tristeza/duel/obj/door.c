/*
 
  Compability for
  Mapmaster's object
  loader.
 
*/
 
/* Rather obsolete, should be removed. -+ Doomdark 28-Aug-98 +- */

#include <door.h> 
 
set_all(str) 
{
 if (!str)
   return 0;
 if ( sscanf( str, "%s %s %s %d %d %d", direction, key_type, 
             key_code, is_closed, is_locked, can_lock) == 6 ) {
   if( key_type == "0")
     key_type = 0;
   if(is_closed) environment()->set_block(direction, "Door");
   return 1;
 }
 return 0;
}
 
 go_door()
 {
   string str;
 
 
   if ( is_closed ) {
     write("You can't do that, the door is closed.\n");
     return 1;
   }
 
   if (partner_door) {
     str = call_other( this_player(), "query_name", 0);
     call_other( partner_door, "player_enters", str);
     write( "You go through the " + direction + " door.\n");
   }
   return 0;
                
 }
    
set_closed(val) {
 
  if(val) environment()->set_block(direction, "Door");
  else environment()->del_block(direction);
::set_closed(val);
}

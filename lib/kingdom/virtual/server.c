// Virtual server for rooms in the castles
//
// V1.0 17-sep-95 by Doomdark
// V1.1 25-sep-95 by Kelgath
// V1.2 20-dec-95 by Minx 

#include "../kingdoms.h"


object *rooms;

void
create(){
        rooms = ({ });
}

void
reset(){
        rooms -= ({ 0 });
}

object
compile_object(string fname){

string kingdom,owner,name;
string tmp;
object room;

//Filename: /lib/kingdoms/virtual/kingdom_name/castle_owner/room_name
        if(sscanf(fname,
	"%s/kingdom/virtual/%s/%s/%s.c",tmp,kingdom,owner,name) <4){
                return 0;
        }

        room = clone_object(VIRTUAL+kingdom+"/"+owner+"/base_room");
	room->castle_setup(kingdom, owner, name);
        rooms += ({ room });
        return room;
}

object *
query_rooms()
{
        rooms -= ({ 0 });
        return rooms;
} 

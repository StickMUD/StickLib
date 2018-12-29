
mapping	roomQueue;
mixed   *destrQueue;

nomask void
create() {

   destrQueue = ({ });
   roomQueue = ([ ]);
   call_out("_vroomUpdate", 300);
}

nomask void
_vroomDestructSequence(mixed data) {
int i;

   if(data) {
      if(!sizeof(destrQueue)) call_out("_vroomDestructSequence", 2, 0);
      destrQueue += ({ data });
      return;
   }

   if(i = sizeof(destrQueue)) call_out("_vroomDestructSequence", 2, 0);
   else return;

   while(i--) {
      int j;

      data = destrQueue[i][1];
      data -= ({ 0 });

      j = sizeof(data);
      while(j--) destruct(data[j]);

      destrQueue[i] = 0;
   }
   destrQueue = ({ });
   remove_call_out("_vroomDestructSequence");
}

nomask void
_vroomUpdate() {
mixed obs;
mixed args;
int i;

   obs = m_indices(roomQueue);
   args = m_values(roomQueue);

   i = sizeof(roomQueue);
   while(i--) {
      if(!obs[i]) _vroomDestructSequence(args[i]);
      else args[i][1] -= ({ 0 });
   }
   roomQueue = m_delete(roomQueue, 0);         

   if(m_sizeof(roomQueue)) call_out("_vroomUpdate", 300);
}

nomask int
vRoomStoreRoom(object room) {
mixed data;

   if(!room) return 0;

   data = roomQueue[previous_object()];
   if(!data) {
      if(!m_sizeof(roomQueue)) call_out("_vroomUpdate", 300);
      roomQueue += ([ previous_object() : 
                    ({ file_name(previous_object()), ({ room }) }) ]);
   }
   else data[1] += ({ room });
    
   return 1;     
}

nomask int
vRoomDeleteRooms() {
mixed data;

   data = roomQueue[previous_object()];
   if(data) {
     _vroomDestructSequence(data);
     roomQueue = m_delete(roomQueue, previous_object());
   }
   else return 0;

   return 1;
}

nomask int
vRoomQueryStaleRooms() {
int i;
string name;

   name = file_name(previous_object());
   i = sizeof(destrQueue);
   while(i--) {
      if(destrQueue[i][0] == name) return 1;
   }
   return 0;
}

mixed *
query_stale_rooms() { return destrQueue; }

mapping
query_room_queue() { return roomQueue; }


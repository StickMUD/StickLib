 /*****************************************************
 
  NAME:    virtual_room.c
 
  PURPOSE: This room code is used in virtual room
           system. Room code is compatible with
           /lib/room.c with some exeptions. Study
           files in /doc/virtual directory for more
           info about virtual rooms and setting up
           your own virtual environment.
 
  AUTHOR:  Tron

  CHANGES:
        14-nov-95: Tried to add compatibility with
            /lib/room.c; especially when moving.
            -+ Doomdark +-
******************************************************/
 
#include <nroom.h>
#include <player_defs.h>
#include <daemons.h>
 
#define	F_VIRTUAL_INIT		1
#define F_VIRTUAL_HB_ON         2
#define F_VIRTUAL_HB_ENABLED      4
#define	F_CREATE_CALLED		8

private string room_map;        // Map of environment.
private string objects;          // Temporary object list.
private mixed  obj_list;         // Active object creation list.
private string rooms_home;       // Home for your virtual rooms.
private string descs_home;       // Home for specific descriptions.
private string server_ob;        // Our server.
private string *random_objects;  // Temporary random object list.
private string *effects;         // List of effects in this terrain.
private int    effect_chance;    // Chance of effects to appear.
private string beat_function;    // Your own beat function in HB.
private string moving_command;   // Special moving command.
private string moving_msgout;    // Special leaving message.
private string moving_msgin;     // Special arriving message.
private status moving_flag;      // 1 if players moves with special command.
private status distant_look;     // Flag for seeing in other rooms.
 
// private status hb;               // 1 if heart_beat is running.
// private int    hb_enabled;       // 1 if heart_beat is available.
private status read;             // 1 if special description has been read.
 
int X, Y;                        // Coordinates of this room.
static int virtualFlags;	// Used to hold some useful info
 
 
/******************************************
 
 DESC: Inform that this is a virtual room
 
 RET:  Returns constant value 1.
 
*******************************************/
 
nomask status
query_virtual_room() { return 1; }
 
/******************************************
 
 DESC: Deliver coordinates of this room.
 
 RET:  Returns mixed array.
 
*******************************************/
 
nomask mixed
query_coordinates() { return ({ X, Y }); }
 
/*************************************************
 
 DESC: Reset objects set for this room.
 
 ARG:  st  - 0	Don't decode objects to obj_list
	     1  Decode objects from string.
 
**************************************************/
 
nomask static void
reset_objects(int st) {
 
string *list;
int i, n;
object ob;
string objec;
string fun, arg;
string tail;
mixed a;
mixed b;
 
   if(st) obj_list = 0;
 
   if(!obj_list) {
      if(!objects) return;
      obj_list = ({});
      list = explode(objects, "@");
      objects = 0;
 
      for(i=sizeof(list)-1; i>-1 ; i--) {
           if(sscanf(list[i], "%d:%s", n, tail)!=2) continue;
           while(n) {
              obj_list += ({ 0, tail });
              n--;
            }                             
        }
     } 
 
   while((n=member_array(0, obj_list))!=-1) {
       tail = obj_list[n+1];
       if(sscanf(tail, "%s:%s:%s", objec, fun, arg)!=3)
          if(sscanf(tail, "%s:%s", objec, fun)!=2)
             if(sscanf(tail,"%s", objec)!=1) continue;
     
       ob = clone_object(objec);
       move_object(ob, this_object());
       if(fun) call_other(ob, fun, arg);
       obj_list[n] = ob;
    }
}
 
/******************************************
 
 DESC: Resets random objects when room is
       created.
 
*******************************************/
 
nomask static void
reset_random_objects() {
object ob;
int i, n, chance, siz;
string tail, objec, tmp;
string fun, arg;
 
   if(!random_objects) return;
   siz = sizeof(random_objects);
 
   for(i=0; i < siz; i++) {
       if(sscanf(random_objects[i], "%d:%s", n, tail) != 2) continue;
      
       if(sscanf(tail, "%d:%s:%s:%s", chance, objec, fun, arg)!=4)
         if(sscanf(tail, "%d:%s:%s", chance, objec, fun) !=3)
           if(sscanf(tail, "%d:%s", chance, objec)!=2) continue;
                
       if(random(100) < chance) {
           while(n--) {
             ob = clone_object(objec);
             move_object(ob, this_object());
             if(fun) call_other(ob, fun, arg);
	    }
         }
      } // FOR
 
  random_objects = 0;
}
 
/******************************************
 
 DESC: Gets server filename from according
       to the room file name.
 
 RET:  Returns server filename.
 
*******************************************/
 
nomask static string
get_server() {
string name;
string tmp;
 
   if(sscanf(file_name(this_object()), "%s/virtual/%s", name, tmp) == 2)
       return name+"/virtual/server";
   else return 0;
}
                 
/******************************************
 
 DESC: Creates the room. Loads objects and
       random objects and calls coder's
       create function.
 
*******************************************/
 
nomask static void
create_room() {
 
mixed  *tmp;
 
	if (function_exists("init_virtual"))
		virtualFlags |= F_VIRTUAL_INIT;
   if(server_ob = get_server()) {
       if(tmp = (mixed *)server_ob->query_virtual_room_server())
	 { 
           X = tmp[0];
           Y = tmp[1];
           objects = tmp[2];
           descs_home = tmp[3];
           rooms_home = tmp[4];
	 }  
 
        else server_ob = 0;
     }
 
	if (!(virtualFlags & F_CREATE_CALLED)) {
		virtualFlags |= F_CREATE_CALLED;
		this_object()->create_virtual();
	}
	add_command("terrain", "show_terrain");
 
   obj_list = 0;
   reset_objects(1);
   reset_random_objects();
 }

/***********************************************
 DESCRIPTION: Perform init.
 ***********************************************/

nomask static void
init_room()
{
	if (virtualFlags & F_VIRTUAL_INIT)
		this_object()->init_virtual();

   if(moving_command) {
      add_action("virtual_move", moving_command);
   }

   if (!(virtualFlags & F_VIRTUAL_HB_ENABLED) || 
        (virtualFlags & F_VIRTUAL_HB_ON)) return;

   virtualFlags |= F_VIRTUAL_HB_ON;
   set_heart_beat(1); 
}

/***********************************************
 DESCRIPTION: Show short desc of terrain.

 ARGUMENT:    str -terrain type.

 RETURN:      1 if success, 0 for failure.
 ***********************************************/

nomask status
show_terrain(string str) {
string s;
string server;

   if(!str) {
       notify_fail("Usage: terrain <terraintype>\n");
       return 0;
   }
  
   server = get_server();
   if(server) {
        s = (string) server->query_terrain_short(str);
        if(!s) {
          write("No such terrain.\n");
          return 1;
         }
        write(str+": "+s+"\n");
    }
   else { write("No server!!!\n"); }
   return 1;
}
   
/******************************************
 
 DESC: Resets room. Loads objects and calls
       coder's reset function.
 
*******************************************/
 
nomask static void
reset_room()
{
   reset_objects(0);
   if(!first_inventory(this_object()))
        destruct(this_object());
   else this_object()->reset_virtual();
}
 
/******************************************
 
 DESC: Sets random objects for the room.
 
 RET:  Returns 1 if success, 0 for failure.
 
*******************************************/
 
nomask status
set_random_objects(string *objec)
{

  if(!pointerp(objec)) return 0;
   random_objects = objec;
   return 1;
}
 
/*********************************************
 
 DESC: Adds new random object to object list.
 
 RET:  Returns 1 for success, 0 for failure.
 
**********************************************/
 
nomask status
add_random_object(string objec) {
 
   if(!stringp(objec)) return 0;
 
   if(!random_objects) random_objects = ({});
   random_objects += ({objec});
 
return 1;
} 
 
/******************************************
 
 DESC: Masks room.c add_exit function and
       makes needed modifications for the
       virtual room.
 
 ARG: dir  - direction description
      file - filename for direction
 
*******************************************/
 
nomask private static void
add_exit(string dir, string file) {
string str;
 
   if (server_ob)
   if (server_ob->is_virtual_room(file)) {
      if (moving_command)
        dir = moving_command+" "+dir;
      ::add_exit(dir, rooms_home+file);
    }
 }
 
/******************************************
 
 DESC: Sets all virtual exits.
 
 ARG:  Mapping of exit names and directions
       type of ([ exit:({x, y}), ... ])
 
*******************************************/
          
nomask void
set_virtual_exits(mapping dir)
{
mixed dir_names;
mixed *dir_dest;
mixed x, y;
int i, n;
 
	if (!mappingp(dir)) return;
 
   dir_names = m_indices(dir);
   dir_dest  = m_values(dir);
      
   n = m_sizeof(dir);
   for(i=0; i<n; i++) {
      x = X + dir_dest[i][0];
      y = Y + dir_dest[i][1];
      add_exit(dir_names[i], x+"_"+y);        
     }
}
 
/******************************************
 
 DESC: Add new virtual exit to exit list.
 
 ARG: dir - direction description
      x   - relative x coordinate
      y   - relative y coordinate
 
*******************************************/
 
nomask void
add_virtual_exit(string dir, int x, int y) {
 
mixed xx, yy;
 
    xx = X + x;
    yy = Y + y;
    add_exit(dir, xx+"_"+yy);
}
 
/******************************************
 
 DESC: Pick one description from given ones
 
 ARG:  list - array of descriptions
 
 RET:  Returns 1 for success, 0 for failure.
 
*******************************************/
 
nomask status
set_random_desc(string *list)
{
   if(!pointerp(list)) return 0;
  
	set_long(list[random(sizeof(list))]);
 
  return 1;
} 
 
/******************************************
 
 DESC: Enables/disables heart beat.
 
 ARG:  i - 1 enables, 0 disables.
 
 RET:  1 for success, 0 for failure.
 
*******************************************/
 
nomask status
set_hb(int i)
{
  if(i < 0 || i > 2) return 0;
 
    if (i) {
       virtualFlags |= (F_VIRTUAL_HB_ENABLED | F_VIRTUAL_HB_ON);
    } else {
// hb_enabled = i;
// hb = 0;
        virtualFlags &= (~(F_VIRTUAL_HB_ENABLED | F_VIRTUAL_HB_ON));
    set_heart_beat(0);
  }
 
  return 1;
}
 
/******************************************
 
 DESC: Set 'visual' or 'sound' effects.
 
 ARG:  s - array of effect texts.
 
 RET:  Returns 1 for success, 0 for failure.
 
*******************************************/
 
nomask varargs status
set_effects(string *s, int chance)
{
    if(pointerp(s)) {
        effects = s;
        if(!chance || chance < 0 || chance > 100) effect_chance = 5;
        else effect_chance = chance;
    }
    else return 0;
 return 1;
}
 
/******************************************
 
 DESC: Sets special moving description for
       the terrain type instead of 'leaves'
       or 'arrives'. Also sets the command
       that enables moving to other rooms.
 
       Example: swim east
      
       where "swim" is the moving command.
 
 ARG:  s - moving command
       o - description instead of 'leaves'
       i - description instead of 'arrives'  
 
 RET:  Returns 1 for success, 0 for failure.
 
*******************************************/
 
nomask status
set_moving_command(string s, string o, string i)
{
    if(stringp(s)) {
        moving_command = s;
        if(stringp(o)) moving_msgout = o;
        if(stringp(i)) moving_msgin = i;
       }
    else return 0;
 return 1;
}
 
/******************************************
 
 DESC: Sets function called from heart
       beat.
 
 ARG:  s - function name
 
 RET:  Returns 1 for success, 0 for failure.
 
*******************************************/
 
nomask static status
set_beat_function(string s)
{
    if(stringp(s)) {
      beat_function = s;
    }
    else return 0;
 
 return 1;
}
 
/*****************************************
 
 DESC: Check if we can see in different
       directions here.
 
 ARG:  Id (from look etc.)
 
 RET:  Id matches when 1, 0 when failed.
 
******************************************/
 
varargs status
id(string s, object me)
{
 
	if (s && distant_look && query_exit(s)) return 1;
 
    return ::id(s, me); //check items.
}
 
/******************************************
 
 DESC: Enable/disable distant look mode.
       This mode enables players to see
       to the other rooms.
 
 ARG:  i - 1 enables, 0 disables.
 
 RET:  1 for success, 0 for failure.
 
*******************************************/
 
nomask status
set_distant_look(status i)
{
 
	if (i && !query_exits()) return 0;	// Directions not set yet!
   distant_look = i;
 
   return 1;
}

/***********************************************
 DESCRIPTION: Show map and short desc.
 
 RETURN:      Map and short desc.
 ***********************************************/
 
varargs string
query_short(int a, object b)
{
string server;
string *map;
string s;
int x, y;

    if (!b) b = this_player();
   if(distant_look && (!b || !b->test_dark())) {
      server = get_server();
      if(server && !room_map) {
          map = (string *)server->get_environment(X-1, Y-1, 2, 2);
          x = y = 1;
          if(!map) {
             x = 1; y = 0;
             map = (string *)server->get_environment(X-1, Y, 2, 1);
             if(!map) {
                x = 0; y = 1;
                map = (string *)server->get_environment(X, Y-1, 1, 2);
                if(!map) {
                   x = y = 0;
                   map = (string *)server->get_environment(X, Y, 1, 1);
                 }
              }
           }
          if(map) {
             if(!x) {
		if (sizeof(map[y]) > 1) map[y] = "@" + map[y] [1..<1];
                else map[y] = "@";
              }
             else {
		if (sizeof(map[y]) > (x + 1))
			map[y] = map[y] [0..x - 1] + "@"
				+ map[y][x + 1.. <1];
		else map[y] = map[y] [0.. x-1] + "@";
              }
             room_map = implode(map, "|\n");
             room_map = "---+\n"+room_map;
             room_map += "|\n---+\n";
             return room_map + "\n" + ::query_short(a, b);
            }
        }
      else if(room_map && (s = ::query_short(a, b)))
           return room_map + "\n" + s;
           
    }
    else return ::query_short(a, b);
}

/******************************************
 
 DESC: Shows room descriptions.
 
 ARG:  n - argument of look command.
           Able to look in other rooms.
 
*******************************************/
 
varargs void
long(string n, object who)
{
string tmp;
object ob, t_ob;
int i;
	if (!who && !(who = this_player())) return 0;
  if(distant_look) {
    // We are able to see in other rooms.
    if(n) {
	if (tmp = query_exit(n)) {
		who->tell_me("You look "+n+"...\n"
			+call_other(tmp, "query_short"));
		ob = first_inventory(tmp);
          if(!ob) return;
          while(ob) {
              if(random(2)) { //Can't always see what's in there.
                  if(ob->short()) {
                      if(living(ob)) 
			write("Someone or something is moving there.\n");
                      else 
                        write("You can see there is something there but "+
			      "you are not quite sure what it is.\n");
                      break;
		   }
	       }
              ob = next_inventory(ob);
          } // while
 
          return;
       }
    } else write(short()+"\n");
  }
 
  if(this_player()->query_coder_level()) write("("+X+", "+Y+")\n");
 
  if(!read) {
    read = 1; 
    tmp = read_file("/"+descs_home+X+"_"+Y);
	if (tmp) set_long(tmp);
   }
 
  ::long(n);
}
  
 
nomask void
heart_beat()
{
object ob;          
object ob2;
 
    ob = first_inventory(this_object());
    while(ob) {
        if (virtualFlags & F_VIRTUAL_HB_ENABLED) {
           if(interactive(ob)) break;
         }
        else if(living(ob)) break;
        
        ob = next_inventory(ob);
      }            
    if (!ob) {
        virtualFlags &= (~F_VIRTUAL_HB_ON);
        set_heart_beat(0);
        return;
    }
 
    if (effects && random(100) < effect_chance) 
	tell_here(effects[random(sizeof(effects))]);
    if(beat_function) call_other(this_object(), beat_function, 0);
 
}

status
virtual_move(string s)
{
#if 0
      if (interactive(this_player()) && !this_player()->query(LIV_IS_FLEEING))
	this_player()->tell_me("It takes some time for you to realize that\
 you should "+moving_command+" to some direction.");
      else ::move(0); //monsters can move with directions only.
                      //also players when fleeing!!!
      return 1;
#endif
   if(!s) { 
      this_player()->tell_me(capitalize(moving_command)+" where?");
      return 1;
   }
   command(s, this_player()); //move() needs verb, let's change it.
    if (this_player() && environment(this_player()) != this_object()) {
        PARTY_D->player_moved(this_player(), moving_command, s, this_object());
    }
 
   return 1;
}

 
string
query_msgout(object x) { return moving_msgout; }
 
string
query_msgin(object x) { return moving_msgin; }

#if 0
void
set_light_level(int i) {
string s;
int    j;

   if(sscanf(file_name(this_object()), "%s#%d", s, j) == 2) return;
   ::set_light_level(i);
}
#endif

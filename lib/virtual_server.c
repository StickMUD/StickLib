 
/******************************************
 
 NAME:    virtual_server.c
 
 PURPOSE: Create virtual world rooms.
 
 AUTHOR: Tron
 
*******************************************/
 
#include <daemons.h>

#define X1 0
#define Y1 1
#define X2 2
#define Y2 3
#define MP 4
 
#define R_OBJS   0
#define R_MAPS   1
#define R_TERR   2
#define R_ALL    3
 
#define REBOOT   0
#define TELL     1
 
mapping terrains;
mapping objects;
mixed   *maps;    // ({ x1, y1, x2, y2, mapname })
 
static int X, Y;               // Last used coordinates.
static mapping map_memory;     // Maps in here.
static int mpt;
 
static string virtual_home;
static string terra_home;
static string maps_home;
static string rooms_home;
static string descs_home;
static status stale_rooms;
 
static string terra_cfg;
static string obj_cfg;
static string map_cfg;
 
static string virtual_save;
 
static string universum;
 
static int    memory;
static int    tdesc_x;
static status loading_masters;
 
static string create_func;	// Name of the optional set up function.

/********************************
 Configuration data.
 ********************************/
mixed *
query_setup() {
 
  return ({ terrains,
            objects,
            maps,
            virtual_home,
            terra_home,
            maps_home,
            rooms_home,
            descs_home,
            terra_cfg,
            obj_cfg,
            map_cfg,
            virtual_save,
            universum,
            tdesc_x
          });
}
 
            
/**************************************
 
 Function identifies this object
 as a server.
 
 RETURN: Mixed array is returned.
         Array contains configuration
         information and currently
         processed virtual room
         coordinates and its objects.
**************************************/
 
nomask mixed *
query_virtual_room_server() {
 
   return ({
            X,
            Y,
            (!loading_masters ? objects[X+"_"+Y] : 0),
            descs_home,
            rooms_home,
	  });
 }
 
/************************************
 
 Function handles configuration
 files.
 
 ARGUMENT: "map" is action selector.
 
*************************************/
 
nomask static void
read_data(status map) {
 
string buf, *file;
string crd, obj;
int i, a, b, c, d, f;
 
   file = allocate(3);
   file = ({0, 0, 0});
 
   switch(map) {
 
     case R_ALL:
          terrains = ([]);
          objects  = ([]);
          maps     = ({});
          file[R_TERR] = terra_cfg;
          file[R_OBJS] = obj_cfg;
          file[R_MAPS] = map_cfg;
          break;
 
     case R_OBJS:
          objects  = ([]);
          file[R_OBJS] = obj_cfg;
          break;
   
     case R_MAPS:
          maps     = ({});
          file[R_MAPS] = map_cfg;
          break;
 
     case R_TERR:
          terrains = ([]);
          file[R_TERR] = terra_cfg;
          break;
 
     default: return;
   }
   
   for(i=1, f=0; f<3; f++, i=1) {
 
     if(file[f])
        while(buf = read_file("/"+file[f], i++, 1)) {
 
           if(buf[0..0] == "!") continue; /* Commented line */
 
           switch(f) {
 
             case R_OBJS:
                   if(sscanf(buf,"%s:%s\n", crd, obj)==2) {
                      if(objects[crd]) 
                         objects[crd] += ("@"+obj);
                      else
                         objects += ([ crd:obj ]);
                    }
                   break;
            
             case R_MAPS:
                   if(sscanf(buf,"%s:%d,%d,%d,%d", crd, a, b, c, d)==5) 
                      maps += ({ ({a, b, c, d, crd}) });
                   break;
 
             case R_TERR:
                  if (sscanf(buf, "%s:%s\n", crd, obj)==2)
                     terrains += ([ crd:obj ]);
                   break;
 
             default: break;
            }
 
         } //WHILE
 
    } //FOR
}
 
/***********************
 
 Function re-configures
 all objects.
 
************************/
 
nomask void
read_objects() {
 
   read_data(R_OBJS);
}
 
/***********************
 
 Function re-configures
 all maps.
 
************************/
 
nomask void
read_maps() {
 
   read_data(R_MAPS);
}
 
/***********************
 
 Function re-configures
 terraintypes. Terrain
 types are not compiled.
 
************************/
 
nomask void
read_terrains() {
 
   read_data(R_TERR);
}
 
/***************************
 
 Function compiles and loads
 all the terrain types to
 find errors. Server won't
 start if errors are found.
 
****************************/
 
nomask static status
get_terrain_masters() {
mixed tmp;
int i, n;
object ob;
string err;
status ret;
 
   ret = 1;
   tmp = m_values(terrains);
   n = sizeof(tmp);

   loading_masters = 1;
   
   for(i=0; i<n; i++) {
       write("Loading "+tmp[i]+"...");
       if(!(ob=find_object(terra_home+tmp[i]))) 
           err = catch(call_other(terra_home+tmp[i], "????", 0));
       else {
		while(first_inventory(ob))
			destruct(first_inventory(ob));
          destruct(ob);
          err = (string)call_other(terra_home+tmp[i], "????", 0);
	}
      if(err) {
         write("FAILED!\n"+err+"\n");
         err = 0;
         ret = 0;
        }      
     else write("OK.\n");
   }
   loading_masters = 0;
 
return ret;
}
 
/***********************************
 
 Function handles all players on the
 are of this virtual server.
 
 ARGUMENT: "st" is action selector.
           "str" is message sent
           to all players on the
           area of this server.
************************************/
 
nomask varargs static void
virtual_users(int st, string str) {
object *obs;
object env;
string tmp;
int i, n;
 
    obs = users();
    n = sizeof(obs);
    for(i=0; i<n; i++) {
        env = environment(obs[i]);
        if(!env) continue;
        if(sscanf("/"+object_name(env), rooms_home+"%s", tmp))
	  {
            if(st == REBOOT) {
              tell_object(obs[i], "VIRTUAL WORLD: Rebooting world NOW!!!\n"+
                                  "VIRTUAL WORLD: Moving to church...\n");
              move_object(obs[i], "room/church");
	     }
            else if(st == TELL) 
              tell_object(obs[i], str);
          }
      }
}
 
/*********************************
 
 Function resets server. New 
 configurations are read and
 all the terrain types are
 re-compiled for errors.
 
 ARGUMENT: "arg" is action
           selector. if "arg" is
           zero configurations
           are re-read from the
           configuration files.
           If "arg" is 1, only
           save file is read in.
 
**********************************/
            
nomask void
reset_server(status arg) {
object ob;
 
  if(get_terrain_masters() == 0) { 
     write("Errors encountered, reseting halted!\n");
     return;
   }
  if(!arg || !restore_object(virtual_save)) {
      read_data(R_ALL);
      save_object(virtual_save);
  }
  virtual_users(REBOOT);
  VROOM_D->vRoomDeleteRooms();

 
}
 
/************************************
 
 Function starts virtual server.
 Call this function in your own
 virtual server code to start
 server after setting all variables.
 
*************************************/
 
nomask void
start_server() {
 
  terrains = ([]);
  objects = ([]);
  maps = ({});
  
  map_memory = ([]);
  reset_server(1);
  stale_rooms = (int)VROOM_D->vRoomQueryStaleRooms();
  if(stale_rooms) call_out("check_stale", 5);
}
 

void
check_stale() {

  stale_rooms = (int)VROOM_D->vRoomQueryStaleRooms();
  if(stale_rooms) call_out("check_stale", 5);
}
/**************************************
 
 Function manages memory and
 maps kept in memory.
 
 ARGUMENTS: "dir" is directory
            for map database.
            "file" is a mapfile
            in database.
 
 RETURN: The whole map is returned or
         zero if file didn't exist.
***************************************/
 
nomask static mixed 
map_manager(string dir, string file) {
 
string s;
mixed m;
 
#ifdef DEBUG
   write("Map_manager: /"+dir+file+"\n");
#endif
 
   if(!(s=read_file("/"+dir+file))) return 0;  
   
   m = explode(s, "\n");
   
   map_memory += ([file:m]);
   if(m_sizeof(map_memory) >= memory) 
       map_memory = m_delete(map_memory, m_indices(map_memory)[0]);
 
   return m;
}
 
/*******************************************
 
 Function reads data from given
 map.
 
 ARGUMENTS: "x" and "y" are relative
            map coordinates.
            "mp" is a map descriptor.
 
 RETURN: String is returned representing
         the possible terrain descriptor
         on the given map and coordinates.
         Zero is returned if map can't be
         read or coordinates are off the
         range.
*******************************************/
 
nomask static string
read_map(int x, int y, string mp) {
mixed m;
int xx;
    
#ifdef DEBUG
  write("Read_map: "+x+", "+y+" : "+mp+"\n");
#endif
 
  if(!(m = map_memory[mp])) 
     if(!(m = map_manager(maps_home, mp))) return 0;
 
  xx = x*tdesc_x;
 
  if(sizeof(m[y]) > xx) 
       return m[y][ xx..(xx+tdesc_x-1) ];
 
 return 0;
}
 
/***************************************
 
 Function finds the right map for
 given coordinates.
 
 ARGUMENTS: "x" and "y" are absolute
            map coordinates.
 
 RETURN: Map descriptor is returned
         or zero if no such map exists
         for given coordinates.
****************************************/
 
nomask static mixed
find_map(int x, int y) {
int i;
int n;
 
#ifdef DEBUG
   write("Find_map: "+x+", "+y+"\n");
#endif
 
   n = sizeof(maps);
   for(i=0; i < n; i++) 
      if( (x >= maps[i][X1] && y >= maps[i][Y1]) &&
          (x <= maps[i][X2] && y <= maps[i][Y2])   )
        {
         mpt=i;
         return maps[i];
        }
 
    mpt = -1;  
   
return 0;
}   
 
/**************************************
 
 Function finds terraintype
 descriptor on the map.
 
 ARGUMENTS: "x" and "y" are absolute
            map coordinates.
 
 RETURN: Terrain descriptor string
         is returned or zero if no
         descriptor was present in
         given coordinates.
***************************************/
 
nomask static string
get_terrain(int x, int y) {
mixed crd;
int rx, ry;
 
#ifdef DEBUG
  write("Get_terrain: "+x+", "+y+"\n");
#endif
 
  if(!(crd = find_map(x, y))) return 0;
 
  rx = x - crd[X1];
  ry = y - crd[Y1];
 
return  read_map(rx, ry, crd[MP]);
}  
 
/*******************************************
 This function is called every time
 when there is a virtual object request.
 
 ARGUMENT: "path" is the filename of
           requested virtual object.
 
 RETURN: Created virtual object is returned.
         Zero if failed.
********************************************/
 
/* nomask */
object
compile_object(string path) {
 
int x,y;
string head;
string terrain;
string file;
object ob;
 
 
   if(!path || stale_rooms) return 0;
#ifdef DEBUG
 write("Compile_object:"+path+"\n");
#endif
 
   if(sscanf(path, rooms_home+"%d_%d", x, y) != 2) 
       return 0;
 
   if(!(terrain = get_terrain(x, y))) return 0;
   if(!(file = terrains[terrain])) return 0;
 
   X = x; Y = y;  // Set up current coordinates.
 
   ob = clone_object(terra_home+file);
	if (create_func) call_other(ob, create_func);
 
   VROOM_D->vRoomStoreRoom(ob);
return ob; 
}
 
/***************************
 
 Room object asks for its
 coordinates through this
 function.
 
 RETURN: Array of last used
         coordinates.
 ***************************/
 
nomask mixed
query_coordinates() {
 
  return ({ X, Y });
}
 
/********************************
 
 Room object asks for its
 objects through this 
 function.
 
 ARGUMENT(S): "x", "y", where
              x and y are
              coordinates.
 
 RETURN: String of object files.
********************************/
 
nomask mixed
query_objects(int x, int y) {
  return objects[x+"_"+y];
}
 
/***************************
 Room object needs the home
 directory for rooms.
****************************/
 
nomask string
query_rooms_home() { return rooms_home; }
 
/*********************************
 Room object needs home directory
 for external descriptions.
**********************************/
 
nomask string
query_descs_home() { return descs_home; }
 
/******************************
 Room needs information of its
 environment.
*******************************/
 
nomask status
is_virtual_room(string file) {
int x, y;
string t;

   if(!file || sscanf(file, "%d_%d", x, y)!=2) return 0;
 
    if(t = get_terrain(x, y))
       if(terrains[t]) return 1;
 
 return 0;
}

nomask varargs string *
get_environment(int x, int y, int xrange, int yrange) {
mixed mapd;
string *tmpx, *tmpy;
int ylen, xlen;
string *map;
string *map2;
int y2;
int x2;
int n;

   mapd = find_map(x, y);
   if(!mapd) return 0;
   map = map_manager(maps_home, mapd[MP]);
   if(!map) return 0;

   y2 = y+yrange;
   x2 = x+xrange;

   if(x2 > mapd[X2]) {
       n = x2 - mapd[X2];
       tmpx = get_environment(mapd[X2]+1, y, n, yrange);
       xrange -= n;
       x2 = mapd[X2];
    }
   if(y2 > mapd[Y2]) {
       tmpy = get_environment(x, mapd[Y2]+1, xrange, y2 - mapd[Y2]);
       y2 = mapd[Y2];
    }
   
   x -= mapd[X1];
   y -= mapd[Y1];

   x2 -= mapd[X1];
   y2 -= mapd[Y1];  
   
   if(tmpy) {
      map += tmpy;
      y2 += sizeof(tmpy);
    }
   if(tmpx) {
      x2 += sizeof(tmpx[0]);
    }
   map2 = ({});       
   for(n = y; n <= y2; n++) {
      if(tmpx) map[n] += tmpx[n-y];
      map2 += ({map[n][x..x2]});
    }
   return map2;
}             

/*******************************************
 DESCRIPTION: Get short desc of terrain.

 ARGUMENT(S): s -terrain type.

 RETURN:      short desc.
 *******************************************/

nomask string
query_terrain_short(string s) {

   if(!s) return 0;

   s = terrains[s];
   if(!s) return 0;
   else return (string)(terra_home+s)->query_short();
}
   
/**************************
 Some utitility functions.
***************************/
 
 
/*************************************
 Get map for given coordinates.
 Return value is array of strings
 where each index (y) contains all
 rooms on x-axis. Zero if coordinates
 do not match any of the maps in
 memory or data base.
***************************************/
 
nomask string*
query_map(int x, int y) {
mixed tmp;
 
   if(tmp=find_map(x, y)) {
      if(!(tmp=map_memory[tmp[MP]])) 
           return map_manager(maps_home, tmp[MP]);
       else return tmp;
      }
  return 0;
}
 
static int reboot;
 
/********************************
 Function is used to inform all
 players on the area of this
 virtual server about reboot.
*********************************/
 
nomask static void
inform_reboot(int time) {
int minutes;
 
    minutes = time / 60;
 
    virtual_users(TELL, "VIRTUAL WORLD: Reboot in "+minutes+" minutes!!!\n"+
                        "VIRTUAL WORLD: You will be transfered to church.\n");
}
 
/**************************************
 Function starts reboot sequence.
 Argument 'time' is time in minutes.
 If 'time' was zero and reboot
 sequence was not running the reboot
 will be instant without any warnings.
 If sequence was running it is aborted.
 If 'time' was non zero the reboot
 sequence is started with given time
 even if there was a sequence running
 already.
***************************************/
 
nomask void
reboot_world(int time) {
 
  if(time < 0) time = 0;
 
  if(!time) {
     if(reboot) {
         reboot = 0;
         if(find_call_out("count_reboot")) remove_call_out("count_reboot");
         write("Reboot aborted!!!\n");
         virtual_users(TELL, "VIRTUAL WORLD: Reboot aborted!!!\n");
       }
     else {
         reset_server(0);
       }
 
     return;
   }
  
   reboot = time * 60;
 
   write("Reboot in "+time+" minutes.\n");
   inform_reboot(reboot);
 
   call_out("count_reboot", 60);
 
return;
}
 
/***************************
 Function counts minutes to
 reboot and informs players.
****************************/
 
nomask void
count_reboot() {
 
  reboot -= 60;
  if(reboot <= 0) {
     reset_server(0);
     reboot = 0;
     return;
   }
 
   inform_reboot(reboot);
  
call_out("count_reboot", 60);
return;
}

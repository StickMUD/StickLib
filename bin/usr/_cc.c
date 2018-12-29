/****************************************************************
*								*
* Module:							*
*	/bin/usr/_cc.c						*
* Description:							*
*	The command with which LPC-objects can be 'compiled';	*
*	it destructs the old "blueprint object" and tries to	*
*	reload the object in question. There are all kinds of	*
*	neat options and so on; check out 'man cc' to know	*
*	more about them.					'
*	This version of cc is not directly taken from StickMud;	*
*	it's a version developed originally for MurderMud, and	*
*	was coded by Germ, edited by Doomdark.			*
* Version:							*
*	1.0a (for StickLib) Aug-98				*
* Last modified:						*
*	23-Aug-98, Doomdark					*
*								*
* NEW:								*
*								*
****************************************************************/

// Original info from MurderMud:

/************************************************************************
//  _cc.c - Compile files 						//
//									//
//  Usage:   cc [-p][-c][-i] files					//
//									//
//  Flags are:								//
//	    -p : preserve objects in a room.				//
//	    -c : compile and clone an object				//
//	    -i : also compile the inherited objects.			//
//	default: only players are preserved.				//
//									//
//  Note: Having two compile commands (cc and cn) is not redundant.	//
//	If you are working on the compile command itsself, you need	//
//	another compile command to compile it.				//
*************************************************************************/

#include <config.h>
#include <generic_rooms.h>

#define _PRESERVE_OBJECTS	0x0001
#define _CLONE_OBJECTS		0x0002
#define _OBJECT_FLAG		0x0004
#define _INHERIT_FLAG		0x0008


static string *backup_room =  ({
	GENERIC_ROOM_CHURCH,
	GENERIC_ROOM_SHOP
});

// Function prototypes
//
nomask status compile_single_file(string filepath, int options, object me);

nomask string *
expand_wildcards(object me, string file, int path_flag, int check_special )
{
   string path;
   string *list, *tmp;
   int i;

	if ( !file ) return 0;

	if ( !stringp(path = me->absolute_path(file, check_special)))
		return ({ });

	if ( !sizeof(list=get_dir(path)))
		return ({ }); // No files match.
	list -= ({ ".", "..", ".ACCESS" });
	if ( path_flag ) {
	  tmp = explode(path,"/");
	  tmp = tmp[0..<2];
	  path = implode(tmp,"/");
	  for ( i=0; i<sizeof(list); i++ )
		list[i] = sprintf("%s/%s",path,list[i]);
	}
	return list;
}

nomask int
cc_cmd(string str, object me)
{
   object ob;
   string tmp,tmpstr,valid;
   string *files, *dirs, *list;
   int i,j,k,options;
   mapping object_path;

   if ( !me && !me=this_player()) return 0;

   if( !str || !files = explode(str," ") || !sizeof(files))
     {
	notify_fail("Usage: cc [-p][-c] <file/files>.\n\
	Options are:\n\
		-p : Preserve objects in the room to be compiled.\n\
		-c : Compile and clone an object.\n");
	return 0;
     }


// Deal with command-line options 
//
   options = 0;
   while( files[0][0] == '-' )
     {
	switch(files[0][1])
	  {
	     case 'p' : options |= _PRESERVE_OBJECTS; break;
	     case 'c' : options |= _CLONE_OBJECTS;    break;
	     case 'i' : options |= _INHERIT_FLAG;     break;
	     default:
		me->tell_me(sprintf("Compile: Unknown command option - %c",files[0][1]));
	  }
	files -= ({ files[0] });
     }

   list = ({ });
   object_path = ([ ]);

   for (i=0; i<sizeof(files); i++)
    {
// See if the desired file is actually a non-player object in either the
// inventory of me or environment(me).
// If so, we will dest it, use it's filename, and save it's environment
// in the object_path mapping so we can clone it afterwards.
//
      if ( ob = present4(files[i],me, me, -1)
	  || ob = present4(files[i], environment(me),me,-1))
	{
	   sscanf(object_name(ob),"%s#%s",files[i],tmp);
	   if (!living(ob) || query_once_interactive(ob))
	     {
		options |= _OBJECT_FLAG; 
		object_path+=([sprintf("%s.c",files[i]):environment(ob)]);
		me->tell_me(sprintf("Compile: destructing\t%s",object_name(ob)));
		destruct(ob);
	     }
	}

// The file is not an object, so we must convert the filename to
// an absolute path and expand any wildcards

	// Force filename to end in .c
	if ( files[i] != "here" && files[i] != "me" )
	  {
	    if ( member( ({"*c",".c","??",".?","?c"}),files[i][<2..<1])==-1)
	      files[i] = sprintf("%s.c",files[i]);
	 
	    // expand wildcards and add full pathname to the expanded files
	    list += expand_wildcards(me, files[i],1,1);
	  }
	else
	// if we are compiling "here" or "me", we don't have wildcards
	// and don't need to expand for them.  This lets "here" be a
	// virtual room.
	//
	    if ( stringp(tmp=(string) me->absolute_path(files[i],1)))
		list += ({ tmp });
    }

   if ( !sizeof(list))
     {
	notify_fail("Compile: no files found to compile.\n");
	return 0;
     }

// If the inherit-compile option was specified, we will get all
// of the inherited files of each object to be compiled, and
// compile the inherited files in the order of lowest->highest.
// 
   if ( options&_INHERIT_FLAG)
     {
	files = ({ });
	for ( i=0; i<sizeof(list); i++)
	  {
	    if ( !ob = to_object(list[i]))
	      {
		files += ({ list[i] });
		continue;
	      }
	    dirs = inherit_list(ob);
	    for ( j=sizeof(dirs)-1; j>-1; j--)
	      {
		tmp = ( dirs[j][0]=='/') ? dirs[j] : sprintf("/%s",dirs[j]);
		// Lets not compile the same inherited object more than once.
		if ( member(files, tmp)==-1 ) files += ({ tmp });
	      }
	    if ( member(files, list[i])==-1 ) files += ({ list[i] });

	  }
     }
   else
	files = list;


// Compile the files...
//
   for (i=0; i<sizeof(files); i++)
     {
     if ( compile_single_file(files[i], options, me))
	if ( (options&(_CLONE_OBJECTS) || member(object_path,files[i]) ))
	{
	// If this was originally an object, we will clone it and move it
	// to it's original environment stored in the mapping object_path.
	// If the clone-objects (-c) flag was specified, we will try to clone
	// this object and either leave it at me or environment(me)
	//
	   tmpstr = catch(ob = clone_object(files[i]));
	   if (!ob)
	     {
		me->tell_me(sprintf("Compile: clone failed\t%s",files[i]));
		return 1;
	     }
	// Don't move it anywhere if it is a room. Being a room
	// in this context simply means it has no environment()...
	   if (!environment(ob))
		continue;

	   if ( member(object_path,files[i]))
		 move_object(ob, object_path[files[i]]);
	// Or perhaps we need to try to take the object.
	// We'll take a short cut here too and assume that
	// "A coder can take anything!" (yelled like "a marine
	// can take anything").  :-)
	   else {
		move_object(ob, me);
	   }
	   me->tell_me(sprintf("Compile: cloning\t%s", object_name(ob)));
	}

     }
   return 1;
}

//
// This function will compile a single file.
// Arguments:
//	(string) filepath:
//		The complete file path, without any wildcards.
//	(int) options:
//		The binary-coded option flags.
//		The flags used in this function are:
//			_PRESERVE_OBJECTS - Don't destruct objects that
//				are in the room's inventory.
// Returns:
//	(status) : 0 => compile failed.
//		   1 => compile succeeded.
//

nomask status
compile_single_file(string filepath, int options, object me)
{
   object ob, current, *objects, backuproom;
   mixed feat1, feat2, feat3;
   string path, tmp, tmpstr;
   int i, j;

   path = filepath;	
   if (path[<2..<1] == ".c") path = path[0..<3];	//strip the '.c'


// If the object already exists, we must deal with its inventory
// This should only be done for rooms.

   if ( ob = find_object(path) )
     {

	objects = 0;

	// Locate a backup room in which to temporarily store objects

	for ( j=0; j<sizeof(backup_room); j++ )
	   if ( object_name(ob)!=backup_room[j] 
		&& backuproom = find_object(backup_room[j])) break;

	if ( j>0 )
	  me->tell_me("Compile: Compiling default backup room - selecting new backup room.");
		 
	if ( !backuproom )
	   {
	      me->tell_me("Compile: Unable to find an acceptable backup room - aborting.");
	      return 0;
 	   }

	// Deal with objects in the room

	objects = ({ });
	while( current = first_inventory(ob) )
	  if ( (options&_PRESERVE_OBJECTS) || interactive(current) )
	    {
		// Preserve players
		// Preserve objects if preserve or compile is set. 

		if ( interactive(current) && current != this_player())
		   current->tell_me("Compile: compiling room.");
		objects += ({ current });
		if ( tmp = object_name(current))
		me->tell_me(sprintf("Compile: preserving\t%s (%s).", tmp,
			(string)current->query_name(0, me)));
		move_object(current, backuproom);
	    }
	  else
	    {
		// Destruct objects if we are not preserving/compiling.
		//
		if ( tmp = object_name(current))
		   me->tell_me(sprintf("Compile: destructing\t%s.", tmp ));
		destruct(current);
	    }

	// If this object is not the backup room, destruct it.

	destruct(ob);
      } 
//    destruct(path);

// If this file is in a coder's home directory tree, gamedriver will
// log any compile-errors to /log/<Codername>.
// We rm this file so it will contain only the errors from this
// compile attempt.
//
    tmp = 0;
    if ( sscanf(path, PATH_USER_FNAME "%s/%s", tmp, tmpstr ))
      {
	tmp = sprintf("/log/%s", capitalize(tmp));
	if (file_size(tmp) >= 0) rm(tmp);
      }


// Attempt to compile the file...

	if ((tmpstr = catch(call_other(path, "??"))))	// Compile 
	  {
	// Compile has failed.
	//
	     me->tell_me(sprintf("\nCompile: failed\t\t%s.\n%s", path,tmpstr));
	     if(tmp)
		if(file_size(tmp) > 0)
		  me->tell_me(sprintf("Errors logged to: %s.\n",tmp));
		else
		  me->tell_me(sprintf("No logged errors for: %s.\n",path));

	// Inform objects that compile failed
	// and that they have been moved.
 
	     if(objects)
		for (j=0; j<sizeof(objects); j++)
		  if ( interactive(objects[j]))
		    objects[j]->tell_me(sprintf("\nCompile: failed\tMoving to %s.\n",
			 (string)backuproom->query_short(0, objects[j])));
	     return 0;

	  }

     me->tell_me(sprintf("Compile: successful\t%s.c\n",path));


// Move preserved objects back to the room.
//
     if ( pointerp(objects) && sizeof(objects))
       for (j=0; j<sizeof(objects); j++)
	if ( objectp(objects[j]) )
	  {
	     // Move preserved objects back to the room.
	     if ( interactive(objects[j]) )
		objects[j]->tell_me("Compile: done.");

	     if ( objects[j] )
		catch(move_object(objects[j], path));
	  }

     return 1;
}


void
create()
{
   int i;
	// Make sure the backup rooms are loaded...
	for ( i=0; i<sizeof(backup_room); i++ )
		call_other(backup_room[i], "query");
}



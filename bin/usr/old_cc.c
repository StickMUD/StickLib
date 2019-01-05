/*
	From an idea by Val & Rincewind
	All original code for this release.
	All code by: Chaotic
	Chaotic is: Nick Hirschberg, halcyon@expert.cc.purdue.edu
*/

#include <generic_rooms.h>
#include <config.h>

#define	HELP_FILE	"/doc/commands/usr/cc.man"

object *objects;
object destination;

/* Returns a ~ foramted file name for an object
        ob:     the object we want a name for
*/
string
tilde_name(object obj)
{
string str;
	if (!obj)
		return 0;

	str = object_name(obj);
	if (sscanf(str, PATH_USER_FNAME "%s",str) )
            str = sprintf("~%s",str);
	return str;
}

/* Handle all objects, be desting or saving or whatever
	ob:		current object to process
	saveroom:	just what are we saving
*/
void
handle_obj(object ob, int saveroom)
{
object current;

	current = first_inventory(ob);
	if (saveroom) {
		while(current) {
		  if(current != this_player())
			tell_object(current,"Compile: compiling room.\n");

			if (objects)
				objects += ({current});
			else
				objects  = ({current});

			this_player()->tell_me(sprintf("Compile: preserving\t%s.",
				tilde_name(current)));
			move_object(current,destination);
			current=first_inventory(ob);
		}
		return;
	} else {
		while(current) {
			if(interactive(current)) {
			  if(current != this_player())
			    tell_object(current,"Compile: compiling room.\n");

			  if (objects)
				objects += ({current});
			  else objects  = ({current});

			  this_player()->tell_me(sprintf("Compile: preserving\t%s.\n",tilde_name(current)));
				move_object(current,destination);
			} else {
				handle_obj(current,saveroom);
				this_player()->tell_me(sprintf("Compile: destructing\t\t%s.\n",tilde_name(current)));
				destruct(current);
			}
			current=first_inventory(ob);
		}
	}
	return;
}


/* 
	Take a list of files and make them absolute.
	files:		'file names' to make absolute
	mode:		argument to pass on to absolute_path
*/
string *
abs_paths(string *files, int mode)
{
int x,y;
string tmp;
string *list;

	x=sizeof(files);
	for(y=0;y<x;) {
		tmp=(string)this_player()->absolute_path(files[y],mode);


		if(tmp != "NONE") {
			if(list) list += ({ tmp });
			else	 list =  ({ tmp });
			y++;
		} else {
			this_player()->tell_me(sprintf("Warning(absolute_path): unable to locate file for %s.\n",files[y]));
		}
	}
	return(list);
}


/* Check to see if the file exists, if it is non-zero
	and if we care.
	file:		file to check
	fileonly:	what kind of compile are we doing
*/

string
check_file(string filename, int fileonly, string valid)
{
int success;

	if (valid) {
	  if ((file_size(filename)) < 0) {
		this_player()->tell_me(sprintf("\nWarning: unable to locate file \"/%s\".\n\n",filename));
		return 0;
	  }

	  if((file_size(filename)) < 1) {
		this_player()->tell_me(sprintf("\nWarning: \"/%s\" is a 0 legth file.\n\n",filename));
		return 0;
	  }

	  success=1;
	} else {
	  success=0;
	}

	if(fileonly && !success) {
	  this_player()->tell_me(sprintf("\nWarning: file: %s not found, skipping(file-only compile).\n",filename));
	  return 0;
	}
        return filename[1..<1];
}


/* Take a list of strings, attemped to convert them to files
	expanding for *s and etc
	files:		list of 'files' to convert
	fileonly:	status variable, are we *not* compling objects
*/
string
*compse_list(string *files, int fileonly)
{
	int l,m,x,y,z;
	string tmp,tmp_path,valid;
	string *list,*dirs;

	x=sizeof(files);
	for(y=0; y<x ; y++) {
		valid = (string) MASTER_OB->valid_read(files[y], 0,
			"read_file", this_player());
		if(valid) {
			z=sizeof(files[y])-1;
			while(files[y][z] != '/') z--;
			tmp_path=files[y][0..z];

			tmp=files[y];
			m=sizeof(tmp);
			if (tmp[m] != '*' &&
                            tmp[<2..<1] != "*c" && tmp[<2..<1] != ".c")
				if (tmp[<2..<1] != "??" && tmp[<2..<1] != ".?"
                                && tmp[<2..<1] != "?c")
					tmp=sprintf("%s%s",tmp,".c");

			dirs=get_dir(tmp);
			if(!dirs) {
				this_player()->tell_me(sprintf("Warning(get_dir): no files found for %s.",files[y]));
				continue;
			}

			l=sizeof(dirs);
			for(m=0;m<l;m++) {
				tmp=sprintf("%s%s",tmp_path,dirs[m]);
				if(tmp=check_file(tmp,fileonly,valid)) {
					if(list) list += ({tmp});
					else	 list  = ({tmp});
				}
			}
		} else {
			if(fileonly) {
				this_player()->tell_me(sprintf("Warning: object compile disabled.\n\
Warning(valid_read): unable to access file %s.",files[y]));
				continue;
			}

			if(tmp=check_file(files[y],fileonly,valid)) {
				if(list) list += ({tmp});
				else	 list  = ({tmp});
			}
		}
	}
	return list;
}

string process_args(string tmp) {
	int x;
	string str;
	str="";
	for(x=sizeof(tmp)-1;x > -1;x--) {

		if(tmp[x]=='s') {
			str=set_bit(str,1);
			continue;
		}

		if(tmp[x]=='h') {
			str=set_bit(str,2);
			continue;
		}

		if(tmp[x]=='f') {
			str=set_bit(str,3);
			continue;
		}

		if(tmp[x]!='-') this_player()->tell_me(sprintf("Compile: error\tunknown command option\t%c.",tmp[x]));

	}
	return str;
}


void
compile_file(string path, string tmp)
{
	int m,l;
	string error;

	if((error = catch(call_other(path, "??")))) {
		this_player()->tell_me(sprintf("\nCompile: failed\t\t\"%s\".\n%s",path,error));
		if(tmp) {
			if(file_size(tmp) > 0) {
				this_player()->tell_me(sprintf("Errors logged for: %s.\n",path));
				cat(tmp);
				this_player()->tell_me(sprintf("\n"));
			} else {
				this_player()->tell_me(sprintf("No logged errors for: %s.\n",path));
			}
		}

		if(objects) {
			m=sizeof(objects);
			for(l=0;l < m;l++) {
				objects[l]->tell_me(sprintf("\nCompile: failed\t\tMoving to %s.\n",
					(string)destination->query_short() ) );
			}
		}

	} else {

		this_player()->tell_me(sprintf("Compile: successful\t\"%s\".\n",path));
		if(objects) {
			m=sizeof(objects);
			for(l=0;l < m;l++) {
				if(objectp(objects[l])) {
					tell_object(objects[l], "Compile: done.\n");
					catch(move_object(objects[l], path));
				}
			}
		}

	}
	return;
}

int
cc_cmd(string str, object me)
{
	object ob;
	string error,tmp,path;
	string *files;
	int j,k;
	int fileonly,saveroom,help;

	fileonly=saveroom=help=0;

	if(!str) {
		notify_fail("Usage: cc -[hfs] <file/files>.\n");
		return 0;
	}

	files = explode(str," ");
	files -= ({ "" });

	if(files) {
		tmp=files[0];
		if(tmp[0] == '-') {
			files -= ({tmp});
			if(sizeof(files) == 0) files = 0;

			tmp=process_args(tmp);
			if(test_bit(tmp,1))
				saveroom = 1;
			if(test_bit(tmp,2))
				help = 1;
			if(test_bit(tmp,3))
				fileonly = 1;
		}
	}

	if(help)
		return me->more(HELP_FILE), 1;

	if(!files)
		return notify_fail("Usage: "+query_verb()+" -[hfs] <file/files>.\n"),0;

	if(!(files=abs_paths(files,1)))
		return notify_fail(sprintf("Compile: error\t\tPlease use valid file names.\n")),0;
	
	if(!(files=compse_list(files,fileonly)))
		return notify_fail("Compile: no files found to compile.\n"),0;
		
	j=sizeof(files);
	for(k=0;k < j;k++) {
		path=files[k];	
		objects=0;
		
		if(path[<2..] == ".c")
                    path=path[0..<3];

		if(ob = find_object(path)){
			objects = 0;

			if(object_name(ob)!= GENERIC_ROOM_CHURCH_FNAME) {
				destination=find_object(GENERIC_ROOM_CHURCH);
			} else {
				this_player()->tell_me("Complie:\tCompiling the church selecting new backup room");
				destination=0;
			}
			if(!destination) {
				destination=find_object(GENERIC_ROOM_SHOP);
				if(object_name(ob)==GENERIC_ROOM_SHOP_FNAME)
					destination=0;
				if(!destination){
					destination=find_object(GENERIC_ROOM_WELL);
					if(object_name(ob)==GENERIC_ROOM_WELL_FNAME)
						destination=0;
				}
			}
			if(!destination) {
				this_player()->tell_me(
"Unable to find an acceptable backroom!!\n\
Aborting compile of " GENERIC_ROOM_CHURCH ".");
				continue;
			}

			handle_obj(ob,saveroom);
			destruct(ob);
		}

		tmp = 0;
		sscanf(path, PATH_USER_FNAME "%s/", tmp);

		if(tmp) {
			tmp=sprintf("/log/%s",tmp);
			if(file_size(tmp) >= 0)
				rm(tmp);
		}

		sscanf(path, "%s.c", path);

		compile_file(path,tmp);
	}
	return 1;
}

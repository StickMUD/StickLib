/****************************************************************
*								*
* Module:							*
*	/bin/usr/_ls.c						*
* Description:							*
*	The standard "ls" (~= "dir" for (Win)dos users) that	*
*	lists the files in directories.				*
*	This version of cc is not directly taken from StickMud;	*
*	it's a version developed originally for MurderMud by	*
*	Doomdark.						*
* Version:							*
*	1.0a (for StickLib) Aug-98				*
* Last modified:						*
*	23-Aug-98, Doomdark					*
*								*
* NEW:								*
*								*
****************************************************************/

#include <tell_me.h>

#define LS_NOTHING 256	// -q = quick list
#define LS_DEFAULT  0
#define LS_s        1	// file sizes
#define LS_d        2	// show dirs only
#define LS_F        4	// show loaded objects with an asterisk (*)
#define LS_l        8	// long ls - all of the above options.

#define BLOCKSIZE   512
#define BLOCKSIZE2  511
#define BLOCKBITS   9

nomask status
ls_cmd(string str, object me, string mode, int ff)
{
string tmp, path, name;
mixed  *dir, *tmp_list;
int i, j, flags, maxlen;
int num, size, sizeall, blocks, dirall;
status Fflag,sflag;

	if (!me) return 0;

	while (str) {
	  if (strlen(str) < 1 || str[0] != '-') break;
		if (sscanf(str, "%s %s", tmp, str) < 2) {
			tmp = str;
			str = 0;
		}
		i = strlen(tmp);
		while (--i) {
		  switch (tmp[i]) {
			case 's': flags |= LS_s; break;
			case 'd': flags |= LS_d; break;
			case 'q': flags |= LS_NOTHING; break;
			case 'F': flags |= LS_F; break;
			case 'l': flags |= LS_l; break;
			default: me->tell_me(sprintf("Unknown flag: '%c'.", tmp[i]));
		  }
		}
	}

	if (!flags) flags = LS_DEFAULT;
	else if (flags & LS_NOTHING) flags = LS_NOTHING;

	if (!str) {
		path = me->query_env("cwd");
		if (path == "") path = "/";
		else if (strlen(path) > 1) {
			if (path[0] != '/') {
				if (path[<1] != '/')
					path = sprintf("/%s/", path);
				else path = "/" + path;
			} else if (path[<1] != '/')
				path = path + "/";
		}
	} else {
		path = me->absolute_path(str, 0);
		i = strlen(path) - 1;
		if (i != -1 && path[i]!='/' && file_size(path+"/")==-2) {
			path += "/";
		}
	}

	dir = get_dir(path, 3);

	if (!dir) {
	  me->tell_me(sprintf("No such directory (%s) or no rights for it.", path));
	  return 1;
	}
	num = sizeof(dir);

	for (i = strlen(path) - 1; i > 0 && path[i] != '/'; --i)
		;
	path = path[0..i];
	me->tell_me(sprintf("Directory: '%s':", path));
    /** directory flag **/
	if (flags & LS_d) {
	 for(i = j = 0; i < num; i += 2)
		if (dir[i+1] == -2) {
		  if (j != i) {
			dir[j] = dir[i];
			dir[j+1] = -2;
		  }
		  j += 2;
	 }
	 if (num = j--) {
		dir = dir[0..j];
	 } else {
		dir = ({ });
	 }
	}

	if (!num) {
		me->tell_me("No files.\n", 0, TELL_NO_WRAP, 0, 0,0);
		return 1;
	}

	for(i = maxlen = 0; i < num; i += 2) {
		if ((j = strlen(dir[i])) > maxlen)
			maxlen = j;
	}
    /** most of flags **/

	if (flags & LS_l) {
	  if (maxlen > 20)
		maxlen = 20;
	  for (i = j = 0; i < num; i += 2) {
		name = dir[i];
		size = dir[i+1];
		if (size == -2) {
			dirall++;
			tmp_list = get_dir(path + name + "/", 0x22);
			if (tmp_list)
			  dir[j++] = sprintf("  (dir) %-:*s %2d file%s",
				maxlen, name, sizeof(tmp_list),
				(sizeof(tmp_list) == 1 ? "" : "s"));
			else
			  dir[j++] = sprintf("  (dir) %-:*s --error--",
				maxlen, name);
		} else {
			if (name[<2..<1] == ".c" && find_object(path + name))
				tmp = "*";
			else tmp = "";
			sizeall += size;
			blocks += (size+BLOCKSIZE2) >> BLOCKBITS;
			if (strlen(name) > maxlen) {
				name = name[0..maxlen-1];
				name[maxlen-1] = '#';
			}
			dir[j++] = sprintf("%7d %-:*s %s", size, maxlen,
			name, tmp);
		}
	  }
	  i = me->query_env("columns");
	  if (!intp(i) || i < TELL_MIN_X)
		i = TELL_STD_X;
	  me->tell_me(sprintf("%-*#s\n", i,
		//me->query_player_value(VAL_PLR_SCREEN_X),
		implode(dir[0..j-1], "\n"), "\n"), 0, TELL_NO_WRAP, 0, 0, 0);
	} else {
	  sflag = flags & LS_s;
	  Fflag = flags & LS_F;
	  for (i = j = 0; i < num; i += 2) {
		name = dir[i];
		size = dir[i+1];
		if (size == -2) {
			++dirall;
			if (sflag) {
				blocks++;
				dir[j++] = sprintf(" 1 %s/", name);
			} else {
				dir[j++] = sprintf("%s/", name);
			}
		} else {
		  sizeall += size;
		  if (Fflag && name[<2..<1]==".c" && find_object(path + name))
			name +="*";
		  if(sflag) {
		    blocks += (size+BLOCKSIZE2) >> BLOCKBITS;
		    size = (size+1023) >> 10;
		    if (size < 10) name = sprintf(" %d %s", size, name);
		    else if (size < 100) name = sprintf("%d %s", size, name);
		    else name = sprintf("** %s", name);
		}
		dir[j++] = name;
	    }
	  }
	  i = me->query_env("columns");
	  if (!intp(i) || i < TELL_MIN_X)
		i = TELL_STD_X;
	  me->tell_me(sprintf("%-*#s\n", i,
		//me->query_player_value(VAL_PLR_SCREEN_X),
		implode(dir[0..j-1], "\n"), "\n"), 0, TELL_NO_WRAP, 0, 0, 0);
	}

	if (sizeall || blocks) {
		if (blocks) {
			me->tell_me(sprintf("\nTotal of %d (%d) bytes in ",
			  sizeall, (blocks*BLOCKSIZE)),
			  0, TELL_NO_WRAP, 0, 0, 0);
		} else {
			me->tell_me(sprintf("\nTotal of %d bytes in ", sizeall),
			  0, TELL_NO_WRAP, 0, 0, 0);
		}
	} else {
		me->tell_me("\nTotal ", 0, TELL_NO_WRAP, 0, 0, 0);
	}
	num /= 2;	/* As the array contained 2 entries for each file/dir. */
	if (num != dirall) {
		me->tell_me(sprintf("%d files%s", (num-dirall),
			(dirall ? ", " : "")), 0, TELL_NO_WRAP, 0, 0, 0);
	}
	if (dirall) {
		me->tell_me(sprintf("%d directories.", dirall));
	} else me->tell_me(".\n", 0, TELL_NO_WRAP, 0, 0, 0);
	return 1;
}

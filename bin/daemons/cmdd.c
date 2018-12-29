/****************************************************************
*								*
* Module:							*
*	/bin/daemons/cmdd.c					*
* Description:							*
*	The daemon that handles the execution of almost all	*
*	commands players and coders enter (the few exceptions	*
*	are certain commands that need exceptional security).	*
*	The commands are passed by /lib/player.c calling	*
*	function 'do_command()'; the daemon checks that the	*
*	caller is a valid interactive user and then finds out	*
*	which command object is to be called for the actual	*
*	execution of the command.				*
* Version:							*
*	1.0a, Aug-98.						*
* Last update:							*
*	22-Aug-98 by Doomdark					*
*								'
* Modifications:						*
*								*
****************************************************************/

#include <daemons.h>

#define PUB_CMD_PATH	"/bin/pub"
#define USR_CMD_PATH	"/bin/usr"
#define	LORD_CMD_PATH	"/bin/lord"
#define SKILL_CMD_PATH "/bin/skills"
#define FEEL_CMD_PATH	"/bin/feel"
#define RESOLVE_PATH(P,O) (objectp(O)?(string)(O)->absolute_path(P):(P))

#define	F_PUB_CMDS	1
#define	F_USR_CMDS	2
#define	F_LORD_CMDS	4
#define	F_SKILL_CMDS	8

int CmdsLoaded;		// Used for efficiency.
mapping PlayerCmds, CoderCmds, LordCmds, SkillCmds;

mapping hash;
status cmds_loaded;
string *feelings;	// Standard feelings that used to reside in soul.c

int rehash(string path);

void
rehash_commands()
{
	if (!this_player()) return;
	if (!feelings)
		feelings = (string *) FEEL_D -> get_feelings();
	rehash(PUB_CMD_PATH);

// Let's be sure we do have some commands...

	if (!mappingp(hash) || !mappingp(hash[PUB_CMD_PATH])
	  || !sizeof(hash[PUB_CMD_PATH])) return;

	rehash(USR_CMD_PATH);
	rehash(SKILL_CMD_PATH);
	rehash(LORD_CMD_PATH);
	cmds_loaded = 1;
}

void
create()
{
	hash = ([ ]);
	cmds_loaded = 0;
}

void
reset()
{
    if (!hash) create();
}

// This is the reason for the daemon.  Returns mapping that contains full
// paths to all commands.

varargs mapping
query_hash( string path_list, object player )
{
string *paths;
mapping ret;
int i;

    if( !stringp( path_list ) ) return 0;
    i = sizeof( paths = explode( path_list, ":" ) );
    ret = ([ ]);
    while( i-- ) {
	paths[i] = RESOLVE_PATH(paths[i], player);
	if( !hash[ paths[i] ] && !rehash( paths[i] ) ) continue;
	ret += hash[ paths[i] ];
    }
    return ret;
}

// This function updates information in single directory.
int
rehash( string path )
{
string *files;
string d, d2;
string path_add, str;
mapping map;
int i;
	if (stringp(path) && sscanf(path, "%s %s", d, d2) < 2 &&
	(files = get_dir(path+"/_*.c"))) {
	        path_add = path + "/_";
		map = m_allocate(sizeof(files));
		for( i = sizeof( files ); i--; ) {
		    sscanf(files[i], "_%s.c", str);
		    map += ([ str : ({ path_add + str, 0, 0, 0, 0.0 }) ]);
	        }
		if (path[0] != '/') path = "/" + path;
		hash[path] = map;
		switch (path) {
		    case PUB_CMD_PATH:
			PlayerCmds = map;
			break;
		    case USR_CMD_PATH:
			CoderCmds = map;
			break;
		    case LORD_CMD_PATH:
			LordCmds = map;
			break;
		    case SKILL_CMD_PATH:
		        SkillCmds = map;
			break;
		}
		return 1;
	}
	return 0;
}

nomask public object cmdd() { return this_object(); }

nomask public string*
query_paths(string path, object player)
{
string s, t, *c;
int i;
	while (stringp(path) && sscanf(path, "%s:%s", s, t) == 2) {
		path = t;
		s = RESOLVE_PATH(s, player);
		if (!hash[s] && !rehash(s)) continue;
		if (!c) c = ({ });
		c += ({ s });
	}

	if (stringp(path) && sizeof(path)) {
		s = RESOLVE_PATH(path, player);
		if (hash[s] || rehash(s)) {
			if (!c) c = ({ });
			c += ({ s });
		}
	}

	return c;
}

// Following routine actually tries to execute the given command.
// Note that monster give 0 as first argument; maybe players should too?

public int
do_command(string *c, string v, string s, int is_coder)
{
  int i, ecost;
  object x;
  mixed *tmp;
  mapping dir;
  mixed cmd_file;

  if (!cmds_loaded)
    rehash_commands();

  if (feelings && member(feelings, v) >= 0)
    return (int) call_other(FEEL_D, v, s);

  if (!cmds_loaded) return 0;
  // Now; if we still don't have commands, let's return... :-(

  // First let's check "normal" commands. Common to players and monsters.
  if (mappingp(PlayerCmds)) {
    if (tmp = PlayerCmds[v]) {
      ecost = get_eval_cost();
      if (!tmp[1]) {
	i = (int) call_other(tmp[0], v + "_cmd", s, previous_object());
	x = find_object(tmp[0]);
	if (objectp(x)) {
	  tmp[1] = x;
	  tmp[2] = symbol_function(v + "_cmd", x);
	}
      } else {
	i = (int) funcall(tmp[2], s, previous_object());
      }
      // Let's keep count as well as total cost updated! -+ Doomdark +-
      if (i) {
	tmp[3]++;
	tmp[4] += ((ecost - get_eval_cost()) / 1000.0);
	return i;
      }
    }
  } else {
    // Let's try loading them now, then.
    rehash(PUB_CMD_PATH);
  }
#if 1
  if (mappingp(SkillCmds)) {
    if (tmp = SkillCmds[v]) {
      ecost = get_eval_cost();
      if (!tmp[1]) {
	i = (int) call_other(tmp[0], v + "_cmd", s, previous_object());
	x = find_object(tmp[0]);
	if (objectp(x)) {
	  tmp[1] = x;
	  tmp[2] = symbol_function(v + "_cmd", x);
	}
      } else {
	i = (int) funcall(tmp[2], s, previous_object());
      }
      // Let's keep count as well as total cost updated! -+ Doomdark +-
      if (i) {
	tmp[3]++;
	tmp[4] += ((ecost - get_eval_cost()) / 1000.0);
	return i;
      }
    }
  } else {
    // Let's try loading them now, then.
    rehash(SKILL_CMD_PATH);
  }
#endif

  if (mappingp(LordCmds)) {
    if (tmp = LordCmds[v]) {
      ecost = get_eval_cost();
      if (!tmp[1]) {
	i = (int) call_other(tmp[0], v + "_cmd", s, previous_object());
	x = find_object(tmp[0]);
	if (objectp(x)) {
	  tmp[1] = x;
	  tmp[2] = symbol_function(v + "_cmd", x);
	}
      } else {
	i = (int) funcall(tmp[2], s, previous_object());
      }
      // Let's keep count as well as total cost updated! -+ Doomdark +-
      if (i) {
	tmp[3]++;
	tmp[4] += ((ecost - get_eval_cost()) / 1000.0);
	return i;
      }
    }
  } else {
    // Let's try loading them now, then.
    rehash(LORD_CMD_PATH);
  }

  // And finally the coder commands... if we are coders

  if (!is_coder) return 0;

  if (mappingp(CoderCmds)) {
    if (tmp = CoderCmds[v]) {
      ecost = get_eval_cost();
      if (!tmp[1]) {
	i = (int) call_other(tmp[0], v + "_cmd", s, previous_object());
	x = find_object(tmp[0]);
	if (objectp(x)) {
	  tmp[1] = x;
	  tmp[2] = symbol_function(v + "_cmd", x);
	}
      } else {
	i = (int) funcall(tmp[2], s, previous_object());
      }
      // Let's keep count as well as total cost updated! -+ Doomdark +-
      if (i) {
	tmp[3]++;
	tmp[4] += ((ecost - get_eval_cost()) / 1000.0);
	return i;
      }
    }
  } else {
    // Let's try loading them now, then.
    rehash(USR_CMD_PATH);
  }

  // If it wasn't one of standard commands, perhaps it's a special
  // command defined by the coder in question?

  if (!c) return 0;

  for (i = sizeof(c) - 1; i >= 0; i--)
    if (tmp = hash[c[i]][v]) {
      // NO! We get it on the next line! - Graah
	if (stringp(tmp[0]) || objectp(tmp[0]))
	return (int) call_other(tmp[0], v + "_cmd", s, previous_object());
    }

  return 0;
}

// Let's not return the 'real' mapping here:
mapping
query_full_hash()
{
	return efun::copy(hash);
}

mixed
query_cmd_stats(string dir)
{
mapping x;
	if (mappingp(x = hash[dir]))
		return efun::copy(x);
	return 0;
}

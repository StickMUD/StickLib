/****************************************************************
*								*
* Module:							*
*	/secure/master.c.					*
* Description:							*
*	The master object for StickLib - distribution.		*
*	Modify with care; this object has access to everything	*
*	and thus is the place all cheaters try to hit. :-)	*
*								*
*								*
* Version:							*
*	1.0a Aug-98						*
* Last modified:						*
*	19-Aug-98, Doomdark					*
*								*
* NEW:								*
*								*
****************************************************************/

#pragma strong_types
#pragma strict_types

#include "/sys/driver_hook.h"
#include "/sys/interactive_info.h"
#include "/sys/object_info.h"

//#include "/include/driver.h" // If nothing is found broken delete this later
//#include "/include/driver_hook.h"

#include "/include/config.h"

#include "/include/coder_levels.h"
#include "/include/daemons.h"

#include "/include/mud_name.h"
// Areas.h needs to be included first... Due to few probs with
// include dirs otherwise. :-/
#include "/include/areas.h"
#include "/include/generic_rooms.h"

#define PLANFILE "/www/.plan"
#define	HTMLPLANFILE	"/www/finger.html"

// Some of these need to be without the leading slash, at least
// so long as we're in compat mode:
#define INIT_FILE	"/secure/init_file"
#define	SHUT_D		"secure/shut"
#define	PLAYER_OBJECT	"lib/player"
#define	PLAYER_OBJECT2	"lib/player.c"
#define	PLAYER_CLONE	"lib/player#%d"
#define	PLAYER_CLONE2	"lib/player#"
#define	SIMUL_EFUN	"secure/simul_efun"
#define PLAYERFILE	"secure/login"
#define	PURGATORY	"room/purgatory"
#define	DEFAULT_OWNER	"<NONE>"
#define	MET_DIR		"data/met/%s"
#define	REAL_NAME(x)	((string) x->query_real_name())
#define	CODER_LEVEL(x)	((int) x->query_coder_level())
#define	GET_NAME(x)	((string)x->query_name())
#define	DESTRUCT(x)	destruct(x)

#define	CODER_ROOT_DIR	"/u/"

#define USER_IS(x)	(this_interactive() && (int)this_interactive()\
			->query_coder_level() >= x)

// Time of boot
int _reboot_time;

// Mapping containing access data of all directories; the access rights
// are read from file ".ACCESS" from the directories when they are
// accessed for the first time. 
private static mapping _access;

// Mapping containing name and coder level of all the coders (and, actually,
// players) accessing the files; these values are only fetched once, when
// the player tries to access a file, and access is verified.
// NEW: Also contains "met"-data; 3rd element...
private static mapping _players;

#define	_MASTER_C

#include "/secure/simul_efun/amylaar.c"

int check_access(string path, mixed caller, string func, status write);
#include "/secure/ftp.c"

/*
 * Define AUTOREBOOT if you want the mud automatically reboot every work day
 * at approximately 10:10 am. We handle that by asking
 * /secure/shut to reboot the game in 30 mins at 9:30.
 */
#define AUTOREBOOT
#undef AUTOREBOOT

/*
 * Define WORKING_HOURS if you want mortals to be kicked out every workday
 * (that's monday-friday) at 10.00 am. If defined, they will be also warned
 * at frequent intervals from 9.00 9.15 9.30 9.45 9.50 9.55 9.57 9.58 to 9.59.
 *
 * Another part of the 'working hours' system resides at /room/church.c and
 * /lib/player.c, it blocks players from entering the game during working
 * hours. This define does not apply there.
 */
#undef WORKING_HOURS

/*
 * Define FINGER_WHO if you want the mudlib to automatically write the
 * player list into the .plan file at /ftp/.plan, so that it can be fingered
 * from outside.
 */
#define FINGER_WHO

int next_reboot_time;    /* Timestamp of our next scheduled reboot */

/****************************************************************
*								*
*			Query-functions:			*
*								*
****************************************************************/

int
query_reboot_time()
{
	return _reboot_time;
}

status
exists_player(string s)
{
#if 0
	return file_size(sprintf("/data/plr/%s.o", lower_case(s))) > 0;
#else
	return file_size(sprintf("%s.o", PATH_FOR_PLAYER_SAVE(lower_case(s)))) > 0;
#endif
}

mixed *
get_player_data(object x)
{
mixed *y;
	if (!x) return 0;
	if (!(y = _players[x])) {
	  y = ({
		REAL_NAME(x),
		CODER_LEVEL(x),
	  });
	  _players[x] = y;
	}
	return y;
}

mixed *
get_full_player_data(object x)
{
mixed *y;
	if (!x) return 0;
	if (!(y = _players[x])) {
	  y = ({
		REAL_NAME(x),
		CODER_LEVEL(x),
	  });
	  _players[x] = y;
	}
	return y;
}

// A file ".ACCESS" on a dir we're trying to access might look like this:

//owner:Doomdark
//100:r
//200:rw
//:Kelgath:rw:Foobar:r:

// Resulting array would look like:
// ({ "Doomdark", 100, "r", 200, "rw", "Kelgath:rw:Foobar:r:" })
// This would mean that the owner of the directory is Doomdark, and
// coders on (and above) coder level 100 can read the files in this dir;
// and on coder level 200 and up can also write to the dir. "Extra" access
// is defined so that Kelgath has both read and write access (like the
// coders on level 200+ had), and Foobar has the read access.

nomask status
load_access_data(string dir)
{
mixed *data;
string tmp, owner;
int level;
	if (file_size(dir) != -2)
		return 0;
	if (file_size(tmp = sprintf("%s.ACCESS", dir)) < 0)
		_access[dir] = ({ DEFAULT_OWNER });
	else {
		tmp = read_file(tmp);
		sscanf(tmp, "owner:%s\n%s", owner, tmp);
		data = ({ owner });
		while (sscanf(tmp, "%d:%s\n%s", level, owner, tmp) == 3)
			data += ({ level, owner });
		data += ({ tmp });
		_access[dir] = data;
	}
	return 1;
}

// This will simply rewrite the access-file in the specified directory...
static status
save_access_data(string dir)
{
mixed *data;
string afile;
int i;
	if (file_size(dir) != -2)
		return 0;
	if (!(data = _access[dir])) {
		if (!load_access_data(dir))
			return 0;
		data = _access[dir];
	}
	if (!sizeof(data))
		data = ({ DEFAULT_OWNER });
	afile = sprintf("%s/.ACCESS", dir);
	if (file_size(afile) >= 0)
		rm(afile);
	write_file(afile, sprintf("owner:%s\n", data[0]));
	for (i = 2; i < sizeof(data); i += 2)
		write_file(afile, sprintf("%d:%s\n", data[i-1], data[i]));
	if (sizeof(data) > 1 && !(sizeof(data) & 1)) {
		if (data[<1][<1] == '\n')
			write_file(afile, data[<1]);
		else
			write_file(afile, sprintf("%s\n", data[<1]));
	}
	return 1;
}

private mixed
get_access_data(string dir)
{
mixed data;
	if (!(data = _access[dir])) {
		load_access_data(dir);
		return _access[dir];
	}
	return data;
}

int
set_access_data(string dir, mixed who, string acc)
{
int i;
string rn, a, b, c, fn;
mixed data, old_data;
	if (dir[<1] != '/')
		dir = dir + "/";
	if (dir[0] != '/')
		dir = "/"+dir;
	if (file_size(dir) != -2)
		return 0;
	if (!(data = get_access_data(dir)))
		return 0;
/* Can only call this function from the player object... */
/* Or from simul_efun... */
	if ((fn = object_name(previous_object())) == "secure/simul_efun") {
		if (acc != "owner" || !stringp(who))
		    if(acc != "r"  ||  who != 0)
			return 0;
		old_data = data;
		if(acc == "r")
			data += ({ who, acc });
		else
			data[0] = who;
		_access[dir] = data;
		if (save_access_data(dir))
			return 1;
		_access[dir] = old_data;
		return 0;
	}
	if ((!interactive(previous_object()) || !sscanf(fn, PLAYER_CLONE, i)))
		return 0;
	i = CODER_LEVEL(previous_object());
	rn = REAL_NAME(previous_object());
// Only (co)admins or the owner can change the access...
	if (i < LVL_COADMIN && rn != data[0])
		return 0;
	old_data = data;
// Adding level-based access-rights?
	if (intp(who)) {
// Already have access set for this coder level?
		if ((i = member(data, who)) >= 0) {
		  if (!acc) {
			data = data[0..i-1] + data[i+2..];
		  } else {
			data[i+1] = acc;
		  }
		} else if (acc) {
// Nope. Then we need to decide where to put it...
			i = 1;
			while (i < sizeof(data) && intp(data[i]) && data[i] < who)
				i += 2;
			data = data[0..i-1] + ({ who, acc }) + data[i..];
		}
// ... or player-specific ones?
	} else {
		who = lower_case(who);
// Are we changing the owner of the dir; or
	  if(acc == "owner") {
		data[0] = who;
// just modifying the access
	  } else {
		if (sizeof(data) < 2 || (sizeof(data) & 1)) {
		  if (acc)
			data += ({ sprintf(":%s:%s:", who, acc) });
		} else {
		  if (sscanf(data[<1], ("%s:"+who+":%s:%s"), a, b, c) < 2) {
			if (!acc) ;
			else {
			  a = data[<1];
			  if (sizeof(a) && a[<1] == ':')
				a = sprintf("%s%s:%s:", a, who, acc);
			  else
				a = sprintf("%s:%s:%s:", a, who, acc);
			  data[<1] = a;
			}
		  } else {
			if (!acc)
				data[<1] = sprintf("%s:%s", a, c);
			else
				data[<1] = sprintf("%s:%s:%s:%s", a, who, acc, c);
		  }
		}
	  }
	}
// *Sigh* we still have to be careful here... :-/
	_access[dir] = data;
	if (save_access_data(dir))
		return 1;
	_access[dir] = old_data;
	return 0;
}

#if 0
mapping
query_access_data()
{
	return copy_mapping(_access);
}

mapping
query_coder_data()
{
	return copy_mapping(_players);
}
#endif

/************************************************************************
*									*
*			File access functions...			*
*									*
************************************************************************/

int
check_access(string path, mixed caller, string func, status write)
{
mixed *pl_data;
mixed *acc_data;
int cl, cl2, i;
string rn, file, rn2, tmp;
	if (pointerp(caller)) {
		rn = caller[0];
		cl = caller[1];
	} else {
		pl_data = get_player_data(caller);
		cl = pl_data[1];
		rn = pl_data[0];
	}
	if (cl >= LVL_COADMIN)
		return 1;
// Player object needs to be able to restore_/save_object from/to /data/plr...
	if (!strstr(path, "/data/plr/") && (func == "save_object"
	  || func == "restore_object"))
		return 1;
// Also, let's let them have full access to their home dir; an easy hack here.. :-/
	if (sscanf(path, CODER_ROOT_DIR "%s", rn2)) {
		if (sscanf(rn2, "%s/%s", rn2, tmp) == 2) {
			if (lower_case(rn) == rn2)
				return 1;
		} else if (lower_case(rn) == rn2)
			return 1;
	}
	i = sizeof(path);
	while (--i)
		if (path[i] == '/')
			break;
	file = path[i + 1..];
	path = path[0 .. i];
	acc_data = get_access_data(path);

// Owner of a dir can edit it as he/she wishes to...
	if (sizeof(acc_data) && rn == acc_data[0])
		return 1;
// But, only owner (or a (co)admin) can alter the access rights!
	if (file == ".ACCESS" && write)
		return 0;
	for (i = 2; i < sizeof(acc_data); i += 2) {
	  if (cl < acc_data[i-1]) break;
	  if (write) {
		if (member(acc_data[i], 'w') >= 0)
			return 1;
	  } else {
		if (member(acc_data[i], 'r') >= 0)
			return 1;
	  }
	}
	if (sizeof(acc_data) > 1 && !(sizeof(acc_data) & 1)) {
	  tmp = sprintf(":%s:", rn);
	  if ((i = strstr(acc_data[<1], tmp)) >= 0) {
	   tmp = acc_data[<1][i+sizeof(tmp)..];
	    if (write) {
		if (!strstr(tmp, "w:") || !strstr(tmp, "rw:") || !strstr(tmp, "wr:"))
			return 1;
	    } else {
		if (!strstr(tmp, "r:") || !strstr(tmp, "rw:") || !strstr(tmp, "wr:"))
			return 1;
	    }
	  }
	}
	if (!strstr(path, "/open"))
		return 1;
	if (!strstr(path, "/doc")) {
		if (!write)
			return 1;
		return (cl >= LVL_SENIOR) ? 1 : 0;
	}
	if (!strstr(path, "/log")) {
		if (!write)
			return 1;
		if (cl >= LVL_SENIOR || func == "write_file"
		|| func == "save_object" || func == "write_bytes")
			return 1;
		return 0;
	}
	return 0;
}


// Validate a reading/writing file operation.
//
// Arguments:
//   path   : The (possibly partial) filename given to the operation.
//   euid   : !euids: 0;
//             euids: the euid of the caller (might be 0).
//   fun    : The name of the operation requested (see below).
//   caller : The calling object.
//
// Result:
//   The full pathname of the file to operate on, or 0 if the action is not
//   allowed.
//   You can also return 1 to indicate that the path can be used unchanged.
//
// The path finally to be used must not contain spaces or '..'s .
//
// These are the central functions establishing the various file access
// rights.
//
// Note that this function is called in compat mode as well!
// If you need to be compatible with the old 2.4.5-mudlib, redirect these
// calls to the valid_read/valid_write in the player object.
//
// valid_read() is called for these operations:
//   ed_start        (when reading a file)
//   file_size
//   get_dir
//   print_file
//   read_bytes
//   read_file
//   restore_object
//   tail
//
// valid_write() is called for these operations:
//   cindent
//   ed_start     (when writing a file)
//   do_rename    (twice for each the old and new name)
//   mkdir
//   save_object
//   remove_file
//   rmdir
//   write_bytes
//   write_file

mixed
valid_read(string path, string euid, string fun, object caller)
{
string dummy, name;
string *caller_dir, *dest_dir;
int i;
	if (!stringp(path)) return 0;
	if (caller == this_object())
		return path;
	if (strstr(path, "/../", 0) >= 0)
		return 0;
	if (path == "/")
		return path;
/* Need to take care of excessive leading & trailing slashes... */
	while (path[i] == '/')
		i++;
	if (!i)
		path = "/"+path;
	else if (i > 1)
		path = path[i-1..];

// Coders have some "extra rights" when using standard file handling
// commands...
	if (objectp(caller))
		name = object_name(caller);
	else name = "UNKNOWN";
/* Have to strip the leading slash off... */
	if (name[0] == '/')
		name = name[1..];

	if (!strstr(name, PLAYER_CLONE2)) {
// Kludge to allow player object to use restore_object when player logs in. :-/
		if (!strstr(path, "/data/plr/") && fun == "restore_object")
			return path;
		if (check_access(path, caller, fun, 0))
			return path;
	} else if (!strstr(name, "bin/usr/")) {
		if (check_access(path, this_interactive(), fun, 0))
			return path;
	} else if (sscanf(name, "bin/daemons/%s", dummy)) {
// Daemons have access to their 'own' dirs; but nowhere else...
		if (sscanf(path, sprintf("/data/d/%s/%%s", dummy), dummy))
			return path;
		if ((dummy == "fingerd" || dummy == "stat_d")
		    && !strstr("/data/plr/", path))
			return path;
	}

	dest_dir = explode(path[1..<1], "/");
	caller_dir = explode(name, "/");

	if (sizeof(dest_dir) < 2)
		return path;	// Ok to read at main level.
	if (caller_dir[0] == "secure")
		return path; // Always ok from secure...

	switch (dest_dir[0]) {
	case "admin":
		return 0;	// No access. Or should admins have?
	case "u":
// Rule 1: It's ok for an object under some subdir of /u to
// read files on same subdir....
		if (caller_dir[0] == dest_dir[0] && dest_dir[1] == caller_dir[1])
			return path;
		if (!strstr(name, "bin/daemons/cmdd"))
			return path;
		return 0;
// Otherwise, only coders can read them, not non-interactive objects. Then again,
// public code shouldn't be located in /u anyway, only temporary stuff.

	case "data":
		if (caller_dir[0] == "bin" && (caller_dir[1] == "daemons"
		  )) return path;
// Let's allow /bin - objects (daemons, commands) to read the data.
// This one's bit kinky... :-/
		if (sizeof(dest_dir) < 3) return path;
// Let 'em ls the dir and such, at least...
// (a kludge to let player object read it ok: )
		if (dest_dir[1] == "plr" &&
			fun == "restore_object" &&
		  sscanf(object_name(caller), PLAYER_CLONE, dummy))
			return path;
// This will allow, for example /guild/priest/symbol.c to read sub-directories
// of /data/guild/priest.
		if (sizeof(caller_dir) >= 2
		  && dest_dir[1] == caller_dir[0]
		  && dest_dir[2] == caller_dir[1]) return path;
	case "guilds":
		if (caller_dir[0] != "guilds") return 0;
		if (caller_dir[1] == dest_dir[1]) return path;
		return 0;
	}
// But main idea is; if it's not prevented, it's allowed (for reading).
/* Exception being /guilds-dir. Thank you idiotic newbie coders telling
 * players too much info. -+ Doomdark +-
 */
	return path;
}

mixed
valid_write(string path, string euid, string fun, object caller)
{
string dummy, name;
string *caller_dir, *dest_dir;
int i;

	if (strstr(path, "..", 0) >= 0)
		return 0;
	if (caller == this_object())
		return 1;
	i = 0;

	if (path != "/") {
/* Need to take care of excessive leading & trailing slashes... */
	 while (path[i] == '/')
		i++;
	 if (!i)
		path = "/"+path;
	 else if (i > 1)
		path = path[i-1..];
	}

// Coders have some "extra rights" when using standard file handling
// commands...

	if (objectp(caller))
		name = object_name(caller);
	else name = "UNKNOWN";
/* Have to strip the leading slash off... */
	if (name[0] == '/')
		name = name[1..];

	if (!strstr(name, PLAYER_CLONE2)) {
		if (check_access(path, caller, fun, 1))
			return path;
	} else if (!strstr(name, "bin/usr/")) {
		if (check_access(path, this_interactive(), fun, 1))
			return path;
	} else if (sscanf(name, "bin/daemons/%s", dummy)) {
// Daemons have access to their 'own' dirs; but nowhere else...
		if (sscanf(path, sprintf("/data/d/%s/%%s", dummy), dummy))
			return path;
		return 0;
	}

	dest_dir = explode(path[1..], "/");
	caller_dir = explode(name, "/");

// These objects should be able to do it:
	if (caller_dir[0] == "secure")
		return path;

	switch (dest_dir[0]) {
// First some public directories:
	case "open":
		return path;
	case "log":
		if (dest_dir[1] == "log")
			return 0;	// This log should be public, I guess...
		switch(caller_dir[0]) {
		case "std": case "bin":
			return path;
		}
		return 0;
	case "guilds":
// Guild objects should be able to write to their 'own' dir.
		if (dest_dir[0] == caller_dir[0] && dest_dir[1] == caller_dir[1])
			return path;
		return 0;
	case "admin":
	case "secure":
		return 0;
	case "u":
		if (sizeof(dest_dir) < 3)
			return 0;

// Rule 1: It's ok for an object under some subdir of /u to
// write files on same subdir....
		if (dest_dir[0] == caller_dir[0] && dest_dir[1] == caller_dir[1])
			return path;
		return 0;
	case "data":
// No. No write access to plr-savefile dir for anyone but (co)admins
// (and that's checked much earlier)
		if (sizeof(dest_dir) < 2 || dest_dir[1] == "plr")
			return 0;
// This will allow, for example /guild/priest/symbol.c to write sub-directories
// of /data/guild/priest.
		if (sizeof(caller_dir) >= 2 
		  && dest_dir[1] == caller_dir[0]
		  && dest_dir[2] == caller_dir[1])
			return path;
		return 0;
	}
// Still one rule to allow miscellaneous writing; we are allowed to write
// to "sibling" files and files in dirs in the directory we are;
	if (sizeof(caller_dir) <= sizeof(dest_dir)) {
		i = sizeof(caller_dir) - 1;
		while (--i >= 0) {
			if (caller_dir[i] != dest_dir[i]) break;
		}
		if (i == -1) return path;
	}

// Ok. Main rule; if write is not explicitly allowed, it's prevented. Simple.
	return 0;
}

/****************************************************************
*								*
*			Standard lfuns:				*
*								*
****************************************************************/

nomask void
create()
{
	_access = ([ ]);
	_players = ([ ]);
	_ftp_users = ([ ]);

	if (find_call_out("time_glass") < 0)
		call_out ("time_glass", 60);
#ifdef AUTOREBOOT

// Calculate next_reboot_time, the timestamp of next reboot. Do this by
// first calculating how many seconds it would take from this moment to
// the reboot and then add time() to it

// Autoreboots at 10:00 every work day. Let's cheat the time variable
// by reducing those 10 hours from it... Be sure to add it to the final value.

	t = time() - 36000;

  sscanf (ctime(t)[11..18], "%d:%d:%d", hour, min, sec);
  weekday = ctime(t)[0..2];

  /* Now we can very easily calculate how long it is till midnight */

  sec = (59 - sec) + 60 * (59 - min) + 60 * 60 * (23 - hour);

  /* So sec now stands for seconds we have left for this day. For Mon, */
  /* Wed and Sat add one day, two for Fri. */
/*
  if (weekday == "Fri" || weekday == "Tue")
    sec += 86400*2;
  else if (weekday == "Mon")
    sec += 86400*3;
  else if (weekday == "Wed" || weekday == "Sat")
    sec += 86400;
*/
  if (weekday == "Fri")
    sec += 86400*2;
  else if (weekday == "Sat")
    sec += 86400;

  /* Finally add the seconds until reboot to the current time stamp, and */
  /* voila! Don't forget to add the 36000 seconds we borrowed. */

  next_reboot_time = sec + t + 36001;

#endif /* AUTOREBOOT */
}

/****************************************************************
*								*
*		And then other lfuns:				*
*								*
****************************************************************/

nomask void
time_glass()
{
int hour, min;
string day;
#ifdef FINGER_WHO
object *u;
string s, *st;
int i;
#endif
	if (find_call_out("time_glass") == -1)
		call_out ("time_glass", 60);

	sscanf (ctime(time())[11..15], "%d:%d", hour, min);
	day = ctime(time())[0..2];
#ifdef FINGER_WHO
  u = users();
  st = ({ });
  for (i = sizeof(u) - 1; i >= 0; i--)
    if (u[i] && !u[i]->query_invis())
      st += ({ capitalize(REAL_NAME(u[i])) });
  s = "\n"+MUD_NAME+" at "+ctime(time())+" EET:\n\n";
  if (!sizeof(st))
    s += "No visible players in "+MUD_NAME+".\n\n";
  else
	s = sprintf("%sTotal of %d visible players in "+MUD_NAME+":\n\n%s\n\n"
+MUD_NAME+" is located at <address of the mud> <port>.\n\n",
s, sizeof(st), sprintf("%-=78s", implode(sort_array(st, #'<), ", ", " and ")));
	rm (PLANFILE);
        rm (HTMLPLANFILE);
	write_file (PLANFILE, s);
//NEW! A HTML version of the finger information 10.1.1996 //F
        write_file(HTMLPLANFILE, "<pre>");
        write_file(HTMLPLANFILE, s);
        write_file(HTMLPLANFILE, "</pre>");
#endif
#ifdef AUTOREBOOT
	if (time() > (query_reboottime() - 1800) /*1800=30 mins before reboottime*/
	  && !find_object(SHUT_D)) {
	  if (query_uptime() < 300) { /* If we have been up for lesser than */
		create();  /* 5 minutes it is possible we just experienced a */
	  } else {      /* reboot and reboottime is messed. Re-calculate. */
		SHUT_D->shut(30);
	  }
	}
#endif /* AUTOREBOOT */
#ifdef WORKING_HOURS
  if (day != "Sat" && day != "Sun" &&
      /* The standard holidays //Graah */
      (member_array(ctime(time())[4..9],
		      ({
			"May 25", /* Another "holy" day shit...temporary. */

			"May  1", /* Communists' day */
			"Dec  6", /* Finland Indepencece Day */
			"Dec 24", /* Some religious shit... */
			"Dec 25", /* which goes on and on... */
			"Dec 26", /* And on... */
			"Jan  1", /* 1996, monday */
			"Jan  2", /* 1996 temporary */
			"Dec 27"  /* Temporary (1995) */
		      })) == -1)
      )
    {
      if (hour == 9)
	{
	  if (member_array (min, ({ 0,15,30,40,45,50,55,57,58,59 })) != -1) {
	    SHOUT ("Armageddon shouts: All mortals will be kicked out in " +
		   (60-min) + " minute" + (min==60?"!\n":"s!"));
	  }
      } else if (hour == 10 && min >= 0 && min <= 10) {
	/* Kicked only at 10.00, sometimes let players continue playing...
	   now it checks them for 10 minutes. //Graah */
	object *wh_u;
	int wh_i;
	for (wh_i = 0, wh_u = users(); wh_i < sizeof(wh_u); wh_i++) {
	  if (!wh_u[wh_i] || wh_u[wh_i]->query_coder_level() ||
	      wh_u[wh_i]->query_is_testplayer())
	    continue;
	  wh_u[wh_i]->tell_me(
	      "Armageddon tells you: I will kick you out now. " +
	      "Welcome back in 6 hours.");
	  remove_interactive (wh_u[wh_i]);
	}

	/* Throw mortal link deads too */
	PURGATORY->bail_out(1);

      }
    }
#endif /* WORKING_HOURS */
}

void
SHOUT(string msg)
{
object *u;
int i;
	u = users();
	for (i = sizeof(u) - 1; i >= 0; i--)
		if (u[i]) u[i]->tell_me(msg);
}

// parse "-fcall yy xx arg" "-fshutdown"
// This looks BS. :-/

nomask void
flag(string str)
{
	printf("master: Unknown flag %s.\n", str);
}

//---------------------------------------------------------------------------
static string _auto_include_hook (string base_file, string current_file, int sys_include)
// Optional string to be included when compiling <base_file>.
//
// Argument:
//   base_file: The file to be compiled.
//   current_file: When handling #include statements, the file to be included
//   sys_include:  1, when <current_file> is a system include
//

{
    // Do nothing for includes.
    if(current_file)
        return 0;

    // Add the light mechanism to every object except the light object itself.
    // And of course ignore master and simul-efun.
    if(base_file[0] != '/')
        base_file = "/" + base_file;

    if(member((["/lib/light.c", "/secure/simul_efun.c", "/secure/simul_efun/amylaar.c", "/secure/master.c" ]), base_file))
        return 0;

    return "virtual inherit \"/lib/light\";\n";
}

// Perform mudlib specific setup of the master.
//
// Argument:
//   arg: 0 if the mud just started.
//        1 if the master is reactivated destructed one.
//        2 if the master is a reactivated destructed one, which lost all
//             variables.
//        3 if the master was just reloaded.
//
// This function is called whenever the master becomes fully operational
// after (re)loading (it is now recognized as _the_ master).
// This doesn't imply that the game is up and running.
//
// This function has at least to set up the driverhooks to use. Also, any
// mudwho or wizlist handling has to be initialized here.
//
// Besides that, do whatever you feel you need to do,
// e.g. set_auto_include_string(), or give the master a decent euid.
nomask void
inaugurate_master(int arg)
{
	if (arg == 0) {
		_reboot_time = time();
	} else {
		_reboot_time = (int) "/secure/simul_efun"
			-> query_uptime();
	}

    // Updated efun308 to set_environment

    set_driver_hook(
      H_MOVE_OBJECT0,
      unbound_lambda( ({'item, 'dest}), ({#',,
	({#'efun::set_environment, 'item, 'dest}),
	({#'?, ({#'living, 'item}), ({#',,
	  ({#'efun::set_this_player, 'item}),
	  ({#'call_other, 'dest, "init"}),
	  ({#'?, ({#'!=, ({#'environment, 'item}), 'dest}), ({#'return})}),
	}) }),
	({#'=, 'others, ({#'all_inventory, 'dest}) }),
	({#'=, ({#'[, 'others, ({#'member, 'others, 'item}) }), 0}),
	({#'filter, 'others,
	  ({#'bind_lambda,
	    unbound_lambda( ({'ob, 'item}),
	      ({#'?, ({#'living, 'ob}), ({#',,
		({#'efun::set_this_player, 'ob}),
		({#'call_other, 'item, "init"}),
	      }) })
	    )
	  }),
	  'item,
	}),
	({#'?, ({#'living, 'item}), ({#',,
	  ({#'efun::set_this_player, 'item}),
	  ({#'filter_objects, 'others, "init"}),
	}) }),
	({#'?, ({#'living, 'dest}), ({#',,
	  ({#'efun::set_this_player, 'dest}),
	  ({#'call_other, 'item, "init"}),
	}) }),
      }) )
    );

// Stick-specific; don't care about flags here. -+ Doomdark +-

    set_driver_hook(
      H_LOAD_UIDS,
      unbound_lambda( ({'object_name}),
	({#'?,
	  ({#'==,
	    ({#'sscanf, 'object_name, "u/%s", 'wiz_name}),
	    1,
	  }),
	  ({#'?,
	    ({#'==,
	      ({#'sscanf, 'wiz_name, "%s/%s", 'start, 'trailer}),
	      2,
	    }),
	    ({#'&&, ({#'sizeof, 'start}), 'start}),
	    'wiz_name
	  }),

	  ({#'==,
	    ({#'sscanf, 'object_name, "areas/%s", 'area_name}),
	    1,
	  }),
	  ({#'?,
	    ({#'==,
	      ({#'sscanf, 'area_name, "%s/%s", 'start, 'trailer}),
	      2,
	    }),
	    ({#'&&, ({#'sizeof, 'start}), 'start}),
	    'area_name
	  }),

	  ({#'&&,
	    ({#'!=, ({#'[..], 'object_name, 0, 3}), "www/"}),
	    ({#'!=, ({#'[..], 'object_name, 0, 4}), "open/"}),
	  })
	})
      )
    );
    set_driver_hook(
      H_CLONE_UIDS,
      unbound_lambda( ({'blueprint, 'new_name}), ({
	#'||,
	  ({#'creator, 'blueprint}),
	  ({#'creator, ({#'previous_object})}),
	  1
      }) )
    );

// Note; no arguments here; this way will call lfuns of the object in
// question?
// These lambdas will call "create()" if it's defined, otherwise "reset(0)".

	set_driver_hook(H_CREATE_SUPER, 0);
// What would we need that for? Idea of masking create is to block it,
// right??
	set_driver_hook(H_CREATE_OB, unbound_lambda( ({ 'x }),
		({ #'?, ({ #'function_exists, "create", 'x }),
			({ #'call_other, 'x, "create" }),
			({ #'call_other, 'x, "reset", 0 }) }) ));
	set_driver_hook(H_CREATE_CLONE, unbound_lambda( ({ 'x }),
		({ #'?, ({ #'function_exists, "create", 'x }),
			({ #'call_other, 'x, "create" }),
			({ #'call_other, 'x, "reset", 0 }) }) ));

	set_driver_hook(H_RESET,        "reset");
	set_driver_hook(H_CLEAN_UP,     "clean_up");

	set_driver_hook(H_MODIFY_COMMAND, "modify_command");
	set_driver_hook(H_MODIFY_COMMAND_FNAME, "modify_command");

	set_driver_hook(H_NOTIFY_FAIL, unbound_lambda( ({ 'cmd }),
		({ #'call_other, "/bin/daemons/remarks",
			"query_random_notify_fail", 'cmd }) ));

	set_driver_hook(H_INCLUDE_DIRS, ({
		"/include/std/", "/include/",
// "/secure/", "/room/"
        }) );

	set_driver_hook(H_AUTO_INCLUDE, #'_auto_include_hook);

	if (!_access) create();
}

/*
 * Function name:   get_root_uid
 * Description:     Gives the uid of the root user
 * Returns:         Name of the 'root' user
 */
nomask string
get_root_uid()
{
	return "root";
}

/*
 * Function name:   get_bb_uid
 * Description:     Gives the uid of the backbone
 * Returns:         Name of the 'backbone' user
 */
nomask string
get_bb_uid()
{
	return "backbone";
}

// Return the string to be used as uid (and -euid) of a (re)loaded master.
// Under !native, the function may also return a non-zero number.
// In that case, the uid is set to 0, as is the euid.
nomask string
get_master_uid()
{
	return "root";
}

// Perform final actions before opening the game to players.
//
// Arguments:
//   eflag: This is the number of '-e' options given to the parser.
//          Normally it is just 0 or 1.
//
// Result:
//   An array of strings, which traditionally designate the objects to be
//   preloaded with preload().
//   Any other result is interpreted as 'no object to preload'.
//   The resulting strings will be passed one at the time as
//   arguments to preload().
nomask string *
epilog(int eflag)
{
string inf, *ini;
int i;

	inf = read_file(INIT_FILE);
	ini = explode(inf, "\n");

	for (i = sizeof(ini) - 1; i >= 0; i--)
	  if (!sizeof(ini[i]) || ini[i][0] == '#')
		ini[i] = 0;

	ini -= ({ 0 });


	printf("About to preload %d objects.\n", sizeof(ini));
	return ini;
}

// Preload a given object.
//
// Arguments:
//   file: The filename of the object to preload, as returned by epilog().
//
// It is task of the epilog()/preload() pair to ensure the validity of
// the given strings (e.g. filtering out comments and blank lines).
// For preload itself a 'call_other(file, "???")' is sufficient, but it
// should be guarded by a catch() to avoid premature blockings.
// Also it is wise to change the master's euid from master_uid to something
// less privileged for the time of the preload.
//
// You can of course do anything else with the passed strings - preloading
// is just the traditional task.
nomask void
preload(string file)
{
//  seteuid("preload");
	catch(call_other(file, "?"));
//  seteuid(getuid());
}

// Master was reloaded on external request by SIGUSR1.
//
// If the gamedriver destruct and reloads the master on external request
// via SIGUSR1, it does this by a call to this function.
// It will be called after inaugurate_master() of course.
// If you plan to do additional magic here, you're welcome.
nomask void
external_master_reload()
{
}

// Reactivate a formerly destructed master.
//
// Arguments:
//   removed: True if the master was already on the list of destructed
//            objects.
//
// This function is called in an formerly destructed master since a new master
// couldn't be loaded.
// This function has to reinitialize all variables at least to continue
// operation.
nomask void
reactivate_destructed_master(int removed)
{
}

// Notify the loss of the erq demon.
//
// Argument:
//   callback: the callback closure set for an erq request.
//
// If the erq connection dies prematurely, the driver will call this lfun for
// every pending request with set callback. This function should notify the
// originating object that the answer will never arrive.
nomask void
stale_erq(closure callback)
{
}

// Return a printable name for an object.
//
// Arguments:
//   obj: The object which name is of interest.
//
// Result:
//   A string with the objects name, or 0.
//
// This function is called by sprintf() to print a meaningful name
// in addition to the normal file_name().
// If this functions returns a string, the object will be printed
// as "<filename> (<obj_name>)".
// This used to be object_name
nomask string
detail_name(object obj)
{
	return GET_NAME(obj);
}

// Prepare the destruction of the given object.
//
// Argument:
//   obj : The object to destruct.
//
// Result:
//   Return 0 if the object is ready for destruction, any other value
//   will abort the attempt.
//   If a string is returned, an error with the string as message will
//   be issued.
//
// The gamedriver calls this function whenever an object shall be destructed.
// It expects, that this function cleans the inventory of the object, or
// the destruct will fail.
// Furthermore, the function could notify the former inventory objects that
// their holder is under destruction (useful to move players out of rooms which
// are updated); and it could announce mudwide the destruction(quitting) of
// players.
nomask mixed
prepare_destruct(object obj)
{
object *inv, dest;
int i;
string s;
	s = object_name(obj);
	if (sscanf(s, "%s/server", s)) {
		obj->destruct_server();
		if (!obj) return 0;
	} else if (sscanf(s, "%s_cmd", s)) {
		obj->destruct_command();
		if (!obj) return 0;
	}
	inv = all_inventory(obj);

	if (sizeof(inv) < 1) return 0;

#if 0
	dest = environment(obj);
	if (!dest) dest = find_object(GENERIC_ROOM_VOID);
	if (!dest)
		return "Unable to remove stuff from object!";
#endif

	for (i = sizeof(inv) - 1; i >= 0; i--) {
		DESTRUCT(inv[i]);
	}

	return 0;
}

// Handle quotas in times of memory shortage.
//
// This function is called during the final phase of a garbage collection if
// the reserved user area couldn't be reallocated. This function (or a called
// demon) has now the opportunity to remove some (still active) objects from
// the game. If this does not free enough memory to reallocate the user
// reserve, slow_shut_down() will be called to start Armageddon.
//
// Note: Up to now, the wizlist lacks various informations needed to detect
//   the memory-hungriest wizards.

nomask void
quota_demon()
{
	return;
}

// Handle a received IMP message.
//
// Arguments:
//   host: Name of the host the message comes from.
//   msg : The received message.
//   port: the port number from which the message was sent.
//
// This function is called for every message received on the IMP port.

#if 0
nomask void
receive_imp(string host, string msg, int port)
{
}
#endif

// Schedule a shutdown for the near future.
//
// Argument:
//   minutes: The desired time in minutes till the shutdown:
//             6, if just the user reserve has been put into use;
//             1, if the (smaller) system or even the master reserve
//                has been put into use as well.
//
// The gamedriver calls this function when it runs low on memory.
// At this time, it has freed its reserve, but since it won't last long,
// the games needs to be shut down. Don't take the 'minutes' as granted
// remaining uptime, just deduce the urgency of the shutdown from it.
// The delay is to give the players the opportunity to finish quests,
// sell their stuff, etc.
// It is possible that the driver may reallocate some memory after the
// function has been called, and then run again into a low memory situation,
// calling this function again.
//
// For example: this function might load an 'Armageddon' object and tells
// it what to do. It is the Armageddon object then which performs
// the shutdown.
//
// Technical:
//   The memory handling of the gamedriver includes three reserved areas:
//   user, system and master. All three are there to insure that the game
//   shuts down gracefully when the memory runs out: the user area to give
//   the players time to quit normally, the others to enable emergency-logouts
//   when the user reserve is used up as well.
//   The areas are allocated at start of the gamedriver, and released when
//   no more memory could be obtained from the host. In such a case, one
//   of the remaining areas is freed (so the game can continue a short
//   while) and a garbagecollection is initiated.
//   If the garbagecollection recycles enough memory (either true garbage
//   or by the aid of the quota_demon) to reallocate the areas, all is
//   fine, else the game is shut down by a call to this function.
void
slow_shut_down(int minutes)
{
       call_other(SHUT_D, "shut", minutes);
}

// Notify the master about an immediate shutdown.
//
// If the gamedriver shuts down, this is the last function called before
// the mud shuts down the udp connections and the accepting socket for new
// players.
// The function has the opportunity to perform any cleanup operation, like
// informing the mudwho server that the mud is down. This can not be done
// when remove_player() is called because the udp connectivity is already
// gone then.
void
notify_shutdown()
{
}

// Handle a dangling lfun-closure.
//
// This is called when the gamedriver executes a closure using a vanished lfun.
// A proper handling is to raise a runtime error.
//
// Technical:
//   Upon replacing programs (see efun replace_program()), any existing
//   lambda closures of the object are adjusted to the new environment.
//   If a closure uses a lfun which vanished in the replacement process,
//   the reference to this lfun is replaced by a reference to this function.
//   The error will then occur when the execution of the adjusted lambda
//   reaches the point of the lfun reference.
//   There are two reasons for the delayed handling. First is that the
//   program replacement and with it the closure adjustment happens at
//   the end of a backend cycle, outside of any execution thread: noone
//   would see the error at this time.
//   Second, smart closures might know/recognize the program replacement
//   and skip the call to the vanished lfun.
void
dangling_lfun_closure()
{
	raise_error("dangling lfun closure\n");
}

// Announce an error in the heart_beat() function.
//
// Arguments:
//   culprit: The object which lost the heart_beat.
//   err    : The error message.
//   prg    : The executed program (might be 0).
//   curobj : The object causing the error (might be 0).
//   line   : The line number where the error occured (might be 0).
//
// Result:
//   Return anything != 0 to restart the heart_beat in culprit.
//
// This function has to announce an error in the heart_beat() function
// of culprit.
// At time of call, the heart_beat has been turned off.
// A player should at least get a "You have no heartbeat!" message, a more
// advanced handling would destruct the offending object and allow the
// heartbeat to restart.
//
// Note that <prg> denotes the program actually executed (which might be
// inherited one) whereas <curobj> is just the offending object.
mixed
heart_beat_error(object culprit, string err,
		 string prg, string curobj, int line)
{
	if (object_info(culprit, OI_ONCE_INTERACTIVE)) {
		culprit->tell_me("You have no heart beat! But I'll try\
 to restart it.");
		return 1;
	}

	if (!environment(culprit)) {
	log_file("HB", sprintf("%s: Room (?) '%s' dested due to error '%s' in:\n\
%s::%s, line %d.\n", ctime(time()), object_name(culprit), err, prg,
curobj, line));
		culprit->tell_here("There is black smoke in the room\
 indicating serious internal problems in mudlib.\n\
Please contact (co)admins!");
		return 0;
	}
	log_file("HB", sprintf("%s: (%s, in room '%s'):\n'%s'.\n\
Code in %s::%s, line %d.\n",
	object_name(culprit), ctime(time()),
	environment(culprit) ? object_name(environment(culprit)) : "<NONE>",
	err, prg, curobj, line));
// Just get rid of easy monsters etc.
	if (living(culprit)) {
		if (environment(culprit))
		environment(culprit)->tell_here(capitalize(GET_NAME(culprit))
+" vanishes in a puff of black smoke!");
		DESTRUCT(culprit);
		if (culprit)
			destruct(culprit);
	}
	return 0;
}


// Validate the execution of a privileged operation.
//
// Arguments:
//   op   : the requestion operation (see below)
//   who  : the object requesting the operation (filename or object pointer)
//   arg  : additional argument, depending on <op>.
//   arg2 : additional argument, depending on <op>.
//
// Result:
//     >0: The caller is allowed for this operation.
//      0: The caller was probably misleaded; try to fix the error
//   else: A real privilege violation; handle it as error.
//
// Privileged operations are:
//   attach_erq_demon  : Attach the erq demon to object <arg> with flag <arg2>.
//   bind_lambda       : Bind a lambda-closure to object <arg>.
//   call_out_info     : Return an array with all call_out informations.
//   erq               : A the request <arg4> is to be send to the
//                       erq-demon by the object <who>.
//   input_to          : Object <who> issues an 'ignore-bang'-input_to() for
//                       commandgiver <arg3>; the exact flags are <arg4>.
//   nomask simul_efun : Attempt to get an efun <arg> via efun:: when it
//                       is shadowed by a 'nomask'-type simul_efun.
//   rename_object     : The current object <who> renames object <arg>
//                       to name <arg2>.
//   send_imp          : Send UDP-data to host <arg>.
//   set_auto_include_string : Set the string automatically included by
//                       the compiler.
//   get_extra_wizinfo : Get the additional wiz-list info for wizard <arg>.
//   set_extra_wizinfo : Set the additional wiz-list info for wizard <arg>.
//   set_extra_wizinfo_size : Set the size of the additional wizard info
//                       in the wiz-list to <arg>.
//   set_driver_hook   : Set hook <arg> to <arg2>.
//   set_this_object   : Set this_object() to <arg>.
//   shadow_add_action : Add an action to function <arg> from a shadow.
//   symbol_variable   : Attempt to create symbol of a hidden variable
//                       of object <arg> with with index <arg2> in the
//                       objects variable table.
//   wizlist_info      : Return an array with all wiz-list information.
//
// call_out_info can return the arguments to functions and lambda closures
// to be called by call_out(); you should consider that read access to
// closures, mappings and pointers means write access and/or other privileges.
// wizlist_info() will return an array which holds, among others, the extra
// wizlist field. While a toplevel array, if found, will be copied, this does
// not apply to nested arrays or to any mappings. You might also have some
// sensitive closures there.
// send_imp() should be watched as it could be abused to mess up the IMP.
// The xxx_extra_wizinfo operations are necessary for a proper wizlist and
// should therefore be restricted to admins.
// All other operations are potential sources for direct security breaches -
// any use of them should be scrutinized closely.
int
privilege_violation(string op, mixed who, mixed arg, mixed arg2)
{
	if (!objectp(who)) who = find_object(who);
	if (!who) return 0;

// Not sure if master object's things are checked but if they are...
	if (who == this_object()) return 1;

// Well. Let's prevent _all_ operations we don't think are needed;
// if (when?) we need 'em, we can allow some...
	switch (op) {
        case "attach_erq_demon":
            if (CODER_LEVEL(who) > LVL_SENIOR)
                return 1;
            return 0;
	case "erq":
		return 1;
	case "send_imp":
	case "bind_lambda":
	case "nomask simul_efun":
	case "rename_object":
	case "set_driver_hook":
	case "set_auto_include_string":
	case "shadow_add_action":
	case "symbol_variable":
		return 0;
// Should log these too but...
	}
	return 1;
}

// Validate if the snoopers of an object may be revealed by usage of the
// efun query_snoop().
//
// Arguments:
//   previous_object(): the asking object.
//   obj              : the object which snoopers are to be revealed.
//
// Result:
//   Return a non-zero number to allow the action,
//   any other value to disallow it.
int
valid_query_snoop(object obj)
{
        if (previous_object() && object_name(previous_object()) ==
           "bin/usr/_snooplist")
		return 1;

	return 0;
}

// Validate the start/stop of a snoop.
//
// Arguments:
//   snooper: The wannabe snooper, or 0 when stopping a snoop.
//   snoopee: The victim of the snoop.
//
// Result:
//   Return a non-zero number to allow the action,
//   any other value to disallow it.
int
verify_snoop(object snooper, object snoopee)
{
int my_clevel;
	if (!snoopee) return 1;
	if (!interactive(snooper) || !(my_clevel = CODER_LEVEL(snooper)))
	if (CODER_LEVEL(snoopee) >= my_clevel)
		return 0;
	return 1;
}

// Validate the change of an objects euid by efun seteuid().
//
// Arguments:
//   obj    : The object requesting the new euid.
//   neweuid: The new euid requested.
//
// Result:
//   Return 1 to allow the change, any other value to disallow it.
int
valid_seteuid(object obj, string neweuid)
{
  /* Todo? */
// Perhaps, _if_ we ever begin using UIDs.
  return 1;
}

/*
 * These functions are for testing purposes.
 * New login object can be tested with this.
 */

private static string test_connect;

// argument is filename of new login object.
nomask status
set_test_connect(string str)
{
	if (!this_interactive()
	  || CODER_LEVEL(this_interactive()) < LVL_COADMIN)
		return 0;

	test_connect = str;
	return 1;
}

nomask object connect();

private object
testConnect()
{
object ob;
string err;

err = catch(ob = clone_object(test_connect));

if(err) {
   write(err+"\n");
   test_connect = 0; // we return to the old login.
   return connect();
 }
return ob;
}

// This function is called every time a player connects.
// input_to() can't be called from here.
nomask object
connect()
{
  object ob;
  string ret, res;

  // Hm, who's playing with this function?
  if (!interactive(this_object())) return 0;
  if (test_connect) return testConnect(); // For testing login.

  // Is an IP _stopped_ immediately?
  if (call_other(BANISH_D, "is_ip_stopped", this_object()))
    return 0;

  ret = catch(ob = clone_object(PLAYERFILE));

    write("\n");

    if(ret) {
        res = sprintf("Couldn't clone player.c: %s.\n", ret);

        ob = find_object(PLAYERFILE);
        if(ob) {
            res += "Recompiling...\n";
            destruct(ob);
            ret = catch(call_other(PLAYERFILE, "???"));
            if (ret) {
                res += sprintf("Errors occured: %s.\n", ret);
            } else {
                ret = catch(ob = clone_object(PLAYERFILE));
                if (!ret) {
                    if (res) { write(res); write_file("/log/LOGIN", res); }
                    return ob;
		}
           }
        }
        if (res) { write(res); write_file("/log/LOGIN", res); }
        if (file_size ("/no_login_shut") != -1)
	    return 0;
        write ("Shutting down.\n");
        write_file("/log/LOGIN", "=> Shutdown\n");
        shutdown ();
        return 0;
    }

    return ob;
}


nomask
int xx(int arg)
{
    return 1/arg;
}

// Get the owner of a file. This is called from the game driver, so as
// to be able to know which wizard should have the error.
nomask string
get_wiz_name(string file)
{
string name, rest;

	if(sscanf(file,"/u/%s/%s", name, rest) == 2)
		return name;
// Perhaps this would allow nice tricks like adding /areas-areas to wizlist?
// -+ Doomdark 11-aug-95 +-
	if (sscanf(file, "/areas/%s/%s", name, rest) == 2)
		return name;
	return 0;
}

// Write an error message into a log file. The error occured in the object
// 'file', giving the error message 'message'.

nomask void
log_error(string file, string message)
{
string name;

	name = get_wiz_name(file);
	if (name == 0) name = "log";
	log_file(name, message);
}

/* save_ed_setup and restore_ed_setup are called by the ed to maintain
   individual options settings. These functions are located in the master
   object so that the local gods can decide what strategy they want to use.
   suggestions:
	A setup file for every wizard.
		advantages:	transparent to the user
				independent of wizard count
		disadvantage:	extra file access at ed invocation
	An array in the master object, wizards are searched by member_array
		advantage:	easy to implement
		disadvantage:	performance degradation with high wizard counts
	An AVL-tree to access wizards by name
		advantage:	can fit any need
		disadvantage:	hard to implement, will need more overhead on
				small and medium muds than it can ever make
				good by lg(wizcount) complexity
	Dedicated flags in every wizard object, inherited from /lib/living
		advantages:	easy to implement ( as shown here)
				independent of wizard count
				Will also work for nonm-wizards.
		disadvantage:	care has to be taken to avoid collision with
				other uses of the /lib/living flags.
*/

/*
 * The wizard object 'who' wants to save his ed setup. It is saved in the
 * file /u/wiz_name/.edrc . A test should be added to make sure it is
 * a call from a wizard.
 *
 * Don't care to prevent unauthorized access of this file. Only make sure
 * that a number is given as argument.
 */
nomask int
save_ed_setup(object who, int code)
{
string file;
	if (!intp(code))
		return 0;
	file = sprintf("/u/%s/.edrc", lower_case(REAL_NAME(who)));
	rm(file);
	return write_file(file, code + "");
}

// Retrieve the ed setup. No meaning to defend this file read from
// unauthorized access.
nomask int
retrieve_ed_setup(object who)
{
string file;
int code;

	file = sprintf("/u/%s/.edrc", lower_case(REAL_NAME(who)));
	if (file_size(file) <= 0)
		return 0;
	sscanf(read_file(file), "%d", code);
	return code;
}

/*
 * When an object is destructed, this function is called with every
 * item in that room. We get the chance to save players !
 */
nomask void
destruct_environment_of(object ob)
{
	if (!interactive(ob)) return;
	ob->tell_me("Everything you see is disolved. Luckily, you are \
transported somewhere...");
	ob->move_player("is transferred", GENERIC_ROOM_VOID);
}

/*
 * The master object is asked if it is ok to shadow object ob. Use
 * previous_object() to find out who is asking.
 *
 * In this example, we allow shadowing as long as the victim object
 * hasn't denied it with a query_prevent_shadow() returning 1.
 */
nomask int
query_allow_shadow(object ob)
{
	if (ob == this_object()) return 0; // never let us to be shadowed
	return !ob->query_prevent_shadow(previous_object());
}

// Default language functions used by parse_command() in non -o mode
nomask string *
parse_command_id_list() { return ({ "one", "thing" }); }
nomask string *
parse_command_plural_id_list() {
   return ({ "ones", "things", "them" });
}
nomask string *
parse_command_adjectiv_id_list() {
    return ({ "iffish" });
}
nomask string *
parse_command_prepos_list() {
    return ({ "in","on","under","behind","beside" });
}
nomask string
parse_command_all_word() {
    return "all";
}

// Give a file name for edit preferences to be saved in.
nomask string
get_ed_buffer_save_file_name(string file)
{
string *file_ar;

  if (!this_player()) return 0;
  file_ar=explode(file,"/");
  file=file_ar[sizeof(file_ar)-1];
  return "/u/"+REAL_NAME(this_player())+"/.ed/"+file;
}

// compile_object() for virtual objects
// Now all files under any virtual directory can be virtual. ++Tron
// They only need their own server.
nomask object
compile_object(string path)
{
  string server;
  string tmp;
   object ob;

// URGH!
	if (path[0] != '/')
		path = "/" + path;
//printf("Virtual path: %s.", path);

  if (sscanf(path, "%s/virtual/", tmp))
    {
      server = tmp+"/virtual/server.c";
      if (file_size(server) > 0) 
       if(ob = (object)server->compile_object(path)) {
          rename_object(ob, path);
       }
       return ob;
    }
  return 0;
}

// Give a path to a simul_efun file. Observe that it is a string returned,
// not an object. But the object has to be loaded here. Return 0 if this
// feature isn't wanted.
nomask string
get_simul_efun()
{
string fname;
string error;

	fname = SIMUL_EFUN;

	if (error = catch(fname->start_simul_efun())) {
		write("Failed to load " + fname + "\n\
error message :"+error+"\n");
		shutdown();
		return 0;
	}

	return fname;
}

// Get the file sizes for the 'ls' command. It is open so wizards
// can use it too. This opens a small security hole allowing 'ls'
// virtually to anywhere. No write access or file reading thou.

nomask mixed *
get_file_sizes(string path)
{
mixed *flist;
string *name_list;
int *size_list;
int     i, min,max;

	if (!(flist = get_dir(path, 3))) return 0;

	i = sizeof(flist) >> 1;
	name_list = allocate(i);
	size_list = allocate(i);
	while (--i >= 0) {
		name_list[i] = flist[i << 1];
		size_list[i] = flist[(i << 1) + 1];
	}
	return ({ name_list, size_list });
}

/*
 * There are several occasions when the game driver wants to check if
 * a player has permission to specific things.
 *
 * These types are implemented so far:
 * "error messages":	If the player is allowed to see runtime error
 *			messages.
 * "trace":		If the player is allowed to use tracing.
 * "wizard":		Is the player considered at least a "minimal" wizard ?
 * "error messages":	Is the player allowed to get run time error messages ?
 */
nomask int
query_player_level(string what)
{
    int clevel, ret;

    if (!this_player() || !interactive(this_player())) ret = 0;
    else {

	clevel = CODER_LEVEL(this_player());
	switch(what) {
	case "wizard":
		ret = (clevel >= LVL_APPRENTICE);
	case "error messages":
		ret = (clevel >= 0);
	case "trace":
		ret = (clevel >= LVL_COADMIN);
	}
    }

    return ret;
}

// Announce a runtime error.
//
// Arguments:
//   err    : The error message.
//   prg    : The executed program.
//   curobj : The object causing the error.
//   line   : The line number where the error occured.
//
// This function has to announce a runtime error to the active player.
// If it is a wizard, it might give him the full error message together
// with the source line; if it is a player, it should issue a decent
// message ("Your sensitive mind notices a wrongness in the fabric of space")
// and could also announce the error to the wizards online.
//
// Note that <prg> denotes the program actually executed (which might be
// inherited one) whereas <curobj> is just the offending object.
void
runtime_error(string err, string prg, string curobj, int line)
{
	write_file("/log/driver_runtime.log", sprintf(
"Program: %s, Cur obj: %s, line: %d,\nError: %s\n",
prg, curobj, line, err));
	if (this_player() && interactive_info(this_player(), II_IP_NUMBER))
	  catch( write(curobj ?
		 err +
		 "program: " + prg +
		 ", object: " + curobj +
		 " line " + line + "\n"
		 :
		 err
		 ));
}


/*
 * Function name:   valid_exec
 * Description:     Checks if a certain 'program' has the right to use exec()
 * Arguments:       name: Name of the 'program' that attempts to use exec()
 *                        Note that this is different from file_name(),
 *                        Programname is what 'function_exists' returns.
 *                  NOTE, the absence of a leading slash in the name.
 * Returns:         True if exec() is allowed.
 */

nomask int
valid_exec(string name, object ob, object obfrom)
{
	if (name[0] != '/') name = "/"+name;
	switch(name) {
		case "/basic/player/relogin.c":
		case "/secure/master.c":
		case "/bin/usr/_switch.c":
		case "/secure/login.c":
		case PLAYER_OBJECT2: 
		  return 1;
	}

	return 0;
}

nomask string
make_path_absolute(string p)
{
	if (this_player() && interactive(this_player())) {
		return (string)this_player()->absolute_path(p);
	}
}

// Don't allow master.c to be shadowed!
nomask int query_prevent_shadow() { return 1; }

status
valid_snoop(object me, object you)
{
	if (!you) return 1;
	if (!interactive(me) || !interactive(you))
		return 0;
	return (int) me->query_coder_level() > (int) you->query_coder_level();
}

/* Arghh! Don't ask why... //Graah */
nomask void
thief_log(string fn, string x)
{
  string tmp1, tmp2;

  if (!x || !fn || !stringp(fn)) return;

  if (sscanf(fn, "%s..%s", tmp1, tmp2) == 2)
    {
      log_file("ILLEGAL", "Illegal attempt at master:thief_log()\n");
      return ;
    }

  if (file_size("/guilds/thieves/" + fn) > 256000)
    {
      rm("/guilds/thieves/" + fn + ".old");
      rename("/guilds/thieves/" + fn, "/guilds/thieves/" + fn + ".old");
      write_file("/guilds/thieves/" + fn,
		 "Started new log file at " + ctime(time()) + "\n");
    }

  catch(write_file("/guilds/thieves/" + fn, x));
}

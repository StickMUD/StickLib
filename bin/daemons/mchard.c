/**************************************************

 NAME:    mchard.c

 PURPOSE: Daemon for multiple character registering

 AUTHOR:  Tron

 CHANGES:
    13-nov-95: Re-coded "query_can_login"; now returns
	string to be printed, if login is not
	allowed, otherwise 1.
     4-Jan-97 / Graah: Coders and up only can now read

 **************************************************/

#include <coder_levels.h>

#define	MCHARD_SAVE	"data/d/mchard/MCHARD"
#define DELAY_MIN   5

void clean_logouts();

private string mchard_chars;          // "\n#owner#mchar1#mchar2#\n..."
static mapping logouts;

nomask status
query_prevent_shadow() { return 1; }

nomask private status
_validUser() {
    if(!this_player() || !interactive(this_player()) ||
      (int)this_player()->query_coder_level() < LVL_CODER)
	return 0;

    return 1;
}

/* Allows everybody to check for his/her own multiple chars. */
nomask private status
_validQuery(string plr) {

    if(!this_player() ||
      !interactive(this_player()) ||
      !( (string)this_player()->query_real_name() == plr ||
	(int)this_player()->query_coder_level() >= LVL_CODER ) )
	return 0;

    return 1;
}

/****************************************************

 DESCRIPTION: Check that no other than master object
	      can be used. Load multiple char list.

 ****************************************************/

nomask void
create() {
    string file;
    int clone;

    /* Master object only */
    if(sscanf(object_name(this_object()), "%s#%d", file, clone) == 2) 
    {
	destruct(this_object());
	return;
    }

    mchard_chars = read_file(MCHARD_SAVE);
    if(!mchard_chars) mchard_chars = "\n";

    logouts = ([ ]);
}

nomask void
reset()
{
    // Only needed temporarily. -+ Doomdark +-
    if (!logouts)
	create();
    clean_logouts();	// Let's only clean 'em at reset...
}

/****************************************************

 DESCRIPTION: Get player's multiple chars.

 ARGUMENT(S): plr -player's name.

 RETURN:      Array of char names, 0 for failure.

 ****************************************************/

nomask string
query_multiple_char(string plr) {
    string head, tail, *tmp;
    string sa, sb;
    int i;

    if(!plr || !mchard_chars) return 0;

    if (extern_call() && !_validQuery(plr))
	return 0;
    if(sscanf(mchard_chars, "%s#"+plr+"#%s", head, tail) != 2) return 0;

    i = sizeof(head)-1;
    if(head[i..i] == "\n") head = "";
    else {
	tmp = explode(head, "\n");
	head = tmp[sizeof(tmp)-1];
    }
    if(tail[0..0] == "\n") tail = ""; 
    else {
	if(sscanf(tail, "%s\n%s", sa, sb) != 2)
	    if(!sscanf(tail, "%s\n", sa)) return 0; //something wrong.
	tail = sa;
    }

    return head+"#"+plr+"#"+tail;
}

// Function name: nomask int query_no_of_mchars(string plr)
// Description	: Give the number of mchars the plr has.
nomask int
query_no_of_mchars(string plr)
{
    string head, tail, *tmp, m;
    string sa, sb;
    int i;

    if (!plr || !mchard_chars) return 0;

    if (extern_call() && !_validQuery(plr)) return 0;
    if (sscanf(mchard_chars, "%s#"+plr+"#%s", head, tail) != 2) return 0;

    i = sizeof(head)-1;
    if (head[i..i] == "\n") head = "";
    else {
	tmp = explode(head, "\n");
	head = tmp[sizeof(tmp)-1];
    }
    if (tail[0..0] == "\n") tail = ""; 
    else {
	if (sscanf(tail, "%s\n%s", sa, sb) != 2)
	    if (!sscanf(tail, "%s\n", sa)) return 0; //something wrong.
	tail = sa;
    }

    if (head != "")
    {
	if (head[0] == '#') head = head[1..<1];
	head += "#";
    }

    if (tail != "")
    {
	if (tail[<1] == '#') tail = tail[0..<2];
	tail = "#" + tail;
    }

    // Some mchar entries have "##"
    m = head+plr+tail;
    while ((i = strstr(m, "##")) >= 0)
	m = m[0..i-1] + m[i+1..<1];

    return sizeof(explode(m, "#"));
}

/****************************************************

 DESCRIPTION: Add new multiple char pair in list.

 ARGUMENT(S): p1 -owner char's name.
	      p2 -multiple char's name.

 RETURN:      1 for success, 0 for failure.

 ****************************************************/

nomask status
add_multiple_char(string p1, string p2) {
    string chars, *parts;

    if(!p1 || !p2 || !mchard_chars) return 0;
    if(!_validUser()) return 0;

    if(query_multiple_char(p2)) return 0; //this char already exists.
    if(chars = query_multiple_char(p1)) {
	parts = explode(mchard_chars, chars+"\n");
	if(!parts) return 0; // error in list!

	chars += (p2+"#\n");
	mchard_chars = parts[0]+ chars +(sizeof(parts) == 2 ? parts[1] : "");

    } else mchard_chars += ("#"+p1+"#"+p2+"#\n");

    rm(MCHARD_SAVE);
    write_file(MCHARD_SAVE, mchard_chars);

    return 1;
}

/****************************************************

 DESCRIPTION: Delete multiple char or owner char
	      and all multiple chars if given
	      argument is owner char.

 ARGUMENT(S): plr -name of the player.

 RETURN:      1 for success, 0 for failure.

 ****************************************************/

nomask status
del_multiple_char(string plr) {
    string chars, *parts;
    string *char_l;

    if(!plr || !mchard_chars) return 0;   
    if(!_validUser()) return 0;

    if(chars = query_multiple_char(plr)) {
	char_l = explode(chars, "#");
	if(!char_l) return 0; // something wrong with the list!

	if(char_l[0] == plr) { // this is the owner.
	    parts = explode(mchard_chars, chars+"\n");
	    if(!parts) return 0; // error in list!

	    mchard_chars = parts[0] + (sizeof(parts) == 2 ? parts[1] : "");
	}
	else {
	    parts = explode(mchard_chars, chars+"\n");
	    if(!parts) return 0;

	    char_l -= ({plr});
	    if(sizeof(char_l) == 1)  // only owner left.
		mchard_chars = parts[0] + (sizeof(parts) == 2 ? parts[1] : "");
	    else mchard_chars = parts[0] + "#" + implode(char_l, "#") + "#\n" +
		(sizeof(parts) == 2 ? parts[1] : "");
	}
    } else return 0; // no such multiple char!

    rm(MCHARD_SAVE);  
    write_file(MCHARD_SAVE, mchard_chars);

    return 1;   
}

/****************************************************

 DESCRIPTION: Get multiple char string exactly the
	      way it is saved.

 RETURN:      string type of: "\n#owner#multiple#..."

 ****************************************************/

nomask string
query_multiple_char_string() {
    if (extern_call() && !_validUser())
	return 0;
    return mchard_chars;
}

/*****************************************************

 DESCRIPTION: Get array of multiple char pairs.

 RETURN:      array type of: ({ "#owner#multiple#", ...})

 *****************************************************/

nomask string *
query_multiple_char_array() { 
    if (extern_call() && !_validUser())
	return 0;
    return (mchard_chars ? explode(mchard_chars, "\n") : 0);
}

/***************************************************************

  DESCRIPTION: Store logout name and time of players.

****************************************************************/

status add_logout(string name)
{
    logouts[name] = time();
    return 1;
}

/*************************************************************

  DESCRIPTION: Return mapping containing log out names & times

*************************************************************/

mapping
query_logouts()
{
    return logouts;
}

/************************************************************

 DESCRIPTION: Checks current mapping of users/logout times

  RETURNS:  Returns 1 if character can login.
	    Returns _string_ ('error msg') if char can't login.

*************************************************************/

mixed
query_can_login(string name)
{
    int last_out, i;
    string s;
    string *names;
    if (!(s = query_multiple_char(name))) return 1;
    if (!(names = explode(s, "#"))) return 1;
    i = sizeof(names);
    while (i--) {
	if (names[i] == name) continue;
	if (find_player(names[i])) {
	    return sprintf(
	      "Your multiple character %s is already playing; you can't enter.",
	      capitalize(names[i]));
	} else if ((last_out = logouts[names[i]])
	  && (last_out = time() - last_out) < (DELAY_MIN * 60)) {
	    return sprintf(
	      "Your multiple character %s quitted only %d minutes %d seconds ago;\
 delay between logout / login of multiple characters needs to be\
 at least %d minutes.\n\
So, come back after %d minutes %d seconds.",
	      capitalize(names[i]), last_out / 60, last_out % 60, DELAY_MIN,
	      (60 * DELAY_MIN - last_out) / 60, (60 * DELAY_MIN - last_out) % 60);
	}
    }
    return 1;
}

/********************************************************

  DESCRIPTION:  Clean the logouts map.  Get rid of logouts
		over delay time.

**********************************************************/

void clean_logouts()
{
    string *names;
    int i;

    if (!logouts) return;

    names = m_indices(logouts);

    for(i=0;i < sizeof(names);i++)
    {
	if (((time() - logouts[names[i]]) / 60) > DELAY_MIN)
	    logouts = m_delete(logouts,names[i]);
    }
}

/****************************************************

  DESCRIPTION: Query the logouts map names.

  RETURNS: 1 - array of names

*******************************************************/

string *
query_logout_names()
{
    return logouts ? m_indices(logouts) : 0;
}

/******************************************************8

  DESCRIPTION:  Query logout map times

  RETURN: 1 - array of logout times

***********************************************************/

int *
query_logout_times()
{
    return logouts ? m_values(logouts) : 0;
}

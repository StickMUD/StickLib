/***************************************************

 NAME:    login.c

 PURPOSE: Handle players' login sequence.

 AUTHOR:  Tron

 NOTE:    Constructed from the old /secure/login.c

 TODO:    Add logs, code a new object for guest.

 CHANGES:

  19-Aug-98, Doomdark: Modified a bit for StickLib-
	distribution.

 ***************************************************/

#include <coder_levels.h>
#include <daemons.h>
#include <mchar.h>
#include <mud_name.h>
#include <tell_me.h>
#include <client_defs.h>
#include <player_defs.h>
#include <player_defs.h>
#include <generic_rooms.h>
#include <config.h>

#define	LOGIN_C

#include "/basic/living/tell_me.c"

private static int _value_plr_client;	// Client he/she is using, if any

int set_env(mixed value, mixed arg);

#include "/basic/player/client.c"

#define TELL_ME(x)      tell_object(this_object(),(x))
#define LOGIN_GUEST     "lib/guest"
#define LOGIN_FILE      "secure/login"
#define LOGIN_DUMMY	"secure/dummy"

/* DO customize these if you want to be an admin / a coadmin: */
#define LOGIN_ADMIN_L    ({ "tamarindo", "kieve", "graah", "doomdark", "starks", })

#define LOGIN_COADMIN_L  ({ })

#define LOGIN_NAMELEN   11     /* Player's max name length          */
#define LOGIN_PASSLEN   6      /* Min length for password           */

#define LOGIN_PASSWORD_FAIL 1  /* user can fail once with password. */
#define LOGIN_PASSWD_EXPIRE 2592000 /* 30 days */

/*********************************
 Reserved: 2 coadmins, 5 coders
           and 90 mortals
 *********************************/

#define LOGIN_COADMINS  97
#define LOGIN_CODERS    95
#define LOGIN_MORTALS   90


/* Timeout in seconds (every input) */
/* Was 60, changed to 120. -Doomdark */
#define LOGIN_TIMEOUT   120

/* Welcome information is in this file... Customize it */
#define LOGIN_WELCOME   "/data/login/WELCOME"

/*********************************
 Values loaded from save file.
 *********************************/

private string  name;
private string  password;
private int     passwd_time;
private string  mailaddr;
private int     level;
private int     coder_level;
private int     gender;

/* Internal values */

private static int    login_state;         //what state are we in?
private static int    login_password_fail; //# wrong passwords.
private static string password_again;      //store password here temporarily
private static object other_copy;          //old copy of this player on game
private static status interactive_copy;    //other copy is interactive
private static string expire_passwd;       //expired password.

private static int _window_x, _window_y;	// Window size of the client, if any.

/* Login states */

#define LOGIN_STATE_NULL  0   /* starting with this */
#define LOGIN_STATE_NAME  1   /* ask for name       */
#define LOGIN_STATE_PASS  2   /* ask for password   */
#define LOGIN_STATE_MAIL  3   /* ask for mail       */
#define LOGIN_STATE_GEND  4   /* ask for gender     */
#define LOGIN_STATE_CONN  5   /* connect to player  */

/* No shadowing */

nomask status
query_prevent_shadow() {
    return 1;
}

/**********************
 Needed for compat mode
 **********************/

nomask string
valid_read(string file) {
	return stringp(file) ? file[1..<1] : 0;
}

nomask string
valid_write(string file) {
    return 0;
}

/********************************
 Functions for information about
 the player logging on.
 ********************************/

nomask string
query_real_name() {
    return (name ? name : "logon");
}

nomask string
query_name() {
    return (name ? capitalize(name) : "Logon");
}

nomask int
query_coder_level() {
    return (coder_level > 0 ? coder_level : 0);
}

nomask int
query_testplayer() {
    return (coder_level < 0 ? 1 : 0);
}

nomask int
query_level() {
    return level;
}

nomask object
check_unique(string name)
{
    object *plrs;
    int i;
 
    plrs = users();
    i = sizeof(plrs);
 
    while(i--  &&  !other_copy)
        if(plrs[i]  &&  plrs[i] != this_object()  &&
            plrs[i]->query_real_name() == name)
            other_copy = plrs[i];
 
    return other_copy;
}
 

/********************************
 Make sure this object is not
 manipulated.
 ********************************/

nomask void
create() {
string file;
int clone;

  /* No clones of this object are allowed, just the master */
  if(sscanf(file_name(this_object()), "%s#%d", file, clone)!=2)
    file = file_name(this_object());

  /* This file is always secure/login.c */

  if(file != LOGIN_FILE) {
     destruct(this_object());
     return;
  }
  
  level  = -1; 
  gender = -1;
  password_again = 0;

}

/***********************************
 Close connection because of timeout
 ***********************************/

nomask void
login_close() {

  if(this_player() != this_object()) return; //Not anyone can call this.

  if (interactive(this_object())) {
#if 0
	write("\nTimeout!!! Connection closed.\n");
#else
	tell_me("\ntimeout!!! Connection closed.");
#endif
  }
  destruct(this_object());
}

nomask void
login_timeout() {

  login_close();
}


/********************************
 Manage input from this function.
 Set timeout and write messages,
 then get input.
 ********************************/

private static string _func_name;
private static int _input_mode;

nomask static void
login_input(string txt, string fun, status a) {
   
   remove_call_out("login_timeout");
   call_out("login_timeout", LOGIN_TIMEOUT);
   if (txt) TELL_ME(sprintf("\n%s",txt));
	_func_name = fun;
	_input_mode = a;
	input_to("catch_it", a);
//input_to(fun, a);
}

/* Let's make all the commands go through this function, so the client might
 * be able to give commands that 'by-pass' the normal mode...
 */
nomask void
catch_it(string txt)
{
string s, args;
int x, y;
/* We don't want others calling this function from outside, though: */
	if (previous_object() && previous_object() != this_object()) {
		tell_me(sprintf("SECURITY WARNING: Another object (%O) tried to\
 call a function in the login-object!", previous_object()));
		return;
	}

/* First we'll check if the client wants to communicate with us. Possible. */
	if (txt && txt[0] == '<') {
	  sscanf(txt, "<%s%t%s>", txt, args);
	  s = do_client_command(txt, args);
	  if (stringp(s))
		tell_me(s);
	  input_to("catch_it", _input_mode);
	} else {
	  call_other(this_object(), _func_name, txt);
	}
}

/*********************************
 Take care of connection to the
 player object.
 *********************************/

nomask static void
login_connect()
{
  string login_file;
  string res;
  object player_ob;

  mapping attrs;

  if (!check_unique(name)  &&  !find_player(name)) {
      //No other copies of this char on the game.
     // if(name == "guest") login_file = LOGIN_GUEST;
	login_file = PLAYER_FILE;
      if (!(player_ob = clone_object(login_file)))
	{
	  write("Error in cloning player object...Quitting...\n");

	  if (player_ob) destruct(player_ob);
	  destruct(this_object());
	  return;
	}
  }
  else { 
    if(!other_copy)  other_copy = find_player(name);

    if(interactive(other_copy)) {
      // oops, not link dead.
      interactive_copy = 1;
      write("\nYour character is active already!\n");
      login_input("Throw it out? (y/n): ", "login_throwOut", 0);
      return;
    }
    else write("Reconnecting...\n");

    player_ob = other_copy;
  }

  if (!player_ob || !exec(player_ob, this_object())) {
    write("\nFailed to connect! Quitting.\n");
    if (player_ob)
	destruct(player_ob);
  }
  else if(player_ob) {

	attrs = ([
		"password" : password,
		"password_time" : passwd_time,
	]);
	if (_window_x && _window_y) {
	  attrs["window_x"] = _window_x;
	  attrs["window_y"] = _window_y;
	}
	if (_value_plr_client)
		attrs["client"] = _value_plr_client;

     if(interactive_copy)
       player_ob->initialize(name, password, passwd_time, 
                             level, mailaddr, gender, 1, attrs);
     else
       player_ob->initialize(name, password, passwd_time,
                             level, mailaddr, gender, 0, attrs);
  }
  else write("\nFATAL: Player object was lost! Quitting.\n");

  remove_call_out("login_close");
  remove_call_out("login_timeout");

  destruct(this_object());
  return;
}

/**********************************************
 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

 Function is called when player first connects
 to MUD_NAME. Login sequence is controlled from
 here!
 
 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 **********************************************/

static nomask varargs int
logon(string msg) {

object player_ob;
string tmp;

  /* NEW! -+ Doomdark 18-Dec-97 +- */
  // I hope this prevents log from getting trashed...
  if (!interactive(this_object())) {
	login_close();
	return 1;
  }

  switch(login_state) {
  
      case LOGIN_STATE_NULL: 
/* Stickmud feature alert!	Starks
	// Some random titles to scare them with /Graah
	write((string)DESC_D->query_mud_name() + "\n");
*/

             cat(LOGIN_WELCOME);
             /* Every input has its own timeout but also the
                whole login sequence has a max time. (check        
                login_input()). 
              */
             call_out("login_close", LOGIN_TIMEOUT * 5);
             
             login_state = LOGIN_STATE_NAME;

      case LOGIN_STATE_NAME:             
             tmp = msg ? msg : "Give your name: ";
             login_input(tmp, "login_name", 0);
             break;
  
      case LOGIN_STATE_PASS:
             tmp = msg ? msg : "Password: ";
             login_input(tmp, "login_password", 1);
             break;
    
      case LOGIN_STATE_MAIL:
             tmp = msg ? msg : "Give your email address: ";
             login_input(tmp, "login_email", 0);
             break;
     
      case LOGIN_STATE_GEND:
             tmp = msg ? msg : "Are you male or female: ";
             login_input(tmp, "login_gender", 0);
             break;
     
      case LOGIN_STATE_CONN:

             login_connect();
             break;
 
      default: write("Illegal login state!\n");
   }

   return 1;
}

/******************************
 Check if the name given by a
 player is acceptable.
 ******************************/

static nomask status
validName(string str)
{
  int len;
  int i;
  string *offensive;

  if (!str) return 0;

  /*   if(member_array(str, LOGIN_COADMIN_L) == -1) {
       write("Sorry, game closed for now. Fixes under way...please wait.\n");
       write("Email to 'stick@jyu.fi' to give information about the crash.\n");
       destruct(this_object());
       return 0;
       }
       */

  /* No login sequence name or master UIDs */ 

  if (str == "logon" || str == "root" || str == "backbone")
    {
      write("This is not a valid name, it's reserved for other purposes.\n");
      return 0;
    }

  len = strlen(str);

  if (len > LOGIN_NAMELEN)
    {
      write("Too long a name - maximum is "+LOGIN_NAMELEN+" characters.\n");
      return 0;
    }

  if (len < 2)
    {
      write("Too short a name - must be at least two characters long.\n");
      return 0;
    }

  for(i=0; i < len; i++) {
    if(str[i] < 'a' || str[i] > 'z') {
      write("Invalid character(s) in name: "+str[i]+"\n");
      write("Character number was "+ (i+1) +".\n");
      return 0;
    }
  }
   
  offensive = ({  ".*fuck.*", ".*cunt.*", ".*arse.*",
		  ".*shit.*", ".*gay.*", ".*lesb.*",
		  ".*penis.*", ".*bitch.*",
		  ".*suck.*",".*masturb.*", ".*clit.*", ".*cock.*"   
  });

  for (i = sizeof(offensive) - 1 ; i >= 0 ; i--)
    {
      if (sizeof(regexp( ({ str }), offensive[i] ))) {
	write("\nThat name is too offensive. Come up "+
	      "with something better.\n\n");
	return 0;
      }
    }

  if ((str != "guest") &&
      (str[0..4] == "guest"))
    {
      write("Name of the guest is 'guest'. Try again.\n\n");
      return 0;
    }

  return 1;
}

/**************************************
 Check if the game is too full for new
 players, coders and coadmins. Admins
 must be able to log on every time.
 **************************************/

static nomask int
checkFullness() {
int    i, coadmins, coders, mortals, clevel;
object *u;

   /* Admins must always be able to login. */
   if (coder_level >= LVL_ADMIN) return 0;

   u = users();

// Nah, let's cut the crap. This is too inefficient a scheme most
// of the time. -+ Doomdark +-

	if (sizeof(u) < LOGIN_MORTALS
	  || (coder_level && sizeof(u) < LOGIN_CODERS)
	  || (coder_level >= LVL_COADMIN && sizeof(u) < LOGIN_COADMINS))
		return 0;

   for (i = sizeof(u) - 1; i >= 0; i--) {
      clevel = (int)u[i]->query_coder_level();
      if (clevel <= 0) mortals++;
      else if (clevel < LVL_COADMIN) coders++;
      else if (clevel < LVL_ADMIN) coadmins++;
   }

   if (coder_level <= 0 && mortals >= LOGIN_MORTALS) return 1;
   if (coder_level < LVL_COADMIN && mortals + coders >= LOGIN_CODERS) return 1;
   if (mortals + coders + coadmins >= LOGIN_COADMINS) return 1;
   return 0;
}

/*************************************
 Get character's name. Check if the
 name is acceptable and the game is
 not too full before asking password.
 *************************************/

static nomask void
login_name(string str) {
string tmp;
object *plrs;
int i;

   if(!str) str = "";

   str = lower_case(str);

 //  if(str != "guest") //Many guests on the game possibly ++Tron
    // set the other_copy with the check_unique function --K
    check_unique(str);

   if(!validName(str)) {
      logon("Try another name: "); //ask for name again.
      return;
   }

   if(tmp = (string)BANISH_D->is_banished(str)) {
      write(tmp);
      logon("Try another name: "); //ask for name again.
      return;
   }

   if (str != "guest")
     {
       // IP banishment (just throw 'em out)
       if (BANISH_D->is_ip_banished(this_object(), lower_case(str)))
	 {
	   if (master_object()->exists_player(lower_case(str)))
	     {
	       write(
"\nYou are not currently allowed to enter from your site.\n\
If you think this is an error, mail to <mud-admins@their.site>.\n");
	     }
	   else
	     {
	       write("\nNo new players are currently allowed from your site.\n\
If you really want to have a new character (or a new multiple\n\
character), mail to <mud-admins@their.site>. Tell your e-mail address, and\n\
the name _and_ gender of your new character. Remember to give\n\
alternative names in case some are already reserved. Also, if\n\
it is going to be a second, you must tell the name of your primary\n\
character!\n");
	     }
	   destruct(this_object());
	   return;
	 }
     }
   else
     {
       if (BANISH_D->is_guest_ip_banished(this_object(), lower_case(str)))
	 {
	   write(
"\nSorry, but you are not allowed to use Guest from your site at the moment.\n\
If you think this is an error, mail to <mud-admins@their.site>.\n");
	   destruct(this_object());
	   return;
	 }
     }
   
	if (!restore_object(PATH_FOR_PLAYER_SAVE(str))) {
	write("\nNew character.\n\n"
#ifdef SCARE_NEWBIES
"If you already have a character in this MUD, and you are now\n\
creating another one, you MUST REGISTER IT as a mchar immediately!\n\n\
You MAY NOT PLAY WITH AN UNREGISTERED CHARACTER. You can CREATE a\n\
second, but PLAYING with an unregistered second is CHEATING!\n\
Number of mchars is limited to "+MAX_NO_OF_MCHARS+" at the moment.\n"
#endif
	);
      name = str; // Name must be set here.
   }
#if 0
   else {
	if (coder_level >= LVL_COADMIN) {
       if(coder_level > LVL_COADMIN) {
          if(member_array(name, LOGIN_ADMIN_L) == -1) {
             write("\nYou are not an ADMINISTRATOR!!!\n");
             destruct(this_object());
             return;
          }
       } else if(member_array(name, LOGIN_COADMIN_L) == -1) {
           write("\nYou are not a CO-ADMINISTRATOR!!!\n");
           destruct(this_object());
           return;
         }
     }
   }
#endif

   if(checkFullness()) {
      write("\n"+MUD_NAME+" is full. Please try again after a moment.\n");
      destruct(this_object());
      return;
   }

   /* This is not neccessary now... //Graah
      if (!(GENERIC_ROOM_CHURCH)->query_allow_enter(this_object())) return;
   */

   if (MUDLOCK_D->query_mudlock() && !coder_level) {
     write(MUD_NAME+" is currently locked by admins for maintenance.\n");
     this_object()->login_timeout();
     return;
   }
   if(name == "guest") login_state = LOGIN_STATE_CONN;
   else login_state = LOGIN_STATE_PASS;

   logon(); // Go back to login control.

return;
}

/**********************************
 Get player's password and check
 if it is acceptable. Also verify
 the password in case of new player
 or password is NULL.
 **********************************/

static nomask void
login_password(string str) {
string tmp;
    write ("\n");

    if(!str || str == "") {
       write("Try again another time then.\n");
       destruct(this_object());
       return;
    }

    if(strlen(str) < LOGIN_PASSLEN) {
       write("The password must have at least "+LOGIN_PASSLEN+
             " characters.\n");
       logon();
       return;
    }

    if(password == 0) {
       if(expire_passwd) {
          if(crypt(str, expire_passwd) == expire_passwd) {
             write("That was your old password, now give a different one.\n");
             logon();
             return;
	   }
	}
       if(!password_again) { 
         password_again = str;
         logon("Password (again): ");
         return;
        }
       else if(str != password_again){
          write("\nYou changed!\n");
          if(login_password_fail++ < LOGIN_PASSWORD_FAIL) {
             password_again = 0;
             write("Try giving it again.\n");
             logon();
             return;
          }
          destruct(this_object());
          return;
        }
       else {
         passwd_time = time();     
         password = crypt(password_again, 0); //New seed.
       }
    }   
    else if(crypt(str, password) != password) {  
       write("Wrong password!\n");
       if(login_password_fail++ < LOGIN_PASSWORD_FAIL) {
           logon();
           return;
        }
       destruct(this_object());
       return;
     }
    
    if(!passwd_time) passwd_time = time(); //with new chars
    else {
       if((time() - passwd_time) > LOGIN_PASSWD_EXPIRE) {
          write("Your password has expired! Please give a new password.\n");
          login_password_fail = 0;
	  expire_passwd = password;
          password = 0; //This causes the passwd to be asked twice.
	  login_state = LOGIN_STATE_PASS;
	  logon();
	  return;
	}
     }
    if(!mailaddr) login_state = LOGIN_STATE_MAIL;    
    else if(gender == -1) login_state = LOGIN_STATE_GEND;
    else login_state = LOGIN_STATE_CONN;

// Checking mchar moved here...
// Actually, whole check routine moved to MCHAR_D... :-)

   if (stringp(tmp = (mixed) MCHAR_D -> query_can_login(name)))
     {
        tell_me(tmp);
       destruct(this_object());
       return;
     }        
    
    logon();
}

/********************************
 Get player's email address.
 ********************************/

static nomask void
login_email(string str) {

string login;
string addr;

    if(!str) str = "";

    if(sscanf(str, "%s@%s", login, addr) != 2) {
        write("Email address should be type of <login>@<address>\n");
        logon();
        return;
     }

    write("\nMake sure your email address is correct!\n\n\
Use command \"email\" to change your email address.\n\n"
#ifdef SCARE_NEWBIES
"If your character has no email address set, or you\n\
set a wrong e-mail address, ALL YOUR CHARACTERS WILL\n\
be REMOVED!\n"
#endif
	);

    mailaddr = str;

    if(gender == -1) login_state = LOGIN_STATE_GEND;
    else login_state = LOGIN_STATE_CONN;

    logon();
}

/*****************************
 Get player's gender, male or
 female. So far no others are
 allowed.
 *****************************/

static nomask void
login_gender(string str) {

    if(!str) str = "";

    if(!strlen(str)) {
       logon("\"m\" or \"f\": ");
       return;
    }

    if(str[0] == 'm') {
        write("Welcome, Sir!\n");
        gender = 1;
    }
    else if(str[0] == 'f') {
        write("Welcome, Madam!\n");
        gender = 2;
    }
    else {
        logon("\"m\" or \"f\": ");
        return;
    }

    // Log new created player here. / Graah
    log_file("NEW_PLAYER",
       name + " from " + query_ip_name(this_object()) +
       " at " + ctime(time()) + "\n");

    login_state = LOGIN_STATE_CONN;

    logon();
}

/*********************************
 In case of active copy, throw it
 out or quit.
 *********************************/

nomask static void
login_throwOut(string str) {

    if(!str) str = "";

    if(!strlen(str)) {
	login_input("(y/n): ", "login_throwOut", 0);
	return;
    }

    str = lower_case(str);

    // re-check the other player
    check_unique(name);
    
    if(!other_copy) {
         interactive_copy = 0;
         write("There was no copy present anymore...connecting.\n");
     }
    else if(!interactive(other_copy)) {
         interactive_copy = 0;
         write("There was no ACTIVE copy present anymore...connecting.\n");
    }
    else if(str[0] == 'y'  &&  find_player(name)) { 
	 object ob;
	   
	 if (catch(ob = clone_object(LOGIN_DUMMY))) {
	     other_copy->quit();
	 } 
         else {
	     exec(ob, other_copy);
	     tell_object(ob, "--relogin-from-"+query_ip_name(this_object())+
                             "-\n");
	 }
    }
    else { //answer was 'n'
        if(!find_player(name))
            write("\nI don't think so...\n\n");
        else
            write("\nOk! Quitting...\n\n");
	destruct(this_object());
	return;
    }

    logon();
}

/*
 * Login object must never modify commands in any way!
 */
nomask string
modify_command(string str)
{
  return str;
}

/* And this is needed to be able to pass stuff client sends us: */
nomask int
set_env(mixed value, mixed arg)
{
	switch (value) {
	case "columns":
		_window_x = arg;
		break;
	case "rows":
		_window_y = arg;
		break;
	default:
		return 0;
	}
	return 1;
}

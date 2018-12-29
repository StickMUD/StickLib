/****************************************************************
*								*
* Module: player_cmds.c						*
* Contains: commands for all players in the mud.		*
* Last update:							*
* 5-Sep-98 / Graah: for stick-dist.				*
*								*
* Commands in this file at the moment:				*
*								*
* - Kill							*
* - Converse							*
* - Stop							*
* - Quit							*
* - Password							*
*								*
****************************************************************/

#ifndef PLAYER_C

#define	INVIS_LEVEL	query_invis()
#define	TOO_RELAXED	"You are too relaxed to do that."
#define	TP	this_player()

#include <mud_name.h>
#include <player_defs.h>
#include <invis_levels.h>
#include <guild.h>
#include <daemons.h>

#include "/basic/player/player_private.h"

static status ghost;
static int liv_Flags;
static object *hunted;
static object guild_object;
static int partyId;
static string cap_name;
static string guild;
static int GuildFlags;
static mapping GuildHooks;

string mailaddr;
string name;
int Str, Dex, Con, Int, max_Str, max_Dex, max_Con, max_Int;
string password;                // This player's crypted password.
int passwd_time;
int coder_level;
string guild;

status attack_object(object x);
int query(int x);
varargs string query_name(int x, object o);
void inform_us(string s);
int query_invis();
status query_can_quit();
void save_me(int x);
varargs void tell_me(string s);
varargs status set(int x, mixed y);
void drop_all(int x);
varargs status set_sit(int x, int y);
varargs status set_rest(int x, int y);
varargs object is_fighting(object x);
void stop_fight(object x);
varargs string session(string a, string b, mixed c, mixed d);

#endif

#define	SESSION_CMD	"/bin/pub/_session"
static string password2;        // Temporary when setting new password


nomask int
kill2(string str)
{
object ob;

	set(PLR_BUSY_KILL_OFF);

	if (ghost) return 0;

	if (!str) {
		tell_me("KILL! KILL!... kill... huh, kill what???");
		return 1;
	}
	if (!(ob = present(lower_case(str), environment()))) {
		TP -> tell_me(sprintf("You can't see any '%s' here!", str));
		return 1;
	}
	if (ob == this_object()) {
		if (random(2)) tell_me("What? Attack yourself?");
		else tell_me("Suicide is not the answer.");
		return 1;
	}
	if (is_fighting(ob)) {
		tell_me("Yes, yes.");
		return 1;
	}
	if (!living(ob) || !ob->query(LIV_IS_PLAYER_OR_NPC)) {
		tell_me(sprintf("%s is not a living thing!", str));
		environment() -> tell_here(sprintf("%s tries foolishly to attack %s.",
query_name(0, this_object()), str), this_player());
		return 1;
	}

	attack_object(ob);
	return 1;
}

// Prevent the speed-kill
nomask status
kill_cmd(string str)
{
object x;
	if (liv_Flags & (F_LIV_SITTING | F_LIV_RESTING)) {
		tell_me(TOO_RELAXED);
		return 1;
	}
	if (query(PLR_BUSY_KILL_ON)) {
		tell_me("Yes, yes.");
		return 1;
	}
	if (str && !(x = present4(str, environment(), this_object(),
	IL_VISIBLE))) {
		tell_me(sprintf("You can't see any '%s' here!", str));
		return 1;
	}
	if (x && is_fighting(x)) {
		tell_me(sprintf("But you are already fighting with %s.",
			(string) x->Objective()));
		return 1;
	}
#ifdef GN_NINJA
	if (guild == GN_NINJA)
		return kill2(str);
	else {
#endif
	set(PLR_BUSY_KILL_ON);
// Both thieves and fighters should get small 'bonus' here, in addition to ninjas.
		call_out("kill2", ((guild == "thief" || guild == "fighter")
		? 0 : 1), str);
#ifdef GN_NINJA
	}
#endif
	return 1;
}

static int
converse_cmd()
{
	tell_me("Give '**' to stop.\n]",1);
	input_to("converse_more",0);
	return 1;
}

static void
converse_more(string str)
{
string cmd;
	if (str == "**") {
		tell_me("Ok.");
		return;
	}
	if(str[0] == '!') {
		sscanf(str, "!%s", cmd);
		command(cmd);
	} else if(str != "") {
		environment()->tell_here(sprintf("%s says: %s",
			query_name(), str), this_object());
	}
	tell_me("]",1);
	input_to("converse_more",0);
}


// Player can also 'stop resting' and 'stop sitting'
int
stop_cmd(string arg)
{
  int i;
  string s;
  object ob;

  if (!arg) return notify_fail("Stop what?\n"), 0;

  if (arg == "resting" || arg == "rest") {
    if (!(liv_Flags & F_LIV_RESTING)) tell_me("You are not resting.");
    else set_rest(0);
    return 1;
  }

  if (arg == "sitting" || arg == "sit") {
    if (!(liv_Flags & F_LIV_SITTING)) tell_me("You are not sitting.");
    else set_sit(0);
    return 1;
  }

  if (sscanf(arg, "fighting %s", s) || sscanf(arg, "hunting %s", s)) {

    tell_me("You can't stop fighting or hunting, even other players. \
Thanks to cheaters again!");
    return 1;


    if (!sscanf(s, "with %s", arg))
      arg = s;

    if (!(ob = find_player(arg))) {
      tell_me(
"No such player in game currently. Note that you can only stop fighting \
players, not monsters (because of possible cheating).");
      return 1;
    }

    if (!is_fighting(ob) && member(hunted, ob) < 0) {
      tell_me(sprintf("You are not fighting with %s.",
		      (string)ob->Objective()));
      return 1;
    }

    stop_fight(ob);

    tell_me(sprintf("Ok. Note that %s has to stop fighting/hunting too, \
otherwise fight may continue.", capitalize(arg)));
    return 1;
  }
}

nomask static int
email_cmd(string str)
{
	if(!str) {
		tell_me(sprintf("Your official electric mail address is: %s\n\
Your connection seems to be from %s", mailaddr, query_ip_name(this_object())));
		return 1;
	}
	if (strstr(str, " ", 0) != -1)
	    return notify_fail("Email addresses do not have spaces in them!.\n"), 0;

	mailaddr = str;
	tell_me("Changed your email address.");
	return 1;
}


nomask int
quit()
{
	if (!query_can_quit()) {
		tell_me("You can't quit now, sorry.");
		return 1;
	}

	tell_me("Saving...");
	if (guild_object && (GuildFlags & (1 << G_HOOK_QUIT)))
		funcall(GuildHooks[G_HOOK_QUIT], this_object());

	save_me(6);
// Seems like favourite trick for cheater coders is to quit to give
// the stuff. Oh well.
	if (!coder_level)
		drop_all(3);
// Let's make sure we do drop all of them. -+ Doomdark +-
	while (first_inventory())
		destruct(first_inventory());

// Show session status. //Graah
	tell_me("Session status:");
	catch(SESSION_CMD->session_cmd("s"));
	tell_me("\n",1);

// Lets log some exp data
	if (coder_level < 1)	// ONly players...
log_file("EXPRATE",name+","+session("query","exp")+","+query(PLR_CONNECT_TIME)+
	","+guild+"\n");
	if (interactive(this_object())) {
#ifdef LOG_ENTER
    /* Log quit too. 12/7/91 Frobozz */
	log_file ("ENTER", sprintf("%s %s (quit) l:%d,x:%d,m:%d,%d/%d,%d/%d,\
%d/%d,%d/%d\n", ctime(time())[4..15], cap_name, level, experience, money,
		Str, max_Str, Con, max_Con, Dex, max_Dex, Int, max_Int));
#endif
	inform_us("leaves " + MUD_NAME);
	if (TP == this_object())
#if 0 /* JOULU ON TAAS */
	tell_me(read_file("/MERRYXMAS"));
#else
	tell_me(read_file("/FORTUNE"));
//	tell_me("- No fortune due to efficiency problems -");
//	show_fortune();
#endif
  }

	if (!INVIS_LEVEL && environment())
	  environment() -> tell_here(sprintf( "%s left "+
					      MUD_NAME+".", cap_name),
			this_object());

// Let's inform party daemon we're about to quit.
	if (partyId)
		PARTY_D -> player_quitted(this_object());
// And also mchar daemon... It needs to know. :)
	MCHAR_D->add_logout(name);

	destruct(this_object());
	return 1;
}

// This one is called when the player wants to change his password.

nomask static int
change_password(string str)
{
	if (password != 0 && !str) {
		tell_me("Give old password as an argument.");
		return 1;
	}
	if (password != 0 && password != crypt(str, password)) {
		tell_me("Wrong old password.");
		return 1;
	}
	password2 = 0;
	input_to("change_password2", 1);
	tell_me("New password: ", 1);
	return 1;
}

nomask static void
change_password2(string str)
{
	if(!str) {
		tell_me("Password not changed.");
		return;
	}
	if (!password2) {
		if (sizeof(str) < 8) {
			tell_me("Must be at least 8 characters!");
			return;
		}
		password2 = str;
		tell_me("\nAgain: ", 1);
		input_to("change_password2", 1);
		return;
	}
	if (password2 != str) {
		tell_me("Wrong! Password not changed.");
		return;
	}
	password = crypt(password2, 0);	// Generate new seed
	password2 = 0;
        passwd_time = time();
	tell_me("Password changed.");
}

/****************************************************************
*								*
* Module: coder_cmds.c						*
* Contains: commands only allowed for coder/co-admins/admins;	*
*	actually only ones not (yet) moved to /bin/usr.		*
*	Note that few of these commands can't be moved... :-(	*
*	(yeah. gamedriver sucks)				*
* Last update: 12-nov-95.					*
*								*
* Commands in this file at the moment:				*
*								*
* Banish:	co-admin or higher				*
* Banlist:	all coders (from apprentice on)			*
* Wizlist:	- " -						*
* Localcmd:	- " -						*
* Snoop:	- " -						*
* Earmuffs:	- " -						*
* Shutdown:	co-admin or higher				*
*								*
****************************************************************/

#ifndef PLAYER_C

#include <daemons.h>
#include <coder_levels.h>
#include <generic_rooms.h>
#include <mud_name.h>

varargs void tell_me(string s, status x);
void listen_shout(int l);
string query_real_name();
void save_me(int x);

int coder_level;
string name;
static int listen_to_shouts_from_level;

#endif

#include <config.h>

static string reason;	// "Reason" for shutdown, to be logged..

/* New banishment system using banish server of Eru Mudlib //Graah */

static int
banish_wizcmd(string str)
{
  string who;
  int lvl;
  int d, h;
  string tmp;

  if (coder_level < LVL_SENIOR && name != "police")
    return notify_fail("This command is only for co-admins!\n"), 0;

  notify_fail("Usage: banish <who> <value>\n\
or: banish <who> for <value> days/hours/minutes\n");

  if (!str) return 0;

  lvl = -666;

  if (sscanf(lower_case(str), "%s for %d %s",who,lvl,tmp) == 3)
    {
      if (tmp == "minutes") lvl = (time() + (lvl * 60));
      else if (tmp == "hours") lvl = (time() + (lvl * 3600));
      else if (tmp == "days") lvl = (time() + (lvl * 3600 * 24));
    }

  if (lvl == -666 && sscanf(lower_case(str),"%s %d",who,lvl) != 2)
    return 0;

  if (!BANISH_D->banish(who,lvl)) tell_me("Failed.");
  else tell_me("Ok: " + (string) BANISH_D->list_banished(who));

  return 1;
}

static int
banlist_wizcmd(string arg)
{
  tell_me((string) BANISH_D->list_banished(arg));
  return 1;
}


static int
wizlist_wizcmd(string arg)
{
	if (arg) wizlist(arg);
	else wizlist();
	return 1;
}

/* 3.0.53 does not have localcmd(object), yet. /Graah */
static status
localcmd_wizcmd(string str)
{
object ob;

	localcmd();
	return 1;
#if 0
   if(!str) {
      localcmd();
      return 1;
   }
   ob = find_player(str);
   if(!ob) ob = find_living(str);
   if(!ob) ob = present(str, this_player());
   if(!ob) ob = present(str, environment());
   if(!ob || !living(ob)) {
      tell_me("No such living object.");
      return 1;
   }
   if (query_ip_number(ob) && ob->query_coder_level() >= coder_level) {
      tell_me("You fail.");
      return 1;
   }
   localcmd(ob);
   return 1;
#endif

}

nomask static int
snoop_wizcmd(string str)
{
object ob;
int ob_level;

	if(!str) {
		snoop();
		return 1;
	}

	if (!(ob = find_player(str)) || !interactive(ob)) {
		tell_me("No such interactive player.");
		return 1;
	}

	if (coder_level != LVL_POLICE && coder_level < LVL_JUNIOR) {
		tell_me("Apprentice coders can't snoop anyone.");
		return 1;
	}
	ob_level = (int)ob->query_coder_level();

	if (coder_level < LVL_SENIOR)
	log_file("SNOOP", query_real_name() + " snooped " + str + " on "
		+ ctime(time()) + "\n");
	if (ob_level >= coder_level) {
		tell_me("You fail.");
		return 1;
	}

	if (snoop(ob)) {

// A (less than a senior) coder snooping a coder?
		if (ob_level && coder_level < LVL_SENIOR)
		  ob -> tell_me(capitalize(name) + " starts a SNOOP on you!");
	}

	return 1;
}

static int
earmuffs_wizcmd(string str)
{
int lev;
	if (str && sscanf(str, "%d", lev) == 1)
		listen_shout(lev);
	tell_me("Earmuffs at level " + listen_to_shouts_from_level + ".");
	return 1;
}

static int shutdown_wizcmd(string str)
{
    int shut_time;
    if (!str  ||  str == "")
    {
        tell_me("Usage: shutdown <minutes | 'abort' | 'NOW'> [reason]");
        return 1;
    }
    if(coder_level < LVL_CODER)
    {
        tell_me("You must be a Coder to do that.");
        return 1;
    }
    if(sscanf(str, "%d", shut_time) == 1)
    {
        if(!reason) reason = "";
        // now we call the shut down daemon
        SHUT_OB->shut(shut_time);
        tell_me("Shutdown will happen in "+shut_time+" minutes.");
        return 1;
    }
    if(sscanf(str, "abort %~s") == 1  ||   str == "abort")
    {
        // Stop the count down.
        SHUT_OB->abort_shutdown();
        tell_me("Ok.");
        return 1;
    }
    if(sscanf(str, "NOW %s", reason) == 1)
    {
        // Are we sure? Today I accidentally shut it down :-( //Graah
        if(coder_level <  LVL_SENIOR)
        {
            tell_me("You must be a Senior Coder to do that.");
            return 1;
        }
        tell_me("Requesting IMMEDIATE shutdown.\nAre you sure? (y/n) ", 1);
        input_to("shut2",0);
        return 1;
    }
    if(str == "NOW")
    {
        tell_me("You must give a reason for an immediate shutdown!");
        return 1;
    }
 
    tell_me("Usage: shutdown <minutes | 'abort' | 'NOW'> [reason]");
    return 1;
}
 
static int
shut2(string str)
{
        if(str != "y" && str != "Y") {
                tell_me("Shutdown canceled.");
                return 1;
        }
        shout(MUD_NAME + " is shut down by " + capitalize(name) + ".\n");
// No Someones in logs 12/7/91 Frobozz
#ifdef LOG_SHUTDOWN
    log_file("GAME_LOG", ctime(time()) + " Game shutdown by " +
        capitalize(name) + "(" + reason + ")\n");
#endif
// First force link-deaders to sell their stuff and quit //Rincewind
        catch(call_other(GENERIC_ROOM_PURGATORY, "bail_out"));
        shutdown();
        return 1;
}

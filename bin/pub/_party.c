/************************************************

 NAME:    _party.c

 PURPOSE: Provide playing in parties.

 AUTHOR:  Tron.

 Modifications:

 20-sep-95 Doomdark. Rewrote few things; writess
  -> tell_me's etc.
 25-oct-95 Doomdark. Added support for new version
	of party follow.
  3-Jan-96 Graah. Why did 'party' show the
  	   party status of all parties to anyone
	   who wanted to know?!? Do they really
	   need to know how much exp others have
	   collected? Fixed it.
 19-jan-96 Doomdark. Added restrictions for member
        levels. Perhaps this helps a bit with
        plastic lords?
 26-Jul-96 Graah. No, it doesn't. Ponde just joined
        Altair (level 17 plastic) into his party by
        using other people as mediators. I'll try to
        fix.

*************************************************/

#include <daemons.h>
#include <party.h>

#define NAME	((string)me->query_name())

mapping invited;

static private string print_status(mixed x);
static private string party_show(mixed list);


void
create()
{
	invited = ([]);
}


status
party_list(string s, object me)
{
mapping parties;
int *ids;
mixed list;
string *txt;
int i;
string tit;

  parties = (mixed)PARTY_D->query_parties();
  if (!parties || !(i = m_sizeof(parties))) {
      me->tell_me("There are no parties active at the moment.");
      return 1;
    }

    ids = m_indices(parties);
    txt = allocate(sizeof(ids));

    while (i--) {
      PARTY_D->update_party(ids[i]);
    list = (mixed)PARTY_D->query_party(ids[i]);
/* Check added by Doomdark 12-Jan-97... */
 if (!pointerp(list)) continue;
      if (!list[0] || !(tit = (string)list[0]->query_party(PARTY_TITLE)))
	txt[i] = sprintf("Party:\n%s", party_show(list));
	else txt[i] = sprintf("%s:\n%s", tit, party_show(list));
    }

    me->tell_me(efun::implode(txt, "\n"));
    return 1;
}

status
party_cmd(string s, object me)
{
string cmd, arg;
mixed list;
int id;
	if (!me && !(me = this_player())) return 0;

	if(!s) {
		return party_list(s, me);  
	} else {

		id = (int)this_player()->query_party(PARTY_ID);
  
		if (sscanf(s, "%s %s", cmd, arg) < 2) cmd = s;

		if (cmd != "invite" && cmd != "help" && cmd != "list") {
		  if (!id) {
		    if (cmd != "join") {
			notify_fail("But you are not a member of any party.\n");
			return 0;
		    }
		  } else if (cmd == "join")
		    return notify_fail("But you are a member of one party already.\n"), 0;
		}
		notify_fail("Unknown party command; try 'party help'?\n");
		return (int)call_other(this_object(), "party_"+cmd, arg, me);
	}
}

static varargs string
_party_msg_str(string s, string info)
{
	if (!info) info = "Party";
	if (!s) s = "";

	return "["+info+": "+capitalize((string)this_player()->query_real_name())
		+ "] "+ s;
}

nomask void
timeout(object ob)
{
	if (ob && invited[ob]) {
		if (this_player())
			ob->tell_me((string)this_player()->query_name()+"'s offer no longer stands.");
			invited = m_delete(invited, ob);
	}
	invited = m_delete(invited, ob);
}

status
party_invite(string s, object me)
{
  mapping p;
  object ob;
  int id, i, j, *pins;
  int lvl_diff, max_lvl_in_party;

  if(!s) return notify_fail("Who is that you want to invite?\n"), 0;

  ob = present(s, environment(me));
  if (!ob || !ob->query_short(0, me))
    return notify_fail("Hmmm... you can't see that person here.\n"), 0;
  if (!interactive(ob))
    return notify_fail("Not even you want that 'thing' in your party.\n"), 0;
  if (ob == me)
    return notify_fail("You start to party with yourself, PARTY ON!!!\n"), 0;

  max_lvl_in_party = -1;

  if ((p = (mapping)PARTY_D->query_parties()) &&
      mappingp(p) &&
      (pins = m_indices(p)) &&
      (i = sizeof(pins)) > 0)
    {
      for (i--; i >= 0; i--)
	{
	  if (member_array(me, p[pins[i]]) != -1)
	    {
	      for (j = sizeof(p[pins[i]]) - 1; j >= 0; j--)
		if ((int)p[pins[i]][j]->query_level() > max_lvl_in_party)
		  max_lvl_in_party = (int)p[pins[i]][j]->query_level();
	      break;
	    }
	}
    }

  if (max_lvl_in_party == -1)
    max_lvl_in_party = (int) me->query_level();

  lvl_diff = max_lvl_in_party - (int) ob->query_level();

  // So; level difference can be at maximum 5 levels or 33%.
  // Trying to prevent helping low-level players too much wiht partying.
  if ((lvl_diff > 5) && (lvl_diff > (max_lvl_in_party / 3))) {
    me->tell_me(sprintf("You can't invite %s because %s level is\
 too low compared to the highest level in your party. Level \
difference needs to be 5 or under, or under 33%% of your own \
level.", (string)ob->Objective(), (string)ob->Possessive()));
            return 1; 
  }

  if ((lvl_diff < -5) && ((-lvl_diff) > (max_lvl_in_party / 2))) {
    me->tell_me(sprintf("You can't invite %s because %s level is\
 too high compared to the highest level in your party. \
Level difference has to be to be 5 or under, or under \
33%% of your own level.", (string)ob->Objective(), (string)ob->Possessive()));
            return 1; 
	}

  id = (int)me->query_party(PARTY_ID);

  ob->tell_me(NAME+" asks you to join the party. You could 'party join'.");
  environment(me) -> tell_here(sprintf("%s asks %s to join the party.",
NAME, (string) ob->query_name()), ({ob, me}));

  s = _party_msg_str(capitalize(s), "Party invite");

  if (invited[ob])
    invited[ob] += ({me});

  else invited += ([ob : ({me})]);
  call_out("timeout", 30, ob);

  PARTY_D->set_party_data(id, "tell_me", s);
  me->tell_me("Ok.");

  return 1;
}

status
party_join(string name, object me)
{
mixed l;
int id;
object ob;
int i;
	l = invited[me];
	if (l) l -= ({ 0 });
	else return notify_fail("You suddenly remember that nobody invited you.\n"), 0;
	if (!sizeof(l)) return notify_fail("The inviter is not around anymore.\n"), 0;

	if (!name) {
	  if(sizeof(l) > 1)
		return notify_fail("There is more than one offer, which one?\n"), 0;
	  ob = l[0];
	  if (!present(ob, environment(me)) || !ob->query_short(0, me))
		return notify_fail("You can't see that person here.\n"), 0;
	} else {
	  ob = present(name, environment(me));
	  if (!ob || !ob -> query_short(0, me))
		return notify_fail("You are about to join but you can't that person here.\n"), 0;
	  i = member_array(ob, l);
	  if (i == -1) return notify_fail("You mixed things up now, who was it again?\n"), 0;
	}
	if (!interactive(ob)) return
		notify_fail("Hmmm...that person is currently not able to\
 introduce you to the party.\n"), 0;
	invited = m_delete(invited, me);
	if(!(id = (int)PARTY_D->add_party(ob, me)))
		return notify_fail("Something went wrong.\n"), 0;
	return (status)PARTY_D->set_party_data(id, "tell_me",
		_party_msg_str(0, "Party join"));
}    

status
party_leave(string s, object me)
{
object po;
int id;
	if(PARTY_D->del_party(me, id = (int)me->query_party(PARTY_ID))) {
	  me->tell_me("Ok, you leave the party.");
          PARTY_D->set_party_data(id, "tell_me", _party_msg_str(0, "Party leave"));
	  return 1;
	}
	return notify_fail("Something went wrong.\n"), 0;
}

status
party_follow(string s, object me)
{
object ob;
int i, id;

	if (me->query_party(PARTY_LEADER))
		return notify_fail("You are the party leader, you need not\
 follow anyone.\n"), 0;

	i = (int)me->add_party(PARTY_FOLLOW, -1);
	id = (int)me->query_party(PARTY_ID);

	PARTY_D->set_party_data(id, "tell_me", _party_msg_str(0, "Party follow " + (i ? "on" : "off")));

	return 1;
}

status
party_disband(string s, object me)
{
int id;

	if (me->query_party(PARTY_LEADER)) {
	  id = (int)me->query_party(PARTY_ID);
	  PARTY_D->set_party_data(id, "tell_me", 
		_party_msg_str("Party is disbanded!", "Party disband"));

	  PARTY_D->disband_party(id);
	  return 1;
	}
	return notify_fail("You must be the party leader to do that.\n"), 0;
}

status
party_title(string s, object me)
{
int id;
	id = (int)me->query_party(PARTY_ID);
	PARTY_D->set_party_data(id, "tell_me",  _party_msg_str((s ? s : ""), "Party title"));

	PARTY_D->set_party_data(id, "set_party", PARTY_TITLE, s);
	return 1;
}

status
party_kick(string s, object me)
{
int id;
mixed *names;
int i;
	if (!me->query_party(PARTY_LEADER))
		return notify_fail("You have to be the party leader to do that.\n"), 0;
	if (!s) return notify_fail("You stop and ponder who you should kick out this time.\n"), 0;
	s = lower_case(s);

	id = (int)me->query_party(PARTY_ID);
	names = (mixed *) PARTY_D->query_party_data(id,"query_real_name", 0);   
	if (!names) return notify_fail("Something went wrong.\n"), 0;
	i = member_array(s, names);
	if (i == -1) return notify_fail("No such member in party.\n"), 0;
	names = (mixed)PARTY_D->query_party(id);
	PARTY_D->del_party(names[i], id);
	PARTY_D->set_party_data(id, "tell_me", _party_msg_str(capitalize(s)
	 + " was kicked out of the party.", "Party kick"));
	return 1;
}
     
status
party_say(string s, object me)
{
	if (!s) return notify_fail("What would you want to tell your party?\n"), 0;

	PARTY_D->set_party_data((int)me->query_party(PARTY_ID), "tell_me",
		_party_msg_str(s));
	return 1;
}

status
party_retreat(string s, object me)
{
mixed list;
mixed attack_list;
object ob;
int id;
int i;

	if ((ob = (object)me->query_hitter()) &&
	  present(ob, environment(me))) {
		PARTY_D->update_party(id = (int)me->query_party(PARTY_ID));
		list = (mixed)PARTY_D->query_party(id);
		if (!list || !(i = sizeof(list))) return 1;
		attack_list = ({});
		while (i--) {
		  if (list[i] != me)
		  if (present(list[i], environment(me))) 
		  attack_list += ({ list[i] });
		}
		if (!sizeof(attack_list)) {
			me->tell_me("WHERE ARE MY BUDDIES???");
			command("flee", me);
		} else {
		  ob->stop_fight();
		  ob->attack_object(ob = list[random(sizeof(list))]);
		  environment(me)->tell_here((string)me->query_name() + 
" retreats from the battle.", me);
		  ob->tell_me((string)me->query_name()+"'s opponent attacks you!");
		  me->tell_me("You retreat from the battle.");
		}
	} else {
		if (me->is_fighting())
			me->tell_me("You are safely positioned in the battle.");
		else me->tell_me("But you are not in a battle now.");
	}
	return 1;
}
         
status
party_help(string s, object me)
{
	me->tell_me(
"Party commands start with word 'party'. Followed by following words\
 the can be used to:\n\n\
disband..................PL can disband the party.\n\
follow...................Follow the party leader (toggle).\n\
help.....................This help.\n\
lead.....................Lead the party. Only PL can lead.\n\
invite <name>............Invite a player to your party.\n\
join [name]..............Accept invitation.\n\
kick <name>..............PL can kick player out of the party.\n\
leave....................Leave the party.\n\
retreat..................Retreat from the battle.\n\
say <str>................Party channel.\n\
status...................Party status.\n\
title [str]..............Set session title.\n\
.........................Parties currently in the game.\n\n\
<> = argument, [] = optional argument. PL = Party Leader\n");
	return 1;
}

static private string
print_status(mixed list)
{
int i, j;
int texp;
string txt;

	if (!list) return "";

	txt = sprintf("%-15s %6s %9s\n\
-----------------------------------------", "Name:", "Follow", "Exp");
	i = sizeof(list);
	while (i--) {
		if (!list[i]) continue;
		txt += sprintf("\n%-15s %-6s %9d",
		  capitalize((string)list[i]->query_real_name()),
		  (list[i]->query_party(PARTY_LEADER) ? "Leader" :
		  (list[i]->query_party(PARTY_FOLLOW) ? "On" : "Off")),
		  j = (int)list[i]->query_party(PARTY_EXP));
		  texp += j;
	}
	txt += sprintf("\n-----------------------------------------\
\nTotal %26d\n", texp);
	return txt;
}
   
status
party_status(string s, object me)
{
int id;
mixed list;
string n, txt;

	id = (int)me->query_party(PARTY_ID);
	PARTY_D->update_party(id);

	list = (mixed)PARTY_D->query_party(id);
	if (!list) return notify_fail("Something went wrong.\n"), 0;
	txt = "Party: "+ ((n = (string)me->query_party(PARTY_TITLE)) ?
                     n : "<Unknown>") +"\n\n" + print_status(list);
	me->tell_me(txt);
	return 1;
}

int
party_lead(string s, object me)
{
int id, i;
	if (!me->query_party(PARTY_LEADER))
		notify_fail("But you aren't the leader of the party!\n"), 0;
	i = (int)me->add_party(PARTY_LEAD, -1);
	id = (int)me->query_party(PARTY_ID);

	PARTY_D->set_party_data(id, "tell_me", _party_msg_str(0,
		"Party lead " + (i ? "on" : "off")));
	return 1;
}

static private string
party_show(mixed list)
{
  int i, j;
  string txt;

  if (!list) return "";

  j = sizeof(list);
  txt = "";

  for (i = 0; i < j; i++)
    {
      if (!list[i]) continue;

      txt += sprintf("%-15s %-6s\n",
		     capitalize((string)list[i]->query_real_name()),
		     (list[i]->query_party(PARTY_LEADER) ?
		      "Leader" : "Member"));
    }

  return txt;
}

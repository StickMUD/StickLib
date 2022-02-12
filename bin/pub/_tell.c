//
//  _tell.c -- Tell someone something
//
//  - Alcal 12/95 -> 01/96:
//      Moved this from player object here.
//      Did major changes to 'reply' and 'tell'.
//      Added 'retell', multi_tell, 'tell last #'.
//      Changed totally_invis-coder to have tell added to his history.
//      Players have maximum 20 tells in history, while coders have 50.
//
// Graah 17-Jul-96: Isolated rooms
//



#include "/sys/interactive_info.h"
#include "/sys/object_info.h"

#include <living_defs.h>
#include <coder_levels.h>
#include <invis_levels.h>
#include <room_defs.h>
#include <macros.h>
#include <tell_me.h>
#include <std_lib>
#include <cmd.h>

#define TELL_HISTORY_SIZE 20
#define WIZARD_HISTORY_SIZE 50
#define SHOW_TELLS_DEFAULT 10

#define NO_PLAYER "No player with that name."
#define ERROR_MSG "Tell whom and what?\n"

/* History of last incoming tells */
static mapping last_tells_mapping=([ ]);

/* Previous tell to make 'retell' possible. e.g. In case name typed wrong. */
static mapping prev_tells=([ ]);

/* Previous tellers */
static mapping teller=([ ]);
static mapping away=([ ]);

void add_tell_history(string msg,object ob);
void clear_quitters();

void
reset()
{
    m_delete(teller, 0);
    m_delete(last_tells_mapping, 0);
    m_delete(prev_tells, 0);
    m_delete(away, 0);
}

int away(string str)
{
    if(!str && member(away, this_player()))
    {
	write("Your current away message is: "+
	  away[this_player()][0] + "\n");
	return 1;
    }
    if(!str)
    {
	write("You are not marked away.\n");
	return 1;
    }
    if(str == "-clear") 
    {
	m_delete(away,this_player());
	write("Away message cleared.\n");
    }
    else
    {
	away[this_player()] = ({ (string) str });
	write("You are now marked away.\n");
    }
    return 1;
}

int
tell_cmd(string str)
{
    object ob,tp;
    int ilevel,i,s,t;
    string who, msg, it, tmp, mud, outp;
    string *last_tells;
    string *multi_tell;

    tp=this_player();
    // Frogs can't use tell. //Graah
    if (tp->query_ghost() || tp->query_frog())
	return notify_fail("You fail.\n"),0;

    if(!str) return notify_fail(ERROR_MSG),0;

    // 'tells' changed to 'tell last'  - Alcal
    s = 0;
    if("last" == str || sscanf(str,"last %d",s)) {
	if(!s) s = SHOW_TELLS_DEFAULT;
	last_tells = last_tells_mapping[tp];
	if (!last_tells) {
	    tp->tell_me("No tells in history.");
	    return 1;
	}
	if(s > sizeof(last_tells)) s = sizeof(last_tells);

	t=sizeof(last_tells)-s;
	tmp = "Tell history:";
	for (i = 0; i < s; i++)
	    tmp = sprintf("%s\n%s", tmp, last_tells[i+t]);

	tp->tell_me(tmp, 0, TELL_TYPE_TELL);
	return 1;
    }

    if (sscanf(str, "%s %s", who, msg) != 2)
	return notify_fail(ERROR_MSG), 0;

    // Multi Tell == 'tell alcal,brat Great!'  - Alcal
    if ( strstr(who,",") != -1 ) {
	multi_tell = explode(who, ",");
	s = sizeof(multi_tell = m_indices(mkmapping(multi_tell, multi_tell)));
	if( s > 1) {
	    tp->tell_me("Telling to more than one person.");
	    for(i=0; i < s; i++ ) {
		tp->tell_me(sprintf("Telling to %s: ",capitalize(multi_tell[i])),1);
		tell_cmd(multi_tell[i]+" "+msg);
	    }
	    return 1;
	}
	who = multi_tell[0];
	tp->tell_me(who);
    }

    // Something to implement someday  - Alcal
    if(sscanf(who, "%s@%s", tmp, mud) == 2)
	return notify_fail("Telling to other muds is not available yet.\n"), 0;

    it = lower_case(who);
    ob = find_living(it);

    // Memorize the tell for retell and the teller for reply  - Alcal
    prev_tells[tp] = msg;
    // Well, reply should't be here, because what if they're ignoring.
    // what if they don't get the tell?  --Kieve
    //teller[ob] = (string)tp->query_real_name();

    if (!ob || !environment(ob) ||
      (environment(ob)->query(ROOM_ISOLATED) && !tp->query_coder_level())) {
	tp->tell_me(NO_PLAYER);
	return 1;
    }

    // Link dead?
    if (object_info(ob, OI_ONCE_INTERACTIVE) &&
      !interactive_info(ob, II_IP_NUMBER) && !ob->query(LIV_IS_NPC)) {
	tp->tell_me(sprintf("%s is currently link dead (and cannot hear you).",
	    capitalize(it)));
	return 1;
    }


    // Check ignoration
    if (ob->query_is_ignoring((string)tp->query_real_name()) ||
      ob->query_is_ignoring_level(tp->query_level())) {
	tp->tell_me(sprintf("You can't! %s is IGNORING you!",capitalize(it)));
	return 1;
    }
    // Check 'ignoration' the other way --Kieve
    if (tp->query_is_ignoring((string)ob->query_real_name()))
    {
	tp->tell_me(sprintf("Not while you're ignoring %s!",
	    ob->query_objective()));
	return 1;
    }

    // Isolation: no tells (unless coder)
    if (!tp->query_coder_level() && ob != tp &&
      environment(tp)->query(ROOM_ISOLATED))
    {
	tp->tell_me(NO_PLAYER);
	return 1;
    }

    /* Let's fool mortals that there are NO invisible immortals.
       Lords over rank 1 can see us, so they have to be able to tell us too.
       18.1.1992 //Frobozz
     */
    if ((IS_CODER_OB(ob) && (ilevel = (int) ob->query_invis()) >= IL_TOTALLY_INVISIBLE)
      && (!tp->query_coder_level())) {
	tmp = NO_PLAYER;
	msg = sprintf("%s is unaware of telling you: %s",
	  capitalize((string)tp->query_real_name()), msg);
    }
    else {

	// Only invisibles may tell anonymously. Disguise does
	// not allow anonymous tells any more. //Graah

	if (!tp->query_invis() || (int)ob->query_level() > 19
	  || ob->query_coder_level()) {
	    // Start of scouts mods 12/98
	    if (query_verb() == "reply") {
		this_player()->tell_me(sprintf("You reply to %s: %s", capitalize(who), msg));
	    } else {
		this_player()->tell_me(sprintf("You tell %s: %s",capitalize(who), msg));
	    }
	    // Try and do a query name for invis and sneaking
	    msg = sprintf("%s tells you: %s",
	      capitalize((string)tp->query_real_name()), msg);
	} else {
	    if (query_verb() == "reply") {
		this_player()->tell_me(sprintf("You reply to %s: %s", capitalize(who), msg));
	    } else {
		this_player()->tell_me(sprintf("You tell %s: %s",capitalize(who), msg));
	    }
	    msg = sprintf("%s tells you: %s", (string)tp->query_real_name(), msg);
	}
	if (object_info(ob, OI_ONCE_INTERACTIVE) && interactive_info(ob, II_IP_NUMBER) && interactive_info(ob, II_IDLE) > 300)
	{
	    tmp=sprintf("Ok, but %s has been idle for %s.", capitalize(it),
	      secs2string(interactive_info(ob, II_IDLE)));
	    if(member(away,ob))
		tmp=sprintf("Ok, but %s is idle %s and away: %s",
		  capitalize(it),secs2string(interactive_info(ob, II_IDLE)),away[ob][0]);
	}
	else
	if(member(away,ob))
	    tmp = sprintf("Ok, but %s is away: %s",
	      capitalize(it),away[ob][0]);
	else
	    tmp="";
    }

    // now we know the tell went through, so add them to the teller's
    // reply list --Kieve
    teller[ob] = (string)tp->query_real_name();

    add_tell_history(msg,ob);
    if(tmp != "")
	tp->tell_me(tmp);
    ob->tell_me(msg, 0, TELL_TYPE_TELL);
    // Clear quitted people every now and then  - Alcal
    if(random(100)==0) clear_quitters();
    return 1;
}

void
clear_quitters() {
    while(teller[0]) m_delete(teller, 0);
    while(last_tells_mapping[0]) m_delete(last_tells_mapping, 0);
    while(prev_tells[0]) m_delete(prev_tells, 0);
}

void
add_tell_history(string msg, object ob)
{
    string *last_tells;
    int s;

    if (!msg) return;
    last_tells=last_tells_mapping[ob];
    if (!last_tells) {
	last_tells_mapping[ob]=({ msg });
	return;
    }
    s = TELL_HISTORY_SIZE;
    if(IS_CODER_OB(this_player())) s = WIZARD_HISTORY_SIZE;

    while (sizeof(last_tells) > s)
	last_tells = last_tells[1..<1];

    last_tells += ({ msg });
    last_tells_mapping[ob]=last_tells;
}

int  // Alcal 12/95
do_reply(string str) {
    string who;

    if(query_verb() != "reply") return 0; // No calling !!
    if (!str) return notify_fail("Reply what?\n"), 0;
    if(!(who = teller[this_player()])) {
	this_player()->tell_me("No tells in history.");
	return 1;
    }
    return tell_cmd(sprintf("%s %s",who,str));
}

int  // Alcal 12/95
do_retell(string who) {
    string msg;

    if(query_verb() != "retell") return 0;  // No calling !!
    if(!who) return notify_fail("Retell who?\n"),0;
    if(!(msg = prev_tells[this_player()])) {
	this_player()->tell_me("You haven't told anyone anything.");
	return 1;
    }
    this_player()->tell_me(sprintf("Retelling %s", who));
    //  this_player()->tell_me(sprintf("You tell %s: %s", 
    //     capitalize(who), msg));
    return tell_cmd(sprintf("%s %s",who,msg));
}


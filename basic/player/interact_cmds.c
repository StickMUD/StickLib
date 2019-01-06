/*
 * interact_cmds.c
 *
 * All mortal communication commands and functions should be here.
 * That is, those that are not yet moved to /bin/pub.
 *
 * Copyright (c) 1993 The Stickmud Project,
 * All rights reserved.
 *
 * 27-Jan-1998 New: Deaf people can't hear you. //Chopin
 */

#pragma strict_types

#include <coder_levels.h>
#include <invis_levels.h>
#include <living_defs.h>
//#include <macros.h>
#include <std_lib>

#define TELL_HISTORY_SIZE 10

/* if TERMTYPES is defined bold each tell to the person for now */
/* History of last incoming tells */
static string *last_tells;

string mumble(string s);

#ifndef PLAYER_C
// Prototypes
int query_intoxication();
string query_race();
status query_ghost();
status query_frog();
int query_level();
int query_coder_level();
string query_real_name();
varargs void tell_me(string s, status a, status b);
int query_invis();
string query_name();
#endif

string bright_text(string s);

int
communicate(string str)
{
    string verb, dummy;
    int i;

    verb = query_verb();

    if (!str) str = "";
    else {
	if (sscanf(str,"PMF_CRYPTSAY:%s",dummy)==1) return 0;
    }

    if (verb[0] == "'"[0])
	str = "/secure/simul_efun/amylaar"->extract(verb, 1) + " " + str;

    //   if (dump_esc(str)) return 1;

    if (i = sizeof(str))
	switch (str[i - 1]) {
    case '?':
	dummy = "asks";	// TODO: (perhaps) call ask_living()s
	break;
    case '!':
	switch (query_race()) {
	case "dwarf": 
	    dummy = "bellows"; break;
	case "half-orc":
	    dummy = "roars"; break;
	default:
	    dummy = "exclaims"; break;
	}
	break;
    default:
	dummy = "says";
	break;
    } else dummy = "says";

    if (query_intoxication()) str = mumble(str);

    if (!str) str = "<something incomprehensible>";
    tell_me("Ok.");

    if (query_ghost()) {
	environment()->tell_here(({
	    sprintf("A hollow voice %s: %s", dummy, str),
	  }), ({ LIV_CAN_HEAR }), 0, this_object(),0,0,0);
	return 1;
    }

    environment()->tell_here(({
	":%<me.capname>% " + dummy + ": " + str,
	// Can't use sprintf because of % marks in SSF string
	":%<me.capname>% says something, but you can't hear %<me.objective>%!",
      }), ({ LIV_CAN_HEAR, LIV_CAN_SEE }),0,this_object(),this_object(),0,0);

    return 1;
}


int
whisper_cmd(string str)
{
    object ob;
    string who, msg, it;

    if (this_object()->query_ghost()) {
	tell_me("You fail.");
	return 1;
    }

    if (!str || sscanf(str, "%s %s", who, msg) != 2) {
	tell_me("Whisper what (and to whom)?\n");
	return 1;
    }

    it = lower_case(who);
    ob = find_living(it);

    /* Let's fool mortals with whisper too! 12.1.1992 //Frobozz */

    if (!ob || !present(ob, environment()) ||
      (!query_coder_level() && ob->query_invis())) {
	tell_me("No player with that name in this room.");
	return 1;
    }

    //   if (dump_esc(msg)) return 1;
    if (query_intoxication()) msg = mumble(msg);
    ob->tell_me(({
	":%<me.capname>% whispers to you: " + msg,
	":%<me.capname>% seems to whisper something to you, but \
you can't hear anything!",
      }), ({ LIV_CAN_HEAR, LIV_CAN_SEE }),0,this_object(),0,0);
    tell_me("Ok.");
    environment()->tell_here(({
	":%<me.capname>% whispers something to %<him.name>%.",
      }), ({ LIV_CAN_SEE }),0,({ ob, this_object() }),this_object(),ob,0);
    return 1;
}

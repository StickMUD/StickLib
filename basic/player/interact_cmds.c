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
#include <tell_me.h>
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
	str = verb[1..<0];

    string my_race = query_race();
    dummy = "";

    switch( str[<1] )
    {
      case '?':
        switch( my_race ) {
           case "dark elf":
           case "elf":
           case "half-elf":	dummy = "inquire"; break;
           case "ogre":		dummy = "interrogate"; break;
           case "goblin":
           case "hobbit": 	dummy = "wonder"; break;
           default:		dummy = "ask"; break;
        }
        break;

      case '!':
        switch( my_race ) {
          case "dwarf":		dummy = "bellow"; break;
          case "troll":		dummy = "boom"; break;
          case "half-orc":	dummy = "roar"; break;
          case "goblin":	dummy = "shriek"; break;
          case "ogre":		dummy = "growl"; break;
          case "hobbit":	dummy = "holler"; break;
          default:		dummy = "exclaim"; break;
        }
        break;

      default: dummy = "say"; break;
    }

    if (query_intoxication()) {
	str = mumble(str);
	dummy = "mumble";
    }

    object env = environment(this_object());

    if (!env) {
 	tell_me("Your voice only makes a hollow echo.", 0, TELL_TYPE_SAY);
 	return 1;
    }

   tell_me( sprintf("You %s: %s", dummy, str), 0, TELL_TYPE_SAY);
   env->tell_here( ({ ":%<me.capname>% %<1>%%<me.ending_s>%: %<2>%",
	":%<me.capname>% %<1>%%<me.ending_s>% something,\
 but you cannot hear %<me.objective>%!"}), ({ LIV_CAN_HEAR, LIV_CAN_SEE }),
	({ TELL_TYPE_SAY, dummy, str }), this_object(), this_object(), 0, 0, 1 );

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
      }), ({ LIV_CAN_HEAR, LIV_CAN_SEE }),TELL_TYPE_WHISPER,this_object(),0,0);
    tell_me(":You whisper to "+capitalize(ob->query_real_name())+": " +msg+ " ", 0, TELL_TYPE_WHISPER);
    environment()->tell_here(({
	":%<me.capname>% whispers something to %<him.name>%.",
      }), ({ LIV_CAN_SEE }),TELL_TYPE_WHISPER,({ ob, this_object() }),this_object(),ob,0);
    return 1;
}

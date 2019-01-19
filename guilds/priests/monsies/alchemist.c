#include "priest_monster.h"
#include <living_defs.h>

#define EVAL_COST 10
#define TELL_WHO(x) who->tell_me(x,0,0,this_object(),0,0)

static string* eval_strings = ({
  "inferior",
  "poor",
  "average",
  "good",
  "excellent",
  "superb",
});


void
create_monster()
{
    set_name("alchemist");
    set_short("Alchemist is here");
    set_long("He's the alchemist of the priests' guild.");
    set_level(25);
    set_guild("priest");
    set_alignment(AL_NEUTRAL);
    set_gender(G_MALE);
    set_hp(800);
    set_spell_chance(25);
    set_spell_dam(90);
    set_spell_mess1(":%<me.capname>% takes a pinch of powder from one of \
%<me.possessive>% jars, and throws it %<him.name>%. The stink of \
sulfur permeates the air as %<him.name>% burts into bright blue flames!");
    set_spell_mess2(":%<me.capname>% takes a pinch of powder from one of \
%<me.possessive>% jars, and throws it at you. The powder surrounds \
you, and begins to settle on you. As the powder comes into contact with \
your skin, it bursts into flames!");

    //    set_random_pick(5);
    // alchemist taking things is annoying

    load_chat(7,({
	":%<me.capname>% examines some herbs.",
	":%<me.capname>% says: What can I do for you?",
	":%<me.capname>% mixes something into a boiling liquid.",
	":%<me.capname>% thinks hard.",
	":%<me.capname>% frowns.",
	":%<me.capname>% dips his finger into a tube and tastes.",
	":%<me.capname>% searches something among the shelves.",
	":%<me.capname>% looks at something that looks like a book of \
recipies.",
	":%<me.capname>% says: Would you like me to evaluate a potion \
for you?",
      }));
    load_a_chat(8,({
	":%<me.capname>% shouts: Help! I'm being mugged!",
	":%<me.capname>% says: You filthy heathen!",
	":%<me.capname>% throws some boiling water on you!",
	":%<me.capname>% says: You'll be avenged!",
      }));
}


void do_ask(object who, string what) {
    if(!who || !environment() || !environment(who) ||
      environment(who) != environment()) return;

    switch(what) {
    case "potion": case "potions":
	TELL_WHO(":%<me.capname>% says: You can buy potions here. \
Look at the list to see what's available.");
	// let's tell the following message too

    case "evaluate": case "evaluation":
	who->tell_me(":%<me.capname>% says: If you want me to evaluate \
a potion, just give it to me. One evaluation costs %<1>% gc.",
	  0,({0,to_string(EVAL_COST)}),this_object(),0,0);
	break;

    case "herb": case "herbs":
	TELL_WHO(":%<me.capname>% says: Those are not yet available \
for sale.");
	break;

    default:
	TELL_WHO(":%<me.capname>% mumbles something incomprehensible.");
    }

    environment()->tell_here(({
	":%<me.capname>% says something to %<him.name>%.",
	"You hear someone speaking quietly with someone.",
      }), ({LIV_CAN_SEE, LIV_CAN_HEAR}),
      0,who,this_object(),who,0);
}

int catch_ask(object who, string what) {
    call_out("do_ask", 1, who, what);
    return 1;
}


int give_object_by(object who, mixed what) {
    string str;
    int i;

    if(intp(what)) {
	environment()->tell_here(({
	    ":%<me.capname>% takes %<him.gen_possessive>% donation and puts it somewhere \
in %<me.possessive>% shelves.",
	  }), ({LIV_CAN_SEE}),0,this_object(),this_object(),0,0);
	CASH_D->add_money(sprintf("%s gave to the alchemist",
	    capitalize(who->query_real_name())),what);
	return 1;
    }
    if(!what->is_priest_potion()) {
	TELL_WHO(":%<me.capname>% gives it back to you since it wasn't \
a potion made by a priest.");
    } else {
	if(who->query_money() < EVAL_COST) {
	    TELL_WHO(":%<me.capname>% says: Sorry, but you don't seem \
to have enough money.");
	} else {
	    i = what->query_dist_level();
	    if(i > 119) i = 119; // 100 is "superb"
	    str = eval_strings[i/20];

	    who->tell_me(":%<me.capname>% looks at the potion, sniffs it, \
looks at it again mumbling something, and finally says: This seems to be \
a potion of %<1>% quality.",0,({0,str}),this_object(),0,0);
	    who->add_money(-EVAL_COST);
	}
    }
    if(transfer(what, who)) {
	who->tell_me(":%<me.capname>% says: Since you don't seem to able to \
carry anything more, i'll drop %<it.name>% on the floor.",
	  0,0,this_object(),0,what);
	move_object(what, environment());
    } else {
	who->tell_me(":%<me.capname>% gives %<it.name>% back to you.",
	  0,0,this_object(),0,what);
    }
    environment()->tell_here(({
	":%<me.capname>% says something to %<him.name>%.",
	"You hear someone speaking quietly with someone.",
      }), ({LIV_CAN_SEE, LIV_CAN_HEAR}),
      0,who,this_object(),who,0);
    return 1;
}

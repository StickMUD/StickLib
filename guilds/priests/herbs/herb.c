/*
 * STANDARD HERB OBJECT
 *
 * Implementing this kind of object would be much easier, if there was
 * a global skill system, but because there isn't, I have to put a check
 * for every guild. At the moment priests are the only guild, who are
 * able to recognise herbs, but most likely healers will get that ability
 * too.
 */

#include "../priest.h"
#include "../skills.h"
#include <macros.h>
inherit "/obj/food";
inherit PSKILLS;

string real_name, general_name;
string real_short, general_short;
string real_long, general_long;
int recog_diff; // 0..1000, the skill level required for recognition
string smell_msg;

private static int functions_used;

// No need to define reset, because you can use reset_food function
#define INIT_USED 1


// **** query functions: ****
string query_real_name() { return real_name; }
int query_recog_diff() {return recog_diff; }
status is_herb() { return 1; }


// **** set functions: ****
set_real_name(string s) { real_name = s; }
set_general_name(string s) { general_name = s; }
set_real_short(string s) { real_short = s; }
set_general_short(string s) { general_short = s; }
set_real_long(string s) { real_long = s; }
set_general_long(string s) { general_long = s; }
set_recog_diff(int d) { recog_diff = d; }
set_smell_msg(string s) {smell_msg = s; }


status is_known(object pl) {
    int sk;
    object sym;

    if(!pl) return 0;
    switch(pl->query_guild()) {
    case GN_PRIEST:
	sym = pl->query_guild_object();
	sk = to_int(pex2skill(sym->query_pex(SK_NATURE)));
	return sk >= recog_diff;

    default:
	return 0;
    }
}


string
my_query_short(int mode, object who) {
    if(!who) who = this_player();
    if(who && is_known(who))
	return real_short;
    return general_short;
}

string
my_query_long(string id, object who) {
    if(!who) who = this_player();
    if(who && is_known(who))
	return real_long;
    return general_long;
}


status
smell_cmd(string s) {
    if(!id(s)) return 0;
    this_player()->tell_me(smell_msg);
    return 1;
}


void
init_food() {
    switch(TP->query_guild()) {
    case GN_PRIEST:
	// let's not use set_name, because it sets short and long descs
	// too, and that would just require additional work to be done here
	id_name = real_name;
	break;

    default:
	id_name = general_name;
    }
    add_action("smell_cmd", "smell");
    add_action("smell_cmd", "sniff");
    if(functions_used & INIT_USED) this_object()->init_herb();
}


void
create_food() {
    set_name("herb");
    set_id("herb");
    set_short(#'my_query_short);
    set_long(#'my_query_long);

    real_name = "example herb";
    general_name = "herb";
    real_short = "An example herb";
    general_short = "A herb";
    real_long = "It's an example herb.";
    general_long = "It's a herb.";
    smell_msg = "It doesn't smell for anything particular.";

    if(function_exists("init_herb")) functions_used |= INIT_USED;
    this_object()->create_herb();
}

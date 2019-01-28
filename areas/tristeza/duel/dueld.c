
/********************************************************
 NAME:    dueld.c

 PURPOSE: Manage dueling players.

 AUTHOR:  Tron
 ********************************************************/

#include <areas.h>
#include "dueld.h"
#include "duel_list.c"

static mapping duelists;
static int     duelmode;
static string  *duelstartrooms;
static string  duelendroom;
static status  dueling;
static int     dueltime;


nomask void
create() { dueld_list_create(); }

nomask status
query_prevent_shadow() { return 1; }

nomask private void
_tellDuelArray(string s, mixed *list) {
    int i;

    if(!s || !pointerp(list)) return;

    i = sizeof(list);
    if(!i) return;

    while(i--) if(objectp(list[i])) list[i]->tell_me(s);
}

nomask private void
_tellDuelists(string s) {
    mixed *list;
    int i, j;

    if(!s) return;
    if(!duelists) return;

    list = m_values(duelists);
    if(!list) return;

    i = sizeof(list);
    if(!i) return;

    if(pointerp(list[0])) while(i--) _tellDuelArray(s, list[i]);
    else  _tellDuelArray(s, list);
}

nomask private void
_tellDuelArena(string s) {
    object *list;
    object env;
    int i;

    list = users();
    if(!list) return;

    i = sizeof(list);
    while(i--) {
	env = environment(list[i]);
	if(env && env->query_duel_arena()) list[i]->tell_me(s);
    }
}

nomask varargs private void
_moveDuelArray(mixed *list, string *rlist, status start) {
    int i;
    string room;

    if(!list || !rlist) return;

    i = sizeof(list);
    if(!i) return;

    while(i--) if(objectp(list[i])) {
	    if(interactive(list[i]))
	    {
		if(start) add_dueld_duels((string)list[i]->query_real_name());
		room = rlist[random(sizeof(rlist))];
		move_object(list[i], room);
	    }
	    else call_out("new_ld_room", 5, list[i]);
	}
}

nomask varargs private status
_moveDuelists(mixed dest, status start) {
    mixed *list;
    int i;

    if(!dest || !duelists) return 0;

    list = m_values(duelists);
    i = sizeof(list);
    if(!i) return 0;

    if(pointerp(list[0])) while(i--) _moveDuelArray(list[i], dest, start);
    else _moveDuelArray(list, dest, start);

    return 1;
}

nomask void
new_ld_room(object ob) {

    if(!ob) return;

    if(interactive(ob)) {
	if(environment(ob)->query_duel_arena()) {
	    ob->tell_me("You went LD and someone kicked you out!\n");
	    ob->move_player("X",
	      AREA_TRISTEZA "duel/generate_cool_off");
	}
    }
    else {
	if(environment(ob)->query_duel_arena())
	    ob->move_player("X", AREA_TRISTEZA "duel/generate_cool_off");
	else ob->set_ld_room(AREA_TRISTEZA "duel/generate_cool_off");
    }
}

nomask status
query_dueld_dueling() { return dueling; }

nomask status
set_dueld_mode(int i) {

    //prevent set if duel is going on, only 0 is allowed to reset mode.
    if(duelmode != 0 && i) return 0;

    if(duelmode < 0 || duelmode > DUELD_DUELTYPE_MULTI) return 0;
    duelmode = i;
    return 1;
}

nomask status
set_dueld_duelists(string *duelheader, mixed *duelargs) {

    if(!pointerp(duelheader) || !pointerp(duelargs)) return 0;
    if(!duelmode) return 0; //Set duel mode first.

    switch(duelmode) {

    case DUELD_DUELTYPE_SINGLE:
	if(sizeof(duelheader) != 2 || sizeof(duelargs) !=2) return 0;
	duelists = mkmapping(duelheader, duelargs);
	break;
    case DUELD_DUELTYPE_TEAM:
    case DUELD_DUELTYPE_MULTI:
	if(sizeof(duelheader) < 2 || sizeof(duelargs) < 2) return 0;
	if(sizeof(duelheader) != sizeof(duelargs)) return 0;
	duelists = mkmapping(duelheader, duelargs);
	break;
    default: return 0;
    }
    return 1;
}

nomask status
set_dueld_startrooms(string *rooms) {

    if(!pointerp(rooms)) return 0;
    if(duelmode) return 0; //set before duelmode.

    duelstartrooms = rooms;
    return 1;
}

nomask status
set_dueld_dueltime(int s) {

    if(s < 60) s = 60;

    dueltime = s;
    return 1;
}

nomask status
set_dueld_start() {
    mixed *list;
    int i;

    if(!duelmode || !duelists || !duelstartrooms || !dueltime) return 0;

    list = m_indices(duelists);
    i = sizeof(list);
    if(!i) return 0;

    dueling = 1;
    call_out("dueld_end", dueltime, 0);
    _moveDuelists(duelstartrooms, 1);

    _tellDuelists("Bright flash of light blurs your vision. The next "+
      "moment you find yourself standing somewhere else.");

    _tellDuelArena("The Duel Match Begins!!!");

    return 1;
}


nomask private string
_getDirs(object ob) {
    int *my;
    int *other;
    string ret;

    if(!objectp(ob)) return "<no_obj>";
    if(!environment(this_player())->query_duel_arena()) return "";
    if(environment(ob)->query_duel_arena()) {
	my = (int *)environment(this_player())->query_coordinates();
	other = (int *)environment(ob)->query_coordinates();
	ret = "(";
	if(my[1] > other[1]) ret += "N";
	else if(my[1] < other[1]) ret += "S";
	else ret += ".";

	if(my[0] > other[0]) ret += "W";
	else if(my[0] < other[0]) ret += "E";
	else ret += ".";

	ret += ")"; 
    } else ret = "(LD)";

    return ret;
}

nomask void
query_dueld_status() {

    string *duelheaders;
    mixed  *duelargs;
    string tmp, name;
    status dead;
    int i, j;

    if(!duelists) return;

    duelheaders = m_indices(duelists);
    duelargs = m_values(duelists);

    i = sizeof(duelargs);
    if(!i) return;

    write("Gladiators mentioned in brackets are DEAD.\n"+
      "-------------------------------------------\n\n");

    while(i--) {
	if(duelmode == DUELD_DUELTYPE_TEAM) {
	    tmp = "<Team "+duelheaders[i]+ ">: ";
	    j = sizeof(duelargs[i]);
	    if(!j) {
		tmp += "<No Members>";
		this_player()->tell_me(tmp);
		continue;
	    }
	    while(j--) {
		if(stringp(duelargs[i][j])) {
		    name = duelargs[i][j];
		    continue;
		}
		if(!duelargs[i][j]) tmp += "("+capitalize(name)+") ";
		else tmp += (capitalize(name)+_getDirs(duelargs[i][j])
		      + " ");
	    }
	    this_player()->tell_me(tmp);                
	} 
	else {
	    if(duelargs[i]) dead = 0;
	    else dead = 1;

	    tmp = "Gladiator ";
	    if(dead) tmp += ("("+capitalize(duelheaders[i])+")");
	    else tmp += capitalize(duelheaders[i]) + _getDirs(duelargs[i]);
	    this_player()->tell_me(tmp);
	}
    }
}   

nomask status
dueld_deadobj(object ob) {
    mixed *list;
    string *names;

    int i;
    int ind;
    status ok;
    object corpse;

    if(!ob || !duelists) return 0;

    list = m_values(duelists);
    names = m_indices(duelists);

    if(duelmode == DUELD_DUELTYPE_TEAM) {
	i = sizeof(list);
	if(!i) return 0;
	while(i--) {

	    if((ind = member(list[i], ob)) != -1) {
		_tellDuelArena(capitalize(list[i][ind+1]) + 
		  " is out of the match.");
		list[i][ind] = 0;
		ok = 1;
		break;
	    }
	}
    } else if((ind = member(list, ob)) != -1) {
	_tellDuelArena(capitalize(names[ind]) +
	  " is out of the match.");
	list[ind] = 0;
	ok = 1; 
    }
    if(ok) {
	duelists = mkmapping(names, list);
	corpse = clone_object(DUELD_CORPSE);
	ob->tell_me("Your vision blurs and you fall down, you die.");
	move_object(corpse, environment(ob));
	corpse -> set_visible(1);
	move_object(ob, corpse);
    } 
    return ok;
}

nomask string
query_dueld_winner() {
    int alive;
    int aind;
    mixed *duelargs;
    string *duelheaders;
    int i, j;

    if(!duelists) return 0;

    duelheaders = m_indices(duelists);
    duelargs = m_values(duelists);

    i = sizeof(duelheaders);
    if(!i) return 0;

    while(i--) {
	if(duelmode == DUELD_DUELTYPE_TEAM) {
	    j = sizeof(duelargs[i]);
	    if(!j) continue;
	    while(j--) {
		if(objectp(duelargs[i][j])) {
		    alive++;
		    aind = i;
		    break;
		}
	    }
	}
	else if(objectp(duelargs[i])) {
	    alive++;
	    aind = i;
	}
    }

    if(!alive) return "Nobody";
    if(alive == 1) return duelheaders[aind];
    return 0;
}

nomask void
dueld_rawreset() {

    dueling = 0;
    duelists = 0;
    dueltime = 0;
    duelmode = 0;
    duelendroom = 0;
    duelstartrooms = 0;

    if(find_call_out("dueld_end"))
	remove_call_out("dueld_end");
}

nomask status
set_dueld_endroom(string s) {

    if(!s) return 0;

    duelendroom = s;
    return 1;
}

nomask void
dueld_end(string winner) {
    mixed list;
    object ob;
    int i, mh, ms;

    _tellDuelArena("The Duel is over!");
    if(winner && winner != "Nobody") {
	_tellDuelArena("The winner is "+capitalize(winner)+".");
	list = duelists[winner];
	if(duelmode != DUELD_DUELTYPE_TEAM) list = ({list});
	i = sizeof(list);
	if(i) {
	    while(i--) {

		if(objectp(list[i])) {
		    ob = list[i];
		    add_dueld_victory((string)ob->query_real_name());
		    //ob->reduce_hp(-((int)ob->query_max_hp()));
		    // ob-> add_sp((int) ob->query_max_sp());
		}
	    }
	}

	_tellDuelists("Your vision blurs and you find yourself "+
	  "from a familiar place.");
    } else if(winner && winner == "Nobody")
	_tellDuelArena("There was no winner.");
    else {
	_tellDuelArena("The Match TIMED out!!!");
	if(winner = query_dueld_winner()) return dueld_end(winner);
    }

    _moveDuelists(({duelendroom}));

    dueld_rawreset();
}

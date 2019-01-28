/* This handles all "officer" type functions of the Guild */

#include <coder_levels.h>
#include "../priest.h"

#define S_FILE PRIEST_SAVE_DIR "officers"


static mapping offices;
static mapping long_offices;
mapping officers;

create(){
    restore_object(S_FILE);
    if(!officers) officers = ([]);

    offices = ([
      15: "Guild Admin",
      11: "Avatar",
      10: "Guild Coder",
      9 : "Saint",
      8 : "Vicegerant",
      7 : "Cardinal",
      6 : "Preceptor",
      5 : "Preceptor",
      4 : "Elector",
      3 : "Elector",
      2 : "Inquisitor",
      1 : "Acolyte",
    ]);

    long_offices = ([
      6 : "Preceptor Hospitilar",
      5 : "Preceptor Templar",
      4 : "Elector Secularus",
      3: "Elector Medico",
    ]);
}

int my_rank(string myname) { return officers[myname]; }
string get_office(int i) { return offices[i]; }
string get_long_office(int i) {
    string tmp;
    if(tmp = long_offices[i])
	return tmp;
    return offices[i];
}

string get_officer(int rank) {
    int *ranks, i;

    ranks = m_values(officers);
    i = member(ranks, rank);
    if(i==-1) return 0;
    return m_indices(officers)[i];
}

string
query_title(object who) {
    int t;

    if(!query_once_interactive(who)) return "";
    t = officers[who->query_real_name()];
    if(t) return offices[t];
    switch(who->query_coder_level()) {
    case 0: break;
    case LVL_POLICE: return "Police";
    case LVL_APPRENTICE: return "Novice Coder";
    case LVL_JUNIOR: return "Novice Coder";
    case LVL_CODER: return "Coder";
    case LVL_JUNIOR_JUDGE: return "Junior Judge";
    case LVL_SENIOR: return "Senior Coder";
    case LVL_JUDGE: return "Judge";
    case LVL_COADMIN: return "Co-Admin";
    case LVL_ADMIN: return "Admin";
    default:  return "Some Coder";
    }
    if(who->query_testplayer())
	return "Testchar";
    return "";
}

void
add_officer(string who, int what) {
    if(what) officers[who] = what;
    else officers -= ([who]);
    save_object(S_FILE);
}

void
clear_officers() {
    int i;
    object pl, sym;
    string *t, name;

    t = m_indices(officers);
    i = sizeof(t);
    while(--i>=0) {
	name = t[i];
	if(officers[name] <= VG_RANK) {
	    officers -= ([ name ]);
	    if(pl = find_player(name)) {
		sym = pl->query_guild_object();
		sym->set_rank(0);
	    }
	}
    }
    save_object(S_FILE);
}

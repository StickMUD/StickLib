// GUILD HEADERS **********************************************************
#include "../global.h"

// SAVEFILE ****************************************************************
#define S_FILE (GUILD_SAVE_DIR + "officers")

// GLOBAL VARIABLES ********************************************************
static mapping offices;
mapping officers;

// LFUNS *******************************************************************

/***************************************************************************
 *
 * create(): Sets up the office names and tries to restore object,
 *           if any exist.
 *
 ***************************************************************************/
void create() {
    restore_object(S_FILE);

    if (!officers) {
	officers = ([]);
    }

    offices = ([
        6 : "Guild Admin",
        5 : "Guild Coder",
        4 : "Lord",
        3 : "Scourge",
        2 : "Strider",
        1 : "Champion",
    ]);
}

// INTERNAL METHODS ********************************************************

/***************************************************************************
 *
 * int my_rank(string myname): Returns one's rank via name.
 *
 ***************************************************************************/

int my_rank(string myname) {
    return officers[myname];
}

/***************************************************************************
 *
 * string get_office(int i): Returns office name via integer.
 *
 ***************************************************************************/

string get_office(int i) {
    return offices[i];
}

/***************************************************************************
 *
 * string get_officer(int rank): Returns officer's name via rank.
 *
 ***************************************************************************/

string get_officer(int rank) {
    int *ranks, i;

    ranks = m_values(officers);

    i = member(ranks, rank);

    if (i == -1) {
	return 0;
    }

    return m_indices(officers)[i];
}

/***************************************************************************
 *
 * void add_officer(string who, int what): Adds and officer with name who,
 *     to rank what.
 *
 ***************************************************************************/

void add_officer(string who, int what) {
    if (what) {
	officers[who] = what;
    } else {
	officers -= ([ who ]);
    }

    save_object(S_FILE);
}

/***************************************************************************
 *
 * void clear_officers(): Clears all officers below rank 5
 *
 ***************************************************************************/

void clear_officers() {
    int i;
    object pl, mark;
    string *t, name;

    t = m_indices(officers);
    i = sizeof(t);

    while(--i >= 0) {
        name = t[i];

        if(officers[name] <= 4) {
            officers -= ([ name ]);
        }
    }

    save_object(S_FILE);
}

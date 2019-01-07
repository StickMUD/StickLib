/*
 * /secure/criminal.c
 * (Aarghh...should be /bin/daemons/criminal.c)
 *
 * Handle NPC witnesses, and accusations (player witnesses). Clever.
 *
 */
#pragma strict_types

#include <criminal.h>

#define SAVEFILE "secure/save/criminal"
#define is_NPC_witness(x) ((x)->id("city_guard"))

/* crime_id: ({ whonoticed, whodunnit, whatcrime, when_time })
 *
 * If time is -1, then accusation has been done successfully.
 */
#define CD_WITNESS	0
#define CD_CRIMINAL	1
#define CD_CRIME	2
#define CD_TIME		3

#define cdata(id, dat) crimes[id][dat]

/* Variables */
mapping crimes;
int crime_id;
static mapping crtypes;

/* Prototypes */
nomask void check_crimes(object ob);

nomask void
create()
{
    if (!restore_object(SAVEFILE)) {
	crimes = ([]);
	crime_id = 0;
    }

    crtypes = ([
      CR_DISTURBANCE: "disturbance",
      CR_THEFT: "theft",
      CR_SMUGGLING: "smuggling",
      CR_BURGLARY: "burglary",
      CR_ASSAULT: "assault",
      CR_MANSLAUGHTER: "manslaughter",
      CR_WITCHCRAFT: "witchcraft",
      CR_MURDER: "murder"
    ]);
}

nomask string *
query_crime_names()
{
    return m_values(crtypes);
}

nomask string
crime_name(int c)
{
    if (crtypes[c]) return crtypes[c];
    return "excessive silliness";
}

nomask int
crime_type(string n)
{
    int *ind, i;

    ind = m_indices(crtypes);

    for (i = sizeof(ind) - 1; i >= 0; i--)
	if (crtypes[ind[i]][0..6] == n[0..6])
	    return ind[i];

    return -1;
}

nomask void
add_crime(object ob, int c)
{
    int oc, ct, crit;

    if (!ob || !living(ob)) return;

    /* Clear? */
    if (!c) {
	ob->set_crime_count(0);
	return;
    }

    oc = (int)ob->query_crime_level();
    ct = (int)ob->query_crime_count();

    if (oc < c) {
	ob->set_crime_level(c);

	if (ct >= 0) {
	    switch(c) {
	    case CR_THEFT: crit = (3 * 60 * 30); break;
	    case CR_BURGLARY: crit = (4 * 60 * 30); break;
	    case CR_ASSAULT: crit = (5 * 60 * 30); break;
	    case CR_MANSLAUGHTER: crit = (7 * 60 * 30); break;
	    case CR_MURDER: crit = (8 * 60 * 30); break;
	    default: crit = (4 * 60 * 30); break;
	    }

	    ob->set_crime_count(ct + crit);
	}
    }
}

/* Find the first valid NPC witness from environment of ob */
nomask object
find_witness(object ob)
{
    object env, *inv;
    int i, s;

    if (!(env = environment(ob))) return 0;

    /* Only in city */
    if (!env->query_in_city()) return 0;

    inv = all_inventory(env);
    if (!inv || (s = sizeof(inv)) < 1) return 0;

    for (i = 0; i < sizeof(inv); i++) {
	if ((living(inv[i])) &&
	  (ob != inv[i]) &&
	  (is_NPC_witness(inv[i]))) return inv[i];
    }

    return 0;
}

/* Find players who could later accuse the criminal */
nomask string *
find_player_witnesses(object ob)
{
    object env, *inv;
    string *wits;
    int i, s;

    wits = ({});

    if (!(env = environment(ob))) return wits;

    inv = all_inventory(env);
    if (!inv || (s = sizeof(inv)) < 1) return wits;

    for (i = 0; i < sizeof(inv); i++) {
	if ((living(inv[i])) &&
	  (ob != inv[i]) &&
	  (!inv[i]->query_npc()))
	    wits += ({ (string)inv[i]->query_real_name() });
    }

    return wits;
}

static void
purge_crimes()
{
    int i, n, t;
    mapping ncr;

    ncr = ([]);
    t = time();

    for (i = n = 0; i < crime_id; i++) {
	if (!crimes[i]) continue;

	if (((cdata(i, CD_TIME) + cdata(i, CD_CRIME) * 1800)) > t) {
	    ncr += ([ n: crimes[i] ]);
	n++;
    }
}

crime_id = n;
crimes = ncr;

save_object(SAVEFILE);
}

/*
* Function   :	nomask void crime(object c, int what)
* Description:	c has made a crime - test if there are any witnesses
* Arguments  :	c	: criminal (living object)
*		what	: what is his/her/its crime
* Returns    :	
*
*/
nomask void
crime(object c, int what)
{
    object npc;
    string *wits;
    int i, s;

    if ((npc = find_witness(c))) {
	tell_room(environment(npc), (string)npc->query_name() +
	  " yells: I noticed your crime, " +
	  (string)c->query_name() + "!\n");
	add_crime(c, what);
	return; /* Won't need player witnesses. */
    }

    wits = find_player_witnesses(c);

    if (wits && (s = sizeof(wits)) > 0) {
	for (i = 0; i < s; i++) {
	    crimes += ([
	      crime_id: ({
		wits[i],
		(string)c->query_real_name(),
		what,
		time() })
	    ]);
	    crime_id++;
	}

	save_object(SAVEFILE);
    }

}

/*
 * Function   :	nomask int accuse(object who, string whom, int what)
 * Description:	Accuse a player from a crime.
 *		Criminal doesn't have to be logon when accused.
 * Arguments  : who	: who is accusing
 *		whom	: criminal (real name)
 *		what	: what is his/her/its crime
 * Returns    :	1 if successful, 0 if failed, -1 if wrong accusation
 *
 */
nomask int
accuse(object who, string whom, int what)
{
    object ob;
    string n;
    int i, t, flag;

    if (!who || !living(who) || !whom) return 0;

    t = time();
    n = (string)who->query_real_name();
    whom = lower_case(whom);

    for (i = flag = 0; i < crime_id; i++) {

	if (!crimes[i]) continue;

	/* Are we a witness, accusing this one, from the right crime,
	 * and not done it already? */

	if ((cdata(i, CD_WITNESS) == n) &&
	  (cdata(i, CD_CRIMINAL) == whom) &&
	  (cdata(i, CD_CRIME) == what) &&
	  (cdata(i, CD_TIME) != -1)) {

	    /* This must not be an old crime (max. hour per crime level) */
	    if (((cdata(i, CD_TIME) + cdata(i, CD_CRIME) * 1800)) > t) {
		cdata(i, CD_TIME) = -1;
		if ((ob = find_player(whom))) check_crimes(ob);
		return 1;
	    } else {
		flag = 1;
	    }
	}
    }

    if (flag) {
	purge_crimes();
	return 0;
    }

    return -1;
}

/* Called at login, check if ob is accused successfully, and set
 * up his/her criminal flag if he/she is. Also inform us. */
nomask void
check_crimes(object ob)
{
    string n;
    int i, flag;

    if (!ob || !living(ob)) return;

    n = (string)ob->query_real_name();

    for (i = flag = 0; i < crime_id; i++) {

	if (!crimes[i]) continue;

	if ((cdata(i, CD_CRIMINAL) == n) &&
	  (cdata(i, CD_TIME) == -1)) {

	    if ((int)ob->query_crime_level() < cdata(i, CD_CRIME)) {
		add_crime(ob, cdata(i, CD_CRIME));
		ob->tell_me(sprintf("\n\n%-=75s\n\n",
		    "Judge tells you: You were accused for " +
		    upper_case(crime_name(cdata(i, CD_CRIME))) +
		    ", and found guilty! Surrender to nearest City Guard."));

#if 0
		/* If we want a log file, write into it here */
#endif

	    }

	    cdata(i, CD_TIME) = 0; /* Gets purged */
	    flag = 1;
	}
    }

    if (flag) purge_crimes();
}

nomask void
list_crimes()
{
    int i;

    for (i = 0; i < crime_id; i++) {
	if (!crimes[crime_id]) continue;

	write(sprintf("%-13s accused of %s by %s, %s\n",
	    capitalize(cdata(i, CD_CRIMINAL)),
	    crime_name(cdata(i, CD_CRIME)),
	    capitalize(cdata(i, CD_WITNESS)),
	    (cdata(i, CD_TIME > 0) ?
	      ctime(cdata(i, CD_TIME > 0)) :
	      "Time/code " + cdata(i, CD_TIME > 0))));

    }
}

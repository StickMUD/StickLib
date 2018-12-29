/*
 * Criminal status (<Main Town>)
 *
 * Counter < 0 means that it's permanent. (That's NASTY!)
 *
 * /basic/player/criminal.c
 *
 * CR_THEFT, CR_BURGLARY, and other crime levels are now defined
 * in /include/criminal.h. Use the defines.
 *
 */
#pragma strict_types

#include <criminal.h>

int *criminal;		/* crime count, crime level, prison count, prisoner */

#define CRIME_COUNT	criminal[0]
#define CRIME_LEVEL	criminal[1]
#define PRISON_COUNT	criminal[2]
#define PRISONER	criminal[3]	
#define PRISONER_NO     criminal[4]

static void
check_criminal()
{
    if (!criminal) criminal = ({ 0, 0, 0, 0, 0 });

    /* Check, old version had just 2 numbers */
    while (sizeof(criminal) < 5) criminal += ({ 0 });
}

int
query_crime_level()
{
    check_criminal();

    if (CRIME_LEVEL < 1) return 0;
    if (CRIME_COUNT != 0) return CRIME_LEVEL;

    CRIME_LEVEL = CRIME_COUNT = 0;

    this_object()->tell_me("You are no longer hunted for your crimes.\n");
}

int query_crime_count() { check_criminal(); return CRIME_COUNT; }
void set_crime_count(int t) { check_criminal(); CRIME_COUNT = t; }
void set_crime_level(int l) { check_criminal(); CRIME_LEVEL = l; }
void reduce_crime() {
    check_criminal();
    if (CRIME_COUNT > 0) CRIME_COUNT--;
}

string
query_crime_status()
{
    check_criminal();

    if (CRIME_LEVEL < 1) return 0;

    return (string)CRIME_D->crime_name(CRIME_LEVEL);
}

/* Prisoner's stuff. //Graah */

int query_prisoner() { check_criminal(); return PRISONER; }
void set_prisoner(int x) { check_criminal(); PRISONER = x; }
/* Only the jail cell(s) should reduce prison counter */
void reduce_prisoner() { PRISONER--; }
int query_prison_count() { check_criminal(); return PRISON_COUNT; }
void set_prison_count(int x) { check_criminal(); PRISON_COUNT = x; }

/*
 * Who are you?
 * I am number two.
 * Who is number one?
 * You are number six.
 * I'm not a number, I'm a free man!
 * BUHAHAHAHAHA...
 */
int query_prisoner_no() { return PRISONER_NO; }
void set_prisoner_no(int n) {
    if (n == 1) n = 6; /* Of course... */

    PRISONER_NO = n;
}

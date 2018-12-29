#define	TESTING

#include "../priest.h"
#include	<gametime.h>

#define SAVEDIR PRIEST_SAVE_DIR "staff/"

#define		GET_NAME(x)	(string)x->query_real_name()
#define		NATURE	"/complex/nature"

/* Save-handler; coded by Doomdark 21.11.-92.
 * An object that loads parameter-string from the desired
 * saved player-file. Used by the Staff of Boccob. All the
 * savefiles are in directory /players/priests/staff.
 * use for example:
	'str = "/players/priests/staff_handler" ->
	get_data("doomdark","staff");'
 * or:
	'"/players/priests/staff_handler" ->
	put_data("doomdark","staff","3");'
 *
 * This handler is used at the moment by the staff
 * to check whether player has broken his/her staff and should
 * wait for a new one.
 */

mapping data;	/* Index = name of the player, value = string containing
		 * data. */
/*
 * For example, one element might be:
 * ([ "doomdark", "*#staff:3#scalpel:ok#*" ])
 * Note that the first and the last character may be any character
 * except '#' (they aren't used in any way but there has to be them
 * so that sscanf will work properly).
 */

static string first_letter;
static mapping staffs;

private void
check_file(string letter)
/* Data is split into 26 different files depending on the first letter
 * of player's name. */
{
	if (!first_letter || letter != first_letter) {
/* So, if the first letter of this player != first letter of the one
 * we handled earlier, load new data in. */
		first_letter = letter;
		data = ([ ]);
		restore_object (SAVEDIR + letter); }
	return;
}
varargs string
get_data(string about_whom, string which_data)
{
string s, s2, tmp, tmp2;
	if (!stringp(about_whom) || !strlen(about_whom)) return 0;
	s = lower_case(about_whom);
	check_file (s[0..0]);
	if (!(s2 = data [s])) return 0; // No data about this player!
	if (!which_data) return s2; // Let's give all data about the player
	if (sscanf(s2, "%s#" + which_data + ":%s#%s", tmp, s, tmp2) < 3)
		return 0;	// No data about this subject.
	return s;
}
varargs status
put_data(string about_whom, string which_data, string what_data)
{
string s, s2, s3, tmp, tmp2;
	if (!stringp(about_whom) || !stringp(which_data) ||
		!strlen(about_whom)) return 0;
	s = lower_case(about_whom);
	check_file (s[0..0]);
	if (!(s2 = data[s])) {
		if(what_data)
		data += ([ about_whom:"*#"+which_data+":"+what_data+"#*" ]); }
	else {
	  if (sscanf(s2, "%s#" + which_data + ":%s#%s", tmp, s3, tmp2) < 3)
	    data[s] = what_data ? s2[0..strlen(s2)-2]+ which_data+":"+
		what_data + "#*" : data[s];
	  else
	    data[s] = what_data ? tmp + "#" + which_data + ":" + what_data +
		"#" + tmp2 : tmp + "#" + tmp2;
	  }
	if(strlen(data[s]) < 4) data = m_delete(data, s);
	save_object(SAVEDIR + first_letter);
	return 1;
}
int
beseech(object who)
{
string s, t;
	t = GET_NAME(who);
	s = get_data(t, "staff");
	if(!s) {
		if ((int) who -> query_level() < STAFF_LEVEL) return 0;
		else return 2;
	}
	if(s == "broken") return 3;
	if(s == "ok") {
		if(mappingp(staffs)) {
			if(objectp(staffs[t])) return 5;
	/* Already has the staff! */
			staffs = m_delete(staffs, t);
		}
		return 4;	/* Can be given the staff ok. */
	}
	return 100;	/* 'default'... */
}
void
set_staff(object who, status not_first)
{
	put_data(GET_NAME(who), "staff", "order"+not_first);
}
status
ok_to_order(object who)
{
string s;
	s = get_data(GET_NAME(who), "staff");
	return s == "order0" || s == "order1";
}
int make_staff(object who) /* Returns the cost of the staff. */
{
string s, t;
int *a, lvl;
	s = get_data(GET_NAME(who), "staff");
/*
 * And then let's add info about when the staff is ready...
 * '1:10:603-3:10:603' for example means that the first date etc is the time when
 * the staff is ordered and second when it'll be ready.
 */
	a = (int*) NATURE -> query_game_time();
	t = "" + a[0] + ";" + a[1] + ";" + a[2] + ";" + a[3] + ";" + a[4];
	if (s == "order0") a[2]++;
/* So, the new staffs will only take 1 day to get ready. */
	else {
		lvl = (int) who -> query_level();
		a[1] += 24 + lvl * 8; /* So, once broken, it'll take 1 - 15 days
					* (game time -> real time * 12). */
	}
	while (a[1] >= HOURS) {
		a[1] -= HOURS;
		a[2]++;
	}
	if (a[2] >= DAYS) {
		a[2] -= DAYS;
		a[3]++;
		if (a[3] > MONTHS) {
			a[3] -= MONTHS;
			a[4]++;
		}
	}
	t += "/" + a[0] + ";" + a[1] + ";" + a[2] + ";" + a[3] + ";" + a[4] + "/";
	t += lvl ? (200 + lvl * 40) : 0;
	put_data(GET_NAME(who), "staff", t);
	if (!lvl) return 0;
	return 200 + lvl * 40;	/* So, something between 200 and ~2000 gps... */
}
string
staff_ready(object who)
{
string s, t, u;
int foo, hour, day, month, year;
	s = get_data(GET_NAME(who), "staff");
	if (sscanf(s, "%s/%s/%d", t, u, foo) < 3) return GOD_NAME + " knows when";
	sscanf(u, "%d;%d;%d;%d;%d", foo, hour, day, month, year);
	s = (string) NATURE -> query_month_s(month) + " " + day;
	if(day > 10 && day < 14) s += "th";
	else switch(day % 10) {
	case 1: s += "st"; break;
	case 2: s += "nd"; break;
	case 3: s += "rd"; break;
	default: s += "th";
	}
	s += " at about ";
	if(!(hour % (HOURS/2))) s += "12 ";
	else s += (hour < HOURS/2) ? hour : (hour - HOURS/2);
	s += (hour > 11) ? " pm" : "am";
	return s;
}
int
staff_status(object who)
{
string s, t, u;
int a, *b, foo;
int h1, h2, d1, d2, m1, m2, y1, y2;
int sum1, sum2, sum3;
	s = get_data(GET_NAME(who), "staff");
	if(!s || (sscanf(s, "%s/%s/%d", t, u, a) < 3)) return 0;
	if(sscanf(t, "%d;%d;%d;%d;%d", foo, h1, d1, m1, y1) < 5) return 0;
	if(sscanf(u, "%d;%d;%d;%d;%d", foo, h2, d2, m2, y2) < 5) return 0;
	b = (int*) NATURE -> query_game_time();
	sum1 = h1 + d1 * HOURS + m1 * HOURS * DAYS + y1 * HOURS * DAYS * MONTHS;
	sum2 = h2 + d2 * HOURS + m2 * HOURS * DAYS + y2 * HOURS * DAYS * MONTHS;
	sum3 = b[1] + b[2]*HOURS + b[3]*HOURS*DAYS + b[4]*HOURS*DAYS*MONTHS;
	if ((sum1 > sum3) || (sum2 <= sum3)) return 2;
	return 1;
}
void
staff_got(object who)
{
	put_data(GET_NAME(who), "staff", "ok");
}
object
get_staff(object who)
{
object x;
	x = clone_object(STAFF_NAME);
	x -> set_owner(who);
	if(!staffs) staffs = ([]);
	staffs += ([ (GET_NAME(who)):x ]);
	return x;
}
status staff_ordered(object who)
{
string s, t, u;
	s = get_data(GET_NAME(who), "staff");
	return s && sscanf(s, "%s/%s", t, u) == 2;
}

object query_staff(object who) {
    if(!staffs) return 0;
    return staffs[GET_NAME(who)];
}

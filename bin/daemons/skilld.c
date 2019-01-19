/*
 * /bin/daemons/skilld.c
 *
 * Player's guild object (player->query_guild_object()) should return
 * the skill data when asked. It should define these functions:
 *
 * mapping query_all_skill_data()
 * 
 * ([
 *	"skill name": ({ pause, forget }),
 *	"another": ...,
 *	...
 * ])
 *
 * int *query_skill_data(string skname)
 *
 *  -->   ({ pause, forget })    for skill named skname
 *
 * pause
 *	How many real time seconds must pass before this skill can advance.
 *	(Yes, but we calculate game time from real time too, don't we?)
 *	Default pause is 180 (is that ok?)
 *
 *	With pause 180 it takes 208 full days (24 hours each) of constant
 *	training to get a skill of 100000.
 *
 *	days = ((pause * 125) / 108)
 *
 * forget
 *	How many seconds pass before we forget 1/1000 point of skill.
 *	If forget is 0, this skill is never forgotten. If forget is
 *	equal to or smaller than pause we can never learn the skill.
 *	Usually forget should be 7200, which means that if we don't use
 *	the skill at least once every 2 hours of playing, we forget 1/1000
 *	point. (Players must not know anything about this!)
 *
 * Skills which are not listed here use default pause 180 and forget 7200.
 *
 */

#include <skills.h>

#define MAX_SKNAME_LEN 12

#define __SK_PAUSE 0
#define __SK_FORGET 1

static string *skrat, *sklev;
static int s_skrat, s_sklev;

nomask void
create()
{
}

/*
 * Function    : nomask status query_is_valid_skill(string skname)
 * Description : Checks if skill skname can be used.
 * Arguments   : skname: skill name
 * Returns     : 1 for valid skill, 0 for invalid
 *
 */
nomask status
query_is_valid_skill(string skname)
{
    /* Todo: how and where to report errors? Shouldn't be done with write()? */

    if (!skname || !stringp(skname)) {
	write("query_is_valid_skill: Illegal skill name\n");
	return 0;
    }

    /* Or are we going to have "ki"? Oriental stuff... */
    if (sizeof(skname) < 3) {
	write("query_is_valid_skill: too short skill name \"" +
	  skname + "\"\n");
	return 0;
    }

    if (sizeof(skname) > MAX_SKNAME_LEN) {
	write("query_is_valid_skill: too long skill name \"" +
	  skname + "\"\n");
	return 0;
    }

    return 1;
}

/*
 * Function    : nomask string skill_desc(int value)
 * Description : Return a description of skill from numeric 0..100 value
 * Arguments   : value: skill value, 0..100
 *
 */
nomask string
skill_desc(int value)
{
    int l, r;

    if (!skrat) {
	skrat = ({ "novice ", "junior ", "apprentice ", "", "confident ",
	  "seasoned ", "expert ", "eminent ", "brilliant ", "superior " });
	s_skrat = sizeof(skrat);
    }

    if (!sklev) {
	sklev = ({ "student", "amateur", "layman", "acolyte", "journeyman",
	  "craftsman", "professional", "veteran", "master", "guru" });
	s_sklev = sizeof(sklev);
    }

    if (value > 99) value = 99;

    l = ((s_sklev * value) / 100);
    r = (value - ((100 / s_skrat) * l));

    if (l > (s_sklev - 1)) l = (s_sklev - 1);
    if (l < 0) l = 0;
    if (r > (s_skrat - 1)) l = (s_skrat - 1);
    if (r < 0) r = 0;

    return (skrat[r] + sklev[l]);
}

/*
 * Function   :	nomask string list_skills(object ob)
 * Description:	Return a nicely formatted list of skills of living object,
 *		described with words.
 */
nomask string
list_skills(object ob)
{
    mapping liv_Skills;
    string tmp, *ind;
    int i, l, x, y;

    if (!ob || !living(ob)) return 0;

    if (!(liv_Skills = (mapping)ob->query_liv_Skills())) return 0;

    ind = m_indices(liv_Skills);

    if (!ind || sizeof(ind) < 1) return "Totally unskilled with everything.\n";

    tmp = "";

    for (l = 0, i = sizeof(ind) - 1; i >= 0; i--) {
	x = liv_Skills[ind[i]][SK_SCORE];
	y = x - 100 * (x / 100);
	if (y < 0) y = 0;
	if (y > 99) y = 99;

	tmp += sprintf("%-11s:%02d%% %-23s ",
	  ind[i],
	  y,
	  skill_desc(x / 1000));
	l++;
	if (l > 1) {
	    l = 0;
	    tmp += "\n";
	}
    }

    if (tmp[-1..-1] != "\n") tmp += "\n";

    return tmp;
}


/*
 * Function    :
 * Description :
 * Arguments   :
 * Returns     :
 *
 */

/* EOF */

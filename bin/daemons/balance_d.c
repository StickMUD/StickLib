/*
 * Guild Balance. All guilds would politely ask their damage, defence
 * and heal from this.
 *
 * The numbers are for skills with one round delay. Instants (such
 * as parry/tumble etc.) divide all by 2?
 *
 * Don't know how steals etc. would be done?
 *
 * Only a suggestion. //Graah
 *
 * SP/FP Requirement for maximum effect:
 *
 *	 20 at skill 0
 *	120 at skill 1000
 *
 * damage and melee_damage: if PK, divide damage by five (5).
 *
 * Always take random(damage) etc! Mustn't always use full damage.
 * Durations can be full, or partially random etc.
 *
 * Last update 2-Jul-96, Healer tuning. //Graah
 * Last update 16-Jul-96, Heals tuned again. //Graah
 *
 */

#include <guild.h>

#define W_ATT	0
#define W_DEF	1
#define W_HEAL	2
#define PK_DAM 40

static status critical_success, critical_failure;
static mapping guilds;

nomask void
create()
{
    /* Guild skill area weighs. Every guild has 200% to use. */

    guilds = ([
      /*			  Attack	Defence		Heal	*/

      GN_NONE:	({  80,		 80,		 40, }),

#ifdef GN_FIGHTER
      GN_FIGHTER:	({ 100,		 75,		 25, }),
#endif
#ifdef GN_HEALER
      GN_HEALER:	({  20,		 80,		100, }),
#endif
#ifdef GN_MAGE
      GN_MAGE:	({  80,		 80,		 40, }),
#endif
#ifdef GN_NECROMANCER
      GN_NECROMANCER:	({  85,		 75,		 40, }),
#endif
#ifdef GN_NINJA
      GN_NINJA:	({  95,		 80,		 25, }),
#endif
#ifdef GN_PRIEST
      GN_PRIEST:	({  65,		 60,		 75, }),
#endif
#ifdef GN_THIEF
      GN_THIEF:	({  75,		 90,		 35, }),
#endif
    ]);
}


/*
 * Function name: critical_success()
 * Returns	: 1 if we made a critical success
 */
nomask status critical_success() { return critical_success; }


/*
 * Function name: critical_success()
 * Returns	: 1 if we made a critical failure
 */
nomask status critical_failure() { return critical_failure; }


/*
 * Function Name: int skill_roll(int skill_level)
 * Arguments	: int skill_level: skill level in promilles (0..1000)
 * Returns	: Anything from -999 to 999 (amount of success/failure).
 *			result <= 0 means failure
 *			result >= 1 means success
 * Notes	: Ask critical_failure() or critical_success() from
 *		  this daemon.
 */
int
skill_roll(int skill_level)
{
    int r, cs;

    critical_failure = critical_success = 0;

    if(skill_level > 1000) skill_level = 1000; // Max 100%

    r = random(1001);

    if (r <= 10) { critical_failure = 1; return 0; }

    /* Critical success chance? */
    cs = (990 - (skill_level / 2500));
    if (cs < 900) cs = 900;

    if (r >= cs) critical_success = 1;
    /* Why is this here?
      else if (r < skill_level) return 0;
      //Sumppen */

    return (skill_level - r);
}


/*
 * Function Name: int hit_chance(int skill_level, string guild)
 * Arguments	:
 *	int skill_level	: skill level in promilles (0..1000)
 *	string guild	: GN_FIGHTER etc. (/include/guilds.h)
 * Returns	: Percentage chance to hit (in melee combat).
 *		  Defence percent etc. should be subtracted
 *		  afterwards.
 * Notes	: We consider guild-less people to be fighters.
 *		  Note that they don't have any fighter special
 *		  skills, so they won't be real fighters!
 *
 */
nomask int
hit_chance(int skill_level, string guild)
{
    int s, a;

    if (!guilds[guild])
	a = guilds[GN_NONE][W_ATT];
    else a = guilds[guild][W_ATT];

    s = (60 + (100 * skill_level / 869));

    return (s * a / 100);
}


/*
 * Function Name: status hit_roll(int skill_level, string guild, int ac)
 * Description	: Make a hit roll against (ac)
 * Arguments	:
 * 	int skill_level	: Skill level in promilles (0..1000)
 *	string guild	: GN_FIGHTER etc. (/include/guild.h)
 *	int ac		: Armour class (0..25 or more)
 * Returns	: 1 if we hit, 0 if we don't.
 * Notes	: Ask for critical_success() or critical_failure()
 *		  from this daemon.
 */
nomask status
hit_roll(int skill_level, string guild, int ac)
{
    int c, r;

    critical_failure = critical_success = 0;

    c = (hit_chance(skill_level, guild) - ac);

    if (c < 10) c = 10;
    if (c > 95) c = 95;

    r = random(100);

    if (r >= 99) { critical_failure = 1; return 0; }
    if (r < (1 + (skill_level > 10000 ?
	  1 + (skill_level - 10000) / 1000 : 0)))
    { critical_success = 1; return 1; }

    if (r <= c) return 1;

    return 0;
}


/*
 * Function name: int damage(int skill_level, string guild, int p)
 * Description	: Returns base MK damage for skill and fp/sp used.
 * Arguments	:
 *	int skill_level	: In promilles (0..1000 or more)
 *	string guild	: GN_FIGHTER etc. (/include/guild.h)
 *	int p		: how many sp, fp or hp were used
*      status is_player: shows if the target is a player
 * Notes	: Returns damage in hits.
 */
nomask int
damage(int skill_level, string guild, int p,status is_player)
{
    int a, dmg, sk, pr;

    if (!guilds[guild])
	a = guilds[GN_NONE][W_ATT];
    else a = guilds[guild][W_ATT];

    /* Max possible MK damage for skill */
    dmg = (100 + (skill_level - 50));

    /* FP/SP requirement for max effect at this skill.
     * Reduce damage if less than full is used. */
    pr = (30 + (10 * skill_level / 111));
    if (p < pr) a = (a * p) / pr;

    if(is_player)
	return (PK_DAM * (a * dmg / 100) / 100);
    else
	return (a * dmg / 100);
}


/*
 * Function name: int melee_damage(
 *	      int skill_level, string guild, int p, object wp, object who)
 * Description	: Returns base MK melee damage for skill and fp/sp used.
 * Arguments	:
 *	int skill_level	: In promilles (0..1000 or more)
 *	string guild	: GN_FIGHTER etc. (/include/guild.h)
 *	int p		: how many sp, fp or hp were used
 *	object who	: who is hitting
 *	object wp	: weapon used to hit
*      status is_player : is the target a player
 * Notes	: Returns damage in hits.
 */
nomask int
melee_damage(int skill_level, string guild, int p, object who, object wp,status is_player)
{
    int dmg, wc;

    dmg = damage(skill_level, guild, p,is_player);

    /* Weapon class (treating "20" as maximum at the moment) */
    /* Melee attacks with wc more than 20 increase damage! */
    if (wp) wc = (int)wp->query_wc();
    else if (who) wc = (int)who->query_wc();
    else wc = 10;
    if (wc != 20) dmg = (dmg * wc) / 20;

    return (dmg);
}


/*
 * Function name: int defence(int skill_level, string guild, int p)
 * Description	: Returns base defence percent for skill and fp/sp used.
 * Arguments	:
 *	int skill_level	: In promilles (0..1000 or more)
 *	string guild	: GN_FIGHTER etc. (/include/guild.h)
 *	int p		: how many sp, fp or hp were used
 * Notes	: Returns 0..100%
 */
nomask int
defence(int skill_level, string guild, int p)
{
    int a, def, pr;

    if (!guilds[guild])
	a = guilds[GN_NONE][W_DEF];
    else a = guilds[guild][W_DEF];

    /* Max possible defence for skill */
    def = (30 + (7 * skill_level / 100));

    /* FP/SP requirement for max effect at this skill.
     * Reduce damage if less than full is used. */
    pr = (30 + (10 * skill_level / 111));
    if (p < pr) a = (a * p) / pr;

    return (a * def / 100);
}


/*
 * Function name: int defence(int skill_level, string guild, int p)
 * Description	: Returns base defence as AC for skill & fp/sp used
 * Arguments	:
 *	int skill_level	: In promilles (0..1000 or more)
 *	string guild	: GN_FIGHTER etc. (/include/guild.h)
 *	int p		: how many sp, fp or hp were used
 * Notes	: Returns 0 to 18 (or more?)
 */
nomask int
defence_ac(int skill_level, string guild, int p)
{
    return (defence(skill_level, guild, p) / 5);
}


/*
 * Function name: int duration(int skill_level, string guild, int p)
 * Description	: Returns base duration for skill & fp/sp used, for
 *		  defence, resistance etc. benefical spells.
 * Arguments	:
 *	int skill_level	: In promilles (0..1000 or more)
 *	string guild	: GN_FIGHTER etc. (/include/guild.h)
 *	int p		: how many sp, fp or hp were used
 * Notes	: Returns from 8 to 48 (in heart beats)
 *		  For attack/harmful effects and spells,
 *		  DIVIDE DURATION BY FOUR (4)!!!
 */
nomask int
duration(int skill_level, string guild, int p)
{
    int dur, pr;

    dur = (8 + (skill_level / 25));

    /* FP/SP requirement for max effect at this skill.
     * Reduce damage if less than full is used. */
    pr = (30 + (10 * skill_level / 111));
    if (p < pr) dur = (dur * p) / pr;

    return dur;
}


/*
 * Function name: int healing(int skill_level, string guild, int p)
 * Description	: Returns base healing (hp/sp/fp) for skill and fp/sp used.
 * Arguments	:
 *	int skill_level	: In promilles (0..1000 or more)
 *	string guild	: GN_FIGHTER etc. (/include/guild.h)
 *	int p		: how many sp, fp or hp were used
 * Notes	: Returns 0..400
 *		  NOTE: CAN ONLY HEAL _ONE_ THING, i.e. HP, SP _or_ FP!
 *		  If this heals many things, it has to be divided
 *		  equally to them!
 */
nomask int
healing(int skill_level, string guild, int p)
{
    int a, heal, pr;

    if (!guilds[guild])
	a = guilds[GN_NONE][W_HEAL];
    else a = guilds[guild][W_HEAL];

    /* Max possible heal for skill */
    /* Don't take these "unneccessary" parentheses away any more, please!
       This thing heals zillion points if you do so. /Graah */
    heal = (20 + ((30 * skill_level) / 100));

    /* FP/SP requirement for max effect at this skill.
     * Reduce damage if less than full is used. */
    pr = (20 + (skill_level / 10));
    if (p < pr) a = ((a * p) / pr);

    return ((a * heal) / 100);
}

#define	LIST_GUILDS	\
  write("  Skill%");\
  for (i = 0; i < sizeof(guilds); i++)\
	printf("%8s", guilds[i]);\
  write("\n");\

nomask void
tables(int used)
{
    int i, j, s;
    string *guilds;

    guilds = ({ });

#ifdef GN_FIGHTER
    guilds += ({ GN_FIGHTER });
#endif
#ifdef GN_HEALER
    guilds += ({ GN_HEALER });
#endif
#ifdef GN_MAGE
    guilds += ({ GN_MAGE });
#endif
#ifdef GN_NECROMANCER
    guilds += ({ GN_NECROMANCER });
#endif
#ifdef GN_NINJA
    guilds += ({ GN_NINJA });
#endif
#ifdef GN_PRIEST
    guilds += ({ GN_PRIEST });
#endif
#ifdef GN_THIEF
    guilds += ({ GN_THIEF });
#endif

    if (!used)
	used = 120;
    printf("\
    MK ATTACK, maximum damages (spell or wc 20 weapon, %d sp/fp)\n",used);

    LIST_GUILDS

    for(i=0;i<101;i+=10)
    {
	s = i*10;
	printf("%7d", s / 10);
	for (j = 0; j < sizeof(guilds); j++)
	    printf("%8d", damage(s, guilds[j], used, 0));
	write("\n");
    }

    printf("\n\n\
    PK ATTACK, maximum damages (spell or wc 20 weapon, %d sp/fp)\n",used);

    LIST_GUILDS

    for(i=0;i<101;i+=10)
    {
	s = i*10;
	printf("%7d", s / 10);
	for (j = 0; j < sizeof(guilds); j++)
	    printf("%8d", damage(s, guilds[j], used, 0));
	write("\n");
    }


    printf("\n\n\
    DEFENCE skill, maximum percents (using 120 sp/fp)\n");
    LIST_GUILDS


    for(i=0;i<101;i+=10)
    {
	s = i*10;
	printf("%7d", s / 10);
	for (j = 0; j < sizeof(guilds); j++)
	    printf("%8d", defence(s, guilds[j], 120));
	write("\n");
    }

    printf("\n\n\
    skill/spell DURATION, maximum # of beats (using 120 sp/fp)\n");

    LIST_GUILDS

    for(i=0;i<101;i+=10)
    {
	s = i*10;
	printf("%7d", s / 10);
	for (j = 0; j < sizeof(guilds); j++)
	    printf("%8d", defence(s, guilds[j], 120));
	write("\n");
    }


    printf("\n\n\
    HARMFUL/ATTACK skill/spell DURATION, maximum # of beats (using 120 sp/fp)\n");

    LIST_GUILDS
    for(i=0;i<101;i+=10)
    {
	s = i*10;
	printf("%7d", s / 10);
	for (j = 0; j < sizeof(guilds); j++)
	    printf("%8d", duration(s, guilds[j], 120) / 4);
	write("\n");
    }


    printf("\n\n\
    HEALING, maximum heal (to HP, SP _or_ FP) (using 120 sp/fp)\n");

    LIST_GUILDS

    for(i=0;i<101;i+=10)
    {
	s = i*10;
	printf("%7d", s / 10);
	for (j = 0; j < sizeof(guilds); j++)
	    printf("%8d", healing(s, guilds[j], 120));
	write("\n");
    }
}

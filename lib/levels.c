// Title lists are now much easier to expand. You don't have to have
// any fixed number of titles, nor to calculate their numbers. //Graah
// This should be in /bin/daemons...
// 06-May-96 / Graah : Dark Elves. Slightly weaker but smarter than elves.
// 23-Jun-96 / Graah : Checked stats, are they balanced? Dwarvish dex for
//		       example. Dwarves can't even devour corpses.
// 26-Jun-96 / Graah : Re-checked the racial stat totals
// 27-Jul-96 / Graah : NewLevelLimits Table, not used yet, developing...
//		       Old one will still be used for stats, new one
//		       just for level limits.
// 15-Aug-96 / Graah : Humans now got all stats max at 45.
// 26-Sep-96 / Graah : New level titles. Lord stuff should be at lvl 25.
// 28-Sep-96 / Graah : Max. stat is now 500. Of course, races still
//		       have same limits as before.
//  2-Oct-96 / Graah : Max. level is now 45097156650. Theoretically.
//  3-Oct-96 / Graah : Titles being fixed.

#include <tune.h>
#include <coder_levels.h>
#include <stats.h>
#include <gender.h>



// Variables
static mapping tits;
static string *ladies;
static string *lords;

// Old stat cost modifiers for races...
static mapping RaceModifiers;
// New racial maximum values for the stats:
static mapping RaceMaxStats;

static int *LevelLimits = ({
  0,1014,1522,2283,3425,
  5138,7707,11561,17341,26012,
  // 11 - 20
  39018,58527,77791,97791,131687,
  197530,296296,444444,666666,1000000,
  // 21 - 30
  1500000,2000000,3000000,5000000,8000000,
  15000000, 25000000, 35000000, 45000000, 60000000,
  // 31 - 40
  80000000, 100000000, 125000000, 150000000, 175000000,
  200000000, 225000000, 250000000, 275000000, 300000000,
  // 41 - 50
  350000000, 400000000, 450000000, 500000000, 550000000,
  600000000, 650000000, 700000000, 750000000, 800000000,
  // 51 - 60
  900000000, 1000000000, 1100000000, 1200000000, 1300000000,
  1400000000, 1500000000, 1600000000, 1700000000, 1800000000
  // (max. level 60!)
});

static int No_of_LL;

static int *LevelCosts;


void
create()
{
    int i, cost;

    No_of_LL = sizeof(LevelLimits);

    i = 0;
    LevelCosts = allocate(30);
    while (i < 20) {
	LevelCosts[i] = LevelLimits[i] / 7;
	i++;
    }
    while (i < 30) {
	cost = 10 * ((i + 2) * 438 + (i - 19) * 3300);
	if (i > 23 && i < 28) cost = cost * 3 + 150000;
	else if (i > 27) cost = cost * 6 + 300000;
	LevelCosts[i] = cost;
	i++;
    }

    /* Dwarves had max dex just 25, fixed. 23-Jun-96/Graah. */
    RaceMaxStats = ([
      "dwarf" : ([
	ST_STR : 40,
	ST_CON : 35,
	ST_DEX : 30,
	ST_INT : 25,
	ST_FAT : 50,
	ST_BASE_STR : 40,
	ST_BASE_CON : 35,
	ST_BASE_DEX : 30,
	ST_BASE_INT : 25,
	ST_BASE_FAT : 50
      ]),
      "dark elf" : ([
	ST_STR : 28,
	ST_CON : 30,
	ST_DEX : 40,
	ST_INT : 52,
	ST_FAT : 25,
	ST_BASE_STR : 30,
	ST_BASE_CON : 30,
	ST_BASE_DEX : 40,
	ST_BASE_INT : 50,
	ST_BASE_FAT : 25
      ]),
      "elf" : ([
	ST_STR : 30,
	ST_CON : 30,
	ST_DEX : 40,
	ST_INT : 50,
	ST_FAT : 25,
	ST_BASE_STR : 30,
	ST_BASE_CON : 30,
	ST_BASE_DEX : 40,
	ST_BASE_INT : 50,
	ST_BASE_FAT : 25
      ]),
      "half-orc" : ([
	ST_STR : 40,
	ST_CON : 40,
	ST_DEX : 30,
	ST_INT : 25,
	ST_FAT : 40,
	ST_BASE_STR : 40,
	ST_BASE_CON : 40,
	ST_BASE_DEX : 30,
	ST_BASE_INT : 25,
	ST_BASE_FAT : 40
      ]),
      "hobbit" : ([
	ST_STR : 25,
	ST_CON : 30,
	ST_DEX : 50,
	ST_INT : 40,
	ST_FAT : 30,
	ST_BASE_STR : 25,
	ST_BASE_CON : 30,
	ST_BASE_DEX : 50,
	ST_BASE_INT : 40,
	ST_BASE_FAT : 30
      ]),
      "human" : ([
	ST_STR : 45,
	ST_CON : 45,
	ST_DEX : 45,
	ST_INT : 45,
	ST_FAT : 45,
	ST_BASE_STR : 45,
	ST_BASE_CON : 45,
	ST_BASE_DEX : 45,
	ST_BASE_INT : 45,
	ST_BASE_FAT : 45
      ]),
      "lich" : ([
	ST_STR : 30,
	ST_CON : 30,
	ST_DEX : 40,
	ST_INT : 45,
	ST_FAT : 30,
	ST_BASE_STR : 30,
	ST_BASE_CON : 30,
	ST_BASE_DEX : 40,
	ST_BASE_INT : 45,
	ST_BASE_FAT : 30
      ]),
      "troll" : ([
	ST_STR : 50,
	ST_CON : 50,
	ST_DEX : 20,
	ST_INT : 15,
	ST_FAT : 40,
	ST_BASE_STR : 50,
	ST_BASE_CON : 50,
	ST_BASE_DEX : 20,
	ST_BASE_INT : 15,
	ST_BASE_FAT : 40
      ]),
    ]);

    // Old stat cost modifiers for races:
    // Str, Con, Dex, Int, Fat
    RaceModifiers = ([
      "elf":        ({ 120, 110,  90,  80, 100 }),
      "dark elf":   ({ 122, 110,  90,  78, 100 }),
      "dwarf":      ({ 90,  120, 120, 100, 100 }),
      "hobbit":     ({ 120,  90,  80, 110, 100 }),
      "half-orc":   ({ 80,   80, 110, 120, 100 }),
      "troll":      ({ 60,   60, 130, 200, 100 }),
      "human":      ({ 125, 125, 125, 125, 125 }),
      0:	    ({ 100, 100, 100, 100, 100 })
    ]);

    ladies = ({
      0, "Squiress", "Knightess", "Lady", "Heraldess",
      "Warlady", "Noble Lady", "Lady Superior",
      "Lady Mistress", "Imperial Lady", "Chancellor", "Baronette",
      "Baroness", "Viscountess", "Lesser Countess", "Countess",
      "Greater Countess", "Supreme Countess", "Thane", "Marquise", "Emiress",
      "Duchess", "Archduchess", "Princess", "Crown Princess", "Queen",
      "Monarch", "Pharaoh", "Regentess", "Chairwoman", "Caliph",
      "Sultaness", "Imperial Princess", "Empress", "Tsarina",
      "Sovereigness", "Heroine", "Greater Heroine", "Lesser Legend",
      "Legend", "Greater Legend", "Lesser Myth", "Myth", "Greater Myth",
      "High Mortal", "Lord Mortal", "Supreme Mortal",
      "Empyreal Heraldess", "Empyreal Soldier", "Knight Empyreal",
      "High Empyreal", "Lord Empyreal", "Supreme Empyreal",
      "Lesser Elemental", "Elemental", "Greater Elemental",
      "Lesser Astral", "Astral", "Greater Astral",
      "Lesser Ethereal", "Ethereal", "Greater Ethereal",
      "Lesser Planar", "Planar", "Greater Planar", "Planar Lord",
      "Lesser Avatar", "Avatar", "Greater Avatar", "Supreme Avatar",
      "Novice Half-Deity", "Minor Half-Deity", "Lesser Half-Deity",
      "Half-Deity", "Greater Half-Deity", "Supreme Half-Deity",
      "Novice Demigoddess", "Minor Demigodess", "Lesser Demigoddess",
      "Demigoddess", "Greater Demigoddess", "Supreme Demigoddess",
      "Novice Goddess", "Minor Goddess", "Lesser Goddess",
      "Goddess", "Greater Goddess", "Major Goddess",
      "Goddess Lady", "Outer Goddess", "Novice Entity",
      "Lesser Entity", "Entity", "Major Entity", "Supreme Entity",
      "Primal Goddess", "The One Goddess", "All-Mother"
    });

    lords = ({
      0, "Squire", "Knight", "Lord", "Herald",
      "Warlord", "Noble Lord", "Lord Superior",
      "Lord Master", "Imperial Lord", "Chancellor", "Baronet",
      "Baron", "Viscount", "Lesser Count", "Count",
      "Greater Count", "Earl", "Thane", "Marquis", "Emir",
      "Duke", "Archduke", "Prince", "Crown Prince", "King",
      "Monarch", "Pharaoh", "Regent", "Chairman", "Caliph",
      "Sultan", "Imperial Prince", "Emperor", "Tsar",
      "Sovereign", "Hero", "Greater Hero", "Lesser Legend",
      "Legend", "Greater Legend", "Lesser Myth", "Myth", "Greater Myth",
      "High Mortal", "Lord Mortal", "Supreme Mortal",
      "Empyreal Herald", "Empyreal Soldier", "Knight Empyreal",
      "High Empyreal", "Lord Empyreal", "Supreme Empyreal",
      "Lesser Elemental", "Elemental", "Greater Elemental",
      "Lesser Astral", "Astral", "Greater Astral",
      "Lesser Ethereal", "Ethereal", "Greater Ethereal",
      "Lesser Planar", "Planar", "Greater Planar", "Planar Lord",
      "Lesser Avatar", "Avatar", "Greater Avatar", "Supreme Avatar",
      "Novice Half-Deity", "Minor Half-Deity", "Lesser Half-Deity",
      "Half-Deity", "Greater Half-Deity", "Supreme Half-Deity",
      "Novice Demigod", "Minor Demigod", "Lesser Demigod",
      "Demigod", "Greater Demigod", "Supreme Demigod",
      "Novice God", "Minor God", "Lesser God",
      "God", "Greater God", "Major God",
      "God Lord", "Outer God", "Novice Entity",
      "Lesser Entity", "Entity", "Major Entity", "Supreme Entity",
      "Primal God", "The One God", "All-Father"
    });

    // You can freely add more races and titles

    tits =
    ([
      "dark elf": ({
	({
	  "Harmless","Mostly Harmless","Novice","Novice Wanderer","Wanderer",
	  "Wanderer of Deep","Wanderer of Shadows","Master of Deep",
	  "Master of Shadows","Cavemaster", // 10
	  "Archer","Shadow Warrior","Swordmaster","Wardancer",
	  "Elven Warmaster","Gazer of Lava","Sage",
	  "Elder Sage","Grand Sage","Loremaster", // 20
	  "Elder Loremaster","Master Loremaster",
	  "Archmage of Underdark",
	  "Master of Underdark",
	  "Servant"
	}),
	({
	  "Harmless","Mostly Harmless","Novice","Novice Wanderer","Wanderer",
	  "Wanderer of Deep","Wanderer of Shadows","Mistress of Deep",
	  "Mistress of Shadows","Cavemistress", // 10
	  "Archer","Shadow Warrior","Swordmistress","Wardancer",
	  "Elven Warmistress","Gazer of Lava","Sage",
	  "Elder Sage","Grand Sage","Loremistress", // 20
	  "Elder Loremistress","Master Loremistress",
	  "Archwitch of Underdark",
	  "Mistress of Underdark",
	  "Servant"
	})
      }),
      "elf": ({
	({
	  "Harmless","Mostly Harmless","Novice Wanderer","Wanderer",
	  "Wanderer of Forests","Wanderer of Shadows","Master of Forests",
	  "Master of Shadows","Wildmaster","Archer", // 10
	  "Shadow Warrior","Swordmaster","Wardancer",
	  "Elven Warmaster","Gazer of Stars","Sage",
	  "Elder Sage","Grand Sage","Loremaster",
	  "Elder Loremaster", // 20
	  "Grand Loremaster","Elven Archmage","Hierophant",
	  "Master Hierophant",
	  "Servant"
	}),
	({
	  "Harmless","Mostly Harmless","Novice Wanderer","Wanderer",
	  "Wanderer of Forests","Wanderer of Shadows","Mistress of Forests",
	  "Mistress of Shadows","Wildmistress","Archer", // 10
	  "Shadow Warrior","Swordmistress","Wardancer",
	  "Elven Warmistress","Gazer of Stars","Sage",
	  "Elder Sage","Grand Sage","Loremistress",
	  "Elder Loremistress", // 20
	  "Grand Loremistress","Elven Archmage","Hierophant",
	  "Master Hierophant",
	  "Servant"
	})
      }),
      "dwarf": ({
	({
	  "Harmless","Mostly Harmless","Manic Miner","Miner","Senior Miner",
	  "Tunnelfighter","Hammerwielder","Axewielder",
	  "Warrior","Adept of Stone", // 10
	  "Warrior of Stone","Master of Stone","Adept of Rock",
	  "Warrior of Rock","Master of Rock",
	  "Warrior of Heavy Rock","Warrior of Iron",
	  "Master of Iron","Warrior of Steel",
	  "Warrior of Heavy Metal", // 20
	  "Heavy Metal","Trollslayer","Giantslayer","Dragonslayer",
	  "Servant"
	}),
	({
	  "Harmless","Mostly Harmless","Manic Miner","Miner","Senior Miner",
	  "Tunnelfighter","Hammerwielder","Axewielder",
	  "Warrior","Adept of Stone", // 10
	  "Warrior of Stone","Mistress of Stone","Adept of Rock",
	  "Warrior of Rock","Mistress of Rock",
	  "Warrior of Heavy Rock","Warrior of Iron",
	  "Mistress of Iron","Warrior of Steel",
	  "Warrior of Heavy Metal", // 20
	  "Heavy Metal","Trollslayer","Giantslayer","Dragonslayer",
	  "Servant"
	})
      }),
      "half-orc": ({
	({
	  "Harmless","Mostly Harmless",
	  "Picknose","Blooded One","Scarface",
	  "Goretusk","Trollslayer","Eyegouger",
	  "Barbarian","Blood-drinker", // 10
	  "Red-eyed Killer",
	  "Giantslayer","Berserker","Bonecrusher",
	  "Gutripper","Heart-eater","Dragonslayer",
	  "Younger Chieftain","Chieftain",
	  "Younger Warmaster", // 20
	  "Warmaster","Dwarfslayer","Elfslayer","Devourer",
	  "Servant"
	}),
	({
	  "Harmless","Mostly Harmless",
	  "Picknose","Blooded One","Scarface",
	  "Goretusk","Trollslayer","Eyegouger",
	  "Barbarian","Blood-drinker", // 10
	  "Red-eyed Killer",
	  "Giantslayer","Berserker","Bonecrusher",
	  "Gutripper","Heart-eater","Dragonslayer",
	  "Younger Chieftain","Chieftain",
	  "Younger Warmistress", // 20
	  "Warmistress","Dwarfslayer","Elfslayer","Devourer",
	  "Servant"
	})
      }),
      "hobbit": ({
	({
	  "Harmless","Mostly Harmless",
	  "Tiny Tot Hobbit","Tot Hobbit","Little Hobbit",
	  "Young Hobbit","Feeble Hobbit","Bignose","Big-ear",
	  "Fur-foot", // 10
	  "Plumb","Potbelly","Glutton","Gourmet",
	  "Epicure","Breakfast","Lunch","Dinner",
	  "Strong Hobbit","Brave Hobbit", // 20
	  "Hobbit Adventurer","Great Gourmet",
	  "Brillat-Savarin",
	  "Grand Master Chef",
	  "Servant"
	}),
	({
	  "Harmless","Mostly Harmless",
	  "Tiny Tot Hobbit","Tot Hobbit","Little Hobbit",
	  "Young Hobbit","Feeble Hobbit","Bignose","Big-ear",
	  "Fur-foot", // 10
	  "Plumb","Potbelly","Glutton","Gourmet",
	  "Epicure","Breakfast","Lunch","Dinner",
	  "Strong Hobbit","Brave Hobbit", // 20
	  "Hobbit Adventuress","Great Gourmet",
	  "Brillat-Savarin",
	  "Grand Mistress Chef",
	  "Servant"
	})
      }),
      "troll": ({
	({
	  "Harmless","Mostly Harmless","Starving","Dung-eater",
	  "Carrion Crawler","Rat Catcher", "Cow Devourer", "Man Devourer",
	  "Elf Devourer", "Dwarf Devourer", // 10
	  "Orc Devourer","Giant Devourer", "Dragon Devourer", "Big Devourer",
	  "Furious", "Ferocious", "Savage",
	  "Gourmet", "Master Devourer", "Great Devourer", // 20
	  "Monstrous Appetite","Gigantic Appetite",
	  "True Troll","Kill-em-all",
	  "Servant"
	}),
	({
	  "Harmless","Mostly Harmless","Starving","Dung-eater",
	  "Carrion Crawler","Rat Catcher", "Cow Devourer", "Man Devourer",
	  "Elf Devourer", "Dwarf Devourer", // 10
	  "Orc Devourer","Giant Devourer", "Dragon Devourer", "Big Devourer",
	  "Furious", "Ferocious", "Savage",
	  "Gourmet", "Master Devourer", "Great Devourer", // 20
	  "Monstrous Appetite","Gigantic Appetite",
	  "True Troll","Kill-em-all",
	  "Servant"
	})
      }),
      "human": ({
	({
	  "Harmless","Mostly Harmless","Novice","Simple Wanderer",
	  "Lowrank Ranger","Master Ranger","Small Fighter",
	  "Experienced Fighter","Adventurer","Hero", // 10
	  "Superhero","Warrior","Champion","Conjurer",
	  "Apprentice Magician","Magician","Enchanter","Warlock",
	  "Apprentice Sorcerer","Master Sorcerer", // 20
	  "Apprentice Archmage","Archmage","Master Archmage",
	  "Grand Master Archmage","Servant"
	}),
	({
	  "Harmless","Mostly Harmless","Novice","Simple Wanderer",
	  "Lowrank Ranger","Master Ranger","Siren","Charming Siren",
	  "Adventuress","Heroine", // 10
	  "Superheroine","Warrior","Champion","Conjuress",
	  "Apprentice Magicienne","Magicienne","Enchantress","Witch",
	  "Apprentice Sorceress","Master Sorceress", // 20
	  "Apprentice Archmage","Archmage","Master Archmage",
	  "Grand Master Archmage","Servant"
	}),
	({
	  "Harmless","Mostly Harmless",
	  "Small Maggot","Simple Maggot",
	  "Lesser Maggot","Ugly Maggot",
	  "Greater Maggot","Small Worm",
	  "Slimy Worm","Purple Worm", // 10
	  "Slithering Worm","Long Worm",
	  "Tiny Lizard","Stupid Lizard",
	  "Bulbous-eyed Lizard","Huge Lizard",
	  "Dhole","Komodo Dragon",
	  "Fractal Dragon","Jabberwocky", // 20
	  "Great Dragon","Wyrm","Ancient Wyrm","Chthonian",
	  "Servant"
	})
      })
    ]);
}

// This function calculates how much exp player has used on
// advancing stats. -+ Doomdark 04-feb-96 +-
int
query_exp_used(mixed x)
{
    closure f;
    int total, e, i;
    int *factors;

    if (stringp(x)) x = find_player(x);
    f = symbol_function("query_stat", x);
    factors = RaceModifiers[(string) x->query_race()];
    if (!factors) factors = ({ 125, 125, 125, 125, 125 });

    e = funcall(f, ST_BASE_STR);
    i = 0;
    while (--e > 0) i += LevelCosts[e];
    total = i * factors[0] / 100;

    e = funcall(f, ST_BASE_CON);
    i = 0;
    while (--e > 0) i += LevelCosts[e];
    total += i * factors[1] / 100;

    e = funcall(f, ST_BASE_DEX);
    i = 0;
    while (--e > 0) i += LevelCosts[e];
    total += i * factors[2] / 100;

    e = funcall(f, ST_BASE_INT);
    i = 0;
    while (--e > 0) i += LevelCosts[e];
    total += i * factors[3] / 100;

    return total;
}

// And this function returns the maximum stat level a living of this race
// can have:
int
query_max_stat(int stat, string race)
{
    mapping x;
    int i;

    if (!race) race = "human";
    // Standard value for other races
    if (!(x = RaceMaxStats[race])) return 35;
    i = x[stat];
    return i ? i : 35;
}

// This function returns the cost of advancing specified stat to specified
// level by a living of specified race:

// Special return values:
//
// -1: Trying to raise past the racial maximum.
// 0 : Trying to raise past the max. amount allowed for this race & level.

int
query_stat_cost(int stat, int new_level, string race, int plr_level)
{
    int i;
    float cost_level;

    if ((i = query_max_stat(stat, race)) < new_level) return -1;
    // Let's calculate "relative stat level"; 
    cost_level = (50 * to_float(new_level)) / to_float(i);

    // Now, let's check if it's legal to advance the stat on our current
    // player level. *grin*

    if ((to_float(plr_level) + 5) < cost_level)
	return 0;

    // And then we need to use some cruel formula to generate the exp cost:

    // Levels 2 <-> 20: from 150 to 150.000 exp points:
    if (cost_level < 20.0)
	return to_int(cost_level * cost_level * cost_level * 20);
    cost_level -= 10.0;
    // Levels 20 <-> 50: from 150.000 to 9.600.000 exp points:
    if (new_level < 51)
	return to_int(cost_level * cost_level * cost_level * 150);
    // Levels 50+: Graah!
    return (25000000 + (new_level - 51) * 25000000);
}

// (Rounded up on levels 30+ or so. -+ Doomdark +-)
/* Medium lord levels (from 27 to 31):
 * 10 million experience per each level
 * High lord levels (from 31 up):
 * 25 million experience per each level
 * Super high lord levels (from 40 up):
 * 50 million experience per each level
 * Extra super hyper giga levels (from 50 up):
 * 100 Mexp per each level
 */

/* Function name: query_exp_for_super_level
 * Description  : returns the experience needed for a certain level
 * Arguments    : lvl - the level (must be 62+)
 * Returns      : experience, or -1 if argument was illegal
 * Usage        : guilds' get_next_exp(lvl) function calls this to get
 *                the exp limit for level lvl+1 (next level)
 * Note		: Obsolete, use query_exp_for_level and
 *		  query_exp_times_for_level
 */
int
query_exp_for_super_level(int lvl)
{
    if (lvl < 62) return -1;

    return (((lvl - 62) % 21) * 100000000);
}

/* Function name: query_exp_for_level
 * Description  : returns the experience needed for a certain level
 * Arguments    : lvl - the level
 * Returns      : experience, or -1 if argument was illegal
 * Usage        : guilds' get_next_exp(lvl) function calls this to get
 *                the exp limit for level lvl+1 (next level)
 */
// Actually the name should be query_exp_for_next_level() ?
int
query_exp_for_level(int lvl)
{
    int x;

    if (lvl < 0) return -1;

    if (lvl >= No_of_LL)
    {
	if (lvl >= 62)
	    return (((lvl - 62) % 21) * 100000000);

	x = (1800000000 + 100000000 * (lvl - No_of_LL + 1));
	return x;
    }

    return LevelLimits[lvl];
}

/* Function name: query_exp_times_for_level
 * Description  : returns the exp_times needed for a certain level
 * Arguments    : lvl
 * Returns      : exp_times
 */
int
query_exp_times_for_level(int lvl)
{
    if (lvl < 62) return 0;
    if (lvl < 83) return 1;

    return (2 + ((lvl - 83) / 21));
}

/*
 * Function name: check_level
 * Description  : return correct level for exp (and exp_times)
 */
varargs int
check_level(int exp, int exp_times)
{
    int up, down, i, x;

    // This nerd has a BIG level! / Graah
    if (exp_times > 0)
    {
	x = (63 + ((exp_times - 1) * 21));
	x += (exp / 100000000);
	return x;
    }

    // Checking levels over 60 kludge / Graah
    if (exp >= 1800000000)
    {
	if (exp < 1900000000) return 60;
	else if (exp < 2000000000) return 61;
	return 63;
    }

    // So. Let's use 60 for upper bound. :-p
    // Besides; instead of that stupid linear search, let's use binary one:

    up = 60;
    down = 0;
    do {
	i = (up + down) / 2;
	if (exp == (x = query_exp_for_level(i)))
	    return i;
	if (exp > x) {
	    down = i;
	} else {
	    up = i;
	}
    } while ((up - down) > 1);

    if (exp < x) return i;

    return i + 1;
}

/*
 * Function name: query_title_for_level
 * Description  : get the default title for a level
 * Arguments    : lvl    - the level in question
 *                gender - gender of title (optional; default is neuter)
 *		  race   - optional race
 * Returns      : string containing the full title
 */
varargs string
query_title_for_level(int lvl, int gender, string race)
{
    int ts, g, nt;
    mixed *x;

    if (!(x = tits[race]))
	x = tits["human"];
    ts = sizeof(x);

    // Choose titles to use
    switch (gender) {
    case 1: g = 0; break;
    case 2: g = 1; break;
    default:
	if (ts > 2) g = 2; else g = 0;
	break;
    }

    nt = sizeof(x[g]);

    if (lvl > (nt - 1)) lvl = (nt - 1);
    lvl--;

    if (lvl < 0) lvl = 0;

    return "the " + x[g][lvl];
}

varargs nomask
string query_pretitle(mixed ob, int l_level, int gender)
{
    int l;

    if (intp(ob)) {
	l = ob;
    } else {
	if (ob->query_is_testplayer()) return "Testplayer";
	l = (int)ob->query_coder_level();
	if (l > 0 && l < LVL_NOVICE) return "Apprentice Wizard";
    }

    if (l)
    {
	if (l >= LVL_ADMIN)
	    return "Admin";
	if (l >= LVL_COADMIN)
	    return "Co-Admin";
	if (l >= LVL_ARCH)
	    return "Arch Wizard";
	if (l >= LVL_ELDER)
	    return "Elder Wizard";
	if (l >= LVL_SENIOR)
	    return "Senior Wizard";
	if (l >= LVL_CODER)
	    return "Wizard";
	if (l >= LVL_NOVICE)
	    return "Novice Wizard";
	if (l >= LVL_APPRENTICE)
	    return "Apprentice Wizard";
	else
	    return "Testplayer";
    }

    // Female titles have typos - "-ess" doesn't make anything
    // feminine...someone smarter should check them. //Graah

    if (objectp(ob)) {
	gender = (int) ob->query_gender();
	if ((l_level = (int) ob->query_level() - 25) < 1) l_level = 0;
    }

    if (l_level >= sizeof(lords))
	return "<Too Big>";

    if (gender == G_FEMALE)
	return ladies[l_level];

    return lords[l_level];
}

mapping
query_race_max_stats()
{
    return RaceMaxStats;
}

// 27-Sep-96 / Graah: Return list of available races.
string *
query_races()
{
    return m_indices(RaceMaxStats);
}

void
table()
{
    int i, et;

    write(sprintf("%3s. %27s %27s %13s\n",
	"Lvl", "Male Title", "Female Title", "Experience"));

    et = 0;

    write("\
---------------------------------------------------------------------------\
\n");

    for (i = 1; i <= 121; i++)
	write(sprintf("%3d. %27s %27s %13d (%d)\n",
	    i,
	    (i > 24 ? query_pretitle(0, i - 24, 1) : query_title_for_level(i, 1)),
	    (i > 24 ? query_pretitle(0, i - 24, 2) : query_title_for_level(i, 2)),
	    query_exp_for_level(i - 1),
	    query_exp_times_for_level(i - 1)));
}


// Player object needs this list for some weird reason.
string *query_lord_titles()
{
    return (lords[1..<0] + ladies[1..<0]);
}

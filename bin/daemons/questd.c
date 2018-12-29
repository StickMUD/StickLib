/****************************************************************
*								*
* Module:							*
*	/bin/daemons/questd.c, QUEST_D in <daemons.h>		*
* Description:							*
*	The daemon that handles the things related to solving	*
*	quests, listing quests available for players and so on.	*
*	Note that many quests are NOT included in StickLib	*
*	because it would be too easy to find out the solution	*
*	for them by downloading StickLib sources... *sigh*	*
* Version:							*
*	1.0a (for StickLib), Aug-98				*
* Last update:							*
*	31-Aug-98 by Doomdark					*
*								*
* Modifications:						*
*								*
* 31-Aug-98, Doomdark: Depending on which areas are included	*
*	in the StickLib, certain quests are enabled/disabled.	*
*	<areas.h> defines which areas are actually included.	*
*                                                               *
* 15-Sep-98, Kieve: Moved in the list from /lib/guild.c.        *
*      Now, all the quest listing is done from this daemon.     *
****************************************************************/
/*
 * /bin/daemons/questd.c
 *
 * Handle quests and quest points etc.
 *
 * Copyright (c) 1993 The Stickmud Project,
 * All rights reserved.
 *
 * Re-coded Mar-96 by Germ
 * 29-Apr-96 - Added QUEST_TERAPTUS
 * 12-Jun-96 - Added QUEST_DROW_CITY   //Graah
 * 12-Jun-96 - Added QUEST_DE_PALACE   //Graah
 *  8-Jul-96 - Added QUEST_SILVER_RING //Graah
 * 12-Jul-97 - Added QUEST_KALEVA_ISLAND (very minor) //Graah
 *	       and QUEST_FOR_IMMORTALITY (not solvable) //Graah
 * 13-Jul-97 / Graah: Added short descs for quests.
 *  3-Sep-97 / Graah: Eat at Joe's!
 * 17-Apr-98 / Hawthorn: Maze of Fame
 * 21-Apr-98 / Graah: Barney (simple go and kill quest, 1 QP.)
 * 17-Nov-98 / Kieve: Added hill giant quest - Coded by Scout
* 20-Feb-99 / Tamarindo: Miles Magic Mansion by Ooze
 */

#include <quests.h>
#include <city_name.h>
#include <areas.h>

#define DEBUG(xx)   if(objectp(find_player("kieve"))) \
                        find_player("kieve")->tell_me(xx)

#define QUEST_POINTS	0
#define QUEST_EXP	1
#define QUEST_SHORT	2
#define QUEST_HINT	3
#define MAX_QPS 	781	// Maximum QP's possible
#define MAX_REQ_QPS 	750	// Required QPs for High Levels

static mapping quests;

nomask void
create()
{
//
// Format:  quest_id, quest_points, exp_given, quest_short, quest_hint
//
//*********************************************************************
//                      NON-LORD QUESTS
//*********************************************************************
//
  quests =
    ([
	QUEST_M2	:  2; 50;
	"Beer to Bum";
	"Buy a beer and bring it to the Bum under the bridge west of town.",

#ifdef AREA_KALEVALA
        QUEST_KALEVA_ISLAND: 2; 150;
	"The Island of Desolation";
        "Find the most desolate island on Loob. It is somewhere on \
the Blue sea of Ahti. There is no return from the island...",
#endif

	QUEST_M3	:  3; 200;
	"The Maze Under Well";
	"Solve the maze in the well (under center of village)",

#ifdef AREA_FROBOZZ_INN
	QUEST_M4	:  4; 1000;
	"Black Knight Does Not Say 'Ni!'";
"Go into the forest that is behind (actually, inside!) the Frobozz'\
 Inn for Monsters (which is located northeast from " CITY_NAME
") and cut down the mightiest tree in the forest ... with a herring.\
 (Hint: find the Lumberjack first.)", 
#endif

#ifdef AREA_SEWERS
	QUEST_M5	:  5; 1000;
	"This Quest Stinks";
	"Explore the sewers under the city and find the cavern.",
#endif

#ifdef	AREA_ISLANDS
	QUEST_M6	:  6; 1500;
	"The Adventurer's Contest";
	"On the main island (in townhall) is held a 'Adventurer's contest'.\
 You must pass the 'local' level.",
#endif

#ifdef AREA_TRISTEZA
	QUEST_EAT_AT_JOES: 6; 250;
	"Eat at Joe's";
	"Eat at Joe's!",
#endif

#ifdef AREA_FROBOZZ_INN
	QUEST_M15	: 15; 5000;
	"Quest for the Holy Grail";
	"Find the Holy Grail.",
#endif
	QUEST_HILL_GIANT  :  7; 10000;
	"Ambushed Nobels";
	"Help the Nobels that were ambushed by the hill giants.",

        QUEST_MAGIC_MANSION: 7; 10000;
        "Magic Toy";
        "Find Miles' favorite toy hidden in a magic box.",

#ifdef AREA_SUE
	QUEST_M18	: 17; 10000;
	"My Breath is Like Fire? So, SUE me!";
"Buy a firebreather and bring it to the Adventurer in the fourth level\
 of the Small Underground Empire (which is somewhere to the northeast from "
CITY_NAME "; tough place, don't go alone, unless you know what you are doing).\
 Recommended for a party of TOUGH non-lords.",
#endif

#ifdef AREA_EAST_FOREST
	QUEST_M19	: 18; 10000;
	"The Treasure of the East Forest";
"Find the treasure in the East Forest. This is a dangerous quest,\
 recommended for a party of TOUGH non-lords.",
#endif

#ifdef AREA_KALEVALA
	QUEST_M20	: 19; 10000;
	"The Conquest of Pohjola";
"Conquest the evil land of Pohjola, and slay Louhi, the mistress of Pohjola.",
#endif

	QUEST_DROW_CITY : 10; 1000;
	"The Hidden Llorozorokarak";
"Find the legendary Dark Elven City of Llorozorokarak. No one knows \
where it is, but it is certainly hard to find!",

	QUEST_BARNEY    :  1;  200;
	"The Putrid Purple Pederast";
"B'Harni, a Great Old One, has entered the world in his foul form \
of a purple dinosaur. You must destroy that horrible abomination. \
B'Harni is probably hiding among other dinosaurs...",

#ifdef AREA_ZOO
	QUEST_ZOO	: 10; 1000;
	"Son of Amos the Bold";
"Respected citizen of " CITY_NAME ", Amos the Bold has lost his son. He needs\
 a brave adventurer to fulfill a task for him.  Find Amos at his home on\
 Dragon Street and ask him about his son.",
#endif

#ifdef AREA_HOBBIT_VILLAGE
	QUEST_ZILLGODS	: 25; 50000;
	"Zillgod is very, very angry!";
"The mighty Zillgod is furious, and has to be appeased.\
 Bilbo, of hobbit village,  might help you on your quest.\
 A tough quest for VERY tough players (level 18 and over).",
#endif

#ifdef AREA_KANTELES
	QUEST_KITIARA	: 14; 5000;
	"The Kitiara's Mansion";
"Destroy evil witch, Kitiara. She lives in Mansion of Kantele,\
 south from " CITY_NAME ". Beware, she is really nasty.\
 A quest for a group of tough adventurers.",
#endif

#ifdef AREA_KALEVALA
	QUEST_SAMPO	:  7; 10000;
	"The Sampo Quest";
"Bring a Sampo to the Mistress of Pohjola and you'll\
 be rewarded. Start by asking Vainamoinen of Kalevala about\
 the Sampo.",
#endif

	QUEST_MAZE_OF_FAME: 15; 10000;
	"The Maze of Fame";
"Find and solve the Maze of Fame, rumoured to have been crafted \
in ancient times by dwarven magicians. Bring your weaponry, for \
you might need it; but if you bring your wits, maybe you won't need \
your weaponry after all...",

	QUEST_ORC_SLAYER:  5; 1000;
	"The Orc Slayer";
"Retrieve the Orc Slayer sword from evil orc shaman in orc valley,\
 and give it to Leo the archwizard in the church tower.",

#ifdef AREA_EAST_FOREST
	QUEST_DE_PALACE:   20; 2000;
	"The Spirit of the East Forest";
"The Dark Elves have captured the Spirit of the East Forest and \
taken her somewhere into the Underdark. Find her or the East \
Forest will die! Suitable for one tough or a party of medium \
level adventurers.",
#endif

	QUEST_SILVER_RING: 20; 4000;
	"The Silver Ring";
"Find the Silver Ring. Seek it in the dark. No more hints.",

#ifdef AREA_SEAWORLD
	QUEST_PIRATE	:  25; 1500;
	"The Pirate Quest";
"It is rumored that the Dread Pirate Robinson is in need of\
 brave sailors. He was last seen on the Pirate Island.",
#endif

//
//*********************************************************************
//			LORD QUESTS
//*********************************************************************
//
	QUEST_OGRE_SLAYER	: 100; 20000;
	"The Ogre Slayer";
	"Bring the Ogre Slayer to Leo the archwizard.",

#ifdef AREA_TWS
	QUEST_GARGAMEL		:  70; 10000;
	"Gargamel";
"Help the smurfs to get rid of the evil wizard Gargamel.\
 Abernathy knows more about the quest, ask from him.\
 Note, that this is a really tough quest, suitable\
 for big lords or a party of smaller ones.",
#endif

#ifdef AREA_OGRE_VALLEY
	QUEST_MARGARET		:  50; 10000;
	"Margaret's Holy Quest";
"Help Margaret complete her holy quest.  She was last seen entering\
 the mines north of the clearing.",
#endif

	QUEST_GOLEM		:  40; 5000;
	"The Great Golem";
"An ultimate monster is being constructed in the Abyss. The Great\
 Golem will be controlled by the mightiest of Demon Lords, and their\
 purpose is - of course - to conquer the whole MUD. It is predicted\
 that the terrible construct could be slain only with Death's Scythe.\
 Ask about Death from the priestess of Skert.",

#ifdef AREA_TRON
	QUEST_CLOAK		:  60; 10000;
	"The Tomb of Orchinie";
"Find the tomb of Orchinie. It is rumoured to be behind a huge gate\
 in caves full of orcs.",
#endif

	QUEST_GRENADE		:  70; 10000;
	"The Holy Handgrenade of Antioch";
"Help the holy Knight in his quest. He can be found near the northern\
 highway.",

#ifdef AREA_MONSTER_VILLAGE
	QUEST_COIN		:  60; 10000;		// Coded by Germ
	"The UFO Quest";
"Past the dark forest to the south lies a small village.\
 There, it is rumored, a strange flying object can sometimes\
 be seen over the forest. Investigate these rumors, if you dare.",
#endif

#ifdef AREA_COLLEGE
	QUEST_TERAPTUS		:  75; 1000000;		// Coded by Necros
	"The Sunken Temple";
"South of " CITY_NAME ", past the Troll village and lair of Strabo,\
 past the deserts of Sloth, lies the Swamps of Gluttony. Somewhere\
 admist that place is rumored to be a long forgotten temple, half sunken\
 into the muck that welcomes weary travellers to feast there. Feast with\
 the high priest there and honor the gluttons.",
#endif

	QUEST_FOR_IMMORTALITY   : 100; 2500000;  // This Quest is nowhere.
	"The Quest of Immortality";
"This Quest is impossible to solve. No mortal has ever even \
found out what to do, and the gods don't tell. There can be no \
hints for this Quest. It will take the rest of your life to \
complete."
]);
}


nomask string
query_hint(string questname)
{
  if ( member(quests,questname))
	return quests[questname,QUEST_HINT];
  return 0;
}


nomask string
query_short(string questname)
{
  if ( member(quests,questname))
    return quests[questname,QUEST_SHORT];
  return 0;
}

 
nomask int
query_qp(string questname)
{
  if ( member(quests,questname))
    return quests[questname,QUEST_POINTS];
  return 0;
}

//
// This func is called by set_quest() in /obj/player.c
//
nomask int
recalc_qp(object ob)
{
  int i, q;
  string qstring;
  string *tmp;

  if (!ob) return 0;

  q = 0;
  qstring = (string) ob->query_quests();
  if (!qstring) return 0;
  tmp = explode(qstring, "#");

  while (i < sizeof(tmp)) {
    q += quests[tmp[i],QUEST_POINTS];
    i++;
  }

  return q;
}


int recaq(string name) { return recalc_qp(find_player(name)); }

//
// When player completes a quest, the room/npc code should
// call this func rather than calling player->set_quest.
//
nomask status
add_quest(object ob, string questname)
{
  if ( !ob || !questname ) return 0;
  if ( ob->query_quests(questname)) return 0;
  if ( member(quests,questname)==-1) return 0;
  ob->set_quest(questname);
  ob->add_exp(quests[questname,QUEST_EXP]);
  return 1;
}

//
// This func is called by adv_guild and lords_guild, and defines
// how many quest points are required to advance to the next lvl.
// When adding a quest, it probably will be necessary to re-adjust
// this spread of numbers.
//
nomask int
qp_for_level(int level)
{
  // For now, we don't need any quest points.  --Kieve Nov 21, 1998
  return 0;
  if (level < 3) return 0;
  if (level > 49) return MAX_REQ_QPS;

  return
    ({
				// lvls 1-19 (total qp = 165 )
	2, 5, 8,
	11, 15, 20, 24, 28,
       30, 32, 34, 36, 40,
       50, 65, 92, 147,
//
//      QP	  level   suggested quest
//
	187,	// lvl 20, golem_quest(40)
	207,	// lvl 21, all non-lord quests must be done by now
	207,	// lvl 22, 
	207,	// lvl 23
	207,	// lvl 24
	267,	// lvl 25, margaret(60)
	267,	// lvl 26
	327,	// lvl 27, coin_quest(60)
	362,	// lvl 28, cloak_qust(60)
	387,	// lvl 29
	422,	// lvl 30, gargamel(70)
	457,	// lvl 31
	462,	// lvl 32, grenade(70)
	482,	// lvl 33
	502,	// lvl 34
	512,	// lvl 35
	527,	// lvl 36
	552,	// lvl 37
	577,	// lvl 38
	602,	// lvl 39, orgre_slayer(100)
	627, 627, 627, 627, 627,	// Lvls 40-44
	627, 627, 627, 627, 627,	// Lvls 45-49
     })[level-3];
}


nomask void
qplist()
{
  int i;

  for (i = 3; i < 55; i++)
    printf("Level %2d: %5d\n", i, qp_for_level(i));
}

int num_quests()
{
    return sizeof(m_indices(quests));
}

int filter_func(string str1, object tp)
{
    // return 0 to not include the str1
    if(tp->query_quests(str1))
        return 0;
    return 1;
}
 

int sort_func(string str1, string str2)
{
    // if str1 > str2, return 1
    if(query_qp(str1) > query_qp(str2))
        return 1;
    return 0;
}

// Hmm maybe this should also be done in the quest daemon - Germ
// *nod* Moved it in ;) --Kieve
status list_quests()
{
    int i, qn;
    string *qlist, tmp, *m;
 
    qlist = m_indices(quests);
    for (i=0 ; i < sizeof(qlist) ; i++)
        if ( !this_player()->query_quests(qlist[i]))
            qn++;
 
    if (!qn)
        m = ({ "You have solved all of the Quests!" });
    else
        m = ({ "You have "+qn+" Quests unsolved." });

    qlist = sort_array(qlist, #'sort_func);
    
 
    for (i=qn=0 ; i < sizeof(qlist) ; i++)
        if (!this_player()->query_quests(qlist[i]))
        {
            qn++;
            tmp = query_short(qlist[i]);
            if (strlen(tmp) > 70)
                tmp = tmp[0..69] + "...";
            m += ({ sprintf("%2d. %s", qn, tmp) });
        }
 
    m += ({ "Type quests <num> to see the whole hint text." });
 
    this_player()->more(m);
    return 1;
}
 
status list_single_quest(int qnum)
{
    string *qlist;
 
    qlist = m_indices(quests);
    if(qnum > sizeof(qlist))
        return 0;
    qlist = filter_array(qlist, #'filter_func, this_player());
    qlist = sort_array(qlist, #'sort_func);
    this_player()->tell_me(sprintf("%s\n%s",
        query_short(qlist[qnum-1]), query_hint(qlist[qnum-1])));
 
    return 1;
}

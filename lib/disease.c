/************************************************************************
*									*
* disease.c								*
*									*
* Last Modifications 04-oct-95 by Doomdark				*
*									*
* A selection of acute diseases caused by a virus or a bacterium.	*
*									*
* measles, chickenpox, tonsillitis, scarlet fever, diphtheria,		*
* meningitis, tetanus, flu, typhoid fever, dysentery, spotted fever,	*
* smallpox, cholera, plague, rabies, hallucinations, ergotism		*
*									*
* Includes medieval names i.e. mostly "Plague" or "Leprosy". (query_nickname)
* Only high magic can reveal the real name of the disease.		*
*									*
* Basic usage:								*
*									*
* #include <disease.h>							*
*									*
* ob = clone_object(DISEASE_FILE);					*
* ob->set_disease("tetanus");    (or any of the above)			*
* move_object(ob, poor_victim);						*
*									*
* Diseases should be very very rare!					*
*									*
* NEW:									*
*									*
* - Diseases drain abilities sometimes!					*
* - Now sets C_SICK permanently... Will be removed by 'remove()'.	*
* - Uses new stat-setting functions; set_stat, query_stat, add_stat.	*
*									*
************************************************************************/

#include <conditions.h>
#include <disease.h>
#include <stats.h>

#define NAME ((string)victim->query_name())
#define HE ((string)victim->Pronoun())
#define HIS ((string)victim->Possessive())
#define HIM ((string)victim->Objective())
#define NOT_INVIS (!victim->query_invis())
#define NOT_LD (query_ip_number(victim))
#define TV(x) victim -> tell_me(x)
#define EM(x) xsay(NAME + " " + (x))

// Prototypes
void set_disease_vars(string n);
void set_disease(string n);
void symptom(int symtype);
string random_being();
void xsay(string msg);

string name;				/* Real name of disease */
string nickname;			/* Medieval (false) name */
int incubation, symptom, duration;	/* Counters for saving */
int level;				/* How serious is it? (0..10000) */
int *syms;				/* Symptom numbers */
int infect;				/* Percent change to infect others */
string rash;				/* Rash description */

static object victim;
static int numsym;

static int Flags;			// Some info about this disease.

#define	F_HALLUCINATING		1
#define	F_STUNNED		2
#define	F_UNCONSCIOUS		4
#define	F_SICK			8

void
remove()
{
// Have to modify this a bit. -+ Doomdark +-
int dur;
// Release conditions. We don't know if they were actually set by
// a disease, but who cares...
//
// Actually, we'll try to check that. -+ Doomdark +-

    if (victim) {
        if ((Flags & F_UNCONSCIOUS) && (dur = (int)
	victim->query_condition(C_UNCONSCIOUS) > 1)) {
		victim->set_condition(C_UNCONSCIOUS, 0);
	}
	if ((Flags & F_STUNNED) && (dur = (int)
	victim->query_condition(C_STUNNED) > 1)) {
		victim->set_condition(C_STUNNED, 0);
	}
        if (Flags & F_HALLUCINATING) {
// We only use permanent, object based set_conditions with this one...
	    victim->set_condition(C_HALLUCINATING, 0, this_object());
	}
        if (Flags & F_SICK) {
// We only use permanent, object based set_conditions with this one...
	    victim->set_condition(C_SICK, 0, this_object());
	}
    }

	remove_call_out("incubation_alarm");
	remove_call_out("duration_alarm");
	remove_call_out("symptom_alarm");
	remove_call_out("infect_alarm");

	destruct(this_object());
}

void
init_calls()
{
    if (incubation > 0) call_out("incubation_alarm", incubation);
    else {
	if (duration > 0) {
	    call_out("duration_alarm", duration);
	    if (symptom > 0) call_out("symptom_alarm", symptom);
	    else call_out("symptom_alarm", 10); /* Grrr... */
	} else call_out("duration_alarm", 1);
    }

    call_out("infect_alarm", 13 + random(13));
}

void
incubation_alarm()
{
    incubation = 0;

    if (duration > 0)
        call_out("duration_alarm", duration);
    else call_out("duration_alarm", 1);

    symptom = 10 + random(10);

    call_out("symptom_alarm", symptom);
}

string
query_auto_load()
{
    string tmp;

    if ((incubation = remove_call_out("incubation_alarm")) < 1)
       duration = remove_call_out("duration_alarm");
    symptom = remove_call_out("symptom_alarm");
    remove_call_out("infect_alarm");

    tmp = DISEASE_OBJ + ":" +
	incubation + "#" + symptom + "#" + duration + "#" + name;

/*
    if (victim->query_coder_level())
	victim -> tell_me("Disease info: " + tmp);
*/

    init_calls();

    return tmp;
}

void
init_arg(string arg)
{
    /* Kludge... */

    sscanf(arg,"%d#%d#%d#%s", incubation, symptom, duration, name);
    set_disease_vars(name);
    sscanf(arg,"%d#%d#%d#%s", incubation, symptom, duration, name);

    victim = environment();

    init_calls();
}

/* Check if ob is immune */
int
immune(object ob)
{
  string r;

  if (!ob) return 0;

  r = (string)ob->query_race();

  /* Elves are totally immune,
     Trolls 90% immune,
     Half-orcs 30% immune. */
  if ((r != "elf" && r != "dark elf") &&
      (r != "troll" || random(100) > 90) &&
      (r != "half-orc" || random(100) > 30)) return 0;

  /* These diseases are special cases. */
  if (member_array(name,
      ({ "ergotism", "hallucinations", "tetanus", "rabies" })) == -1)
    return 1;

  return 0;
}

void
init()
{
  if (!this_player()) return;
  if (!living(this_player())) return;
  if (environment() != this_player()) return;

  if (!victim) {
    victim = this_player();

    if (immune(victim)) {
      destruct(this_object());
      return;
    }
  }

  /* No sick Guest */
  /* No sick newbies (level 3 or less) */
  /* No plague epidemic when link dead */
  /* NPCs can still have a disease */

  if ((string)victim->query_real_name() == "guest" ||
      (int)victim->query_level() < 4) {
    call_out("remove", 1);
    return;
  }
}

varargs status
id(string str, object who)
{
	return str == name || str == "disease" ||
	  (nickname && str == lower_case(nickname)) ||
	  (name == "ergotism" && (str == "fire" || str == "holy fire"));
}

int drop() { return 1; }
int get() { return 1; }
varargs string query_short(int mode, object who) { return 0; }
varargs string query_long(string arg, object who) { return 0; }
int query_value() { return 0; }
int query_weight() { return 0; }

string
query_info()
{
	return "It's a disease: " + name + "!\n";
}

int query_level() { return level; }
string query_name() { return name; }
int *query_syms() { return syms; }
int query_infect() { return infect; }
int query_incubation() { return incubation; }
string query_rash() { return rash; }
object query_victim() { return victim; }
string query_nickname() { return nickname; }
void set_nickname(string c) { nickname = c; }

void
set_disease_vars(string n)
{
    name = n;

    /* Incubation and duration are given as hours here; convert them later. */
    /* Infection change is given so that the worst disease has 100. */

    switch(name) {
	case "measles": /* Tuhkarokko */
	    nickname = "Leprosy";
	    level = 100 + random(4000);
	    syms = ({ SYM_COUGH, SYM_FEVER, SYM_RHINITIS, SYM_RASH });
	    rash = "skin is covered by bright red rash";
	    infect = 26;
	    incubation = 240 + random(96); duration = 20 + random(62);
	    break;
	case "chickenpox": /* Vesirokko */
	    nickname = "Leprosy";
	    level = 50 + random(2000);
	    syms = ({ SYM_FEVER, SYM_HEADACHE, SYM_RASH });
	    rash = "skin is covered by red blisters";
	    infect = 33;
	    incubation = 168 + random(520); duration = 20 + random(96);
	    break;
	case "tonsillitis": /* Angina */
	    nickname = "Plague";
	    level = 100 + random(1000);
	    syms = ({ SYM_FEVER, SYM_SHIVER, SYM_HEADACHE, SYM_SORE_THROAT });
	    rash = ""; infect = 14;
	    incubation = 22 + random(56); duration = 48 + random(144);
	    break;
	case "scarlet fever": /* Tulirokko */
	    nickname = "Leprosy";
	    level = 200 + random(4000);
	    syms = ({ SYM_FEVER, SYM_HEADACHE, SYM_VOMIT, SYM_RASH,
		SYM_SORE_THROAT });
	    rash = "skin is covered by scarlet red rash";
	    infect = 24;
	    incubation = 20 + random(60); duration = 96 + random(160);
	    break;
	case "diphtheria": /* Kurkkum{t{ */
	    nickname = "Throat Rot";
	    level = 3000 + random(5000);
	    syms = ({ SYM_SORE_THROAT, SYM_HEADACHE, SYM_FEVER,
		SYM_SUFFOCATE });
	    rash = ""; infect = 35;
	    incubation = 48 + random(96); duration = 144 + random(340);
	    break;
	case "meningitis": /* Tarttuva aivokalvontulehdus */
	    nickname = "Brain Rot";
	    level = 4000 + random(6000);
	    syms = ({ SYM_FEVER, SYM_SHIVER, SYM_HEADACHE, SYM_VOMIT,
		SYM_DELIRIOUS });
	    rash = ""; infect = 12;
	    incubation = 20 + random(50); duration = 144 + random(280);
	    break;
	case "tetanus": /* J{ykk{kouristus */
	    nickname = "Cramp Plague";
	    level = 6000 + random(4000);
	    syms = ({ SYM_SPASMS, SYM_LIGHTSENSITIVE, SYM_MUSCLE_PAIN,
		SYM_HALLUCINATE });
	    rash = ""; infect = 0; /* Not like others (fortunately!) */
	    incubation = 96 + random(480); duration = 190 + random(400);
	    break;
	case "flu": /* Flunssa */
	    nickname = "Fever";
	    level = 30 + random(700);
	    syms = ({ SYM_FEVER, SYM_HEADACHE, SYM_COUGH, SYM_RHINITIS });
	    rash = ""; infect = 55;
	    incubation = 40 + random(96); duration = 24 + random(96);
	    break;
	case "typhoid fever": /* Lavantauti */
	    nickname = "Cholera";
	    level = 4500 + random(4000);
	    syms = ({ SYM_FEVER, SYM_HEADACHE, SYM_DIARRHEA, SYM_RASH });
	    rash = "skin has small pink blots of rash here and there";
	    infect = 20;
	    incubation = 288 + random(48); duration = 500 + random(180);
	    break;
	case "dysentery": /* Punatauti */
	    nickname = "Bowel Rot";
	    level = 3000 + random(4000);
	    syms = ({ SYM_DIARRHEA, SYM_STOMACH_PAIN, SYM_THIRST });
	    rash = ""; infect = 30;
	    incubation = 24 + random(48); duration = 120 + random(300);
	    break;
	case "spotted fever": /* Pilkkukuume */
	    nickname = "Leprosy";
	    level = 5000 + random(4000);
	    syms = ({ SYM_FEVER, SYM_HEADACHE, SYM_MUSCLE_PAIN, SYM_RASH });
	    rash = "skin is covered by the reddish brown boils";
	    infect = 23;
	    incubation = 264 + random(200); duration = 144 + random(350);
	    break;
	case "smallpox": /* Isorokko */
	    nickname = "Boil Rot";
	    level = 7000 + random(3000);
	    syms = ({ SYM_FEVER, SYM_SHIVER, SYM_HEADACHE, SYM_MUSCLE_PAIN,
		SYM_RASH, SYM_DELIRIOUS });
	    rash = "skin is covered by horrible, suppurating blisters";
	    infect = 65;
	    incubation = 200 + random(80); duration = 216 + random(200);
	    break;
	case "cholera":
	    nickname = "Cholera";
	    level = 6000 + random(4000);
	    syms = ({ SYM_DIARRHEA, SYM_FEVER, SYM_STOMACH_PAIN,
		SYM_MUSCLE_PAIN, SYM_THIRST, SYM_SPASMS, SYM_HALLUCINATE });
	    rash = ""; infect = 50;
	    incubation = 24 + random(48); duration = 120 + random(300);
	    break;
	case "plague": /* Rutto */
	    nickname = "Plague";
	    level = 9000 + random(1000);
	    syms = ({ SYM_FEVER, SYM_COUGH, SYM_BOILS, SYM_DELIRIOUS,
		SYM_HALLUCINATE, SYM_SUFFOCATE, SYM_SHIVER, SYM_VOMIT,
		SYM_MUSCLE_PAIN, SYM_RASH });
	    rash =
	"skin is covered by terrible, rotting, huge suppurating boils";
	    infect = 70;
	    incubation = 100 + random(30); duration = 48 + random(248);
	    break;
	case "rabies": /* Vesikauhu */
	    nickname = "Rabies";
	    level = 10000;
	    syms = ({ SYM_RABIES, SYM_SPASMS, SYM_THIRST, SYM_HALLUCINATE,
		SYM_PARALYSIS });
	    rash = ""; infect = 0; /* Can be only got from a bite */
	    incubation = 240 + random(960); duration = 1000;
	    break;
	case "hallucinations":
	    nickname = "Madness";
	    level = 1000;
	    syms = ({ SYM_HALLUCINATE });
	    rash = ""; infect = 0;
	    incubation = 10; duration = 12 + random(48);
	    break;

	/* Poisonous fungi (Claviceps purpurea) in rye caused the
	   'Fire of Holy Antonius'. */
	/* In finnish: Torajyv myrkytys (Pyh n Antoniuksen Tuli) */
	case "ergotism":
	    nickname = "Fire of Holy Antonius";
	    level = 6000;
	    syms = ({ SYM_STOMACH_PAIN, SYM_SPASMS, SYM_MUSCLE_PAIN,
		SYM_PARALYSIS });
	    rash = ""; infect = 0;
	    incubation = 24 + random(48); duration = 120 + random(300);
	    break;

    case "test":
	    nickname = "Plague";
	    level = 9000 + random(1000);
	    syms = ({ SYM_FEVER, SYM_COUGH, SYM_BOILS, SYM_DELIRIOUS,
		SYM_HALLUCINATE, SYM_SUFFOCATE, SYM_SHIVER, SYM_VOMIT,
		SYM_MUSCLE_PAIN, SYM_RASH });
	    rash =
	"skin is covered by terrible, rotting, huge suppurating boils";
	    infect = 0;
	    incubation = 1; duration = 48 + random(248);
	    break;

/* keep this form empty
	case "":
	    level = 000 + random(000);
	    syms = ({});
	    rash = ""; infect = ;
	    incubation = ; duration = ;
	    break;
*/
	default:
	    nickname = "Fever";
	    level = random(5000);
	    syms = ({ SYM_FEVER, SYM_RHINITIS });
	    infect = 2;
	    incubation = 20 + random(12); duration = 12 + random(24);
	    break;
    }

    numsym = sizeof(syms);

    /* Convert incubation and duration to game time */
    /* Should really be * 300, but that's too realistic... */
    incubation = incubation * 5;
    duration = duration * 5; /* Was * 7, but let's be nice. */
}

void
set_disease(string n)
{
    if (!n)
	set_disease_vars(({"measles", "chickenpox", "tonsillitis",
		"scarlet fever", "diphtheria", "meningitis",
		"tetanus", "flu", "typhoid fever", "dysentery",
		"spotted fever", "smallpox", "cholera", "plague",
		"rabies", "hallucinations" })[random(16)]);
    else set_disease_vars(n);

    init_calls();
}

string
extra_look() {
    if (incubation > 0 || rash == "") return 0;

    return NAME + "'s " + rash;
}

void
duration_alarm()
{
int x;
string tmp;

    tmp = nickname;
    if (!tmp) tmp = name;
    if (!tmp) tmp = "disease";

    TV("You have recovered from " + tmp + "!");
    TV("You feel more experienced.");

    if(!victim -> is_npc())
	victim->add_exp(level / 10);

    remove();
    return;
}

void
symptom_alarm()
{
    int x, y;

// If we are dead, then self-destruct

    if (!victim || victim->query_ghost()) {
	remove();
	return;
    }

// Let's set this sick condition. Prevents normal heal.

    if (!(Flags & F_SICK)) {
	victim->set_condition(C_SICK, -1, this_object());
	Flags |= F_SICK;
    }

    // New: IT CAN KILL!
    if (NOT_LD)
      {
	symptom(syms[random(numsym)]);

	x = (int)victim->query_hp();

	if (x < 10)
	  {
	    x = 1 + random(4); y = 0;
	    y = (int)victim->query_random_luck();
	    if (y >= 0) y = 0; else y = -(y / 5);
	  }
	else
	  {
	    x = x / 10;
	    y = (int)victim->query_random_luck();
	    if (y >= 0) y = 0; else y = -(y / 3);
	  }

	victim->add_hp(-(x + y), 1);
      }

    call_out("symptom_alarm", 8 + random(8) + random(6));

    return;
}

void
infect_alarm()
{
object env, *inv, ob;
int i;

    if (!victim) return;
    if (!(env = environment(victim))) return;

    if (NOT_LD) {
	inv = all_inventory(env);

	for(i = sizeof(inv) - 1; i >= 0; i--) {

// Infect only living beings of level 4 or more. Only players may be
// infected, but not Guest. One player won't have many diseases at the
// same time, this would be too cruel (really? -+ Doomdark +-).

	    if (interactive(inv[i]) &&
// Was: living(inv[i]) && inv[i]->query_npc() &&
		random(140) <= (infect / 10) &&
		!immune(inv[i]) &&
		!present("disease",inv[i]) &&
		!inv[i]->query_coder_level() &&
		(string)inv[i]->query_real_name() != "guest" &&
		(int)inv[i]->query_level() > 3) {

	      ob = clone_object(DISEASE_OBJ);
	      move_object(ob, inv[i]);
		if (ob)
	      ob->set_disease(name);
	    }
	}

	/* Give a clue about what is going on. */
	if (incubation > 0 && random(100) < 6)
	  victim -> tell_me(
		({
		  "You feel somehow tired and depressed.",
		  "You have a slight headache.",
		  "Your throat itches.",
		  "Your joints ache a bit.",
		  "You feel tired. Perhaps you should rest a while.",
		  "Your nose is irritated.",
		  "You feel a bit dizzy.",
		  "You feel that you are becoming sick."
		})[random(8)]);
    }

    call_out("infect_alarm", 13 + random(13));
}

// Decide a condition duration based on sickness level and victim's stats

int
random_duration()
{
int x;

    x = (level - ((int)victim->query_level() * 125) -
	((int)victim->query_constitution() * 125)) / 1000;

    if (x < 1) x = 1;
    if (x > 10) x = 10;

    return 1 + random(x);
}

string
delirious_talk()
{
    string tmp;
    int i;

    tmp = "";

    for (i = 0; i < 2 + random(4); i++) {
      if (random(100) < 22) tmp += random_being() + " ";
      else
	tmp += ({ "flowers", "heaven", "trees", "lake", "frigate",
		    "wandering", "glowing", "it is", "...", "I don't",
		    "but why", "let's go", "not that", "...", "utmost",
		    "eternal", "warmth", "beautiful", "since when",
		    "running", "people", "flame", "candle", "ghosts",
		    "that smile", "...", "mystic", "...", "they are coming",
		    "can't you see it", "almost", "when", "who am I",
		    "particular", "delight", "fever", "watch-key",
		    "sword", "combat", "death", "firebreather", "sauna",
		    "large bag", "quest", "kill", "you are a thief",
		    "YOU'LL DIE", "what are you doing"
		    })[random(48)] + " ";
    }

	if (tmp[strlen(tmp) - 1] == ' ')
		tmp = tmp[0..<2];

    return tmp;
}

string
random_happening()
{
    string tmp;

    switch(random(2)) {
	case 0:
	    tmp = ({ "hits", "smashes", "crushes", "kicks",
		"kisses", "slashes", "tickles", "licks" })[random(5)] +
		" you";
	    switch(random(4)) {
		case 0: break;
		case 1:
		    tmp += ({ " ", ", but not", " quite", " very",
			" extremely" })[random(4)] + " hard";
		    break;
		case 2:
		    tmp += " with " + delirious_talk();
		    break;
		default:
		    tmp += " with " + random_being();
	    }
	    break;

	default:
	    tmp = ({ "says", "whispers",
		"tells you", "yells", "shouts" })[random(5)] + ": " +
		delirious_talk();
	    break;
	}

    tmp += ({ ".", "!", "?", "!!!", "..." })[random(5)];
    return tmp;
}

string
random_being()
{
object *u;
int i;

    u = users();

    switch(random(3)) {
	case 0:
	    i = random(sizeof(u));
	    if (!(u[i]->query_coder_level()))
		return capitalize((string)u[i]->query_real_name());
	    /* Don't break; fall into next case */
	default:
	  return
	    ({ "Death", "Large bag", "Sword", "Dragon",
		 "Troll", "Game Driver", "Armageddon", "Batmud",
		 "The clouds", "The ground under your feet",
		 "The tiny green man", "Vainamoinen", "Your liege",
		 "Alice in Wonderland", "Your brain", "Flowers",
		 "Bulletin board", "Aardvark", "Amazon",
		 "Ancient Multi-Hued Dragon", "Attorney", "Audrey II",
		 "Basilisk", "Battlemech", "Beholder",
		 "bookworm", "Boomer", "Borg", "Brogmoid",
		 "Christmas-tree monster", "Creeping coins",
		 "Dalek", "Doppelganger", "Dornbeast",
		 "Earthquake beast", "Efreeti", "Emu", "Ent",
		 "Evil Iggy", "Ewok", "Giant cockroach",
		 "Giant pigmy", "Giant dwarf", "giant slug",
		 "Gnu", "Grue", "Harpy", "Hologram", "Hydra",
		 "Invid", "Jester", "Jumbo shrimp", "Kestrel",
		 "Killer bunny", "Killer penguin", "Killer tomato",
		 "King Kong", "Klingon", "Lion-dog", "Luck eater",
		 "Luggage", "Maggot", "Marid", "Microscopic space fleet",
		 "Nickelpede", "Nightmare", "Ohmu", "Christian Borg",
		 "One-eyed one-horned flying purple people eater",
		 "Paskald", "Pterodactyl", "Pushmi-pullyu",
		 "Rat-ant", "Ravenous Bugblatter Beast of Traal",
		 "Rhinovirus", "Robot", "Rodent of unusual size",
		 "Rot grub", "Samurai rabbit", "Shadow",
		 "Siren", "Sleazoid", "Smokey the bear",
		 "Smurf", "Snark", "Tangle tree",
		 "Teenage mutant ninja turtle", "Terminator",
		 "Totoro", "Tribble", "Tyrannosaurus rex",
		 "Venus flytrap", "White rabbit", "Wiggle",
		 "Witch doctor", "Jabberwock", "Monty Python"
	 })[random(97)];
    }
}

void
symptom(int symtype)
{
string tmp;
int i;

    /* At least 3 different descs for each symptom if possible */

    switch (symtype) {
	case SYM_BOILS:
	    TV( ({ "Pus is flowing out from your boils.",
		"Your whole body is covered by awful, inflamed boils.",
		"Large, sickly yellow boils cover your skin."
		})[random(3)]);
		if (random(100) > 97) victim->add_stat(ST_DEX, -1);
	    break;
	case SYM_COUGH:
	    TV( ({ "You nearly cough your lungs out!",
		"You cough and spit lumps of mucus.",
		"You cough violently and gasp for air.",
		})[random(3)]);
	    switch(random(3)) {
		case 0: EM("nearly coughes "+HIS+ " lungs out!"); break;
		case 1: EM("coughes and spits lumps of mucus."); break;
		default: EM("coughes violently and gasps for air."); break;
	    }
	    break;
	case SYM_DELIRIOUS:
	    switch(random(3)) {
		case 0: EM("mumbles and giggles to "+HIM+"self."); break;
		case 1: EM("looks pale and smiles dreamily."); break;
		default: EM("whispers: " + delirious_talk() + "."); break;
	    }
		if (random(100) > 97) victim->add_stat(ST_INT, -1);
	    break;
	case SYM_DIARRHEA: /* Blargghh...this is SICK! */
	    TV("Owww! The pain forces you to bend and evacuate!");
	    EM("bends down squeezing "+HIS+" stomach with both hands.");
	    switch(random(3)) {
		case 0:
TV("You make only some drops of horrible, rotten black fluid.");
EM("evacuates some drops of horrible, rotten black fluid. How disgusting!");
		    break;
		case 1:
TV("There comes a huge amount of bloody, awful, thick paste.");
EM("evacuates a huge amount of bloody, awful, thick paste. How nauseous!");
		    break;
		default:
TV("You scream in pain as blood and mucus pour from your bowels!");
EM("screams in pain when blood and mucus pour from " + HIS + " bowels!");
		    break;
	    }
		if (random(100) > 97) victim->add_stat(ST_CON, -1);
	    break;
	case SYM_FEVER:
	    TV(({"You feel feverish.", "You feel dizzy. The fever!",
		"The world spins around you. It's the fever..."
		})[random(3)]);
		if (random(100) > 97) victim -> add_stat(ST_STR, -1);
	    break;
	case SYM_HALLUCINATE:
	    Flags |= F_HALLUCINATING;		// Let's mark we set this one!
	    victim->set_condition(C_HALLUCINATING, -1, this_object());
	    if (random(100) < 40)
	        TV(random_being() + " " + random_happening());
	    break;
	case SYM_HEADACHE:
	    TV(({"Your head aches terribly.",
		"Your blinding headache makes thinking difficult.",
		"There are little men inside your head which want out.",
		})[random(3)]);
	    break;
	case SYM_LIGHTSENSITIVE:
	    if (environment(victim) && environment(victim)->query_light())
		TV(({ "You can't stand this light any more!",
		    "It's too bright in here! It's painful!",
		    "The light in here hurts your eyes!"
		   })[random(3)]);
	    break;
	case SYM_MUSCLE_PAIN:
	    TV( ({ "All your muscles ache.",
		"The pain in your muscles is almost unbearable!",
		"You sweat and groan in pain. All your muscles are hurting."
		})[random(3)]);
	    EM("groans and moans in pain.");
		if (random(100) > 97) victim->add_stat(ST_STR, -1);
	    break;
	case SYM_PARALYSIS:
	    if (!victim->query_condition(C_STUNNED)) {
		Flags |= F_STUNNED;	// Let's mark we set this...
		victim->set_condition(C_STUNNED, random_duration());
	    }

	    TV("Your muscles refuse to co-operate. You fall down.");
	    if (NOT_INVIS)
		EM("falls down and seems to be unable to move.");
	    break;
	case SYM_RABIES:
	    TV("Your neck muscles spasm extremely painfully! AARGHHH!");
	    if (NOT_INVIS)
		xsay((NAME +
 "'s neck muscles spasm horribly and "+HE+" screams in agony!"));
	    /* A bad one! */
		if (random(100) > 96) victim->add_stat(ST_CON, -1);
		if (random(100) > 95) victim->add_stat(ST_STR, -1);
		if (random(100) > 94) victim->add_stat(ST_INT, -1);
	    break;
	case SYM_RASH:
	    if (rash != "") TV("Your " + rash + ".");
	    break;
	case SYM_RHINITIS:
	    switch(random(3)) {
		case 0: TV("A-AA-AAAACHOOOOOMMMM!!! You sneeze loudly!");
		    EM("sneezes loudly!");
		    break;
		case 1: TV("You wipe snot off your red, irritated nose.");
		    if (NOT_INVIS)
		  EM(("wipes off snot from "+HIS+" red, irritated nose."));
		    break;
		case 2: TV("Oops, your nose is bleeding.");
		    if (NOT_INVIS)
			xsay("You notice that "+NAME+"'s nose is bleeding.");
		    break;
		default:
		    TV("Sniff! Your nose is full of snot and mucus.");
		    EM("sniffs.");
		    break;
	    }
	    break;
	case SYM_SHIVER:
	    switch(random(3)) {
		case 0: TV("Brrrrr! How it can be so c-c-cold here?");
		    EM("shivers with cold.");
		    break;
		case 1: TV("You shiver and quiver with cold. Brrrr!");
		    EM("shivers and quivers.");
		    break;
		default:
		    TV("Your teeth rattle and you shiver with cold.");
		    xsay(NAME+"'s teeth rattle and "+HE+" shivers.");
		    break;
	    }
	    break;
	case SYM_SORE_THROAT:
	    switch(random(3)) {
		case 0: TV("Khhrmnn! Hrghnn! Your throat is sore.");
		    EM("clears "+HIS+" throat.");
		    break;
		case 1: TV("Your throat is sore. Swallowing hurts.");
		    break;
		default:
		    TV("Your try to clear your sore throat. Krrrhmm!");
		    EM("makes a krrrhmmm sound, trying to clear " +
			HIS + "throat.");
		    break;
	    }
	    break;
	case SYM_SPASMS:
	    if (!victim->query_condition(C_STUNNED)) {
		Flags |= F_STUNNED;	// Let's mark we set this...
		victim->set_condition(C_STUNNED, random_duration() / 2);
	    }

	    switch(random(3)) {
		case 0:
		    TV("Owww! Your all muscles spasm very painfully!");
		    xsay(NAME+"'s all muscles suddenly spasm painfully!");
		    break;
		case 1:
		    TV(
"Aaarghh! The agony! You fall down and convulse with horrible spasms!");
		    EM("falls down and convulses with horrible spasms!");
		    break;
		default:
		    TV(
"Your are struck down by unbearable spasms which twist all your\
 limbs into impossible positions. The pain is enormous.");
		    EM(
"is struck down by unbearable spasms which twist all "+HIS+" limbs\
 into impossible positions. That must hurt a lot!");
		    break;
	    }
	    break;
	case SYM_STOMACH_PAIN:
	    switch(random(3)) {
		case 0:	TV("Auch! Your stomach hurts!"); break;
		case 1:
		    TV("There is an awful pain in your stomach!"); break;
		default:
		    TV(
"Your stomach hurts so much that you are forced to sit down and\
 squeeze it with both hands.");
		    EM(
 "bends down squeezing "+HIS+" stomach with both hands.");
		    break;
	    }
	    break;
	case SYM_SUFFOCATE:
	    if (!victim->query_condition(C_STUNNED)) {
		Flags |= F_STUNNED;	// Let's mark we set this...
		victim->set_condition(C_STUNNED, random_duration());
	    }

	    switch(random(3)) {
		case 0: TV("Hrkkk! Suddenly you cannot breathe!");
		    EM("is desperately trying to breathe.");
		    break;
		case 1:
		    TV(
"Something in your throat makes breathning very hard. You\
 clutche and scratch your neck in panic.");
		    EM("clutches and scratches "+HIS+" neck in panic.");
		    break;
		default:
		    TV(
"You can't breathe! You cough, gasp, rattle and convulse, until blood\
 and mucus pour out from your mouth.");
		    EM(
"seems to be unable breathe! "+HE+" coughes, gasps, rattles and\
 convulses until blood and mucus pour out from "+HIS+" mouth.");
		    break;
	    }
	    break;
	case SYM_THIRST:
	    TV("You feel extremely thirsty.");
	    break;
	case SYM_VOMIT:
	    TV("You feel very nauseous!");
	    switch(random(3)) {
		case 0: TV(
 "You puke a stream of horrible, rotten black fluid.");
		    EM(
 "pukes out a stream of rotten black fluid. Nauseous!");
		    break;
		case 1: TV(
 "You puke out a huge amount of bloody, awful, thick paste.");
		    EM(
 "pukes out a huge amount of bloody, awful, thick paste.");
		    break;
		default: TV(
 "You violently puke out stinking blood and mucus!");
		    EM("violently pukes out blood and mucus!");
		    break;
	    }
		if (random(100) > 98) victim->add_stat(ST_CON, -1);
		if (random(100) > 98) victim->add_stat(ST_STR, -1);
	    break;
	default: break;
    }
}

void
xsay(string msg)
{
  object x;

  if (!victim || !(x = environment(victim))) return;

  x->tell_here(msg, ({ victim }));
}

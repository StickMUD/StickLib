#include <nroom.h>
#include <invis_levels.h>
#include <conditions.h>
#include <living_defs.h>
#include <daemons.h>
#include <areas.h>
#include <city_name.h>

#define RECOVERY "hospital/patient_hall"
#define SEX_CHANGE_COST 20000
#define ABORTION_COST 15000

/* This HAS to be VERY expensive - as potions cost 70k, and this is
 * guaranteed to succeed, AND available to non-lords too. */
#define PLASTIC_COST    150000

/* Prototypes */
void play_seq();
status query_doctor_busy();
int diagnosis();
int cure_disease_cost(int l);
int cure_mental_cost(int l);
void sex_change();
void cure_disease(string n);
void cure_mental(string n);
void race_change(string n);
void abortion();
void plastic_surgery(string type);

object doctor, nurse, patient;	/* Actors of the drama */
string *msg;			/* Operation sequence */
int seq;			/* Sequence counter */

static string *RACES = ({
  "human", "dwarf", "dark elf", "elf", "hobbit", "half-orc", "troll" });

string *race_change_msg = ({
  "Someone grabs your hands and feet from behind!",
  "You are placed on the table and chained into it.",
  "Doctor Mengele notes: This one isn't going to mess up my table.",
  "Doctor Mengele inserts a tube into your rectum.",
  "Nurse giggles insanely.",
  "Doctor Mengele whistles happily and selects an instrument from the table.",
  "It's a LONG, SHARP scalpel!",
  "Nurse smiles happily.",
  "Doctor Mengele slashes your ears off!",
  "Extreme pain explodes through your head!",
  "In an one swift movement, doctor Mengele cuts you open!",
  "You feel that you are going to die from pain!",
  "Doctor Mengele starts to arrange your bowels and organs on the table!",
  "You scream in horror but it's hard to do so with lungs outside.",
  "Doctor Mengele giggles and says: Gee, look at this yellow liver!",
  "Nurse smiles happily.",
  "Doctor Mengele punctures your eyes!",
  "Everything goes dark..."
});

static string *sex_change_male = ({
  "Someone grabs your hands and feet from behind!",
  "You are placed on the table and chained into it.",
  "Doctor Mengele takes something from the smaller table.",
  "It's a small shining steel axe!",
  "Doctor Mengele checks the blade with his thumb.",
  "Doctor Mengele mumbles: Hmmm...where did I put the grinding-machine...",
  "Doctor Mengele finds the grinding machine and starts to sharpen the axe.",
  "Doctor Mengele sharpens the axe. SCREEEECCHH! SCREEEEEEEEEECCCHHH!",
  "The nurse smiles happily.",
  "Doctor Mengele sharpens the axe. SSCREEEECCHH! SCREEECCCHHH! SCREEEEECH!",
  "Doctor Mengele checks the blade with his thumb.",
  "Doctor Mengele says: Ok, it's sharp now.",
  "The nurse pulls your certain body part into position.",
  "Doctor Mengele smiles happily and rises the axe.",
  "Doctor Mengele says: This may hurt a bit.",
  "Doctor Mengele giggles and rises the axe higher.",
  "The nurse smiles happily.",
  "Doctor Mengele whispers something to the nurse.",
  "The nurse grins evilly.",
  "The axe hits its target with a loud THUMP!!!",
  "You lose consciousness."
});

static string *sex_change_female = ({
  "Someone grabs your hands and feet from behind!",
  "You are placed on the table and chained into it.",
  "Doctor Mengele takes a scalpel from the smaller table.",
  "Doctor Mengele whispers something to the nurse.",
  "The nurse gasps: From a dog?!?!?!??",
  "Doctor Mengele nods.",
  "The nurse says 'Ok' and leaves.",
  "Doctor Mengele says: We may start the operation meanwhile.",
  "Doctor Mengele starts cutting you with his scalpel!",
  "You lose consciousness."
});

static string *cure_disease_msg = ({
  "Someone grabs your hands and feet from behind!",
  "You are placed on the table and chained into it.",
  "Doctor Mengele whistles happily and selects an instrument from the table.",
  "It's a small, SHARP knife!",
  "Nurse smiles happily.",
  "Doctor Mengele smiles and starts cutting your wrist!",
  "You watch in horror when blood showers from the wound.",
  "Everything goes dark..."
});

static string *cure_mental_msg = ({
  "Someone grabs your hands and feet from behind!",
  "You are placed on the table and chained into it.",
  "Doctor Mengele whistles happily and selects an instrument from the table.",
  "It's one of those nearly WHITE HOT IRONS!",
  "Nurse smiles happily.",
  "Doctor Mengele presses the hot iron on your forehead!",
  "Extreme pain explodes through your head!",
  "You can smell your burning flesh!",
  "You feel that you are going to die from pain!",
  "Everything goes dark..."
});

static string *abortion_msg = ({
  "Someone grabs your hands and feet from behind!",
  "You are placed on the table and chained into it.",
  "Doctor Mengele whistles happily and selects an instrument from the table.",
  "It's a rusty piece of...hmm...barbed wire?!?",
  "Nurse smiles happily.",
  "Doctor Mengele thrusts the barbed wire into your....eh...!",
  "AAAAAAARRRRRRRRRRGGGGHHHHHHHHH!!!! IT HURTS HORRIBLY!!!",
  "You can see blood spraying on the walls and floor of the room!",
  "You feel that you are going to die from pain!",
  "Everything goes dark..."
});

static mapping Stats;

void
create_room()
{
    set_short("Doctor's Workroom");
    set_long(
      "The walls of this large room are painted white - but badly stained by\
 the blood, mucus and inner organs of the first aid patients. In the\
 middle of the room is a huge wooden table, and beside that is a smaller\
 table filled with various medical instruments. Bookshelves, chests and\
 cupboards cover the back wall. In a corner is the doctor's work table.\
 There is a list of prices on the wall. Perhaps you should examine it.");
    set_light_level(LT_LIGHTS);
    set(ROOM_WD, WD_INDOORS);
    set(IN_CITY);
    set(YES_PK);
    set_exits(([ "south" : AREA_TRISTEZA+"hospital/hall" ]));
    set_items(([
	"table" : 
	"Which table do you mean? Huge wooden, smaller table or work table?",
	({ "huge wooden", "wooden table", "huge wooden table" }) :
	"On this big, thick and heavy table the doctor makes all the operations\
 and examinations. There are strong-looking chains fastened into the\
 table so that the patient may be immobilized. And, of course, the table\
 is totally covered by a thick layer of blood, flesh strips, exretions,\
 guts, hair, pus and vomit from countless patients.",
	({ "smaller", "smaller table" }) :
	"On the smaller table there are many different scalpels, scissors and\
 forceps, a hammer, a hand axe, hand bellows, enema pump, a bowl (for\
 letting blood) and red hot irons in a coal pot.",
	"work table" :
	"It is filled with papers covered with unreadable writing.",
	({ "shelf", "shelves", "bookshelf", "bookshelves" }) :
	"You cannot understand any of the language in those books.",
	({ "cupboard", "cupboards" }) :
	"The cupboards are filled with unlabeled bottles and containers.",
	({ "chest", "chests" }) :
	"The chests are filled with unlabeled bottles and containers.",
	({ "list", "list of prices", "pricelist" }) :
	"    Medical services of " CITY_NAME " Sanatorium\n\
-----------------------------------------------\n\
Examination & Diagnosis		   50 gc\n\n\
Treatment of Diseases\n\
\tVaries			100..100000 gc\n\n\
Treatment of Mental Diseases\n\
\tNicotine addiction	15000 gc\n\
\tSchizophrenia		 4000 gc\n\n\
Other operations\n\
\tSex change (*)		"+SEX_CHANGE_COST+" gc\n\
\tPlastic surgery		"+PLASTIC_COST+" gc\n\
\tAbortion                "+ABORTION_COST+" gc\n\
\tRace change		varies\n\n\
You should 'buy diagnosis' to know what to do.\n\
Note that it costs you 50 gc.\n\
(*) Doesn't cost anything to 1st level people.\n"
      ]));

    set_commands((["buy" : "buy" ]));
    seq = -1;
    Stats = ([
      ST_BASE_STR : "strength",
      ST_BASE_DEX : "dexterity",
      ST_BASE_INT : "intelligence",
      ST_BASE_CON : "constitution",
#if 0
      // Not yet:
      ST_BASE_FAT : "fatigue"
#endif
    ]);
    add_monster(AREA_TRISTEZA+"hospital/doctor", "Doctor Mengele arrives.", 1);
    add_monster(AREA_TRISTEZA+"hospital/nurse", "A nurse arrives.", 1);
}

public object
query_patient()
{
    return patient;
}

void
init_room()
{
    /* Hmmm..
       if ((!patient || !present(patient, this_object())) &&
	 present(this_player(), this_object()) &&
	 this_player() && living(this_player()) && query_ip_number(this_player()))
      */
    if(!patient)
	patient = this_player();
    else if(!present(patient, this_object())) patient = this_player();
}

status
doctor_here()
{
    if(!doctor) doctor=present("mengele",this_object());
    if (!doctor || !present(doctor, this_object())) {
	notify_fail("The doctor is not here now.\n");
	return 0;
    }

    if (doctor->is_fighting() ||
      seq != -1) {
	notify_fail("The doctor seems to be busy right now.\n");
	return 0;
    }

    return 1;
}

/* Tripled cost, helps a bit against stat-cheaters //Graah 06-May-96 */
int
race_change_cost(object who)
{
    int cost, l;

    if (!who) {
	cost =  1000000;
    } else {
	l = (int)who->query_level() - 1;
	cost = 1000 + (500 * l * l / 3 * l / 5) + 1000 * l;
    }

    if (cost > 5000000) cost = 5000000;

    return cost;

#if 0
    int cost;

    if (!who) {
	cost =  5000000;
    } else {
	cost = 100000;
	cost += 10000 * ((int)who->query_level() - 1);
	if ((int)who->query_level() > 20)
	    cost += (((int)who->query_level() - 20) * 300000);
    }

    if (cost > 10000000) cost = 10000000;
    return cost;
#endif
}

int
rob_money(int x)
{
    int m;

    m = (int)this_player()->query_money();

    if (m < x) {
	this_player()->tell_me(sprintf("Doctor says: You don't have %d gc.", x));
	return 0;
    }
    this_player()->tell_me(sprintf("You pay %d gold pieces to the doctor.", x));
    /* This would be realistic, but we would need an ENORMOUS defence!!!
	doctor->add_money(x);
    */
    this_player()->add_money(-x);
    return 1;
}

int
diagnosis()
{
    string tmp;
    object *inv;
    int g, i;

    if (!rob_money(50)) return 1;

    this_player()->tell_me("\nDoctor says: Ok, take all your clothes off...\n\n\
After the examination you get this diagnosis:");

    inv = all_inventory(this_player());

    for (i = 0; i < sizeof(inv); i++) {
	if (inv[i]->id("disease")) {
	    if (!(tmp = (string)inv[i]->query_nickname())) tmp = "Fever";
	    this_player()->tell_me("\nYou have the "+tmp+". Curing costs " +
	      cure_disease_cost((int)inv[i]->query_level()) + ". " +
	      "('buy cure disease').");
	}

	if (inv[i]->id("pregnancy")) {
	    this_player()->tell_me("\nOh, you are pregnant! How cute. Buy abortion \
to get rid of the baby.");
	}

	if (inv[i]->id("mental_disease")) {
	    if (!(tmp = (string)inv[i]->query_name()))
		tmp = "Madness";
	    this_player()->tell_me("\nYou have the "+tmp+". Curing costs "+
	      cure_mental_cost((int)inv[i]->query_level()) + ". "+
	      "('buy cure " + tmp + "').");
	}
    }

    g = (int)this_player()->query_gender();
    if (g == 1 || g == 2) {
	if (((int)this_player()->query_level() > 1))
	    this_player()->tell_me(
	      "\nYou can change your gender ('buy sex change'), which costs " +
	      SEX_CHANGE_COST + " gc.");
	else
	    this_player()->tell_me("\nYou can change your \
gender ('buy sex change') for free.");
    }

    this_player()->tell_me("\nPlastic surgery can add or remove scars ('buy add/remove \
scars'), but that's an expensive operation, "+PLASTIC_COST+" gc.");

    if (this_player()->query_race()) {
	this_player()->tell_me("\nYou can change your race ('buy race <race>'), cost is " +
	  race_change_cost(this_player()) + " gc.");
    }

    return 1;
}

status
buy(string arg)
{
    object ob;
    string tmp, tmp2, n;
    int cost, i, x, *st, stat_value, z, rcc;
    mapping race_maxes;

    if (!doctor_here()) return 0;
    if (!patient || !present(patient, this_object())) patient = this_player();
    if (patient != this_player()) {
	notify_fail("Doctor says: Huh? Who are you? Where's my patient???\n");
	return 0;
    }

    if (!arg) {
	notify_fail("Doctor says: What? Try buying diagnosis first...\n");
	return 0;
    }

    arg = lower_case(arg);
    if (arg == "diagnosis" || arg == "diagnose") return diagnosis();
    if (arg == "sex change") {
	cost = (int)this_player()->query_gender();
	if (cost != 1 && cost != 2) {
	    notify_fail("Doctor says: Huh? Aren't you weird enough?\n");
	    return 0;
	}
	if (((int)this_player()->query_level() > 1))
	    if (!rob_money(SEX_CHANGE_COST)) return 1;
	sex_change();
	return 1;
    }

    if (arg == "abortion")
    {
	if(!present4("pregnancy", this_player(), this_player(),
	    IL_TOTALLY_INVISIBLE))
	{
	    notify_fail("Doctor says: You are not pregnant.\n");
	    return 0;
	}
	if (!rob_money(ABORTION_COST)) return 1;
	abortion();
	return 1;
    }

    if (sscanf(lower_case(arg), "cure %s", tmp) == 1)
    {
	if (tmp == "nicotine addiction" || tmp == "addiction") {
	    this_player()->tell_me(
	      "Dr Mengele states: We don't do those operations yet. Come back later.");
	    return 1;
	}

	if (tmp == "disease") {
	    ob = present4("disease", this_player(), this_player(),
	      IL_TOTALLY_INVISIBLE);
	    if (ob) {
		cost = cure_disease_cost((int)ob->query_level());
		if (!rob_money(cost)) return 1;
		cure_disease(tmp);
		return 1;
	    }
	    notify_fail("Doctor says: What disease?\n");
	    return 1;
	}

	ob = present4(tmp, this_player(), this_player(), IL_TOTALLY_INVISIBLE);
	if (ob) {
	    if (ob->id("mental_disease")) {
		cost = cure_mental_cost((int)ob->query_level());
		if (!rob_money(cost)) return 1;
		cure_mental(tmp);
		return 1;
	    }
	    notify_fail("Doctor gasps: I don't cure " + tmp + "!\n");
	    return 0;
	}

	notify_fail("Doctor says: You don't seem to have any " +
	  "\"" + tmp + "\".\n");
	return 0;
    }

    if (sscanf(lower_case(arg), "race %s", tmp) == 1) {
	if (!(tmp2 = (string)this_player()->query_race())) {
	    notify_fail("Doctor says: Why don't you go to Guild of Races and select your race?\n");
	    return 0;
	}

	if (tmp2 == tmp) {
	    notify_fail(sprintf(
		"Doctor says: What? Change %s to %s?\n", tmp, tmp));
	    return 0;
	}

	if (member(RACES, tmp) == -1) {
	    this_player()->tell_me(sprintf("Doctor says: I only know races %s.\n", implode(RACES, ", ", " and ")));
	    return 1;
	}

	rcc = race_change_cost(this_player());

	if (rcc < 1)
	{
	    this_player()->tell_me("Doctor Mengele says: \
What, race change would cost nothing? Something is bugging here. \
Ask an admin or coder to look at this.");
	    return 1;
	}

	if (!rob_money(rcc)) return 1;

	race_maxes = (mapping) LEVELS_D->query_race_max_stats();
	n = (string) this_player()->query_real_name();

	for (i = z = 0, st = m_indices(Stats); i < sizeof(st); i++) {
	    if ((stat_value = (int) this_player()->query_stat(st[i])) > race_maxes[tmp][st[i]])
	    {
		if (!x++) {
		    this_player()->tell_me(sprintf("Doctor Mengele says: Well, your %s will \
also be lowered due to the operation.", Stats[st[i]]));
		} else {
		    this_player()->tell_me(sprintf("Doctor Mengele says: ... as well as your %s...",
			Stats[st[i]]));
		}
		this_player()->set_stat(st[i], race_maxes[tmp][st[i]]);
		// Let's log them... in case there are problems. :-/
		log_file("mengele", sprintf("%s (level %d), changed race from %O to %O. \
%O lowered from %d to %d (cost %d), at %s\n",
		    n, (int)this_player()->query_level(),
		    tmp2, tmp, Stats[st[i]], stat_value, race_maxes[tmp][st[i]],
		    rcc, ctime(time()) ));
		z++;
	    }
	}
	if (!z)
	    log_file("mengele",sprintf("%O (level %d), changed race from %O to %O (cost %d) at %s.\n",
		n, (int)this_player()->query_level(), tmp2, tmp, rcc, ctime(time())));
	race_change(tmp);
	return 1;
    }

    if (sscanf(lower_case(arg), "%s scars", tmp) == 1) {
	if (tmp != "add" && tmp != "remove") {
	    notify_fail("Doctor says: I can only 'add' or 'remove' scars - what do you mean???\n");
	    return 0;
	}

	/* Not on low level players. They don't get scars when dying. */
	if ((int)this_player()->query_level() < 10) {
	    notify_fail("Doctor says: Your pretty, young face doesn't need plastic surgery.\n");
	    return 0;
	}

	if (!rob_money(PLASTIC_COST)) return 1;
	plastic_surgery(tmp);
	return 1;
    }

    notify_fail(({
	"Doctor says: We can't do that operation before getting some new equipment.",
	"Doctor says: Oh, that requires an instrument which is currently broken.",
	"Doctor says: I don't do those operations. Find a specialist.",
	"Doctor says: Hmm, are you sure that you are in the right place?"
      })[random(4)] + "\n");
    return 0;
}

int
query_doctor_busy()
{
    if ((patient) && (present4(patient, this_object())))
	return 1;

    patient = 0;

    if (!doctor || !present(doctor, this_object())) return 0;

    return 0;
}

void
play_seq()
{

    if (!patient || !present(patient, this_object())) {
	tell_room(this_object(),
	  "Doctor gasps in astonishment and says: Where did our patient go!???!\n");
	patient = 0;
	seq = -1;
	return;
    }
    patient->set_condition(C_PASSED_OUT, 4); /* Keep us stunned */

    if (seq >= sizeof(msg)) {

	seq = -1;

	if (!find_object(AREA_TRISTEZA+RECOVERY))
	    call_other(AREA_TRISTEZA+RECOVERY, "???");

	patient->set_condition(C_STUNNED, 0);
	patient->set_condition(C_HALLUCINATING, 0);
	patient->set_condition(C_PASSED_OUT, 2 + random(4));

	/* Our hp is always at zero after the mad doctor's treatment... */
	patient->add_hp(-100000);

	if (find_object(AREA_TRISTEZA+RECOVERY))
	    move_object(patient, find_object(AREA_TRISTEZA+RECOVERY));

	patient = 0;

	return;
    }

    patient->tell_me(msg[seq]);

    seq++;

    call_out("play_seq", 1);
}

void
sex_change()
{
    /* Comedy... */
    if ((int)patient->query_gender() == G_MALE) {
	patient->set_gender(G_FEMALE);
	msg = sex_change_male;
    } else  {
	patient->set_gender(G_MALE);
	msg = sex_change_female;
	/* Hmm, how is this actually done???... */
    }
    seq = 0;
    call_out("play_seq", 1);
}

int
cure_disease_cost(int l)
{
    if (l < 100) return 100;
    return l;
}

int
cure_mental_cost(int l)
{
    if (l < 1) return 4000;
    if (l < 100) return 100;
    return l;
}

void
cure_disease(string n)
{
    object dis;
    msg = cure_disease_msg;
    dis = present4(n, patient, patient, IL_TOTALLY_INVISIBLE);
    if (dis) {
	dis->remove();
	if (dis) destruct(dis);
    }
    seq = 0;
    call_out("play_seq", 1);
}


void
cure_mental(string n)
{
    object dis;
    if (n == "mental disease") n = "mental_disease";
    msg = cure_mental_msg;
    dis = present4(n, patient, patient, IL_TOTALLY_INVISIBLE);
    if (dis) {
	dis->remove();
	if (dis) destruct(dis);
    }
    seq = 0;
    call_out("play_seq", 1);
}

void
abortion()
{
    object pr;

    msg = abortion_msg;
    pr = present4("pregnancy", patient, patient, IL_TOTALLY_INVISIBLE);
    if (pr) {
	pr->remove();
	if (pr) destruct(pr);
    }
    seq = 0;
    call_out("play_seq", 1);
}


void
race_change(string n)
{
    string tmp;
    string title;

    msg = race_change_msg;
    this_player()->set_race(n);
    /* Set title if needed */
    tmp = (string)(LEVELS_D->query_title_for_level(
	(int)this_player()->query_level(),(int)this_player()->query_gender(),n));
    if (tmp) {
	if ((int)this_player()->query_level() < 21 && !this_player()->query_coder_level())
	    this_player()->set_title(tmp);
	else if (this_player()->query_coder_level())
	    this_player()->tell_me(sprintf("Official title for your race and level: \
%s.", title));
    }
    seq = 0;
    call_out("play_seq", 1);
}

void
plastic_surgery(string type)
{
    int i;
    string tmp;
    tmp = "Doctor Mengele";
    msg = ({
      "Someone grabs your hands and feet from behind!\n",
      "You are placed on the table and chained into it.\n",
      tmp + " whistles happily and selects an instrument from the table.\n"
    });

    if (type == "add") msg += ({
	  "It's a dull, rusty, old saw-edged MACHETE!!!",
	  "Dr. Mengele slashes your face HARD with the weapon!",
	  "Bits of bone and meat, blood and hairs spray all over the room!",
	  "Your mouth is full of blood, bits of flesh and broken teeth!!!"
	});
    else
	msg += ({
	  "It's a piece of ROUGH sandpaper!!!",
	  "Dr. Mengele starts SCRAPING your face VERY HARD with sandpaper!!!",
	  "The pain is horrible!"
	});

    msg += ({
      "Nurse smiles happily.",
      "Everything goes dark..."
    });

    /* Do scar stuff */

    /* Remove: removes ALL scars */
    if (type == "remove") {
	this_player()->clear_scar();
    } else {
	/* 6-10 scars */
	for (i = 0; i < 6 + random(5); i++)
	    this_player()->make_scar();
    }
    seq = 0;
    call_out("play_seq", 1);
}

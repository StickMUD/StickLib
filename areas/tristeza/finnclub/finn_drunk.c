// The genuine Finnish Intoxination!!!

#define NAME (string)victim->query_name()
#define HIS (string)victim->Possessive()

#include <invis_levels.h>

inherit "/basic/id";

void do_silly_things();
string finntalk();

static object victim;
static int counter;
static string *phrases, *swears;
static int s_phrases, s_swears;

void
create()
{
    counter = 10 + random(10); /* Set this correctly in the drink! */
    victim = 0;
    set_name("finn_drunk");
    set_id(({ "fdrunk" }));
    set_long("Wise ass...");
    set_invis(IL_TOTALLY_INVISIBLE);

    phrases = 
    ({ "{l{ k{y k{nisee","kirveell{ p{{h{n",
      "silmien v{lliin ryss{{","viinaa ja {kki{","vippaa viitonen",
      "nyt tulee turpaan","onks tietoo","ja sitten saunaan",
      "heit{ r||ki","rahat loppu","tilaa taksi","Mannerheim oli kova j{b{",
      "onks sulla viinaa","Raatteen tie","ruottalaiset on homoja",
      "Toni Nieminen","Matti Nyk{nen","Esko Aho","V{yrysen Pave",
      "Uffe Sundkvist","presitenttipeli","kunnallisvaalit",
      "keskustapuolue","kokoomus","ei pakolaisia t{nne loisimaan",
      "hyv{ Suomi","hakkaa p{{lle","talvisota","jatkosota",
      "isoviha","h{vitt{j{hankinnat","Eppu Normaali",
      "Juice Leskinen","ty|tt|myys","sotaveteraanit",
      "kulutusjuhla","lama","kelluva markka","devalvaatio",
      "asuntopula","it{mafia", "Onnenpy|r{", "Kauniit ja Rohkeat",
      "ty|tt|myys puolittuu", "You leave you man here! We go room!",
      "T| P|t! Mai neim is Jorma! Open t| d||r pliis!"
    });
    s_phrases = sizeof(phrases);

    swears = ({
      "alap{{", "anaali", "anus","faaraon pieru", "helvetti",
      "herpes", "hevonvittu", "hitto", "Jeesuksen pillu",
      "jumalauta", "kakka", "kalu", "kananpaska",
      "kilinvittu", "kulli", "kuppa", "kusi", "kyrp{",
      "mulkku", "paska", "penis", "perkele", "perse",
      "pillu", "runkku", "rutto", "saatana", "sis{avaruus",
      "sonta", "sperma", "spitalis", "syfilis", "terska",
      "tippuri", "tissi", "torttu", "uloste", "virtsa",
      "visva", "vittu"
    });
    s_swears = sizeof(swears);
}

void set_counter(int x) { counter = x; }
void add_counter(int x) { counter += x; }
int query_counter() { return counter; }

void
init()
{
    if (!this_player() ||
      !living(this_player()) ||
      environment() != this_player())
	return;

    if (!victim)
    {
	call_out("finn_beat",5 + random(5));
	victim = this_player();
    }
}

status drop() { return 1; }
status get() { return 1; }

int query_value() { return 0; }
int query_weight() { return 0; }

string
query_info()
{
    return "Genuine Fhinniishh inhtockhsinatchion! Yeps..HICKK!";
}

string
extra_look()
{
    if (!victim) return "Someone is ROARING drunk! Totally stoned!";
    return (string) victim->query_name() +" is ROARING drunk! Totally stoned!";
}

void
drop_random_item()
{
    object *inv;
    int i;

    if (!victim ||
      !environment(victim) ||
      !victim->query_can_move() ||
      random(100) < 60) return;

    inv = all_inventory(victim);
    i = random(sizeof(inv));

    if (!inv[i]->drop(1))
    {
	// Silently!
	move_object(inv[i], environment(victim));
	environment(victim)->tell_here((string)victim->query_name() +
	  " drops " + inv[i]->query_name() + ".");
    }
}

void
finn_beat()
{
    int i;
    string tmp;

    if (counter <= 0)
    {
	if (!victim) return;
	victim -> tell_me("You get a TERRIBLE hangover!\n\
You PUKE your GUTS out! BLYAAAARGGHH!");
	environment(victim)->tell_here(sprintf("%s falls down, is REALLY SICK\
 and PUKES %s GUTS out!!!", NAME, HIS), victim);
	i = ((int)victim->query_intoxication());
	victim->drink_alco((-i) - 6);
	victim->add_hp(-(1 + random(40)));
	destruct(this_object());
	return;
    }

    counter--;

    /* Keep him quite drunk */
    if ((int) victim->query_intoxication() < 90)
	victim->add_intoxication(20);

    switch(random(5))
    {
    case 0:
	victim->force_us(({"puke","fart","drool","hiccup","flip"})[random(5)]);
	break;
    case 1..2:
	tmp = finntalk();
	victim->tell_me(sprintf("You yell: %s!", (string)victim->mumble(tmp)));
	victim->force_us(sprintf("say %s!", tmp));
	break;
    case 3: do_silly_things(); break;
    case 4: drop_random_item(); break;
    default: break;
    }

    call_out("finn_beat", 10 + random(12));
}

string
finntalk()
{
    string tmp;
    int i;

    tmp = "";
    for (i = 0; i < 3 + random(5); i++)
    {
	tmp += ((random(100) < 50) ?
	  swears[random(s_swears)] : phrases[random(s_phrases)]);
	tmp += " ";
    }

    return tmp;
}

string
chick(int x)
{
    switch(x) {
    case 1: return ({ "boy", "guy", "handsome male", "stud" })[random(4)];
    case 2: return ({ "girl", "chick", "babe", "honey" })[random(4)];
    default:
	return ({ "monster", "creature", "thing" })[random(3)];
    }
}

string
man(int x)
{
    if (x < 0 || x > 2) x = 0;
    return ({ "monster", "man", "girl" })[x];
}

void
do_silly_things()
{
    int g, i, amo, mani;
    object *inv, *targs, targ;
    string name, pronoun;

    /* Find someone of opposite sex */

    if (!victim) return;
    g = (int) victim->query_gender();
    if (g == 0)
	g = random(2) + 1;

    inv = all_inventory(environment(victim));

    if (!inv || sizeof(inv) < 1) return;

    targs = ({});

    for (i = (sizeof(inv) - 1); i >= 0; i--)
	if (living(inv[i]) && (g != (int)inv[i]->query_gender()) &&
	  (!inv[i]->query_invis()))
	    targs += ({ inv[i] });

    if (!targs || sizeof(targs) < 1) return;

    targ = targs[random(sizeof(targs))];

    name = (string) targ -> query_name(0, this_player());

    switch(random(3)) {

    case 0: /* The Classic. Todo: add the List of Bad Pick-Up Lines */
	pronoun = (string) targ -> Pronoun();
	victim->tell_me(sprintf("You ask %s what %s is doing in a place\
 like this with a %s like you.", name, pronoun, man(g)));
	command(sprintf("tell %s %s", name,
	    (string) victim->mumble(sprintf("what's a %s like you doing in a place like this with a %s like me?",
		chick((int)targ->query_gender()), man(g)))), victim);
	break;

    case 1:
	if ((mani = (int)victim->query_money()) < 70) break;
	amo = random(500) + 60;
	if (mani < amo) amo = mani;
	pronoun = (string) victim -> Objective();
	victim->tell_me(sprintf("You like %s and decide to give\
 %s %d gold.", name, pronoun, (amo / 10) * 10));
	command(sprintf("tell %s %s", name,
	    (string)victim->mumble("here's " + (amo / 10) * 10 +
	      " gold to you, do you love me now?")), victim);
	victim->force_us(sprintf("give %d coins to %s", amo, name));
	break;
    default:
	command(({"grope", "drool", "wink", "kiss", "love"})[random(5)] +
	  targ->query_name(), victim);
	break;
    }
}

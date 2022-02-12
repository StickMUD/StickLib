#include <room.h>
#include <daemons.h>
#include <generic_items.h>
#include <door_defs.h>
#include <bboard_defs.h>

#include "thief.h"

#define SAVEFILE GDIR2 + "GUILD_SAVE"
#define REALNAME(x) ((string)(x)->query_real_name())
#define TP this_player()

/* Protos */
void init_titles();
int valid_board_access(object me);

/* Saved guild data */
string guildmaster;
string *co_guildmaster;
string *enforcers;
int tax_base, tax_per_level;

static object board, mirror, player_ob, door, door2, door3, entrdoor;
static object *assassins;
static string title;
static string *m_t, *f_t, *n_t;
static int pulled;

void
create_room() {
    object vote_ob;

    if (!restore_object(SAVEFILE)) {
	guildmaster = tax_base = tax_per_level = 0;
	enforcers = ({});
	co_guildmaster = ({});
    }

    set_short("Guild Hall");
    set_long(
      "You are in the Guild of Thieves. Commands: 'info' (latest news), \
'resign', 'fixtitle' and 'toptencheck'. On the wall hangs the black \
list or those who have recently attacked our great guild. There is a \
mysterious lever in western wall.");
    set_light_level(LT_LIGHTS);
    set_wd(WD_INDOORS);

    set_exits(([
	"north"	: GDIR+"rooms/thief_pub",
	"northwest": GDIR+"rooms/herotemple",
	"south"	: GDIR+"rooms/tshop",
	"west"	: GDIR+"rooms/entry",
	"up"	: GDIR+"rooms/attic",
	"down"	: GDIR+"rooms/tunnels"
      ]));
    set(PREVENT_TELEPORT);
    set(ROOM_GUILD);

    add_object( ({ GENERIC_BBOARD,
	({ "set_board", 100, ([ BB_WRITE_CLOSURE : #'valid_board_access,
	    BB_READ_CLOSURE : #'valid_board_access, ]), "thief_board", 0 }),
      }), 0, 1);

    add_permanent(GDIR+"obj/mirror");
    add_item(({ "lever" }),
      "Pulling this lever perhaps launches some kind of a trap.");

    init_titles();

    assassins = ({ 0,0,0,0,0,0 });

    call_other(TOP_TEN, "reset_top_ten");

    call_out("reset_room", 1);
}

int query_tax_base() { return tax_base; }
int query_tax_per_level() { return tax_per_level; }
void set_tax_base(int b) { tax_base = b; save_object(SAVEFILE); }
void set_tax_per_level(int p) { tax_per_level = p; save_object(SAVEFILE); }

int
query_tax(int lvl)
{
    int t, tf;

    t = (tax_base + (lvl * tax_per_level) / 10);
    tf = (tax_base + (40 * tax_per_level) / 10);

    if (t > 95) t = 95;
    if (t > tf) t = tf;

    return t;
}

int valid_board_access(object me)
{
    return (this_player()->query_coder_level()  ||
      (string)this_player()->query_guild() == "thief");
}

void
rem_assassins()
{
    int i;

    for(i = 0; i < sizeof(assassins); i++)
	if (assassins[i]){
	    move_object(assassins[i], "/room/void");
	    destruct(assassins[i]);
	}
}

void
reset_room()
{
    int i;

    pulled = 0;

    if (!present("black list", this_object()))
	call_other(GDIR + "obj/black_list", "???");

    if (!door) {
	if ((door = clone_object(DOOR_FILE))) {
	    door->add_door( ({
		"It is a reinforced wooden door.",
		"It is a reinforced wooden door." }),
	      "east", GDIR + "rooms/inner", "firm steel",
	      "thief_master_key", 1, 1, 1, 5,
	      "It is an easy lock to pick." );
	}
    }

    if (!door2) {
	if ((door2 = clone_object(DOOR_FILE))) {
	    door2->add_door( ({
		"It is a reinforced ebony door.",
		"It is a reinforced ebony door." }),
	      "northeast", GDIR + "rooms/kitchen", "delicate mithril",
	      "thief_kitchen_key", 1, 1, 1, 10,
	      "It is a lock which can be picked, though it won't be easy." );
	}
    }

    if (!door3) {
	if ((door3 = clone_object(DOOR_FILE))) {
	    door3->add_door( ({
		"It is a heavy metal door.",
		"It is a heavy metal door." }),
	      "southeast", GDIR + "rooms/guard_room", "strong iron",
	      "thief_lender_key", 0, 0, 1, 5,
	      "It is a strong iron lock.");
	}

    }

    if (!present("tax box", this_object()))
	move_object(clone_object(GDIR + "obj/cash_box"), this_object());

    for (i = 0; i < sizeof(assassins); i++)
	if (!assassins[i]) {
	    assassins[i] = clone_object(GDIR + "npc/assassin");
	    move_object(assassins[i], this_object());
	}

    save_object(SAVEFILE);
}

object
is_thief(object who)
{
    object ob;

    if (!who) return 0;
    if (!living(who)) return 0;

    ob = present("tmark", who);

    if (!ob) return 0;

    return ob;
    if (who->query_coder_level()) return ob;

    if (ob->query_thief_level() >= TR_DEMON) return ob;

    if (REALNAME(who) != guildmaster)
    {
	if (member(co_guildmaster, REALNAME(who)) == -1)
	{
	    if (member(enforcers, REALNAME(who)) != -1)
	    {
		ob->set_thief_level(TR_ENFORCER);
	    } else ob->set_thief_level(0);
	} else ob->set_thief_level(TR_CO_GM);
    } else ob->set_thief_level(TR_GM);

    return ob;
}

void
set_guildmaster(string str)
{
    guildmaster = lower_case(str);
    save_object(SAVEFILE);
}

/* Adds one or replaces whole array */
void
set_co_guildmaster(mixed str)
{
    if (stringp(str))
    {
	if (member(co_guildmaster, lower_case(str)) == -1)
	    co_guildmaster += ({ lower_case(str) });
    } else co_guildmaster = str;
    save_object(SAVEFILE);
}

void
remove_co_guildmaster(string str)
{
    if (member(co_guildmaster, lower_case(str)) != -1)
	co_guildmaster -= ({ lower_case(str) });
    save_object(SAVEFILE);
}

void
add_enforcer(string str)
{
    if (member(enforcers, lower_case(str)) != -1) return;

    enforcers += ({ lower_case(str) });
    save_object(SAVEFILE);
}

void
set_enforcers(string *str)
{
    enforcers = str;
    save_object(SAVEFILE);
}

void
rem_enforcer(string str)
{
    if (member(enforcers, lower_case(str)) == -1) return;

    enforcers -= ({ lower_case(str) });
    save_object(SAVEFILE);
}

string
query_guildmaster()
{
    return guildmaster;
}

string *
query_co_guildmaster()
{
    return co_guildmaster;
}

status
query_is_co_gm(string who)
{
    return (member(co_guildmaster, lower_case(who)) != -1);
}

string *
query_enforcers()
{
    return enforcers;
}

status
query_is_enforcer(string who)
{
    return (member(enforcers, lower_case(who)) != -1);
}

status
query_is_guildmaster(object who)
{
    return (REALNAME(who) == guildmaster);
}

status
query_is_co_guildmaster(object who)
{
    return (member(co_guildmaster, REALNAME(who)) != -1);
}

int
query_no_of_enforcers()
{
    return sizeof(enforcers);
}

int
pub_open() {
    object gc;
    int hour;

    hour = NATURE_D->query_24_hour();
    if (hour >= 10 && hour < 16) return 0;

    return 1;
}

void
init_room()
{
    add_action("info","info");
    add_action("info","news");
    add_action("west", "west");
    add_action("north","north");
    add_action("resign","resign");
    add_action("fixtitle","fixtitle");

    // They can now check their top ten status manually.
    add_action("toptencheck", "toptencheck");
}

int
toptencheck()
{
    object tm;

    tm = present("tmark", this_player());

    if (!tm) return 0;

    tm->force_check();
    this_player()->tell_me(
      "Ok. Your position on top ten lists has been checked.");
    return 1;
}

int
pull(string arg)
{
    if (!arg) {
	notify_fail("Pull what?\n");
	return 0;
    }

    if (lower_case(arg) != "lever") {
	notify_fail("You pull " + arg + ", but nothing happens.\n");
	return 0;
    }

    if (pulled) {
	notify_fail("You have to wait for it to return to up position.\n");
	return 0;
    }

    write("Ok. You hear a creaking sound and a crash from the west!\n");
    say(this_player()->query_name() + " pulls the lever.\n"+
      "You hear a creaking sound and a crash from the west!\n");

    call_other(GDIR + "rooms/entry", "trap_door");
    pulled = 1;

    call_out("goppa", 4);

    return 1;
}

void
init_titles()
{
    /* DOES NOT WORK if Graah not in game, damn driver
	m_t = explode(read_file(GDIR + "male_titles"), "\n");
	f_t = explode(read_file(GDIR + "female_titles"), "\n");
	n_t = explode(read_file(GDIR + "neuter_titles"), "\n");
     */

    /* Edit file 'tits' and insert it here when the titles change. */

    m_t = ({
      "the Ninja",
      "the Junkie",
      "the Novice",
      "the Punk",
      "the Drug Dealer",
      "the Pimp",
      "the Crook",
      "the Offender",
      "the Mugger",
      "the Ruffian",
      "the Criminal",
      "the Snatcher",
      "the Swindler",
      "the Shoplifter",
      "the Footpad",
      "the Fence",
      "the Stabber",
      "the Kidnapper",
      "the Sharper",
      "the Defrauder",
      "the Gangster",
      "the Filcher",
      "the Pickpocket",
      "the Sneak-Thief",
      "the Cutpurse",
      "the Pilferer",
      "the Counterfeiter",
      "the Robber",
      "the Burglar",
      "the Magsman",
      "the Strangler",
      "the Blackmailer",
      "the Outlaw",
      "the Backstabber",
      "the Bandit",
      "the Thief",
      "the Extortioner",
      "the Hit-man",
      "the Rogue",
      "the Highwayman",
      "the Thug",
      "the Mafioso",
      "the Master Thief",
      "the Assassin",
      "the Villain",
      "the Prince of Thieves",
      "the Scarface",
      "the Godfather",
      "the Grand Master Thief",
      "the King of Thieves",
      "the Legendary Thief",
      "the Demigodly Thief",
      "the Godly Thief",
      "the Evil Iggy",
      "the Pain in the Ass",
      "the Royal Pain in the Ass"
    });

    f_t = ({
      "the Ninja",
      "the Junkie",
      "the Novice",
      "the Punk",
      "the Drug Dealer",
      "the Pimp",
      "the Crook",
      "the Offendress",
      "the Mugger",
      "the Ruffian",
      "the Criminal",
      "the Snatchress",
      "the Swindleress",
      "the Shopliftress",
      "the Footpad",
      "the Fence",
      "the Stabbress",
      "the Kidnappress",
      "the Sharpress",
      "the Defrauder",
      "the Gangstress",
      "the Filchress",
      "the Pickpocket",
      "the Sneak-Thief",
      "the Cutpurse",
      "the Pilferess",
      "the Counterfeitress",
      "the Robber",
      "the Burglaress",
      "the Magswoman",
      "the Stranglress",
      "the Blackmailress",
      "the Outlawess",
      "the Backstabber",
      "the Bandit",
      "the Thief",
      "the Extortiorness",
      "the Hit-woman",
      "the Rogue",
      "the Highwaywoman",
      "the Thug",
      "the Mafia Queen",
      "the Master Thief",
      "the Assassin",
      "the Villain",
      "the Princess of Thieves",
      "the Scarface",
      "the Godmother",
      "the Grand Mistress Thief",
      "the Queen of Thieves",
      "the Legendary Thief",
      "the Demigoddess Thief",
      "the Goddess Thief",
      "the Evil Iggy",
      "the Pain in the Ass",
      "the Royal Pain in the Ass",
    });

    /* Who cares. There's only one neuter player left. */
    n_t = m_t;

    /* ----end of titles---- */

}

string
get_new_title(int lvl)
{
    if (!m_t) init_titles();

    if (!player_ob) player_ob = TP;

    if (lvl > (sizeof(m_t) - 1)) lvl = (sizeof(m_t) - 1);

    if (!player_ob || !player_ob->query_gender()) return n_t[lvl];
    else if ((int)player_ob->query_gender() == 1) return m_t[lvl];
    else return f_t[lvl];
}

string
query_thief_title(int lvl, int g)
{
    int l;

    if (!m_t) init_titles();

    l = lvl;

    if (l > sizeof(m_t)) l = sizeof(m_t);

    l--;

    if (l < 0) l = 0;

    if (g == 1) return m_t[l];
    else if (g == 2) return f_t[l];
    return n_t[l];
}

int
north()
{
    if (!pub_open() && !TP->query_coder_level()) {
	write("The Cloak and Dagger is closed from 10 AM to 4 PM.\n");
	write("(The game time is now " + 
	  NATURE_D->query_time() + ")\n");
	return 1;
    }
    TP->move_player("north#" + GDIR2 + "rooms/thief_pub");
    return 1;
}

string
gnd_prn()
{
    int gnd;

    gnd = (int)TP->query_gender();

    switch(gnd) {
    case 1:
	return "sir";
	break;
    case 2:
	return "madam";
	break;
    default:
	return "best creature";
	break;
    }
}

int
no_title_show(object who) {
    if (!who) return 1;
    if (!is_thief(who) && !who->query_coder_level()) {
	write("You're not a member, so you won't see them.\n");
	return 1;
    }
    return 0;
}

int
maletitles() {
    if (no_title_show(TP)) return 1;
    write(sprintf("%-=78s\n", "Male titles are: " +
	implode(m_t, ", ", " and ") + "."));
    return 1;
}

int
femaletitles() {
    if (no_title_show(TP)) return 1;
    write(sprintf("%-=78s\n", "Female titles are: " +
	implode(f_t, ", ", " and ") + "."));
    return 1;
}

int
neutertitles() {
    if (no_title_show(TP)) return 1;
    write(sprintf("%-=78s\n", "Neuter titles are: " +
	implode(n_t, ", ", " and ") + "."));
    return 1;
}

/* Also shows levels and experience required */
int
alltitles() {
    int s, i;

    if (no_title_show(TP)) return 1;

    s = sizeof(m_t);

    write(sprintf("%3s. %10s    %-28s %-28s\n",
	"Lvl", "Experience", "Male title", "Female title"));

    for (i = 0; i < s; i++)
	write(sprintf("%3d. %9d     %-28s %-28s\n",
	    i + 1, "/lib/levels"->query_exp_for_level(i), m_t[i], f_t[i]));

    return 1;
}

int
info() {
    if (!is_thief(TP) && !TP->query_coder_level()) {
	write("You are not a member!\n");
	return 1;
    }
    cat(GDIR + "doc/news");
    return 1;
}

int
resign(string arg)
{
    object t;
    int tmp;

    if (!is_thief(TP))
    {
	write("You are not even a member of this guild.\n");
	return 1;
    }

    if (arg != "the guild of thieves")
    {
	write(
	  "NOTE: YOU WILL LOSE 10% OF YOUR EXPERIENCE (max 1000000) IF YOU RESIGN!\n\
You will also LOSE 1/10 of your skills plus 5% each day you \
are gone from the guild!\n\
Are you ABSOLUTELY SURE that you want to resign?\n\n\
If you are, then you must type \"resign the guild of thieves\".\n\
nothing less is sufficient.\n");
	return 1;
    }

    write("\nThe Guildmaster arrives.\n");

    write("\nSO, YOU MEANINGLESS LITTLE SLIME! GET OUT OF HERE!\n");
    write("\nYou are kicked out of the guild!\n");

    t = is_thief(TP);

    if (t && t->query_thief_level() < TR_DEMON)
    {
	if (!TP->query_coder_level())
	{
	    if (t->query_thief_level() >= TR_GM)
	    {
		if (REALNAME(TP) == guildmaster)
		{
		    t->chant(Realname(TP) +
		      ", our GUILDMASTER, resigned the guild!", 3);
		    guildmaster = 0;
		}
		else
		if (query_is_co_gm(REALNAME(TP)))
		{
		    t->chant(Realname(TP) +
		      ", our CO-GUILDMASTER, resigned the guild!", 3);
		    co_guildmaster = 0;
		}
		else
		{
		    if (query_is_enforcer(REALNAME(TP)))
		    {
			t->chant(Realname(TP) +
			  ", one of the ENFORCERS, resigned the guild!", 3);
			rem_enforcer(REALNAME(TP));
		    }
		}
	    }
	    else t->chant(Realname(TP) +
		  " RESIGNED our guild!!!", 3);
	}
    }
    else
	t->chant("Demon " + Realname(TP) + " RESIGNED our guild!");

    TP->set_title("the ex-thief");
    TP->set_guild(0);

    tmp = (int)TP->query_ep() / 10;

    if (tmp > 1000000) tmp = 1000000;

    TP->add_exp(-tmp);

    if (t->query_sneak()) TP->vis();

    CHANNEL_D->remove_from_channel("thief", TP);
    CHANNEL_D->remove_from_channel("thiefgmc", TP);
    CHANNEL_D->remove_from_channel("thiefdmn", TP);
    destruct(t);

    /* No bless no more... */
    t = present("skert_bless",TP);
    if (t) destruct(t);

    TP->move_player("out#" + GDIR2 + "rooms/temple");

    write_file(GDIR+"RESIGN_LOG",
      ctime(time()) + ": " + Realname(TP) + " RESIGNED\n");

    return 1;
}

int
fixtitle(string str) {
    string tit;

    if (!TP) return 0;
    if (!is_thief(TP)) return 0;
    if (TP->query_level() > 25) {
	write("You don't need your title to be fixed.\n");
	return 1;
    }

    tit = get_new_title((int)TP->query_level() - 1);

    if (str != "ok") {
	write("Your new title would be \"" + tit + "\".\n"+
	  "Type 'fixtitle ok' to do the actual change.\n");
	return 1;
    }

    write("Ok, your title is now \"" + tit + "\".\n");
    TP->set_title(tit);

    return 1;
}

status west()
{
    if (!TP->is_npc() &&
      !present("tmark", TP)) return 0;

    /*    write("The door is always open for you.\n"); */
    TP->move_player("west#" + GDIR + "rooms/entry");

    return 1;
}

void
goppa()
{
    pulled = 0;

    tell_room(this_object(),
      "The lever shifts into up position.\n");
}

int query_is_start_location() {
    if (present("tmark",this_player())) return 1;
    write("You aren't a thief!\n");
}

void
cant_msg(string who, string msg)
{
    object *inv, ob;
    string tmp;
    int i;

    if (!who || !msg) return;

    tmp = "[Thf " + who + "] " + msg + "\n";

    inv = users();

    for (i = (sizeof(inv) - 1); i >= 0; i--)
    {
	if ((ob = present("tmark",inv[i])) && ob->query_cant())
	{
	    inv[i]->tell_me(tmp);
	}
    }
}

nomask void
thief_log(string fn, string x)
{
    string tmp1, tmp2;

    if (!x || !fn || !stringp(fn)) return;

    if (sscanf(fn, "%s..%s", tmp1, tmp2) == 2) return;

    if (file_size(GDIR + fn) > 256000)
    {
	rm(GDIR + fn + ".old");
	rename(GDIR + fn, GDIR + fn + ".old");
	write_file(GDIR + fn,
	  "Started new log file at " + ctime(time()) + "\n");
    }

    catch(write_file(GDIR + fn, x));
}

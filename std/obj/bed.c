// by Throdax, Robocop, Rastlin @GENESIS
// Nice, but needed lots of fixing. /Graah
// Edited, fixed, and improved! - Lysar 8/92
// Fixed again a LOT /Graah 22-Jul-96
// Now any number of people can enter this bed!
// 25-Jul-96 / Graah: Now in /lib/bed.c for public use.
// 26-Jul-96 / Graah: Lots of fixes again!
// 28-Jul-96 / Graah:
//  Now this is really SOMETHING! The players can now become
//  pregnant! MALES TOO! Who said this world has the same rules
//  as our world :)
// 28-Aug-95 / Graah: They were cheating with monsters. Not any more.
// 25-Oct-96 / Graah: total invis are not really "in" bed any more.
//  1-Jul-97 / Graah: Beds are not PK areas, ever.
// 16-Sep-97 / Graah: Must enter the bed before can do things.
//  6-Feb-98 / Chopin: tell_here() is now inherited from /basic/room/tell_here
// 26-Apr-98 / Graah: If present("chastity", partner), can't penetrate.
//		      There will be a chastity belt, and perhaps spells.

inherit "/basic/id";
inherit "/basic/room/tell_here";

#include <room_defs.h>
#include <weatherdesc.h>
#include <npc_defs.h>
#include <invis_levels.h>

#define TP this_player()
#define ORGASM (15 + random(6))

mapping ppl;
string ldesc;

static object cob;

status orgasm_fcn(object who, object target, string where);

void
pregnancy(object mother, object father)
{
    object ob;

    if (!mother || !father) return;
    if(present4("pregnancy", mother, mother, IL_TOTALLY_INVISIBLE)) return;

    // Here we check if mother becomes pregnant!
    if (random(100) > 23) return;
#ifndef NO_TROJAN
    return;
#endif

    ob = clone_object("/lib/pregnancy");
    move_object(ob, mother);
    ob->set_father((string)father->query_real_name());
    ob->set_father_race((string)father->query_race());
    ob->set_counter(1);

    log_file("PREGNANCY",
      (string)mother->query_real_name() +
      " by " + (string)father->query_real_name() + " at " +
      ctime(time()) + "\n");
}

mixed
query(int x)
{
    if (member(({ OUT_CITY , PREVENT_TELEPORT_TO,
	  ALLOW_TELEPORT_FROM, PREVENT_TELEPORT,
	  ROOM_GUILD, NO_PK
	}), x) != -1) return 1;

    if (x == ROOM_PREVENT_WHERE) return 0;

    if (x == ROOM_WD) return WD_INDOORS;

    return 0;
}

status
check_bed(object ob)
{
    mapping nppl;
    object *inv;
    string *n;
    status ok, yep;
    int i;

    if (!ppl) { ppl = ([ ]); return 0; }

    inv = all_inventory(this_object());

    if ((i = sizeof(inv)) < 1)
    {
	set_short("Bed");
	set_long(ldesc);
	return 0;
    }

    ok = yep = 0; n = ({ }); nppl = ([ ]);

    for (i--; i >= 0; i--)
    {
	if (living(inv[i]) &&
	  (int)inv[i]->query_invis() < IL_TOTALLY_INVISIBLE)
	{
	    if (ob && ob == inv[i]) ok = 1;
	    n += ({ capitalize((string)inv[i]->query_real_name()) });
	    nppl += ([ inv[i]: ppl[inv[i]] ]);
	yep++;
    }
}

ppl = nppl;

if (yep)
{
    set_short("Bed with " + implode(n, ", ", " and ") + " in it");
    set_long(ldesc + " " + implode(n, ", ", " and ") + " " +
      (i > 1 ? "are" : "is") +
      " currently in the bed.");
}
else
{
    set_short("Bed");
    set_long(ldesc);
}

return ok;
}

void
create()
{
    set_name("bed");
    set_short("Bed");
    ldesc = "This bed is large and spacious. \
Fluffy pillows rest on top of silky satin sheets. \
The bed looks as if it made for just one purpose. \
Try \"help bed\" to see what exciting things you can do. \
This bed is so large that many people could enter it at the same time!";
    set_long(ldesc);
}

#define Add(x,y) add_action(x,y)

void
init()
{
#if 0
    // Stupid necromancer shit crashes the game. How nice.
    if (environment()
      && this_player()
      && (environment(this_player()) == this_object())
      && this_player()->is_npc())
    {
	write((string)this_player()->query_name()
	  + " is kicked out of the bed.\n");
	move_object(this_player(), environment());
	return;
    }
#endif

    if (environment()) check_bed(0);

    Add("enter_fcn", "enter"); Add("exit_fcn", "exit");
    Add("exit_fcn", "leave");
    Add("leave_fcn", "north"); Add("leave_fcn", "south");
    Add("leave_fcn", "west"); Add("leave_fcn", "east");
    Add("leave_fcn", "northeast"); Add("leave_fcn", "southwest");
    Add("leave_fcn", "northwest"); Add("leave_fcn", "southeast");
    Add("leave_fcn", "down"); Add("leave_fcn", "up");
    Add("help_fcn", "help");

    if (environment(this_player()) != this_object()) return;

    Add("sleep_fcn", "sleep");
    Add("mlove_fcn", "mlove"); Add("lick_fcn", "lick");
    Add("kissall_fcn", "kissall"); Add("fuck_fcn", "fuck");
    Add("cmd_emote","emote");
    Add("suck_fcn", "suck");
    Add("hand_fcn", "hand");
    Add("tsuck_fcn","tsuck");
    Add("finger_fcn", "finger");

    if (TP->query_gender() == 2)
    {
	Add("sit_fcn", "sit"); Add("spread_fcn", "spread");
    }

    if (TP->query_gender() == 1)
    {
	Add("assfuck_fcn", "assfuck");
	Add("bre_fuck", "bfuck");
    }

}

status
cmd_emote(string str)
{
    if (!check_bed(TP)) return 0;

    if (!str) { notify_fail("Emote what?\n"); return 0; }

    environment()->tell_here((string)TP->query_name() +
      " " + str);
    tell_here((string)TP->query_name() +
      " " + str);
    return 1;
}

status
get()
{
    write("You can't lift the bed! It's too heavy for you!\n");
    return 0;
}

int query_value() { return 0; }
int query_weight() { return 1000; }

status
sleep_fcn()
{
    if (!(check_bed(TP))) return 0;

    TP->tell_me("You're sleeping....ZZZzzzzz");
    environment(TP)->tell_here((string)TP->query_name() +
      " is sleeping soundly.", ({ TP }));
    return 1;
}

status
mlove_fcn(string str)
{
    object ob, m, f;

    if (!check_bed(TP)) return 0;

    if (!str)
    {
	notify_fail("Make love to whom?\n");
	return 0;
    }

    if (!(ob = present(lower_case(str), this_object())))
    {
	TP->tell_me("But there's no one called " + str + " here!");
	return 1;
    }

    if (TP == ob) { TP->tell_me("That's impossible!"); return 1; }

    if ((int)TP->query_gender() == (int)ob->query_gender())
    {
	TP->tell_me("Well, this kind of stuff isn't available yet. Why \
don't you try with someone of the opposite sex...?");
	return 1;
    }

    if (TP->query_gender() == 1) { m = TP; f = ob; }
    else { m = ob; f = TP; }

    if ((cob = present4("chastity", ob, TP, IL_TOTALLY_INVISIBLE))
      || (cob = present4("chastity", TP, ob, IL_TOTALLY_INVISIBLE)))
    {
	m->tell_me("You are stopped by " + (string)cob->query_name() + "!");
	tell_here((string)m->query_name() + " is stopped by "
	  + (string)cob->query_name() + "!", ({ m }));
	return 1;
    }

    m->tell_me("You slide your massive cock into " +
      (string)f->query_name() +
      "'s tight, wet cunt.");
    f->tell_me((string)m->query_name() +
      " slides his massive cock into your tight, wet cunt.");
    m->tell_me((string)f->query_name() +
      " digs her nails into your back and pushes you in harder.");
    tell_here((string)m->query_name() +
      " slides his massive cock into " + (string)f->query_name() +
      "'s tight, wet cunt.", ({ f, m }));
    if (ppl[m]++ >= ORGASM) orgasm_fcn(m, f, "cunt");
    if (ppl[f]++ >= ORGASM) orgasm_fcn(f, m, 0);
    return 1;
}

status
fuck_fcn(string str)
{
    object ob;
    int i;

    if (!check_bed(TP)) return 0;

    if (!str)
    {
	notify_fail("Fuck whom?\n");
	return 0;
    }

    if (!(ob = present(lower_case(str), this_object())))
    {
	TP->tell_me("But there's no one called " + str + " here!");
	return 1;
    }

    if (TP == ob) { TP->tell_me("That's impossible!"); return 1; }

    if ((int)TP->query_gender() == (int)ob->query_gender())
    {
	TP->tell_me("Well, this kind of stuff isn't available yet. Why \
don't you try with someone of the opposite sex...?");
	return 1;
    }

    if ((cob = present4("chastity", ob, TP, IL_TOTALLY_INVISIBLE))
      || (cob = present4("chastity", TP, ob, IL_TOTALLY_INVISIBLE)))
    {
	TP->tell_me("You are stopped by " + (string)cob->query_name() + "!");
	tell_here((string)TP->query_name() + " is stopped by "
	  + (string)cob->query_name() + "!", ({ TP }));
	return 1;
    }

    if ((int)TP->query_gender() != 2)
    {
	TP->tell_me("You ram your dick into " +
	  (string)ob->query_name() + "'s hot pussy!");
	ob->tell_me((string)TP->query_name() +
	  " rams " + (string)TP->query_possessive() +
	  " dick into your pussy!");
	tell_here((string)TP->query_name() + " rams " +
	  (string)TP->query_possessive() +
	  " dick into " + (string)ob->query_name() +
	  "'s hot pussy!", ({ TP, ob }));
    }
    else
    {
	TP->tell_me("You take " + (string)ob->query_name() +
	  "'s dick slowly into your hot pussy...");
	ob->tell_me((string)TP->query_name() +
	  " takes your dick slowly into " +
	  (string)TP->query_possessive() +
	  " hot pussy...");
	tell_here((string)TP->query_name() + " takes " +
	  (string)ob->query_name() +
	  "'s dick slowly into " +
	  (string)ob->query_possessive() +
	  " hot pussy...", ({ TP, ob }));
    }

    if (random(10) > 5)
	tell_here((string)TP->query_name() + " grunts with pleasure!",
	  ({ TP }));
    else
	tell_here((string)TP->query_name() + " screams in ecstasy!",
	  ({ TP }));

    if (ppl[TP]++ >= ORGASM)
    {
	if ((int)TP->query_gender() != 2)
	    orgasm_fcn(TP, ob, "cunt");
	else
	    orgasm_fcn(TP, ob, 0);
    }

    return 1;
}

status
sit_fcn(string str)
{
    object ob, m, f;

    if (!check_bed(TP)) return 0;

    if (!str)
    {
	notify_fail("Sit on whom?\n");
	return 0;
    }

    if (!(ob = present(lower_case(str), this_object())))
    {
	TP->tell_me("But there's no one called " + str + " here!");
	return 1;
    }

    if (TP == ob) { TP->tell_me("That's impossible!"); return 1; }

    if ((int)TP->query_gender() == (int)ob->query_gender())
    {
	TP->tell_me("Well, this kind of stuff isn't available yet. Why \
don't you try with someone of the opposite sex...?");
	return 1;
    }

    if (TP->query_gender() == 1) { m = TP; f = ob; }
    else { m = ob; f = TP; }

    if ((cob = present4("chastity", ob, TP, IL_TOTALLY_INVISIBLE))
      || (cob = present4("chastity", TP, ob, IL_TOTALLY_INVISIBLE)))
    {
	m->tell_me("You are stopped by " + (string)cob->query_name() + "!");
	tell_here((string)m->query_name() + " is stopped by "
	  + (string)cob->query_name() + "!", ({ m }));
	return 1;
    }

    f->tell_me("You slide down on " + (string)m->query_name() +
      "'s mighty dick.");
    m->tell_me((string)f->query_name() + " jumps up and down on your \
dick, squeezing her cunt like a vice.");
    tell_here((string)f->query_name() + " jumps up and down on " +
      (string)m->query_name() + "'s mighty dick.", ({ m, f }));

    ppl[f] += 2; if (ppl[f] >= ORGASM) orgasm_fcn(f, m, 0);
    if (++ppl[m] >= ORGASM) orgasm_fcn(m, f, "cunt");

    return 1;
}

status
spread_fcn(string str)
{
    object ob, m, f;

    if (!check_bed(TP)) return 0;

    if (!str)
    {
	notify_fail("Spread on whom?\n");
	return 0;
    }

    if (!(ob = present(lower_case(str), this_object())))
    {
	TP->tell_me("But there's no one called " + str + " here!");
	return 1;
    }

    if (TP == ob) { TP->tell_me("That's impossible!"); return 1; }

    if ((int)TP->query_gender() == (int)ob->query_gender())
    {
	TP->tell_me("Well, this kind of stuff isn't available yet. Why \
don't you try with someone of the opposite sex...?");
	return 1;
    }

    if (TP->query_gender() == 1) { m = TP; f = ob; }
    else { m = ob; f = TP; }

    if ((cob = present4("chastity", ob, TP, IL_TOTALLY_INVISIBLE))
      || (cob = present4("chastity", TP, ob, IL_TOTALLY_INVISIBLE)))
    {
	m->tell_me("You are stopped by " + (string)cob->query_name() + "!");
	tell_here((string)m->query_name() + " is stopped by "
	  + (string)cob->query_name() + "!", ({ m }));
	return 1;
    }

    f->tell_me("You turn around " + (string)m->query_name() +
      ". Then you lie down upon him, \
spread your luscious legs, and slowly take him into your wet slit.");
    m->tell_me((string)f->query_name() + " turns you around and lies down \
upon you. Then she spreads her luscious legs, and takes you into her \
wet pussy.");
    tell_here((string)f->query_name() + " turns " +
      (string)m->query_name() + " around and lies down upon him. \
Then she spreads her luscious legs, and takes " +
      (string)m->query_name() + " into her wet pussy.", ({ m, f }));

    ppl[f] += 2; if (ppl[f] >= ORGASM) orgasm_fcn(f, m, 0);
    if (++ppl[m] >= ORGASM) orgasm_fcn(m, f, "cunt");

    return 1;
}

status
suck_fcn(string str)
{
    object ob;
    string her;

    if (!check_bed(TP)) return 0;

    if (!str)
    {
	notify_fail("Suck whom?\n");
	return 0;
    }

    if (!(ob = present(lower_case(str), this_object())))
    {
	TP->tell_me("But there's no one called " + str + " here!");
	return 1;
    }

    if (TP == ob) { TP->tell_me("That's impossible!"); return 1; }

    if (ob->query_gender() != 1)
    {
	TP->tell_me("But " + (string)ob->query_possessive() +
	  " is not a male!");
	return 1;
    }

    her = (string)TP->query_possessive();

    if (cob = present4("chastity", ob, TP, IL_TOTALLY_INVISIBLE))
    {
	TP->tell_me("You are stopped by " + (string)cob->query_name() + "!");
	tell_here(TP->query_name() + " is stopped by "
	  + (string)cob->query_name() + "!", ({ TP }));
	return 1;
    }

    TP->tell_me("You take " + (string)ob->query_name() +
      "'s cock in your mouth \
and start to suck him by running your luscious tongue all over \
the base, shaft and head and taking it deeper and deeper into \
your hot mouth.");
    ob->tell_me((string)TP->query_name() + " takes your dick into " + her +
      " mouth and sucks you...running " + her +
      " luscious tongue all over the \
base, shaft and head and taking it deeper and deeper into " + her +
      " hot mouth.");
    tell_here((string)TP->query_name() + " sucks " +
      (string)ob->query_name() + "'s dick!", ({ TP, ob }));
    ppl[ob] += 2; if (ppl[ob] >= ORGASM) orgasm_fcn(ob, TP, "mouth");
    return 1;
}

status
hand_fcn(string str)
{
    object ob;
    string her;

    if (!check_bed(TP)) return 0;

    if (!str)
    {
	notify_fail("Hand job on whom?\n");
	return 0;
    }

    if (!(ob = present(lower_case(str), this_object())))
    {
	TP->tell_me("But there's no one called " + str + " here!");
	return 1;
    }

    if (TP == ob) { TP->tell_me("That's impossible!"); return 1; }

    if (ob->query_gender() != 1)
    {
	TP->tell_me("But " + (string)ob->query_possessive() +
	  " is not a male!");
	return 1;
    }

    if (cob = present4("chastity", ob, TP, IL_TOTALLY_INVISIBLE))
    {
	TP->tell_me("You are stopped by " + (string)cob->query_name() + "!");
	tell_here((string)TP->query_name() + " is stopped by "
	  + (string)cob->query_name() + "!", ({ TP }));
	return 1;
    }

    her = (string)TP->query_possessive();

    TP->tell_me("You run your nails over " +
      (string)ob->query_name() + "'s gorgeous cock and then \
wrap your whole hand around it and begin to pull lovingly. " +
      (string)ob->query_name() + " moans in pleasure.");
    ob->tell_me((string)TP->query_name() + " runs " + her +
      " nails over your cock and then wraps \
every loving finger around it and begins to tug lovingly on it. \
Overcome, you moan in pleasure.");
    tell_here((string)TP->query_name() + " runs " + her + " nails over " +
      (string)ob->query_name() + "'s gorgeous cock and then \
begins to pull. " +
      (string)ob->query_name() + " moans in pleasure.",
      ({ ob, TP }));

    ppl[ob]++; if (ppl[ob] >= ORGASM) orgasm_fcn(ob, TP, "hand");
    return 1;
}

status
assfuck_fcn(string str)
{
    object ob, m, f;

    if (!check_bed(TP)) return 0;

    if (!str)
    {
	notify_fail("Fuck whom into ass?\n");
	return 0;
    }

    if (!(ob = present(lower_case(str), this_object())))
    {
	TP->tell_me("But there's no one called " + str + " here!");
	return 1;
    }

    if (TP == ob) { TP->tell_me("That's impossible!"); return 1; }

    if (TP->query_gender() != 1)
    {
	TP->tell_me("But you are not a male!");
	return 1;
    }

    if ((cob = present4("chastity", ob, TP, IL_TOTALLY_INVISIBLE))
      || (cob = present4("chastity", TP, ob, IL_TOTALLY_INVISIBLE)))
    {
	TP->tell_me("You are stopped by " + (string)cob->query_name() + "!");
	tell_here((string)TP->query_name() + " is stopped by "
	  + (string)cob->query_name() + "!", ({ TP }));
	return 1;
    }

    TP->tell_me("You go on fucking " +
      (string)ob->query_name() + " in the ass. You sink \
your thick cock in deeper and deeper into her wiggiling ass. \
You grab hold of those gorgeous globes and push with all of \
your might. " + capitalize((string)ob->query_pronoun()) +
      " screams in a combination of pain and ecstasy!");

    ob->tell_me((string)TP->query_name() +
      " goes on fucking you in the ass. He sinks his \
thick shaft deeper and deeper into your saucy ass. He grabs your \
ass in a blaze of passion and pushes with all of his might. You \
scream in a combination of pain and ecstasy!");

    tell_here((string)TP->query_name() +
      " fucks " + (string)ob->query_name() +
      " in the ass. " + (string)ob->query_name() +
      " screams in pain and ecstasy!", ({ TP, ob }));

    ppl[TP] += 2; if (ppl[TP] >= ORGASM) orgasm_fcn(TP, ob, "asshole");
    if (random(100) < 34)
	if (ppl[ob]++ >= ORGASM) orgasm_fcn(ob, TP, 0);

    return 1;
}

status
bre_fuck(string str)
{
    object ob;

    if (!check_bed(TP)) return 0;

    if (!str)
    {
	notify_fail("Fuck whom between the breasts?\n");
	return 0;
    }

    if (!(ob = present(lower_case(str), this_object())))
    {
	TP->tell_me("But there's no one called " + str + " here!");
	return 1;
    }

    if (TP == ob) { TP->tell_me("That's impossible!"); return 1; }

    if (TP->query_gender() != 1)
    {
	TP->tell_me("But you are not a male!");
	return 1;
    }

    if (ob->query_gender() != 2)
    {
	TP->tell_me("But " + (string)ob->query_name() + " is not female!");
	return 1;
    }

    if (cob = present4("chastity", TP, ob, IL_TOTALLY_INVISIBLE))
    {
	TP->tell_me("You are stopped by " + (string)cob->query_name() + "!");
	tell_here((string)TP->query_name() + " is stopped by "
	  + (string)cob->query_name() + "!", ({ TP }));
	return 1;
    }

    TP->tell_me("You turn " + (string)ob->query_name() +
      " around so that she lies on her back. \
After that you place yourself on her stomach. You put your dick \
between her breasts and squeeze them together. You then start to \
ram your dick between her full, creamy breasts.");

    ob->tell_me((string)TP->query_name() +
      " places himself upon you and puts his dick between \
your breasts. You see his monstrously thick dick pulsate as he rams \
it between your breasts. All you can think of is what it would be \
like to suck it...");

    tell_here((string)TP->query_name() +
      " places himself upon " + (string)ob->query_name() +
      " and starts to ram his dick between her full, creamy breasts.",
      ({ TP, ob }));

    ppl[TP] += 2; if (ppl[TP] >= ORGASM) orgasm_fcn(TP, ob, "breasts");
    if (random(100) < 34)
	if (ppl[ob]++ >= ORGASM) orgasm_fcn(ob, TP, 0);

    return 1;
}

status
tsuck_fcn(string str)
{
    object ob;

    if (!check_bed(TP)) return 0;

    if (!str)
    {
	notify_fail("Suck whose breasts?\n");
	return 0;
    }

    if (!(ob = present(lower_case(str), this_object())))
    {
	TP->tell_me("But there's no one called " + str + " here!");
	return 1;
    }

    if (TP == ob) { TP->tell_me("That's impossible!"); return 1; }

    if (ob->query_gender() != 2)
    {
	TP->tell_me("But " + (string)ob->query_name() + " is not female!");
	return 1;
    }

    TP->tell_me("You rub your hands over " + (string)ob->query_name() +
      "'s creamy breasts. Then you flick her nipples with \
your tongue, finally engulfing and sucking all of the luscious \
titty flesh.");

    ob->tell_me((string)TP->query_name() + " runs " +
      (string)TP->query_possessive() + " hands \
over your creamy, saucy breasts. Kneading them lightly, " +
      (string)TP->query_pronoun() +
      " flicks out " + (string)TP->query_possessive() +
      " tongue and bathes your nipples in his saliva. \
Finally, " + (string)TP->query_pronoun() + " engulfs the entire \
things and sucks like possessed!");

    tell_here((string)TP->query_name() + " runs " +
      (string)TP->query_possessive() + " hands \
over " + (string)ob->query_name() + "'s creamy, saucy breasts. \
Then " + (string)TP->query_pronoun() + " starts to lick and suck them.",
      ({ ob, TP }));

    if (ppl[ob]++ >= ORGASM) orgasm_fcn(ob, TP, 0);

    return 1;
}

status
finger_fcn(string str)
{
    object ob;

    if (!check_bed(TP)) return 0;

    if (!str)
    {
	notify_fail("Finger fuck whom?\n");
	return 0;
    }

    if (!(ob = present(lower_case(str), this_object())))
    {
	TP->tell_me("But there's no one called " + str + " here!");
	return 1;
    }

    if (TP == ob) { TP->tell_me("That's impossible!"); return 1; }

    if (ob->query_gender() != 2)
    {
	TP->tell_me("But " + (string)ob->query_name() + " is not female!");
	return 1;
    }

    if (cob = present4("chastity", ob, TP, IL_TOTALLY_INVISIBLE))
    {
	TP->tell_me("You are stopped by " + (string)cob->query_name() + "!");
	tell_here((string)TP->query_name() + " is stopped by "
	  + (string)cob->query_name() + "!", ({ TP }));
	return 1;
    }

    TP->tell_me("You put your finger into " + (string)ob->query_name() +
      "'s wet pussy. You slowly start to move the finger in \
and out. As you do that, she starts to moan loudly.");

    ob->tell_me((string)TP->query_name() + " puts " +
      (string)TP->query_possessive() +
      " finger into your wet pussy. \
A wonderful feeling spreads in your abdomen as " +
      (string)TP->query_pronoun() +
      " starts to move the finger in and out...\n\
You start to moan loudly.");

    if (ppl[ob]++ >= ORGASM) orgasm_fcn(ob, TP, 0);

    return 1; 
}

status
lick_fcn(string str)
{
    object ob;

    if (!check_bed(TP)) return 0;

    if (!str)
    {
	notify_fail("Lick whom?\n");
	return 0;
    }

    if (!(ob = present(lower_case(str), this_object())))
    {
	TP->tell_me("But there's no one called " + str + " here!");
	return 1;
    }

    if (TP == ob) { TP->tell_me("That's impossible!"); return 1; }

    if (ob->query_gender() != 2)
    {
	TP->tell_me("But " + (string)ob->query_name() + " is not female!");
	return 1;
    }

    if (cob = present4("chastity", ob, TP, IL_TOTALLY_INVISIBLE))
    {
	TP->tell_me("You are stopped by " + (string)cob->query_name() + "!");
	tell_here((string)TP->query_name() + " is stopped by "
	  + (string)cob->query_name() + "!", ({ TP }));
	return 1;
    }

    TP->tell_me("You lick " + (string)ob->query_name() +
      "'s wet pussy.");
    ob->tell_me((string)TP->query_name() + " licks your wet \
pussy teasing you with his tongue...\n");

    tell_here((string)TP->query_name() + " licks " +
      (string)ob->query_name() + "'s pussy!", ({ TP, ob }));

    ppl[ob] += 2; if (ppl[ob] >= ORGASM) orgasm_fcn(ob, TP, 0);
    return 1;
}

status
kissall_fcn(string str)
{
    object ob;

    if (!check_bed(TP)) return 0;

    if (!str)
    {
	notify_fail("Kiss whom all over?\n");
	return 0;
    }

    if (!(ob = present(lower_case(str), this_object())))
    {
	TP->tell_me("But there's no one called " + str + " here!");
	return 1;
    }

    if (TP == ob) { TP->tell_me("That's impossible!"); return 1; }

    TP->tell_me("You kiss all over " + (string)ob->query_name() +
      "'s body.");
    ob->tell_me((string)TP->query_name() +
      " kisses you slowly and teasingly all over your body...");
    tell_here((string)TP->query_name() + " kisses " +
      (string)ob->query_name() + " all over " +
      (string)ob->query_possessive() + " body.", ({ TP, ob }));

    if (random(100) < 20)
	if (ppl[ob]++ >= ORGASM) orgasm_fcn(ob, TP, 0);
    return 1;
}

status
orgasm_fcn(object who, object target, string where)
{
    if (!check_bed(who)) return 0;

    switch (where)
    {
    case "hand":
	who->tell_me("AAAAAAHHH! Your cum sprays out while "+
	  (string)target->query_name() +
	  " pumps your dick hard with " +
	  (string)target->query_possessive() + " hand!");
	target->tell_me((string)who->query_name() +
	  " starts to cum! You pump " +
	  (string)who->query_possessive() +
	  " dick hard to the last drop of it!");
	tell_here("Cum sprays out of " + (string)who->query_name() +
	  "'s dick, which " + (string)target->query_name() +
	  " continues to pump with " +
	  (string)target->query_possessive() + " hand!",
	  ({ who, target}));
	break;
    case "breasts":
	who->tell_me("AAAAAAHHHHH! You spray cum onto " +
	  (string)target->query_name() +
	  "'s breasts, neck and face and mouth!");
	target->tell_me((string)who->query_name() +
	  " sprays cum onto your breasts, neck, face and mouth!");
	tell_here((string)who->query_name() +
	  " sprays cum onto " + (string)target->query_name() +
	  "'s breasts, neck and face and mouth!", ({ who, target }));
	break;
    case "asshole":
	who->tell_me("AAAAAAHHHHH! You squirt your hot cum into " +
	  (string)target->query_name() + "'s tight asshole!");
	target->tell_me("Your asshole is filled with squirts of " +
	  " hot cum of " + (string)who->query_name() + "!");
	tell_here((string)who->query_name() +
	  " screams in ecstasy as " + (string)who->query_pronoun() +
	  " cums into " + (string)target->query_name() +
	  "'s tight asshole!", ({ who, target }));
	// They will LOVE THIS!!!!
	pregnancy(target, who);
	break;
    case "cunt":
	who->tell_me("You start to spray cum into " +
	  (string)target->query_name() + "'s red hot cunt! "+
	  "The orgasm seems to last forever...!");
	target->tell_me((string)who->query_name() +
	  " sprays cum into your hot pussy! AAAAHHHHHHH!");
	tell_here((string)who->query_name() +
	  " bursts into a wonderful orgasm as " +
	  (string)who->query_pronoun() +
	  " cums into the hot pussy of " + (string)target->query_name() +
	  "!", ({ who, target }));
	pregnancy(target, who);
	break;
    default:
	if (who->query_gender() == 1)
	{
	    who->tell_me(
	      "You burst into a wonderful orgasm. Quickly you whip \
out your steaming prick and spray your thick, gooey cum everywhere!");
	    tell_here((string)who->query_name() + " bursts into a \
wonderful orgasm as he takes a hold of his cock and sprays gooey, creamy cum \
everywhere!", ({ who }));
	}
	else
	{
	    who->tell_me("You burst into a wonderful orgasm. You feel your \
whole body shake! It seems to last forever...");
	    tell_here((string)who->query_name() + " bursts into a \
wonderful orgasm!", ({ who }));
	}
	break;
    }

    ppl[who] = -(random(6));

#if 0

    /* Necros - I DONT THINK SO. =)

Yeah, and you forgot to tell us why you don't think so. Perhaps because
necromancer's guild has an extremely stupid bed which is entered by
a NPC! This bed was not meant for NPCs at all. This would have worked
fine with players.

      if (target && target->query(LIV_IS_PLAYER))
	who->heal_self(20 + random(10));
    */
    // How about this, since people can abuse this bed into oblivion.
    who->add_fp( -(who->query_fp()/2) );

#endif

    return 1;
}

status
enter_fcn(string str)
{
    check_bed(0);

    if (!str || str != "bed") return 0;

    if (present(TP, this_object()))
    {
	notify_fail("You already are in the bed.");
	return 0;
    }

    // if()...
    //     TP->tell_me("The bed is full, come back later!");
    //     return 1;
    //

    move_object(TP, this_object());
    TP->tell_me("You enter the bed, and discover "+
      "that is was bigger than you thought.");

    if ((int)TP->query_invis() < IL_TOTALLY_INVISIBLE)
    {
	tell_here(capitalize(TP->query_real_name())+
	  " enters the bed.", ({ TP }));
	ppl += ([ TP: 0 ]);
    }

    check_bed(0);

    return 1;
}

status
exit_fcn(string str)
{
    if (!(check_bed(TP))) return 0;

    if (!id(str))
    {
	notify_fail("Eh...exit " + (str ? str : "what") + "?\n");
	return 0;
    }

    if (!present(TP, this_object())) {
	TP->tell_me("You can't exit the bed when you are not in it.");
	return 1;
    }

    TP->tell_me("You step out of the bed. It's cold on the floor.");

    if ((int)TP->query_invis() < IL_TOTALLY_INVISIBLE)
	environment()->tell_here(TP->query_name() +" steps out of the bed.",
	  ({ TP }));

    move_object(TP, environment());

    check_bed(0);

    return 1;
}

status
help_fcn(string str)
{
    if (!str || str != "bed") return 0;

    if (TP->query_gender() != 2) {
	TP->tell_me("    V3.1      These commands are available:\n"+
	  " ----------------------------------------------------------------------------\n"+
	  "     emote           |   As per regular emote.\n"+
	  "     sleep           |   Go to sleep.\n"+
	  "     mlove           |   Fuck a girl in this bed. (If there are any.)\n"+
	  "     bfuck           |   Fuck a girl between her breasts.\n"+
	  "     lick            |   Lick a girl in this bed.\n"+
	  "     fuck            |   Fuck a girl.\n"+
	  "     tsuck           |   Suck a girl's tits.\n"+
	  "     finger          |   Not as the usual 'finger' command.\n"+
	  "     kissall         |   Kiss someone all over the body (hetero/gay).\n"+
	  "     assfuck         |   Fuck someone into ass (gay/hetero).\n"+
	  "     suck            |   Suck a man (gay).\n"+
	  "     hand            |   Give a man a hand job (gay).\n"+
	  "\n"+
	  "   Also remember the usual feeling-commands: Blush, cuddle, fondle,\n"+
	  "      french, groan, grope, hug, kiss, love, snore, snuggle, and wiggle.\n");
	return 1;
    }

    TP->tell_me("    V3.1      These commands are available for women:\n"+
      " ----------------------------------------------------------------------------\n"+
      "     emote           |   As per regular emote.\n"+
      "     sleep           |   Go to sleep.\n"+
      "     mlove           |   Fuck a man in this bed.\n"+
      "     sit             |   Slowly slide down on a man.\n"+
      "     spread          |   Spread your legs, and show him.\n"+
      "     suck            |   Suck him.\n"+
      "     hand            |   Give a man a hand job.\n"+
      "     fuck            |   Fuck a man.\n"+
      "     kissall         |   Kiss someone all over the body (hetero/gay).\n"+
      "     tsuck           |   Suck a girl's tits (gay).\n"+
      "     lick            |   Lick a girl in this bed (gay).\n"+
      "     finger          |   Not as the usual 'finger' command (gay).\n"+
      "\n"+
      "   Also remember the usual feeling-commands: Blush, cuddle, fondle,\n"+
      "      french, groan, grope, hug, kiss, love, snore, snuggle, and wiggle.\n");
    return 1;
}

status
leave_fcn()
{
    if (present(TP, this_object()))
    {
	TP->tell_me("Exit the bed first.");
	return 1;
    }

    return 0;
}

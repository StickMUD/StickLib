/*
 * /bin/daemons/criticalsd.c
 *
 * Criticals daemon
 *
 * This module contains functions which make various critical hits.
 *
 * Copyright (c) 1993 The Stickmud Project,
 * All rights reserved.
 *
 */

#include <attack.h>

inherit "/lib/find_name";

static mapping bps;

/* Prototypes */
static void init_bodies();

/*
 * Function name: critical_hit
 * Description  : Make a critical hit on someone
 * Arguments    :
 *   object victim  : Who is to be hit
 *   object hitter  : Which object is hitting the victim (weapon, monster...)
 *   int damage     : How much damage was delivered by the original attack?
 *		      Not counting victim AC etc.
 *   int type       : (Optional) Type of hit (DT_xxxx), see attack.h
 *   int bonus      : (Optional) Make it worse (>0) or less worse (<0)
 * Returns      : Severity (0..100) of the hit we caused
 * Notes        : Bonus should be a very rare thing, and range from -50 to 50
 *   If there is function extra_critical() in your weapon (or hitter object),
 *   that is called instead of using the critical hits in this daemon.
 *   It gets the same arguments, and should return severity.
 *
 *   You don't have to give hitter object, but it is recommended.
 *
 *   If the attack was just a critical, not a damage-causing attack,
 *   you can leave damage argument out, or give it as zero.
 *
 */
nomask varargs int
critical_hit(object victim, object hitter, int damage, int type, int bonus)
{
    int i, t;
    string rt, *wrd, bp, hname, vname;

    /* Critical hits on non-living things not yet possible */
    if (!victim || !living(victim)) return 0;

    if (hitter) {
        if (function_exists("extra_critical", hitter))
	    return (int)hitter->extra_critical(
		victim, hitter, damage, type, bonus);
    }

    hname = find_name(hitter, (hitter && living(hitter) ? 1 : 0));
    vname = find_name(victim, 1);

    /* Default is a blunt attack */
    if (!type) t = DT_CRUSH;
    else t = type;

    /* Try to decide which kind of creature we are hitting */
    rt = (string)victim->query_race();
    if (!rt) rt = (string)victim->query_short();
    if (!rt) rt = (string)victim->short();
    if (!rt) rt = (string)victim->query_name();
    if (rt && rt == "Someone") rt = (string)victim->query_real_name();
    if (!rt) rt = "human";
    rt = lower_case(rt);

    wrd = explode(rt, " ");

    if (!wrd || sizeof(wrd) < 1) wrd = ({ rt });

    rt = 0;

    for (i = sizeof(wrd) - 1; i >= 0; i--) {
        if (wrd[i] == "skeleton") {
	    rt = "skeleton";
	    break;
	}

        if (member_array(wrd[i], ({ "orc", "half-orc", "elf", "giant",
	     "human", "hobbit", "dwarf", "titan", "zombie",
	     "ghoul", "skeleton" })) != -1) {
	    rt = "human";
	    break;
	}

	if (member_array(wrd[i], ({ "deer", "mouse", "rat", "goose",
	    "bear", "unicorn", "buffalo", "horse", "camel",
	    "sheep", "goat", "wolf", "dog", "pig", "aardvark" })) != -1) {
	    rt = "animal";
	    break;
	}

	if (member_array(wrd[i], ({ "demon", "daemon", "devil" })) != -1) {
	    rt = "demon";
	    break;
	}

	if (member_array(wrd[i], ({ "dragon", "wyvern", "griffon" })) != -1) {
	    rt = "dragon";
	    break;
	}

	if (member_array(wrd[i], ({ "roc", "bird", "cockatrice" })) != -1) {
	    rt = "bird";
	    break;
	}

	if (member_array(wrd[i], ({ "snake", "remorhaz" })) != -1) {
	    rt = "snake";
	    break;
	}

	if (member_array(wrd, ({ "ant", "beetle", "cockroach" })) != -1) {
	    rt = "insect";
	    break;
	}

	if (member_array(wrd[i], ({ "spider" })) != -1) {
	    rt = "arachnid";
	    break;
	}

	if (member_array(wrd[i], ({ "cat", "lion", "tiger", "panther",
	    "puma" })) != -1) {
	    rt = "feline";
	    break;
	}

	/* We hope "cube" is "gelatinous cube"... */
	if (member_array(wrd[i], ({ "slime", "jelly", "ooze",
	     "amoeba", "pudding", "cube" })) != -1) {
	    rt = "jelly";
	    break;
	}

	/* Todo: fish, octopus, hydra, amoeba, jellies etc. */
    }

    /* Default: human */
    if (!rt) rt = "human";

    if (!bps) init_bodies();

    bp = bps[rt][(random(sizeof(bps[rt]) - 1))];

/* The rest is just a test */

    write(sprintf("%-=78s", "Race: " + rt + ", Parts: " +
	implode(bps[rt], ", ", " and ") + ".\n"));

    if (hitter) {
        tell_object(hitter, "You hit " + vname +
	    " into " + (string)victim->query_possessive() +
	    " " + bp + "!\n");
	tell_object(victim, hname +
	    " hits you into your " + bp + "!\n");
    }

    return 1;
}

static void
init_bodies()
{
    bps = ([ ]);

    bps["bird"] = ({ "left wing", "right wing", "left leg", "right leg",
		       "beak", "tail", "back", "chest", "stomach" });

    /* Human, demon and angel are quite similar */
    bps["human"] = ({ "neck", "back", "chest", "stomach", "left eye",
			"jaw", "nose", "forehead", "left cheek",
			"right cheek", "left side", "right side",
			"left arm", "right arm", "left hand", "right eye",
			"right hand", "groin", "left leg", "right leg",
			"pelvis", "right shoulder", "left shoulder" });

    /* Demon has hooves & tail (and a pitchfork...) */
    bps["demon"] = bps["human"] +
        ({ "left wing", "right wing", "left hoof", "right hoof", "tail" });

    /* Demon done, human has feet */
    bps["human"] += ({ "left foot", "right foot" });

    /* Angels are otherwise like humans but they got wings */
    bps["angel"] = bps["human"] + ({ "left wing", "right wing" });

    bps["dragon"] = ({ "left wing", "right wing", "neck", "back",
			 "left front leg", "right front leg", "tail",
			 "left hind leg", "right hind leg", "chest",
			 "jaw", "nose", "forehead", "stomach" });

    /* They had 8 legs? (got an E in biology :-)... */
    bps["insect"] = ({ "head", "body",
			 "first leg", "second leg", "third leg",
			 "fourth leg", "fifth leg", "sixth leg",
			 "seventh leg", "eight leg" });

    /* And these had 6 legs? */
    bps["arachnid"] = ({ "head", "body",
			   "first leg", "second leg", "third leg",
			   "fourth leg", "fifth leg", "sixth leg",
			 "eyes", "stomach" });

    bps["snake"] = ({ "head", "body", "tail" });

    /* Meow! */
    bps["feline"] = ({ "head", "body", "tail", "neck", "back",
			 "left front paw", "right front paw",
			 "left hind paw", "front hind paw",
			 "jaw", "nose", "forehead", "stomach" });

    /* Is "pseudopod" correct word here? */
    bps["jelly"] = ({ "body", "pseudopod" });

    /* Todo: (perhaps) some have hooves, some cloven hooves, some paws,
       horn(s), ridges etc. */
    bps["animal"] = ({ "head", "body", "tail", "neck", "back",
			 "left front leg", "right front leg",
			 "left hind leg", "right hind leg",
			 "jaw", "nose", "forehead", "stomach" });

    /* And the neckbone connected to backbone, ... */
    bps["skeleton"] = ({ "skull", "left shoulder", "right shoulder",
			   "left ribs", "right ribs", "collarbone",
			   "backbone", "pelvis", "left thighbone",
			   "neckbone", "right tighbone",
			   "left legbone", "right legbone",
			 });
}

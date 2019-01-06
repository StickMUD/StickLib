/****************************************************************
*								*
* Standard module defining functions that handle moving of the	*
* living objects. Living.c includes/inherits this.		*
*								*
* V1.1 17-jun-96						*
* Changes: 17-jun-96, Doomdark.					*
*								*
* Graah 30-Nov-94: Pets follow again.				*
* Doomdark 05-may-95: Removed 8 funcs (setmin/query_msgin etc.)	*
*	From now on, use 2 funcs: set_move_msg & query_move_msg	*
* Doomdark 24-nov-95: Now tests if enemy can move before hitting*
*	if we flee.						*
* Doomdark 23-dec-95: Due to rewrite of combat, patched this	*
*	module.							*
* Doomdark 13-jan-96: Totally rewrote light handling.		*
* Doomdark 17-jun-96: Now shows items like "3 x An empty bottle"*
*	like "look"-command does...				*
* 4-Nov-96 / Graah: Anti-client limit: over 10 moves in one	*
*	round means the cheater is using a client.		*
*								*
* NEW:								*
* - Now leave & arrive-msgs will only get told to those		*
*  livings that can see it.					*
* - With move desc 0 no  msg is printed.			*
* - Now checks nighttime other than it used to.			*
* - Spell casting and other queued player initiated events	*
*   are here, so that movement and interuption can be easily	*
*   worked in.
*								*
*****************************************************************/


static	closure	event_pending,	// new spell skill interface
cancel_pending;	// optional closure to handle clean up
string msgin, msgout, mmsgout, mmsgin;	//Msgs when arriving and leaving

#define	TOTAL_LIGHT	(int)environment()->query_total_light()
#include <eventqueue.h>

// Pets follow. TODO: Can some pets teleport after their master?
// This is a macro until someone fixes those move routines a bit! //Graah

#define PET_FOLLOW \
  if (!is_teleport && from && \
      pointerp((inv = (object *)this_object()->query_pets()))) \
    { \
      for (i = (sizeof(inv) - 1); i >= 0; i--) \
	{ \
	  if (inv[i] && \
	      living(inv[i]) && \
	      inv[i]->query_follow_master() && \
	      present(inv[i], from)) { \
	    inv[i]->tell_me(sprintf("You follow %s.",\
	      (string)((object)inv[i]->query_master())->query_name())); \
	    inv[i]->move_player(dir, dest); \
	   } \
	} \
    }

// And then some defines so that we can test this without
// living.c

#ifndef LIVING_C

#include "/basic/living/living_private.h"
#include <room_defs.h>
#include <player_defs.h>
#include <daemons.h>
#include <conditions.h>
#include <tell_me.h>
#include <client_defs.h>

static object *hunted;
static object *attacker;
static int attackers;
status is_invis, ghost, dead, just_moved;
int Dex;
static int liv_Flags;

static int cons_moves;

string Possessive();
status query_can_move();
status query_can_see();
status query_condition(int x);
status query_sit();
status query_rest();
status query_ghost();
string query_race();
varargs status is_fighting(object o);
void set_sit(status a, status b);
void set_rest(status a, status b);
int query_coder_level();
varargs int tell_me(string s, status a, status b, mixed d, mixed e, mixed f);
void attack_object(object o, status no_hit);
object query_attack();
varargs string query_name(int m, object o);
int query(int x);
int query_stat(int x);
varargs int set(int x, int y);
void start_hunting(object o);
varargs int test_dark2(int ll, int n, status liv);
void purge_hunt();
#endif
status add_queued_event(closure cl, int time, closure cancel, mixed data);
closure query_queued_event();
void clean_queue(int reason, mixed data);

#ifndef	INVIS_LEVEL
int query_invis();
#define	INVIS_LEVEL query_invis()
#endif
#ifndef NAME_OF_GHOST
#define	NAME_OF_GHOST	"some mist"
#endif

#define	MAX_ITEMS	80

// NEW! Use these instead of setmout/min/mmout/mmin and
//	query_msgin/msgout/mmsgin/mmsgout

status
set_move_msg(string which, string what)
{
    switch (which) {
    case "mout":
	msgout = what; break;
    case "min":
	msgin = what; break;
    case "mmout":
	mmsgout = what; break;
    case "mmin":
	mmsgin = what; break;
    default: return 0;
    }
    return 1;
}

string
query_move_msg(string which)
{
    switch(which) {
    case "mout":
	if (!msgout) msgout = "leaves";
	return msgout;
    case "min":
	if (!msgin) msgin = "arrives";
	return msgin;
    case "mmout":
	if (!mmsgout) mmsgout = "suddenly vanishes into thin air";
	return mmsgout;
    case "mmin":
	if (!mmsgin) mmsgin = "arrives in a puff of smoke";
	return mmsgin;
    default: return "<ERROR>";
    }
}

// Internal func... Could be in-built to move_player but let's make
// it separate for time being. :-/
void
print_items(object me, mixed *items)
{
    int i, last, amount, same_amount;
    for (i = same_amount = amount = 1, last = 0; i < sizeof(items); i++) {
	if (!stringp(items[i])) continue;
	if (items[i] == items[last]) {
	    same_amount++;
	    items[i] = 0;
	    continue;
	} else if (same_amount > 1) {
	    items[last] = sprintf("%d x %s", same_amount, items[last]);
	    same_amount = 1;
	}
	amount++;	// Amount of lines we'll get...
	last = i;		
    }
    if (same_amount > 1) {
	items[last] = sprintf("%d x %s", same_amount, items[last]);
	same_amount = 1;
    }
    if (stringp(items[last]))
	items[last] += ".";
    if (amount > 20)
	me->tell_me(implode(items, ", ", " and "));
    else
	me->tell_me(implode(items, ".\n"));
}

/*
 * This routine is called from objects that moves the player.
 * Arguments:
 * Either 1 or 2; if only 1 used, the argument is "how#where";
 * otherwise 1st argument is the movement message, and the 2nd
 * either object or filename of the object, where player is to
 * be moved to.
 *
 * Special: direction "X" means teleport.
 *
 * NEW: direction "Vjumps east" means 'Foo jumps east.'
 *
 * New: Teleporting (desc "X" or none) succeeds always, otherwise
 * creature has to be able to move normally. //Graah
 *
 * New: Nights are dark too. //Graah
 * Note: Nights are rather kludgy now...
 *
 * NEW: direction 0 means no message gets printed; for example
 * this_player()->move_player(0, MY_AREA+"some_room");
 * Should only be used if you want to print message by yourself.
 *
 * NEW: cons_moves keeps a count on moves on this round.
 *
 */

// Change this if they really type moves this fast?
// 30-Sep-97 / Graah: Even without a client I could probably move
// more than 10 times per beat. So I'll rise it to 15 for now.
#define MAX_CONS_MOVES 15

varargs void
move_player(string dir_dest, mixed optional_dest_ob)
{
    string dir, dest, short_str, tmp, my_name;
    object ob, from;
    mixed x, y, z, inv;
    int i, night, is_light, see_level, outd;
    status is_teleport, own_verb;

    if (!optional_dest_ob) {
	if (!stringp(dir_dest) || sscanf(dir_dest, "%s#%s", dir, dest) < 2) {
	    tell_me("Move to bad dir/dest");
	    return;
	}
    } else {
	dir = dir_dest;
	dest = optional_dest_ob;
    }

    if (dir == "x" || dir == "X")
	is_teleport = 1;
    else if (stringp(dir) && sscanf(dir, "V%s", tmp)) own_verb = 1;

    if (!is_teleport && !query_can_move()) {
	tell_me("You try to move, but you can't!");
	return;
    }

    // Check the consecutive moves
    if (!is_teleport &&
      !(liv_Flags & F_LIV_IS_NPC) &&
      (cons_moves > MAX_CONS_MOVES))
    {
	if (random(100) < 80)
	    tell_me("Not so fast!");
	else tell_me("Using a client eh?");
	return;
    }

    cons_moves++;

    if (ghost) my_name = NAME_OF_GHOST;
    else if (!(my_name = query_name(0, this_object())))
	my_name = "Someone";

    if (query(LIV_UNDEAD))
	see_level = 7;
    else see_level = 4;

    night = liv_Flags & F_LIV_NIGHT;

    if (query_sit()) {
	tell_me("You stand up.");
	set_sit(0, 1);
	short_str = sprintf("%s stands up.\n", my_name);
    } else if (query_rest()) {
	tell_me("You stop resting.");
	set_rest(0, 1);
	short_str = sprintf("%s stops resting and clambers to %s feet.\n",
	  my_name, Possessive());
    }

    // WRONG!!! Trolls should see in the dark when others leave etc.
    // Also, other creatures having infra-vision can see us.
    // Ok, ok, ok... Let's change it then!

    if (dir && (!INVIS_LEVEL || ghost) && environment()
      && (i = sizeof(inv = all_inventory(environment()))) > 1) {

	// Let's not show leave-msg if player is invisible (except if he/she
	// is a ghost), or if we have no environment, or if no-one else is
	// here except us.

	is_light = (int) environment()->query_total_light();
	x = (int) environment()->query_light_level();
	outd = (int) environment()->query(ROOM_WD);
	y = 0;
	for (i--; i >= 0; i--) {
	    if (!living(inv[i]) || (object)inv[i] == this_object()) continue;
	    z = (int)inv[i] -> test_dark2(is_light, x, outd);
	    if (z < 0) z = -z;
	    if (z < see_level) continue;
	    if (!y) {
		y++;
		if (is_teleport)
		    tmp = query_move_msg("mmout");
		else if (!own_verb && (!(tmp = (string) environment() -> query_msgout(this_object()) )))
		    tmp = sprintf("%s %s", query_move_msg("mout"), dir);
		if (short_str) short_str = sprintf("%s%s %s.",
		      short_str, my_name, tmp);
		else short_str = sprintf("%s %s.", my_name, tmp);
	    }
	    inv[i] -> tell_me(short_str);
	}
    }

    // The hit-and-run cowards deserve a good solid attack in their
    // backs. This works both ways. //Graah

    if (!is_teleport && (ob = query_attack())
      && query_stat(ST_DEX) < (random(40) + random(80))
      && (object)ob->query_attack() == this_object()
      && ob->query_can_move()) {
	if (!(tmp = (string) ob -> query_name(0, this_object())))
	    tmp = "Someone";
	if (random(2))
	    tell_me(sprintf("As you %s, %s gets a chance to hit",
		random(2) ? "cowardly run away" : "try to flee in panic", tmp));
	else tell_me(sprintf("%s, and %s gets an opportunity to hit!",
		random(2) ? "You turn your back" : "You stumble in your retreat", tmp));
	ob -> tell_me(sprintf("%s tries to flee, giving you a chance \
to hit %s back!", my_name, Possessive()));
	ob->attack();

	// We can be dead and destructed now.

	if (!this_object() || dead || query_ghost()) return;
    }

    i = 0;
    purge_hunt();
    while (i < attackers) {
	if (attacker[i] && present(attacker[i], environment())) {
	    start_hunting(attacker[i]);
	    attacker[i] = 0;
	}
	i++;
    }

    from = environment();
    move_object(this_object(), dest);

    // No living should be destructed in init() but currently we have
    // the open_time test in church that does exactly this. We get
    // loads of shit to logfile. --Val

    if (!this_object() || !environment()) return;


    /* Let's remove combat commands from the queue. */
    if (liv_Flags & F_LIV_IS_PLAYER) {
	if (i = set(PLR_COMBAT_QUEUE_CLEAR)) {
	    tell_me(sprintf(
		"[%d command%s interrupted by moving.]", i, (i == 1) ? "" : "s"));
	}
    }

    is_light = (int) environment()->query_total_light();
    x = (int) environment()->query_light_level();
    outd = (int) environment()->query(ROOM_WD);
    short_str = 0;

    if (query_coder_level())
	tell_me(object_name(environment()));

    if (dir && (!INVIS_LEVEL || ghost)
      && (i = sizeof(inv = all_inventory(environment()))) > 1) {

	for (i -= 1; i >= 0; i--) {
	    if (!living(inv[i]) || (object)inv[i] == this_object()) continue;
	    z = (int)inv[i] -> test_dark2(is_light, x, outd);
	    if (z < 0) z = -z;
	    if (z < see_level) {
		inv[i]->tell_me("You hear someone or something arriving.");
		continue;
	    }
	    if (!short_str) {
		if (is_teleport)
		    tmp = query_move_msg("mmin");
		else tmp = (string) environment()->query_msgin(this_object());
		if (!tmp)
		    tmp = query_move_msg("min");
		if (!tmp) tmp = "arrives";
		if (!my_name) my_name = "Someone";
		short_str = sprintf("%s %s.", my_name, tmp);
	    }
	    inv[i] -> tell_me(short_str);
	}
    }

    if (!query_can_see()) {
	tell_me("You cannot see anything.", 0,
	  TELL_TYPE_ROOM_DESC | TELL_BEGIN | TELL_END, 0, 0);
	PET_FOLLOW  /* Pets must follow before returning! */
	return;
    }

    if (query(LIV_BRIEF))
	z = tell_me((string) environment()->short(5, this_object()) + ".", 0, 0,
	  TELL_TYPE_ROOM_DESC | TELL_BEGIN, 0, 0);
    // Will be used when all rooms use "query_short"!!! (never?)
    // else if (short_str = (string) environment()->query_short(0,this_object())
    //   tell_me(short_str);
    /* If we used query_long(), we'd get return value from tell_me()! */
    else if (this_player() == this_object())
	z = (int) environment() -> long(0, this_object());
    else z = 0;
    // Pets cause multiple looks otherwise
    /* ... Nice little pet bugs... (bed bugs?) */
    if ((i = sizeof(inv = all_inventory(environment()))) > 1) {
	see_level = test_dark2(is_light, x, outd);
	if (see_level < 0) see_level = -see_level;
	if (see_level > 3) {
	    if (query_condition(C_HALLUCINATING)) y = 1;
	    else y = 0;
	    if (i > MAX_ITEMS) {
		inv = inv[0..MAX_ITEMS - 1] + ({ "*** TRUNCATED ***" });
		i = MAX_ITEMS - 1;
	    } else i--;
	    // So; with see_level of 7 we see all the objects, with 4-6 only
	    // non-undead NPCs and players, otherwise nothing...
	    for(x = 0; i >= 0; i--) {
		ob = inv[i];
		if (ob == this_object() ||
		  (see_level < 7 && (!living(ob) || ob->query(LIV_UNDEAD)
		      || !ob->query(LIV_IS_PLAYER_OR_NPC)))) {
		    inv[i] = 0;
		    continue;
		}
		// We may be hallucinating. *GRIN*
		if (y && random(100) > 74) {
		    if (living(ob)) inv[i] = (string) HALLU_D->random_short(1);
		    else inv[i] = (string) HALLU_D->random_short(2);
		} else inv[i] = (string) ob->short(0, this_object());
		if(inv[i]) x++;
		// let's not increase the counter if we couldn't see it! - Chopin
	    }

	    if (x)
		print_items(this_object(), inv);
	}
    }


    if (z == TELL_RETURN_CLIENT_IN_USE) {
	if (z = (string) environment()->query(ROOM_SHORT_EXIT_LIST)) {
	    tell_object(this_object(), sprintf(
		CLIENT_TAG_BEGIN CLIENT_TAG_EXITS " %s" CLIENT_TAG_END
		CLIENT_TAG_BEGIN CLIENT_TAG_NEGATE CLIENT_TAG_RDESC CLIENT_TAG_END, z));
	} else {
	    tell_object(this_object(),
	      CLIENT_TAG_BEGIN CLIENT_TAG_EXITS CLIENT_TAG_END
	      CLIENT_TAG_BEGIN CLIENT_TAG_NEGATE CLIENT_TAG_RDESC CLIENT_TAG_END);
	}
    }

    // Look for enemies - after we have seen the place.
    // Perhaps we should check the see_level? *shrug*
    if (query_can_move() && hunted && sizeof(hunted))
	for(i=sizeof(hunted)-1; i >= 0; i--)
	    if (objectp(hunted[i]) && environment(hunted[i]) == environment()) {
		x = hunted[i];
		hunted[i] = 0;
		attack_object(x, 1);
		// Remember to prevent the hit!!! -+ Doomdark 04-jul-96 +-
	    }

    if(query_queued_event())
	clean_queue(INTR_MOVED, ({ this_object() }));
    // And the last thing is pets following us...
    PET_FOLLOW
}

status	add_queued_event(closure cl, int time, closure cancel, mixed data) {
    if(!closurep(cl) || !to_object(cl)) return QUEUE_FULL;
    if(closurep(event_pending) && to_object(event_pending))
	return QUEUE_FULL;
    event_pending	= cl;
    cancel_pending	= cancel;
    call_out(#'clean_queue, time, EVENT_COMPLETE, data);
    return EVENT_QUEUED;
}

closure query_queued_event() {
    return event_pending;
}

void clean_queue(int reason, mixed data) {
    remove_call_out(#'clean_queue);
    if(reason == EVENT_COMPLETE) {
	if(closurep(event_pending) && to_object(event_pending))
	    funcall(event_pending, data);
    }
    else {
	if(closurep(cancel_pending) && to_object(cancel_pending))
	    funcall(cancel_pending, reason, data);
    }
    cancel_pending = 0;
    event_pending = 0;
}

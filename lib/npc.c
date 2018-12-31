/****************************************************************
*								*
* Module:							*
*	Npc.c for Sticklib distribution. Base module on which	*
*	all non-player creatures need to be built on. Either	*
*	clone it (preferable) or inherit it and override	*
*	functions you need to.					*
* Version:							*
*	1.0a, Aug-98.						*
* Last update:							*
*	19-Aug-98 by Doomdark					*
*								'
* Modifications:						*
*								*
****************************************************************/
#include "/sys/configuration.h"

#include <npc_defs.h>

inherit LIVING_FILE;

#include <conditions.h>
#include <tell_me.h>
#include <room_defs.h>
#include <tune.h>

#define	NPC_C
// These are needed by fstring.c; makes it _much_ more efficient.
#define SSF_ID_FUNCS_EXIST
#define SSF_LIV_FUNCS_EXIST

#include "/basic/npc/set_n_query.c"

/****************************************************************
*								*
*			The global variables.			*
*								*
****************************************************************/

// These can now use SSF too! - Germ
static mixed *chat_head;	// Vector with all chat strings.
static int chat_chance;

static mixed *a_chat_head;	// Vector with all a_chat strings.
static int a_chat_chance;

private static mixed *talk_array;

static object dead_ob, init_ob;

static int random_pick, wander;
static private string *wander_dirs;

static int keep_hb;	// To let monsters keep their hb, even if
			// there are no players here //Frobozz
static int hb_left;	// How many rounds we'll still have hb...

static int spell_chance, spell_dmg;
static mixed spell_mess1, spell_mess2;
static int spell_dtype;
static closure spell_closure;

static mixed *breath_attack;

// 1st element: Breath damage
// 2nd element: Breath chance
// 3rd element: Breath message to victim
// 4th element: Breath message to others

static int LastHBTime;	// time() of last hb

// NEW! Directions we'll block:
static mapping blocked_exits;

// To speed things up...
static closure InitFunc;

// Prototypes

void random_move();
void set_hp(int x);
void set_sp(int x);
void init_follow(object attacker, int flag);
void catch_tell(string s);
void do_spell_message(object victim, int real_dmg, int orig_dmg);

#include "/basic/npc/tell_me.c"
#include "/basic/npc/command_driver.c"

// Now, let's define healing formula for NPCs. -+ Doomdark +-
// Notice that when poisoned, NPC will lose hps!

// NEW! Let's also update 'age' variable; this is needed in order to
// get conditions work.

#define	CHECK_HEAL	if (LastHBTime) {\
			 amount = (time() - LastHBTime) / 2;\
			 age += amount;\
			 if ((hit_point < max_hp) || (spell_points < max_sp)) {\
			  if (!query_condition(C_POISONED)) {\
				if (healRate) {\
					if (healRate > 0)\
						amount *= healRate;\
					else\
						amount /= (-healRate);\
				} else {\
					amount = amount * (5 + level / 5) / 5;\
					amount /= (INTERVAL_BETWEEN_HEALING);\
					add_hp(amount);\
					add_sp(amount);\
				}\
			  } else {\
			  	amount = amount * (10 - level / 5) / 10;\
				amount /= (INTERVAL_BETWEEN_HEALING);\
			    if (query_hp() > amount)\
				 add_hp(-(amount / 2 + random(amount / 2 + 1)));\
			  }\
			 }\
			 LastHBTime = 0;\
			}

#define	CHECK_HEAL2	if ( LastHBTime && (amount = (time() - LastHBTime) / 2)) {\
			 age += amount;\
			 if ((hit_point < max_hp) || (spell_points < max_sp)) {\
			  if (!query_condition(C_POISONED)) {\
				if (healRate) {\
					if (healRate > 0)\
						amount *= healRate;\
					else\
						amount /= (-healRate);\
				} else {\
					amount = amount * (5 + level / 5) / 5;\
					amount /= (INTERVAL_BETWEEN_HEALING);\
					add_hp(amount);\
					add_sp(amount);\
				}\
			  } else {\
			  	amount = amount * (10 - level / 5) / 10;\
				amount /= (INTERVAL_BETWEEN_HEALING);\
			    if (query_hp() > amount)\
				 add_hp(-(amount / 2 + random(amount / 2 + 1)));\
			  }\
			 }\
			 LastHBTime = time();\
			}

#define	NPC_START_HB	if (keep_hb < 0)\
				hb_left = -keep_hb;\
			if (!(npc_Flags & F_NPC_HB_IS_ON)) {\
				configure_object(this_object(), OC_HEART_BEAT, 1);\
				npc_Flags |= F_NPC_HB_IS_ON;\
				CHECK_HEAL }

#define	NPC_STOP_HB	hb_left = 0;\
			if (npc_Flags & F_NPC_HB_IS_ON) {\
				configure_object(this_object(), OC_HEART_BEAT, 0);\
				npc_Flags &= (~F_NPC_HB_IS_ON);\
				LastHBTime = time();\
			}
			

#define	_BLOCK_ALL		1
#define	_BLOCK_LEVEL		2
#define	_BLOCK_RACE		3
#define	_BLOCK_NAME		4
#define	_BLOCK_GUILD		5
#define	_BLOCK_ALIGNMENT	6
#define _BLOCK_CLAN            7

#define	STOP_FOLLOWING		0
#define	START_FOLLOWING		1

#define	_FOLLOW_OB		0
#define	_FOLLOW_DELAY		1
#define	_FOLLOW_COUNT		2
#define	_FOLLOW_COUNT_MAX	3
#define	_FOLLOW_BUF_SIZE	4
#define	_FOLLOW_MOVE_PTR	5
#define	_FOLLOW_MOVES_IN_BUF	6
#define	_FOLLOW_MOVES		7

#define	_FOLLOW_DATA_SIZE	8

#include "/basic/npc/attack.c"

/* Some dummy functions to be overridden by inheriting NPCs: */
int extra_aggressive(object x) { return 0; }

/****************************************************************
*								*
*			Query-functions.			*
*								*
****************************************************************/

int query_wander() { return wander; }
// For consideration. //Graah
int query_spell_dmg() { return spell_dmg; }

int
query_breath_dmg()
{
	if (!pointerp(breath_attack)) return 0;
	return breath_attack[1];
}


varargs string
query_long(string what, object who)
{
string cap_pronoun;
mixed s;

	if (!(s = ::query_long(what, who))) s = "";

	if (!query(NPC_NO_CONDITION)  && query(LIV_NOT_UNDEAD)) {
		s += "\n" + capitalize(query_pronoun()) + " is ";
		if(hit_point <= 0) s += "mortally wounded.";
		else if(hit_point == 1) s+= "nearly dead.";
		else if(hit_point < max_hp/10) s += "in very bad shape.";
		else if(hit_point < max_hp/5) s += "in bad shape.";
		else if(hit_point < max_hp/2) s += "not in a good shape.";
		else if(hit_point < max_hp - 20) s += "slightly hurt.";
		else s += "in good shape.";
	}
	return s;
}

nomask int query_coder_level() { return 0;  }
nomask int query_testplayer() { return 0; }
nomask int query_is_testplayer() { return 0; }

mixed query_ctrl() { return query(NPC_POSSESSED_BY); }

int
query_sp()
{
int amount;
	CHECK_HEAL2
	return ::query_sp();
}

int
query_hp()
{
int amount;
	CHECK_HEAL2
	return ::query_hp();
}

/****************************************************************
*								*
*			Set-functions.				*
*								*
****************************************************************/

// Use this (with arg < 0) to let monsters keep their heart_beat even if there
// are no players present. 12.9.1992 //Frobozz

// NEW! Use negative argument to let monster keep its hb only limited
// time. Please _do_ use this, unless absolutely have to keep hb on
// all the time. Ie. if you use "set_keep_hb(-10)"; monster will keep its
// hb on for next 10 heart_beats, even if there are no players present.

void set_keep_hb(int arg) { keep_hb = arg; }

void
set_hb(status i)
{
int amount;
	NPC_START_HB
}

void
set_money(int x)
{
	if (money < 50000 && money >= 0)
		money = x;
}

void set_dead_ob(object ob) { dead_ob = ob; }
void set_init_ob(object ob) { init_ob = ob; }

void set_random_pick(status r) { random_pick = r; }

varargs void
set_wander(int x, string *dirs)
{
	wander = x > 80 ? 80 : x;
	if (dirs && pointerp(dirs)) wander_dirs = dirs;
}

// Call the following functions to setup the monster.
// Call them in the order they appear.

void
set_name(string n)
{
	set_living_name(name = n);
	alignment = 0;		// Neutral monster
	cap_name = capitalize(n);
	set_short(cap_name);

// set_long("You see nothing special.");
// No need. /basic/id.c can handle this. -+ Doomdark +-

	::set_name(n);
}

void
set_level(int l)
{
	level = l;

// Let's set stats bit lower than they've used to... 
	if (l < 20)
		Str = max_Str = Int = max_Int = Con= max_Con= Dex= max_Dex= l;
	else
		Str = max_Str = Int = max_Int = Con= max_Con= Dex= max_Dex =
			20 + l * 3 / 4;


// Wimpy lords died too much,
// Fixed 15.7.1994. //Graah
	weapon_class = (level * 11 / 13) + 3;

// With the new wimpy Lords, this is somewhat smaller.
	armour_class = 1 + (level * 6) / 5;

	// 27-Apr-98 / Graah:
	// Guild powers are now so good that more hit points are given.
	// Especially to tougher level monsters.
	// Was: max_sp = spell_points = max_hp = hit_point = 42 + level * 9;

	max_sp = spell_points = 42 + level * 9;
	max_hp = hit_point = 40 + level * 12;
	if (level > 10) max_hp = hit_point = (max_hp + 20 * (level - 10));
	if (level > 20) max_hp = hit_point = (max_hp + 40 * (level - 20));

	// Level 10: 160, Level 20: 480, Level 30: 1200
	// Even those are quite little amounts!

	if (level < 2)
		experience = 10 + random(491);
	else if (level < 21)
		experience = (int) LEVELS_D->query_exp_for_level(l - 1);
	else
		experience = 1000000 + (level - 20) * 100000;

	if (race) {
		race_update();
		set_hp(max_hp);
		set_sp(max_sp);
	}
}

void set_hp(int hp) { max_hp = hp; hit_point = hp; }
void set_sp(int sp) { max_sp = sp; spell_points = sp; }
	// Now level 20 up monsters can have experience over 1000000
void
set_ep(int ep)
{
	if (ep > experience && level < 20) return;
	if ((ep - 1000000) > (level - 20) * 100000) return;
	experience = ep;
}

varargs void
set_wc(int wc, status force)
{
	if (force || wc > weapon_class)
		weapon_class = wc;
}

varargs void
set_ac(int ac, status force)
{
	if (force || (ac > armour_class))
		armour_class = ac;
}

// Should use argument... But haven't earlier done that.
void set_aggressive(status a) { set(a ? NPC_AGGRESSIVE : NPC_NOT_AGGRESSIVE); }

// Now some functions for setting up spells!
// The percent chance of casting a spell.

// NEW! Use this instead of 4 old functions! -+ Doomdark +-
varargs void
set_breath_attack(int dmg, int chance, string my_msg, string their_msg,
int dtype)
{
	breath_attack = ({ dmg, chance, my_msg, their_msg, dtype });
}

// People should use 'set_spell_chance' and not set_chance!
void set_spell_chance(int c) { spell_chance = c; }

// Message(s) to others
void
set_spell_mess1(mixed m)
{
	spell_mess1 = m;
	if (pointerp(m))
		npc_Flags |= F_NPC_SM1_ARRAY;
	else npc_Flags &= ~(F_NPC_SM1_ARRAY);
}
// Message(s) to the victim.
void
set_spell_mess2(mixed m)
{
	spell_mess2 = m;
	if (pointerp(m))
		npc_Flags |= F_NPC_SM2_ARRAY;
	else npc_Flags &= ~(F_NPC_SM2_ARRAY);
}
// Maximum damage spell can do (actually, max. is this value - 1!)
varargs void
set_spell_dam(int d, int type)
{
	spell_dmg = d;
	if (!type)
		spell_dtype = DT_SLASH;
	else
		spell_dtype = type;
}

varargs void set_frog() { frog = 1; } // Ugh. _Should_ use argument!!!!

// Catch the talk
void
set_match(object ob, string *func, string *type, string *match)
{
int i, x;
string tmp_s, tmp2_s;
	if (!objectp(ob) || !pointerp(func))
		return;

	i = sizeof(func);

	if (!pointerp(type) || !sizeof(type)) x = 1;
	if (!pointerp(match) || !sizeof(match)) x |= 2;

	if (x == 3) return;	// Shouldn't happen, mind you...

// If both were defined, we can make this more efficient...
	if (!x) while (--i >= 0) {
	 switch (x) {
	  case 0:
		if (!match[i]) {
			tmp_s = type[i];
			tmp2_s = "";
		} else {
		tmp_s = type[i] + match[i];
		tmp2_s = sprintf("%s %s", type[i], match[i]);
		}
		type[i] = tmp_s;
		match[i] = tmp2_s;
	 }
	}

	talk_array = ({ ob, func, type, match, 0 });
	npc_Flags |= F_NPC_CATCH_TELL_USED;
}

  /*******************************************/
 /**** NPC FOLLOW STUFF - GERM *************/
/*****************************************/

void
set_follow(int f, int f_delay, int f_count_max, int f_buf_size)
{
	if (f_count_max < f_buf_size)
		f_count_max = f_buf_size;
	else if (!f_buf_size)
		f_buf_size = f_count_max;
	if (!f) {
// If we're following, we have to stop doing so...
		if (npc_Flags & NPC_FOLLOW_ATTACKER) {
			npc_Flags &= (~NPC_FOLLOW_ATTACKER);
			if (follow_data[_FOLLOW_OB])
				PARTY_D->remove_move_hook(follow_data[_FOLLOW_OB],
					this_object());
		}
		follow_data = 0;
	} else {
		follow_data = ({ 0, f_delay, 0, f_count_max, f_buf_size, 0, 0, 0 });
	}
}

varargs void
init_follow( object attacker, int flag )
{
	if ( flag == START_FOLLOWING ) {
// We have to remove the first hook first, if we decide to follow some other
// attacker instead...
		if (npc_Flags & F_NPC_FOLLOW_ATTACKER) {
			if (follow_data[_FOLLOW_OB])
			  PARTY_D->remove_move_hook(follow_data[_FOLLOW_OB],
				this_object());
		} else npc_Flags |= F_NPC_FOLLOW_ATTACKER;
		follow_data[_FOLLOW_MOVES] = allocate( follow_data[_FOLLOW_BUF_SIZE] );
		follow_data[_FOLLOW_COUNT] = 0;
		follow_data[_FOLLOW_MOVE_PTR] = 0;
		follow_data[_FOLLOW_MOVES_IN_BUF] = 0;
		follow_data[_FOLLOW_OB] = attacker;

	        PARTY_D->add_move_hook( attacker, this_object() );
	} else if ( flag == STOP_FOLLOWING ) {
		if (npc_Flags & F_NPC_FOLLOW_ATTACKER) {
			if (follow_data[_FOLLOW_OB])
				PARTY_D->remove_move_hook( follow_data[_FOLLOW_OB],
					this_object() );
			npc_Flags &= (~F_NPC_FOLLOW_ATTACKER);
		}
		follow_data[_FOLLOW_MOVES] = 0;
		follow_data[_FOLLOW_MOVE_PTR] = 0;
		follow_data[_FOLLOW_MOVES_IN_BUF] = 0;
		follow_data[_FOLLOW_OB] = 0;

		while( remove_call_out( "follow_callout" ) >= 0) ;
	}
}

// Used by "follow":
void
player_moved( object attacker, string a, string b, object where )
{
int amount, tot_amount, buf_size;
	if (b) a = sprintf("%s %s", a, b);

	amount = follow_data[_FOLLOW_MOVES_IN_BUF];
	buf_size = follow_data[_FOLLOW_BUF_SIZE];
	follow_data[_FOLLOW_MOVES][(follow_data[_FOLLOW_MOVE_PTR]
	  + amount) % buf_size] = a;
	amount++;
	tot_amount = ++follow_data[_FOLLOW_COUNT];
// Is the follow path becoming too long (total), or distance too big
// (too many moves in buffer)? If so, let's stop following...
	if ((tot_amount > follow_data[_FOLLOW_COUNT_MAX]) ||
	  (amount > buf_size)) {
		init_follow(0, STOP_FOLLOWING);
		return;
	}
	follow_data[_FOLLOW_MOVES_IN_BUF] = amount;
            
/** Only call-out on the 1st move because   **/
/** follow attacker will deal with the rest **/

	if (amount == 1)
		call_out("follow_callout", follow_data[_FOLLOW_DELAY], attacker );
}

void
follow_callout( object attacker )
{
string dir, fn;
int move_ptr, i;
	if (!environment()) return;
	if (!attacker) {
		init_follow(0, STOP_FOLLOWING);
		return;
	}
// This should never happen but...
	if (!follow_data[_FOLLOW_MOVES_IN_BUF]) return;
	move_ptr = follow_data[_FOLLOW_MOVE_PTR];
	dir = follow_data[_FOLLOW_MOVES][move_ptr];
// So. We have to check if we're allowed to move there...
	if (move_area) {
		fn = (string) environment()->query_exit(dir);
		if (stringp(move_area)) {
		  if (!fn || strstr(fn, move_area) < 0) {
			init_follow(attacker, STOP_FOLLOWING);
			return;
		  }
		} else {
			if (!fn) i = sizeof(move_area);
			else
			while (i < sizeof(move_area)) {
				if (strstr(fn, move_area[i]) > 0) break;
				i++;
			}
			if (i == sizeof(move_area)) {
				init_follow(attacker, STOP_FOLLOWING);
				return;
			}
		}
	}
	i = --follow_data[_FOLLOW_MOVES_IN_BUF];
	move_ptr = (move_ptr + 1) % follow_data[_FOLLOW_BUF_SIZE];
	follow_data[_FOLLOW_MOVE_PTR] = move_ptr;
	if (!command(dir)) {
           init_follow( attacker, STOP_FOLLOWING );
           return;
	}

	if (i) call_out("follow_callout", follow_data[_FOLLOW_DELAY], attacker );

/** if attacker is here, we found him  **/
/** so reset all the follow stuff.     **/

	if ( environment( attacker ) == environment() ) {
		follow_data[_FOLLOW_MOVE_PTR] = 0;
		follow_data[_FOLLOW_COUNT] = 0;
		follow_data[_FOLLOW_MOVES_IN_BUF] = 0;
		while ( remove_call_out( "follow_callout" ) != -1 );
	}
}

/*************END OF FOLLOW STUFF ***********/
/********************************************/
/********************************************/

/****************************************************************
*								*
*	Now some functions used like 'set-functions'.		*
*								*
****************************************************************/

// With the following function, we can now easily block some exit(s)
// without coding dedicated functions!
// If you want to remove the blocking, simply call this with second
// argument as 0; 'add_block("east", 0, ...);' for example.
// Note that strings we will use should be in SSF; Stickmud String
// Format. Check out documents about SSF (from /doc/NEW_DOCS/misc?)
status
add_block(string direction, mixed type, mixed crit1, mixed crit2,
string msg1, string msg2, string msg3, string msg4)
{
mixed *tmp;
	if (!type) {
		if (mappingp(blocked_exits)) {
                        // was: blocked_exits = m_delete(blocked_exits, 0);
                        // and how was it supposed to work?? - Chopin
                        m_delete(blocked_exits, direction);
			if (!sizeof(blocked_exits))
				npc_Flags &= (~F_NPC_BLOCKING);
		}
		return 1;
	}
	switch (type) {
	case "all":
		type = _BLOCK_ALL;
		break;
	case "level":
		type = _BLOCK_LEVEL;
		break;
	case "race":
		type = _BLOCK_RACE;
		break;
	case "name":
		type = _BLOCK_NAME;
		break;
	case "guild":
		type = _BLOCK_GUILD;
		break;
	case "alignment":
		type = _BLOCK_ALIGNMENT; break;
     case "clan":
             type = _BLOCK_CLAN; break;
	default: return 0;
	}
	tmp = ({ type, crit1, crit2, msg1, msg2, msg3, msg4 });
	if (!mappingp(blocked_exits))
		blocked_exits = ([ direction : tmp ]);
	else
		blocked_exits[direction] =  tmp;
	npc_Flags |= F_NPC_BLOCKING;
	return 1;
}

/* Debug //Graah */
mapping query_blocked_exits() { return blocked_exits; }

void
load_chat(int chance, string *strs)
{
int i;
// Just make sure it is an array; don't want heart_beat to fail.
	if (!pointerp(strs) || !sizeof(strs)) return;

	chat_head =  allocate( sizeof(strs));
	chat_chance = chance;

// Lets format for SSF - GERM

	for ( i=0; i<sizeof(strs); i++ )
		chat_head[i] = make_format_string( strs[i] );
}

void
load_a_chat(int chance, string *strs)
{
int i;
// Just make sure it is an array; don't want heart_beat to fail.
	if (!pointerp(strs) || !sizeof(strs)) return;

	a_chat_head = allocate( sizeof(strs));
	a_chat_chance = chance;

// Lets format for SSF - GERM

	for ( i=0; i<sizeof(strs); i++ )
		a_chat_head[i] = make_format_string( strs[i] );
}

/****************************************************************
*								*
*		'Query'-like functions.				*
*								*
****************************************************************/

status can_put_and_get(string str) { return str ? 1 : 0; }

/****************************************************************
*								*
*			Then lfuns.				*
*								*
****************************************************************/

varargs status
id(string s, object who)
{
	return ::id(s, who) || (!invis_level && (s == race || s == "monster"));
}

void create_monster() { }

void
reset(int arg)
{
string a;
int b;
int amount;
	if (arg) {
		if (!(npc_Flags & F_NPC_IS_ORIGINAL) || (npc_Flags & F_NPC_CREATE_ORIGINAL)) {
			if (npc_Flags & F_NPC_RESET_USED)
				this_object()->reset_monster();
			reset_living();
			if (query(NPC_MOVE_AT_RESET)) random_move();
		}
		if (random(2)) update_conditions();
		else update_stats();
// Let's update these just to be sure...
// Although these calls shouldn't really be needed. *shrug*
// We'll only call either one, to maintain efficiency
		reset_living();
		return;
	}

	if (function_exists("reset_monster")) npc_Flags |= F_NPC_RESET_USED;
// This will enable tell_me to call catch_tell if that function is defined
// in the object inheritting this file.  - Chopin
	if (strstr(object_name(), function_exists("catch_tell")) >= 0)
		npc_Flags |= F_NPC_CATCH_TELL_USED;
	InitFunc = symbol_function("init_monster", this_object());

	if (!query_conditions())
		set_conditions(([]));

	set(NPC_YES_CONDITION);
// default: show condition to players, unless monster is
// Undead (checked in query_long()) 2.2.1993 -- Frobozz

	liv_Flags = F_LIV_IS_NPC;
	wander = possessed_by = spell_chance = breath_attack = 0;
	damage_type = DT_SLASH;
	configure_object(this_object(), OC_COMMANDS_ENABLED, 1);
	set_master(0);
	create_living();

	if (!(npc_Flags & F_NPC_CREATE_ORIGINAL) &&
	  sscanf(object_name(this_object()), "%s#%d", a, b) < 2) {
		npc_Flags |= F_NPC_IS_ORIGINAL;
		return;
	}

	add_action("command_driver", "", 1);

// Upwards compatibility...
	create_monster();

	if (keep_hb) {
		NPC_START_HB
	}
}

void
init()
{
int i, its_npc, amount;
string *dirs;
	if (this_player() == this_object()) return;
	if (init_ob && init_ob -> monster_init(this_object())) return;

// This is just to ensure we'll get create_monster called for blueprint
// objects too, if they're used for some reason.

	if ((npc_Flags & F_NPC_IS_ORIGINAL) && !(npc_Flags & F_NPC_CREATE_ORIGINAL) &&
	environment()) {
		this_object() -> create_monster();
		npc_Flags &= (~F_NPC_IS_ORIGINAL);
	}

	funcall(InitFunc);

	if ((this_player() && interactive(this_player())) || attackers) {
		NPC_START_HB
	}

// If we were hunting this living, let's attack!
	if ((i = member(hunted, this_player())) >= 0) {

// We have a chance (depending on opponents' levels) of attacking
// hunted right now, otherwise it happens in next heart_beat()
// Invisibles are harder to spot right away //Graah
// Depends on luck too //Frobozz
// Now it's a lot (+30) more often. They deserve it. //Graah

	  if (!attackers && random(level + 30) >=
	   (random((int)this_player()->query_level()
	   +((int)this_player()->query_invis() ? 10 : 0)
	   +(int) this_player()->query_luck())))
		attack_object(this_player(), 1);
		else {
			if (kill_ob && present(kill_ob, environment()))
				attack_object(kill_ob, 1);
			kill_ob = this_player();
		}
		hunted[i] = 0; // Forget hunted on this one
		NPC_START_HB
	}

	if (!this_player()) return;
	its_npc = (interactive(this_player()) ? 0 : 1);
	if ((its_npc && query(NPC_MONS_AGGRESSIVE) && !this_player()->
		query(NPC_MONS_AGGRESSIVE))
	|| (!its_npc && query(NPC_AGGRESSIVE))
	|| extra_aggressive(this_player())) {
		NPC_START_HB
		if (!its_npc && (int) this_player()->query_coder_level() > 0)
		this_player() -> tell_me(sprintf("%s growls before you but\
 doesn't care to attack.", query_name(0, this_player())));
	  else {
		if (kill_ob && present(kill_ob, environment()))
			attack_object(kill_ob, 1);
		kill_ob = this_player();
	  }
	}

// NEW! We can block some exit(s)!
// (too bad we have to use add_actions but...)
    if (npc_Flags & F_NPC_BLOCKING) {
	i = 0;
	dirs = m_indices(blocked_exits);
	do {
		add_action("do_block", dirs[i]);
	} while (++i < sizeof(dirs));
    }
}

/****************************************************************
*								*
*		And now rest of the functions.			*
*								*
****************************************************************/

void
random_move()
{
mapping x;
string *a, dir, dest;
int i;
	if (!environment()) return;
	if (wander_dirs)
		command(wander_dirs[random(sizeof(wander_dirs))]);
// If we have set allowed rooms (ie. part of the path of the rooms' filenames),
// we'll have to check that we're 'allowed' to move there...
// (ie. set(NPC_MOVE_AREA, "/areas/somearea/") -> can only move to rooms like
// "/areas/somearea/room1", "/areas/somearea/cave/x3" and so on).
	else if (move_area) {
            if (mappingp(x = (mapping) environment()->query_exits())) {
			a = m_indices(x);
			dir = a[random(sizeof(a))];
			dest = x[dir];
			if (pointerp(move_area)) {
				do {
				  if (strstr(dest, move_area[i]) >= 0) {
                                        command(dir);
					break;
				  }
				} while (++i < sizeof(move_area));
			} else if (stringp(dest) && strstr(dest, move_area)
			>= 0)
                               command(dir);
			return;
		}
	}
	else switch (random(10)) {
	case 0:	command("north"); return;
	case 1: command("northeast"); return;
	case 2: command("east"); return;
	case 3: command("southeast"); return;
	case 4: command("south"); return;
	case 5: command("southwest"); return;
	case 6: command("west"); return;
	case 7: command("northwest"); return;
	case 8: command("up"); return;
	case 9: command("down"); return;
	}
}

status
test_match(string str)
{
string who, str1;
int i;
string *funcs, *types, *matches;
object x;

	if (!pointerp(talk_array)) return 0;
// Is the object in which this should be called destructed?
	if (!(x = talk_array[0]))
		return (talk_array = 0);

	if (str[<1] == '\n') {
		str = str[0..<2];
	}
	if (str[<1] == '.')
		str = str[0..<2];

	funcs = talk_array[1];
	types = talk_array[2];
	matches = talk_array[3];

	while (i < sizeof(funcs)) {
	  if (types) {
	    if (sscanf(str, "%s "+types[i] + " %s",who,str1) == 2 ||
		sscanf(str,"%s "+types[i], who) ||
		sscanf(str,"%s "+types[i] + "%s",who,str1) == 2
	    ) {
		if (x && funcs[i])
			return call_other(x, funcs[i], str);
		return 0;
	    }
	  }
	  if (matches) {
	    if (sscanf(str,"%s "+matches[i] + " %s",who,str1) == 2 ||
		sscanf(str,"%s "+matches[i], who) ||
		sscanf(str,"%s "+matches[i] + "%s",who,str1) == 2
	    ) return (status) call_other(x, funcs[i], str);
	  }
	i++;
	}
	return 0;
}

void
breath_attack(object obj)
{
object *arm, destr;
int i, arm_ac, a;

	arm = all_inventory(obj);

	for (i = sizeof(arm) - 1; i >= 0; i--)
// Note that if you want an armour that should _NOT_ get dested, you have to
// mask the 'add_damage' (like mages' guild symbol, hat does).
	  if (!arm[i]->is_armour() || !arm[i]->is_worn())
		arm[i] = 0;
	  else
		a++;

	if (!a) return;

	arm -= ({ 0 });

// && !inv[i]->query_auto_load()
// drop(1) should take care of this. if not, let's add this. -+ Doomdark +-

// Not EVERY time, and now gives a message. //Graah
	if (sizeof(arm) && random(100) < 80) {
		a = random(sizeof(arm));
		obj->tell_me(sprintf("Your %s is damaged!",
			(string)arm[a]->query_name()));
		arm[a]->add_damage(1);
// Note: arm[a] might now be destructed.
	}

// New: breath damage
	if (breath_attack[0])
		obj -> hit_player(breath_attack[0], breath_attack[4],
			0, this_object());
// Let's use all arguments! -+ Doomdark +-
}

void
pick_any_obj()
{
object ob;
int wt,old_class,plus_class;
string s;

	if(!(ob = environment())) return;
	ob = first_inventory(ob);
	while(ob) {
	  if (!interactive(ob)
	      && !ob -> is_corpse()
	      && ob -> get() && ob -> short()) {
		s = (string) ob -> query_name(0, this_object());
		if (!s) s = "something";
	    wt = (int) ob -> query_weight();
	    if (!add_weight(wt)) {
		environment() -> tell_here(sprintf("%s tries to take the %s\
 but fails.", query_name(), s), this_object());
		return;
	    }
	    move_object(ob, this_object());
		environment()->tell_here(sprintf("%s takes the %s.", cap_name, s));
// Now the armour class will be added to previous ac if monster can wear item.
// 7.2.1992 //Frobozz
	    if ((int) ob -> query_wc() > weapon_class)
		wield(this_object(), 0);

	    else if (ob -> query_ac()) {
		old_class = query_ac();
		wear(this_object(), 0);
		if (query_ac() < old_class)
			set_ac(old_class, 1);
	    }
	    return;
	}
	ob = next_inventory(ob);
    }
}

void
heart_beat()
{
int c, move_ok;
object ms, ma, ob;

	age++;

// Let's see if we have heal rate set... :-)
	if (healRate > 0) {
		if ((hit_point += healRate) > max_hp)
			hit_point = max_hp;
		if ((spell_points += healRate) > max_sp)
			spell_points = max_sp;
	} else if (healRate < 0 && ++time_to_heal >= 0) {
		time_to_heal = healRate;
		if (++hit_point > max_hp)
			hit_point = max_hp;
		if (++spell_points > max_sp)
			spell_points = max_sp;
	}

	if (!attackers)

// Quicker to check master attacker first, then present if required.

	if ((ms = query_master()) && (query_defend_master()) &&
	(ma = (object)ms->query_attack()) && present(ms, environment())) {
// Fighting our own master? He's no good master then!

	if (ma = this_object()) {
	    set_master(0);
	} else {
// TODO: Messages when master is under attack and we rescue?
	    if ((ma = (object)ms->query_attack()))
	      {
// All monsters "rescue" their master

		ma->stop_fight(ms);
		attack_object(ma);
	      }
	  }
	} else if (wander && (wander >= (random(100) + 1))) {
	  random_move();
	  return;
	}

	if (!random(50))		// Was 10...
		forget_hunted();	// Forget enemies slowly...
// Slowly???? Hmmm, gotta code something better...

// Fix: if there is attacker_ob, don't stop hb yet.
// But let's be sure attacker_ob is in this room! -+ Doomdark +-
// Let's also check if we'll keep hb anyway...

	if (!attackers && keep_hb <= 0) {

// NEW! Negative args to set_keep_hb() will result in hb staying for
// limited amount of hbs after player has left the room.

	  if (hb_left > 0)
		hb_left--;
	  else if (!random(3) && !test_if_any_here()) {
		if (npc_Flags & F_NPC_HAS_TEXT) {
			npc_Flags &= (~F_NPC_HAS_TEXT);
			test_match(talk_array[4]);
		} else {
			NPC_STOP_HB
			return;
		}
	  }
	}

	if ((move_ok = query_can_move()) && kill_ob &&
	environment(kill_ob) == environment()) {
		if (random(2)) return;	// Delay attack some
// Don't detect invisible so easily, depends on monster level
// Added by Graah 8.8.1991

	  if (kill_ob->query_invis()) {
		  if (level < (random(25)-5)) return;
// Invisible coders are never attacked
// TODO: perhaps; explore mode for testplayers and coders
	  }
	  attack_object(kill_ob);
	  kill_ob = 0;
	  return;
	}

// Can't cast spells or breathe when immobilized! //Graah

	if (move_ok && (ob = query_attack())) {
	  if (spell_chance > random(100)) {
	  if (!(npc_Flags & (F_NPC_SM1_ARRAY | F_NPC_SM2_ARRAY))) {
		if (spell_mess2) {
			ob->tell_me(&spell_mess2, 0, 0,
				this_object(), ob, name_of_weapon);
			environment()->tell_here(&spell_mess1, 0, 0, 
				({ ob, this_object() }),
				this_object(), ob, name_of_weapon);
		} else {
			environment()->tell_here(&spell_mess1,
				0, 0, this_object(),
				this_object(), ob, name_of_weapon);
		}
		ob->hit_player(1 + random(spell_dmg), spell_dtype, 0,
			this_object());
	  } else {
		ob->hit_player(1 + random(spell_dmg),spell_dtype,
			spell_closure, this_object());
	  }
// Won't cast spell AND breathe at the same time! //Graah
	} else if (breath_attack && breath_attack[1] > random(100)) {
	    ob->tell_me(breath_attack[2]), 0, 0, this_object(), ob, name_of_weapon;
	    environment()->tell_here(breath_attack[3], 0, 0, ({ ob, this_object() }),
			this_object(), ob, name_of_weapon);
	    breath_attack(ob);
	 } else {

/***********GERM************/
/** ACK we are being attacked, lets follow this **/
/** new attacker if we can.                     **/
// But only if we aren't already "committed". Too inefficient otherwise.
// -+ Doomdark +-
// (btw, this could be enhanced to allow different behaviour later)

                if ( follow_data && !(npc_Flags & F_NPC_FOLLOW_ATTACKER)) {
                     init_follow( ob, START_FOLLOWING );
		}
/***************************/

                attack();
	}

	if (whimpy && (hit_point < (whimpy * max_hp / 100))
	  && (random(100) > 50) && query_attack()) {
// Let's not always wimpy! -+ Doomdark +-

/*********GERM**********/
/** we dont want to follow, we want to flee! **/
		if (npc_Flags & F_NPC_FOLLOW_ATTACKER)
			init_follow(0, STOP_FOLLOWING );
/***********************/

		run_away();
	  }
	}

// Let's see if we've either lost the target, or have done all the moves
// but can't see the enemy anywhere:

	if ((npc_Flags & F_NPC_FOLLOW_ATTACKER)) {
		if (!(ob = follow_data[_FOLLOW_OB]) ||
		 (!follow_data[_FOLLOW_MOVES_IN_BUF] &&
		  !present(ob, environment()))) {
			init_follow(0, STOP_FOLLOWING);
		}
	} 

	if (environment() && move_ok && (chat_chance || a_chat_chance)) {
	c = random(100);

	if (a_chat_head && query_attack()) {
	  if (c < a_chat_chance) {
	      c = random(sizeof(a_chat_head));
	      if (environment())
		environment()->tell_here(&(a_chat_head[c]), 0, 0, this_object(),
			this_object(), query_attack(), 0);
	  }
	} else {
	  if (chat_head && c < chat_chance) {
	      c = random(sizeof(chat_head));
	      if (environment() && chat_head[c])
		environment()->tell_here(&(chat_head[c]), 0, 0, this_object(),
			this_object(), 0, 0);
	  }
	}
	}

	if (random_pick && move_ok && random_pick > random(100))
		pick_any_obj();

	if (npc_Flags & F_NPC_HAS_TEXT) {
		npc_Flags &= (~F_NPC_HAS_TEXT);
		test_match(talk_array[4]);
	}
// Now, let's update these only now, not earlier; otherwise we'll end up
// 'losing' 1 round of effect.
	if (NextConditionCheck && (NextConditionCheck <= age))
		update_conditions();
	if (nextStatCheck && (nextStatCheck <= age))
		update_stats();
}

// This is used as a callback-function for hit_player().
void
do_spell_message(object victim, int real_dmg, int orig_dmg)
{
int x;
	if (!real_dmg) {
		if (spell_mess2[0]) {
		  victim->tell_me(spell_mess2[0], 0, 0, this_object(),
			victim, name_of_weapon);
		  environment()->tell_here(spell_mess1[0], 0, 0,
			({ this_object(), victim }), this_object(),
			victim, name_of_weapon);
		} else {
		  environment()->tell_here(spell_mess1[0], 0, 0,
			this_object(), this_object(), victim, name_of_weapon);
		}
	} else {
		x = -1;
		if (spell_mess2) {
		  x = 1 + (real_dmg * (sizeof(spell_mess2) - 1) / spell_dmg);
		  if (x >= sizeof(spell_mess2))
			x = sizeof(spell_mess2) - 1;
		  victim->tell_me(&(spell_mess2[x]), 0, 0,
			this_object(), victim, name_of_weapon);
		  x = 1 + (real_dmg * (sizeof(spell_mess1) - 1) / spell_dmg);
		  if (x >= sizeof(spell_mess1))
			x = sizeof(spell_mess1) - 1;
		  environment()->tell_here(&(spell_mess1[x]), 0, 0,
			({ this_object(), victim }), this_object(),
			victim, name_of_weapon);
		} else {
		  x = 1 + (real_dmg * (sizeof(spell_mess1) - 1) / spell_dmg);
		  if (x >= sizeof(spell_mess1))
			x = sizeof(spell_mess1) - 1;
		  environment()->tell_here(&(spell_mess1[x]), 0, 0,
			this_object(), this_object(), victim, name_of_weapon);
		}
	}
}

void
catch_tell(string str)
{
string tmp1, tmp2;
object ob;

	if (possessed_by)
		possessed_by -> tell_me(name + ">" + str);

	if (npc_Flags & F_NPC_BUSY_CATCH_TELL) return;

	npc_Flags |= F_NPC_BUSY_CATCH_TELL;

	if (pointerp(talk_array)) {
		if (npc_Flags & F_NPC_HAS_TEXT) {
		  test_match(talk_array[4]);
		  talk_array[4] = str;
		} else {
		  talk_array[4] = str;
		  npc_Flags |= F_NPC_HAS_TEXT;
		}
	}

	npc_Flags &= (~F_NPC_BUSY_CATCH_TELL);
}

// Force the monster to do a command. The force_us() function isn't
// always good, because it checks the level of the caller, and this function
// can be called by a room.

void init_command(string cmd) { command(cmd); }

// If second_life (or monster_died) returns 0, monster will die,
// otherwise it won't.

varargs status
second_life(int x, int y, object killer,object corpse)
{
	if (dead_ob)
		return (status) dead_ob -> monster_died(this_object(),
			killer, corpse);
}

int
clean_up(int refs)
{
	if (!refs || environment() || first_inventory())
		return 0;	// Won't try to clean_up clones any more

	if (refs > 1)
		return 1;
// Clones might get cleaned up, so we may be able to destruct master too!

	destruct(this_object());
	return 0;
}

// This will only get called when we block a direction.
status
do_block(string s)
{
mixed *x, a, b;
int block, foo;
mixed tmp;
status general;
// Let's check if him/her is unable to move; in that case, let's not
// bother (as move won't be done anyway).
	if (!this_player()->query_can_move()) return 0;
	if (!(x = blocked_exits[query_verb()])) return 0;
	if (!pointerp(x) || sizeof(x) < 7) return 0;
	a = x[1];
	b = x[2];
	switch (x[0]) {
// All. Will simply block everyone, no exceptions.
	case _BLOCK_ALL:
		block = 1;
		break;
// Level: We simply check that level is between two criteria; if so,
// we'll do blocking.
	case _BLOCK_LEVEL:
		tmp = (int) this_player()->query_level();
		block = -2;
		break;
	case _BLOCK_ALIGNMENT:
		tmp = (int) this_player()->query_alignment();
		block = -2;
		break;
// Race: We'll use 'general' method to determine the blocking.
	case _BLOCK_RACE:
		tmp = (string) this_player()->query_race();
		block = -1;
		break;
// Name: - " -
	case _BLOCK_NAME:
		tmp = (string) this_player()->query_real_name();
		block = -1;
		break;
// Guild: - " -
	case _BLOCK_GUILD:
		tmp = (string) this_player()->query_guild();
		block = -1;
		break;
// Clan: - " -
        case _BLOCK_CLAN:
                tmp = (string) this_player()->query_clan();
              block = -1;
              break;
              
	default:
		return 0;
	}

// Now, let's do the general checking for name, race or guild...
// Rules are:
// - If attribute is same as the 1st criterium (criterium with which to block,
//    either single criterium, or list of criteria), we'll block him/her/it.
// - If attribute is same as the 2nd criterium (crit. with which to let go,
//   - " -), we'll let him/her/it go.
// - Otherwise, if only "block"-criterium is defined, we'll let him/her/it go.
// - Finally, if none of the above was true, we'll block. :-)

	if (block == -1) {
			if (tmp == a || (pointerp(a) && member(a, tmp) >= 0))
				block = 1;
			else if (tmp == b || (pointerp(b) && member(b, tmp) >= 0))
				block = 0;
			else {
				if (!b) block = 0;
				else block = 1;
			}
	} else if (block == -2) {
		if (b < a) { foo = a; a = b; b = foo; }
		if ((tmp >= a) && (tmp <= b)) block = 1; else block = 0;
	}

// Finally, let's print appropriate messages, if those are defined:

	foo = (block ? 3 : 5);

// Messages, in order, are:
// - When blocking; to blocked living.
// - 	- " -	  ; to other livings in the room.
// - When not blocking; to 'non-blocked' living, leaving the room.
// -	- " -	  ; to other livings in the room.

	if (tmp = x[foo++]) {
	  this_player()->tell_me(tmp, 0, 0, this_object(),
			this_player(), 0);
	  if (tmp = x[foo])
		environment()->tell_here(tmp, 0, 0, ({ this_object(), this_player() }),
			this_object(), this_player(), 0);
/* Don't have to use both messages often... */
	} else if (tmp = x[foo])
		environment()->tell_here(tmp, 0, 0, this_object(),
			this_object(), this_player(), 0);
	return block;
}

/****************************************************************
*								*
*	Compatibility-section. _NOT_ _TO_ _BE_ _USED_		*
*								*
****************************************************************/

// People should use 'set_spell_chance' and not set_chance!
void set_chance(int c) { spell_chance = c; }


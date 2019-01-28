#include <npc.h>
#include <guild.h>
#include "../priest.h"
#include "../skills.h"
#include <room_defs.h>

inherit PSKILLS;

#define TP this_player()
#define ENV environment
#define TO this_object()
#define HERE environment()
#define TH HERE->tell_here

#define OUTRAGE_COST 80 // spell points for angel
#define COMMAND_COST 15 // all commands cost 10 fps except leave

#define INVIS_SHORTS ({ \
    "Some mist is whirling in the air", \
    "A shimmering being", \
    "Something is moving in the air", \
})
#define INVIS_LONGS ({ \
    "You see something whirling in the air.", \
    "You can't quite focus your eyes on it, but you feel it's \
very aware of you.", \
    "All you see is a shimmering being floating in the air.", \
})

// Should be 12 of them:
#define TITLES ({ \
    "Angel of shadow", \
    "Angel of darkness", \
    "Angel of light", \
    "Angel of Boccob", \
    "Guardian angel of shadow", \
    "Guardian angel of darkness", \
    "Guardian angel of light", \
    "Guardian angel of Boccob", \
    "Arch-Angel of shadow", \
    "Arch-Angel of darkness", \
    "Arch-Angel of light", \
    "Arch-Angel of Boccob", \
})

// *** VARIABLES: ***

int vis_level;
int max_damage;
object owner;
status can_follow;

int deliver_status;
object last_room;
string blocked_dir;

mixed *cmd_queue;


// *** PROTOTYPES: ***

varargs void vanish(status dont_destruct);


// *** QUERY & QUERY-LIKE FUNCTIONS: ***

int query_vis_level() { return vis_level; }
object query_master() { return owner; }
status is_priest_angel() { return 1; }
string query_blocked_dir() { return blocked_dir; }

status query_ctrl() { return 1; }


// *** COMMAND QUEUE HANDLING: ***

void
process_queue() {
    closure *cl;

    cl = cmd_queue[0];
    funcall(cl[1],cl[2]);
    cmd_queue = cmd_queue[1..];
    if(sizeof(cmd_queue)>0) {
	cl = cmd_queue[0];
	funcall(cl[0],&(cl[2]));
	call_out("process_queue",2);
    }
}

void
add_command_to_queue(closure *cl) {
    cmd_queue += ({ cl });
    if(sizeof(cmd_queue) == 1) {
	funcall(cl[0],&(cl[2]));
	call_out("process_queue",2);
    }
}


// *** LONG & SHORT DESCS ***

// CAN_SEE
// returns a value between 0 and 100 (or more), where 100 means player
// can see us perfectly well, and 0 that (s)he can't see us at all

private static int
can_see(object pl) {
    int t;

    if(!pl || (pl == owner)) return 100;
    t = pl->query_stat(ST_INT);
    return 100 * t / vis_level;
}

string
query_short(int mode, object who) {
    int t;
    object pl;

    pl = who;
    if(!pl) pl = this_player();
    t = can_see(pl);
    if(t>=100)
	return ::query_short(mode, who);
    if(random(100) < t)
	return INVIS_SHORTS[random(sizeof(INVIS_SHORTS))];
    return 0;
}

string
query_long(string id, object who) {
    int t;
    object pl;

    pl = who;
    if(!pl) pl = this_player();
    t = can_see(pl);
    if(t>=100)
	return ::query_long(id, who);
    if(random(100) < t)
	return INVIS_LONGS[random(sizeof(INVIS_LONGS))];
    return 0;
}


// *** COMMANDS ***

varargs void cmd_ok(int amount)
{
    owner->add_fp(-COMMAND_COST);
    if(amount) {
	// let's increase player's skills somewhat
	PRIEST_SP_DIR "angel"->inc_skills(owner,
	  owner->query_guild_object(), amount);
    }
}


// is this too good???
void
do_outrage(object target) {
    int dam;

    if(!target) return;
    if(!target || !present(target, ENV())) {
	if(owner) owner->tell_me("Your angel has lost it's target!");
	return;
    }
    if(!query_can_move()) {
	if(owner) owner->tell_me("The angel is suddenly paralyzed, \
and thus can't complete your plea.");
	return;
    }
    TH(":A blue sphere bursts around %<me.name>% making %<me.objective>% \
wither in pain!",0,0,target,target,0,0);
    switch(can_see(target)) {
    case 0..59:
	target->tell_me("A bright sphere of blue light bursts around you \
making you wither in pain!"); break;
    case 60..99:
	target->tell_me("A shimmering being creates a painfully bright \
sphere of blue light around you, making you wither in pain!"); break;
    default:
	target->tell_me("An angel creates a sphere of painfully bright \
light around you, making you wither in pain!");
    }
    add_sp(-OUTRAGE_COST);
    dam = max_damage - random(max_damage/2);
    if(query_once_interactive(target))
	dam = 3*dam/4;
    //    target->hit_player(dam, DT_LIGHT, 5, TO);
    target->hit_player(dam, DT_FIRE, 0, TO);
    //Changed to DT_FIRE so that thieves can tumble it //Chopin
    if(target) target->attacked_by(owner,0);
    return;
}


void
begin_outrage(object target) {
    if (!target) return;      // Extra check added by Doomdark 13-sep-97
    // (was causing some errors)
    if(!target->attacked_by(owner,-1)) {
	owner->tell_me("Your angel refuses to attack anything here.");
	target = 0;
	return;
    }
    if(blocked_dir) {
	owner->tell_me("Your angel can't do that while blocking an exit.");
	target = 0;
	return;
    }
    if(query_sp() < OUTRAGE_COST) {
	owner->tell_me("Your angel is too exhausted to do that right now.");
	target = 0;
	return;
    }
    owner->tell_me(":You ask your guardian angel to attack \
%<him.name>%.",0,0,owner,target,0);
}


int
get_attack_spell_train(object me, object target) {
    int my_lvl, t_lvl;
    int d;
    object sym;

    if(!target) return 0;
    my_lvl = me->query_level();
    t_lvl = target->query_level();

    d = 100 + 100*(t_lvl - my_lvl)/(my_lvl + t_lvl);
    d += target->query_ac()/10 + target->query_wc()/10;
    d += target->query_spell_dmg() / 10 + target->query_breath_dmg()/5;
    sym = owner->query_guild_object();
    if(sym->query_debugger())
	sym->inform_debugger(sprintf("SP train: %d\n", d));
    return d;
}


status
special_attack(string tar_name) {
    object target;
    mixed x;

    if(!tar_name) tar_name = "";
    target = TARGET_D->find_target(owner, tar_name);
    if(!target) return 1;

    x = ({ #'begin_outrage, #'do_outrage, target });
    add_command_to_queue(x);
    cmd_ok(get_attack_spell_train(owner, target) / 3);
    return 1;
}


void
go_back_to_owner() {
    if(owner && interactive(owner)) {
	move_object(TO, ENV(owner));
	owner->tell_me("Your guardian angel arrives.");
    }
    else move_player("X", PRIEST_ROOM_DIR "oblivion");
}


void
deliver_stuff(object target) {
    int i;
    object ob, *inv, *given, *dropped;
    string tar_str, env_str;

    if(!target) {
	transfer_all_to(PRIEST_ROOM_DIR "temp_storage");
    } else if(!interactive(target)) {
	owner->tell_me("The player you wanted your angel to deliver \
it's bearings, has become link-dead. Delivering them to guild instead.");
	transfer_all_to(PRIEST_ROOM_DIR "temp_storage");
    } else {
	move_object(TO, ENV(target));
	given = ({ });
	dropped = ({ });
	inv = all_inventory();
	for(i=sizeof(inv);--i>=0;) {
	    if(transfer((ob = inv[i]), target)) {
		dropped += ({ ob->query_short() });
		move_object(ob, ENV(target));
	    } else
		given += ({ ob->query_short() }); 
	}
	tar_str = "An angel appears.";
	env_str = "You feel something arriving.";
	if(sizeof(given)>0) {
	    tar_str += sprintf("\nIt gives you %s.",
	      implode(given, ", ", " and "));
	}
	if(sizeof(dropped)>0) {
	    tar_str += sprintf("\nIt drops %s in front of you.",
	      implode(dropped, ", ", " and "));
	    env_str += "\nSomething is dropped just in the front of " +
	    target->query_name();
	}
	target->tell_me(tar_str);
	TH(env_str, target);
    }
    call_out(#'go_back_to_owner, 2);
}

status
deliver_cmd(string target_name) {
    int w;
    object target;

    if(!target_name)
	return 0;
    if(target_name == "guild")
	target = 0;
    else {
	target = find_player(target_name);
	if(!target)
	    return notify_fail("No such player.\n"),0;
	if((target->query_guild() != GN_PRIEST) &&
	  !GUEST_D->query_guest(target_name))
	    return notify_fail("That player is not a priest.\n"),0;
    }
    if(environment()->query(PREVENT_TELEPORT_FROM))
	return notify_fail("Something prevents your angel to do that.\n"),0;
    if(query_local_weight() == 0)
	return notify_fail("Your angel is not carrying anything.\n"),0;

    owner->tell_me("The angel vanishes.");
    move_object(TO, PRIEST_ROOM_DIR "oblivion");
    call_out(#'deliver_stuff, 2 + random(2), target);
    cmd_ok();
    return 1;
}


status
give_back_cmd(string what) {
    object ob;

    if(!what)
	return notify_fail("Most certainly your angel is not carrying it.\n"),0;
    if(what == "all") {
	transfer_all_to(owner);
	owner->tell_me("Angel gives you everything it's carrying.");
	return 1;
    }
    if(!(ob = present(what, TO)))
	return notify_fail("But angel is not carrying anything called that.\n"),0;
    if(transfer(ob, owner)) {
	move_object(ob, ENV(owner));
	owner->tell_me("You can't carry so much. Angel drops it on the \
ground."),0;
    }
    owner->tell_me("Angel gives it to you.");
    cmd_ok();
    return 1;
}


status
block_dir_cmd(string arg) {
    mapping tmp;
    string* exits;
    int follow;

    if(!arg) {
	if(blocked_dir) {
	    add_block(blocked_dir,0,0,0,0,0,0,0);
	    owner->tell_me(sprintf("Ok, angel stops blocking '%s' exit.",
		blocked_dir));
	    blocked_dir = 0;
	    return 1;
	}
	return notify_fail("Which exit do you want your angel to block?\n"),0;
    }
    if(blocked_dir) return
	notify_fail(sprintf("But your angel is already blocking '%s'.\n",
	    blocked_dir)),0;
    if(environment()->query(NO_PK))
	return notify_fail("Player killing is not allowed here, and thus \
your guardian angel refuses to block any exits.\n"),0;
    tmp = environment()->query_exits();
    if(!tmp) return
	notify_fail("This room doesn't have any exits that could be blocked!\n"),0;
    exits = m_indices(tmp);
    if(member(exits, arg)<0) return
	notify_fail("No such exit here.\n"),0;
    blocked_dir = arg;
    add_block(blocked_dir, "guild", 0, "priest",
      ":%<me.capname>% blocks your way!",
      ":%<me.capname>% blocks %<him.gen_possessive>% way.",
      ":%<me.capname>% lets you pass.",
      ":%<me.capname>% lets %<him.name>% pass.");
    owner->tell_me(sprintf("You ask your angel to block '%s'.", arg));
    follow = SLAVE_D->query_follow(owner, TO);
    if(follow & 1) {
	owner->tell_me("Your angel stops following you.");
	SLAVE_D->set_follow(owner, TO, 2);
    }
    cmd_ok();
    return 1;
}


status
follow_cmd(status follow) {
    if(!can_follow)
	return notify_fail("You don't know how to do that.\n"),0;
    if(follow == (SLAVE_D->query_follow(owner, this_object()) & 1)) {
	return notify_fail(follow ? "But it's already following you!\n":
	  "But it's not following you!\n"),0;
    }
    owner->tell_me(sprintf("You ask your angel to %s.", follow ?
	"follow you" : "stay here"));
    if(follow && blocked_dir) {
	owner->tell_me("Your angel stops blocking.");
	add_block(blocked_dir,0,0,0,0,0,0,0);
	blocked_dir = 0;
    }
    cmd_ok();
    return SLAVE_D->set_follow(owner, TO, follow | 2);
}


void defend_cmd()
{
    object target;

    if(!(target = owner->is_fighting()))
	return notify_fail("But you are not fighting with anyone.\n"),0;
    if(ENV(target) != ENV(owner))
	return notify_fail("But your attacker is not here!\n"),0;
    if(IS_PRIEST(target))
	return notify_fail("Your guardian angel refuses to attack another \
child of " GOD_NAME ".\n"),0;
    owner->tell_me("You pray your guardian angel to defend you against "
      + target->query_name() + ".");
    target->stop_fight(owner);
    owner->stop_fight(target);
    target->attacked_by(owner,1);
    target->attacked_by(TO,1);
    if(target) attack_object(target);
    cmd_ok(get_attack_spell_train(owner, target)/3);
}


status
come_back_cmd() {
    if(ENV() == ENV(owner))
	return notify_fail("But your angel is already here!\n"),0;
    if(blocked_dir) {
	owner->tell_me("Your angel stops blocking.");
	add_block(blocked_dir,0,0,0,0,0,0,0);
	blocked_dir = 0;
    }
    move_object(TO, ENV(owner));
    owner->tell_me("Your angel arrives.");
    return 1;
}


status
do_cmd(string cmd, string arg) {
    mixed x;

    // leave is the only command that doesn't use sp / train skills
    if(cmd == "leave") {
	owner->tell_me("You ask your angel to leave you.");
	call_out("vanish", 1);
	return 1;
    }

    if((cmd != "come") && (ENV() != ENV(owner))) return
	notify_fail("But your angel is not here!\n"),0;

    if(owner->query_fp() < COMMAND_COST) return
	notify_fail("You are too tired to communicate with your angel.\n"),0;

    switch(cmd) {
    case "stay":
	return follow_cmd(0);

    case "follow":
	return follow_cmd(1);

    case "defend":
	x = ({ #'defend_cmd, 0, 0 });
	add_command_to_queue(x);
	return 1;

    case "outrage":
	return special_attack(arg);

    case "deliver":
	return deliver_cmd(arg);

    case "give":
	return give_back_cmd(arg);

    case "come":
	return come_back_cmd();

	// New command!
    case "block":
	return block_dir_cmd(arg);
    }
    return 0;
}


/*** OTHER FUNCS ***/

varargs void
vanish(status dont_destruct)
{
    SLAVE_D->remove_slave(owner, TO);
    if(environment()) {
	TH(":%<me.capname>% returns to its own plane of existance.",
	  0,0,0,TO,0,0);
    }
    remove_call_out("process_queue");
    transfer_all_to(environment());
    if(!dont_destruct)
	destruct(TO);
    return;
}


int
monster_died(object me, object killer) {
    DELAY_D->set_delay(owner, DELAY_TYPE_SUMMON, ANGEL_DIED_DELAY);
    vanish(1);
    return 0;
}

void
inform_owner() {
    while(remove_call_out(#'inform_owner)>=0);
    owner->tell_me("You decide not to attack your angel after all.");
}

int
attacked_by(object ob, int flags) {
    if(ob == owner) {
	owner->stop_fight(TO);
	stop_fight(owner);
	call_out(#'inform_owner,2);
	return -1;
    }
    return ::attacked_by(ob, flags);
}


status
attack() {
    if(object_name(environment()) == "room/church")
	return 0;
    return ::attack();
}


void
check_name(int skill_rank) {
    string tmp;

    tmp = TITLES[skill_rank/2];
    set_name(tmp);
    set_short(tmp);
    // set_long??
}


varargs void
set_owner(mixed pl, int skill)
{
    int lvl;
    int skill_rank;

    if(stringp(pl)) {
	skill = 990;
	pl = find_player(pl);
    }
    if(!pl) {
	SLAVE_D->remove_slave(owner, TO);
	owner = 0;
	return;
    }
    skill_rank = get_skill_rank(skill2pex(skill));
    owner = pl;
    SLAVE_D->add_slave(owner, TO);
    SLAVE_D->set_follow(owner, TO, 2); // no moving messages

    vis_level = pl->query_stat(ST_INT);
    vis_level -= random(vis_level/4);
    max_damage = skill_rank * 150 / MAX_SKILL_RANK;
    can_follow = 1;

    lvl = pl->query_level()/3;
    lvl += random(33)*lvl/100;
    lvl += skill * lvl / 1000;
    set_level(lvl);
    set_hp(lvl*(20+random(5)) + random(100));
    set_ep(0);
    set_stat(ST_STR, skill/50); // max 20
    check_name(skill_rank);
}


void
create_monster()
{
    set_name("angel");
    set_id( ({"shimmering being", "being", "angel", "priest_angel"}) );
    set_short("Guardian angel");

    set_gender(G_MALE);
    set(LIV_HEAL_RATE,1);
    set_level(20);
    set_guild(GN_PRIEST);
    set_dead_ob(this_object());
    set_ep(0);
    set(NPC_NO_CORPSE,1);

    load_chat(2, ({
	"You feel a warm breeze on your skin.",
	"A shimmering being floats in the air.",
      }));
    vis_level = 1;
    cmd_queue = ({});
}


self_destruct() {
    object ob;
    ob = find_player("chopin");
    if(ob) ob->tell_me("Self_destruct called for angel.");
}


debug() {
    printf("vis_level: %d\nmax_damage: %d\nowner: %O\ncan_follow: %d\n\
deliver_status: %d\nlast_room: %O\ncmd_queue: %O\n", vis_level, max_damage,
      owner, can_follow, deliver_status, last_room, cmd_queue);
}

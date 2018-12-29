/*
 * A shield daemon written by Chopin
 *
 * Now there's three kind of shields: barkskin, (normal) shield and
 * fireshield. Fireshield is like normal shield, but hits the attacker
 * back, and protects you better against fire. Barkskin is a normal
 * armour object, and it can be used simultaneously with shields
 * handled by this daemon.
 */

#include <attack.h>
#include "../priest.h"

#define MAX_SHIELD_DUR 250

// how much sps can be stored to the shield to hit back the attacker
#define MAX_SPS 100

// minimum amount of sps required for fireshield to do any damage
#define MIN_SPS 15

// in seconds, how early we will warn player before shield ends
#define WARNING_TIME 10 

// how much our shield does become weaker when we are hit hard enough?
#define SHIELD_WEAKENING_PERCENTAGE 10

// how much damage is needed to have 100% chance that shield becomes
// weaker, if not taking hc into account
#define DAMAGE_LIMIT 200

// amount of sps a shield consumes per hit
#define SP_COST 2


#define K_DURATION 0
#define K_SKILL 1
#define K_TIME 2
#define K_TYPE 3
#define K_SPS_STORED 4

// someone write better messages
static string *messages = ({
    ":As you hit %<me.name>%, a shimmering field around \
%<me.objective>% flares red and burns you!",
    ":As %<him.name>% hits you, the protective force field \
around you flares red and burns %<him.objective>%!",
    ":A shimmering field around \
%<me.name>% flares red and burns %<him.name>%!",
});

#define M_FIRESHIELD_HITTER 0
#define M_FIRESHIELD_ME 1
#define M_FIRESHIELD_OTHERS 2

mapping players;

// prototypes:
int query_guild_hit_player(int dmg, int dt, int hc, object hitter);
int end_shield(object pl);


create() {
    players = allocate_mapping(0, 5);
}


void
update_pl(object pl) {
    int d;
    
    d = time() - players[pl, K_TIME];
    players[pl, K_TIME] = time();
    players[pl, K_DURATION] -= d;
}


void
remove_pl(object pl) {
    m_delete(players, pl);
}


// remember to check, that if the player is already shielded, 
// his/her current shield type matches.
void
add_shield(object pl, int dur, int skill, int type, int sps) {
    if(dur<WARNING_TIME)
        dur = WARNING_TIME + 1;
    if(!member(players,pl)) {
        players += ([pl: dur; skill; time(); type; sps]);
        pl->set_guild_hook(
            G_HOOK_HIT_PLAYER,#'query_guild_hit_player);
        call_out(#'end_shield, dur-WARNING_TIME, pl);
    }
    else {
        update_pl(pl);
        if((players[pl, K_DURATION]+= dur) > MAX_SHIELD_DUR)
            players[pl, K_DURATION] = MAX_SHIELD_DUR;
        players[pl, K_SKILL] = skill;
        if((players[pl, K_SPS_STORED] += sps) > MAX_SPS)
            players[pl, K_SPS_STORED] = MAX_SPS;
        
    }
}


int
query_shield(mixed pl, int type, int sps_stored) {
    if(stringp(pl)) pl = find_player(pl);
    if(!pl || !member(players,pl)) {
        type = sps_stored = 0;
        return 0;
    }
    update_pl(pl);
    type = players[pl, K_TYPE];
    sps_stored = players[pl, K_SPS_STORED];
    return players[pl, K_DURATION];
}


mapping
query_players() { return players; }


void
end_shield(object pl) {
    int dur_left;
    
    if((!pl) || (!member(players, pl))) {
        m_delete(players, pl);
        return;
    }
    update_pl(pl);
    dur_left = players[pl, K_DURATION];
    
    if(dur_left>WARNING_TIME) {
        call_out(#'end_shield, dur_left - WARNING_TIME, pl);
        return;
    }
    
    if(dur_left>0) {
        pl->tell_me("You feel the protective flux surrounding you \
is about to wear out.");
        call_out("end_shield", 2, pl);
        return;
    }
    pl->tell_me("You feel exposed and vulnerable.");
    pl->set_guild_hook(G_HOOK_HIT_PLAYER, 0);
    m_delete(players, pl);
}


void fireshield_damage(object pl, object hitter, int skill)
{
    int dmg;
    object sym;

    if(!hitter || !pl) return;
    if(environment(pl) != environment(hitter)) return;

    if(players[pl, K_SPS_STORED] < MIN_SPS) return;

    if (!random(10) && !present("on_fire", hitter)) {
        move_object(clone_object(PRIEST_OBJ_DIR "on_fire"), hitter);
    }
        
    dmg = BALANCE_D->damage(skill, GN_PRIEST, players[pl, K_SPS_STORED], 0) / 2;
    if(query_once_interactive(hitter))
        dmg = 60 * dmg / 100;
    dmg -= random(dmg / 2);
    players[pl, K_SPS_STORED] = 3*players[pl, K_SPS_STORED]/4;

    hitter->tell_me(&(messages[M_FIRESHIELD_HITTER]),0,0,pl,0,0);
    pl->tell_me(&(messages[M_FIRESHIELD_ME]),0,0,pl,hitter,0);
    environment(hitter)->tell_here(
        &(messages[M_FIRESHIELD_OTHERS]),0,0,({pl,hitter}),pl,hitter,0);

    hitter->hit_player(dmg, DT_FIRE, 0, pl);

    if(sym = pl->query_guild_object())
        sym->inform_debugger(sprintf("Fire shield's damage: %d.", dmg));
}


int
query_guild_hit_player(int dmg, int dt, int hc, object hitter) {
    int skill, t, new_dmg;
    object pl, sym;

    if(!intp(hc)) hc = 0; // It can be a closure too
    // Chopin 16-May-98

    pl = previous_object();
    if(!member(players,pl)) {
        pl->set_guild_hook(G_HOOK_HIT_PLAYER, 0);
        return 0;
    }
    if(dt == DT_FORCE) return dmg; // can't be avoided
    if((!living(hitter) || hitter == pl) && dt != DT_CRUSH)
        return dmg;
    // No protection against traps etc
    // unless damage type is DT_CRUSH

    skill = players[pl, K_SKILL];
    
    if(pl->query_sp() < SP_COST) {
        pl->tell_me("You become too tired to maintain your shield spell. \
It wears out.");
        m_delete(players, pl);
        pl->set_guild_hook(G_HOOK_HIT_PLAYER, 0);
        return 0;
    }
    
    if(pl->query_sp() < SP_COST * 4) {
        pl->tell_me("You feel that you can't maintain your shield spell \
much longer anymore.");
    }
    
    pl->add_sp(-SP_COST);
    
    t = skill / 40;
    t = 100 - t - random(t);
    // at max skill average is 37.5%  (was 45%)
    // too much, too little? any comments?
    
    switch(players[pl, K_TYPE]) {
    case ST_FIRE:
        switch(dt) {
        case DT_FIRE:
            t = 60 * t / 100;
            pl->tell_me("You feel well protected from fire.");
            break;
        case DT_COLD:
            t = t * 2;
            pl->tell_me("Cold hits you through your shield as if \
it didn't exist.");
            pl->add_sp(-random(10));
            break;
        }
 
        if(30 + random(70) > t)
            call_out(#'fireshield_damage, 1, pl, hitter, skill);

    default:

    }

    new_dmg = t*dmg/100;
    
// is 3*hc too much or too little?
    if((dmg + 3*hc) > random(DAMAGE_LIMIT)) {
        players[pl, K_SKILL] -= SHIELD_WEAKENING_PERCENTAGE*skill/100;
        pl->tell_me("You feel slight weakening in the protective flux \
surrounding you.");
    }

    if(dmg == 0) dmg = 1;
    sym = pl->query_guild_object();
    if(sym->query_debugger()) {
        sym->inform_debugger(sprintf("skill:%d. %d/%d (%d%%) of damage received.", 
            skill, new_dmg, dmg, 100*new_dmg/dmg));
    }
    
    return new_dmg;
}


// debug information:
void
debug() {
    this_player()->tell_me(sprintf("%O",players));
}

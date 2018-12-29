#include "priest_spell.h"
#include <attack.h>

static string *messages = ({
    ({
      ":%<me.capname>% mumbles a silent but fervent prayer.",
      "You hear someone mumbling a prayer.",
      ":%<me.capname>% seems to mumble something.",
    }),
});

#define M_BEGIN 0

#define MAX_SPS 100

void begin_fireshield(object who, mixed data);
status do_fireshield(object who, mixed data);

void
create_spell() {
    spell_type = SP_DEFENSIVE;
    dif = 40;
    lvl = 35;
    cost = 45;
    skills_used = ({SK_SERENITY, SK_FORCES, SK_NATURE});
    skill_p = ({30,50,20});
    funs = ({ #'begin_fireshield, #'do_fireshield });
}


void
begin_fireshield(object who, mixed data) {
    int type, has_shield, amount;
    
    has_shield = SHIELD_D->query_shield(who, &type);
    if(type && type != ST_FIRE) {
        who->tell_me("You already have some other kind of shield protecting you!");
        data = 0;
        return;
    }
    if(sscanf(data, "%d", amount) != 1)
        amount = 0;
        
    if(!has_shield) {
        who->tell_me("You mumble a silent prayer to " GOD_NAME
" entreating his protection for the troubled times ahead.");
        ENV(who)->tell_here(messages[M_BEGIN], ({
        LIV_CAN_SEE | LIV_CAN_HEAR, LIV_CAN_HEAR, LIV_CAN_SEE
        }),0,0,who,who,0,0);
    } else {
        who->tell_me("You call upon " GOD_NAME " asking him to \
extend his protecting hand over you.");
    }
    data = ({amount, has_shield});
}
 

status
do_fireshield(object who, mixed data) {
    int dur, skill, new_cost, amount, sps_stored;
    int cur_dur, type, can_be_stored;
    int already_shielded;
    object sym;
    
    amount = data[0];
    already_shielded = data[1];
    
    sym = who->query_guild_object();
    skill = get_skill(sym);
    dur = 15 + skill/8;

    cur_dur = SHIELD_D->query_shield(who, &type, &sps_stored);
    if(already_shielded) {
        who->tell_me("You feel the force field around you growing \
stronger.");
        dur -= random(dur/2);
        // max shield is 240 -> max cost is 50
        new_cost = 20 + cur_dur / 8;
    } else {
        who->tell_me("You feel a force field forming around you.");
        dur -= random(dur/3);
        new_cost = cost;
    }
    who->add_sp(-new_cost);
    can_be_stored = MAX_SPS - sps_stored;
    if(!can_be_stored) {
        who->tell_me("Shield already had maximum amount of mana stored to it.");
        amount = 0;
    }
     else if(amount > can_be_stored) {
        who->tell_me(sprintf("Only %d spell points could be stored.",
            can_be_stored));
        amount = can_be_stored;
    }
    if(who->query_sp() < amount) {
        amount = who->query_sp();
        who->tell_me("You don't have that much spell points. Storing "+
            amount +" sps instead.");
    }
    who->add_sp(-amount);
    SHIELD_D->add_shield(who, dur, skill, ST_FIRE, amount);
    
    inc_skills(who,sym,100);
    return 1;
}

#include "priest_spell.h"
#include <attack.h>

static string* messages = ({
    ({
      ":%<me.capname>% mumbles a silent but fervent prayer.",
      "You hear someone mumbling a prayer.",
      ":%<me.capname>% seems to mumble something.",
    }),
});

#define M_BEGIN 0

void begin_shield(object who, mixed data);
status do_shield(object who, status already_shielded);

void
create_spell() {
    spell_type = SP_DEFENSIVE;
    dif = 0;
    lvl = 7;
    cost = 35;
    skills_used = ({SK_SERENITY, SK_FORCES});
    skill_p = ({30,70});
    funs = ({ #'begin_shield, #'do_shield });
}


void
begin_shield(object who, mixed data) {
    int type, has_shield;
    int dummy;
    
    has_shield = SHIELD_D->query_shield(who, &type, &dummy);
    if(type && type != ST_NORMAL) {
        who->tell_me("You already have other kind of shield protecting you!");
        data = 0;
        return;
    }
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
    data = 1+has_shield;
}
 

status
do_shield(object who, int already_shielded) {
    int dur, skill, new_cost;
    object sym;
    
    already_shielded -= 1;
    
    sym = who->query_guild_object();
    skill = get_skill(sym);
    dur = 15 + skill/5;

    if(already_shielded) {
        who->tell_me("You feel the protective flux around you growing \
stronger.");
        dur -= random(dur/2);
        // max shield is 240 -> max cost is 50
        new_cost = 20 + SHIELD_D->query_shield(who) / 8;
    } else {
        who->tell_me("You feel a protective flux forming around you.");
        dur -= random(dur/3);
        new_cost = cost;
    }
    SHIELD_D->add_shield(who, dur, skill, ST_NORMAL, 0);
    who->add_sp(-new_cost);
    inc_skills(who,sym,100);
    return 1;
}

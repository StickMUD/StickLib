#include "priest_spell.h"

void begin_stonerain(object who, mixed target);
status end_stonerain(object who, mixed target);

static string *messages = ({
    ":%<me.capname>% raise%<me.ending_s>% %<me.possessive>% \
hands towards the heavens, chanting a prayer.",
    ":You call a rain of stones down to hit %<him.name>%. \
%<him.capname>% screams in pain as the stones hit %<him.objective>%.",
    ":As %<me.name>% suddenly lowers %<me.possessive>% hands, \
a rain of stones hits you hard, making you scream in pain!",
    ":Suddenly %<me.name>% lowers %<me.possessive>% hands, \
and a massive rain of stones hit %<him.name>%, making %<him.objective>% \
scream in pain.",
});

#define M_BEGIN 0
#define M_END_ME 1
#define M_END_HIM 2
#define M_END_OTHERS 3

void
create_spell() {
    spell_type = SP_OFFENSIVE;
    check_target = 1;
    dif = 30;
    lvl = 25;
    cost = 40;
    spell_damage_rate = 75.0;
    dam_p = 25; // 75%.. full damage
    skills_used = ({SK_SERENITY, SK_NATURE, SK_FORCES});
    skill_p = ({30,35,35});
    acolyte_spell = 1;
    funs = ({ #'begin_stonerain, #'end_stonerain });
}


void
begin_stonerain(object who, mixed target)
{
    HERE->tell_here(&(messages[M_BEGIN]),0,0,0,who,target,0);
}


status
end_stonerain(object who, mixed target)
{
    int dam, t;
    object here, sym;

    sym = who->query_guild_object();
    here = HERE;
    
    who->add_sp(-cost);
    who->tell_me(&(messages[M_END_ME]),0,0,who,target,0);
    target->tell_me(&(messages[M_END_HIM]),0,0,who,target,0);
    here->tell_here(&(messages[M_END_OTHERS]),
        0,0,({who,target}),who,target,0);
    
    inc_skills(who, sym, get_attack_spell_train(who, sym, target));
    
    dam = get_damage(who,sym,interactive(target));
    // let's make sure that we can't cast another attack spell at this round
    who->attacked_by(target,1);
    target->hit_player(dam, DT_EARTH, 7, who);
    return 1;
}

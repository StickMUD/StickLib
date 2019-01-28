#include "priest_spell.h"

void begin_engulf(object who, mixed target);
status end_engulf(object who, mixed target);

static string *messages = ({
  ":%<me.capname>% wave%<me.ending_s>% %<me.possessive>% \
index finger, chanting an ancient prayer.",
  ":You whisper the final words of the ancient prayer, pointing \
at %<him.name>%. Suddenly %<him.capname>% is engulfed by raging \
flames of blue fire, making %<him.objective>% scream in pain!",
  ":As %<me.name>% completes the ancient prayer, your whole body \
is engulfed by raging flames of blue fire!",
  ":As %<me.name>% completes the ancient prayer, %<him.name>% seems \
catch fire and is soon engulfed by raging flames of blue fire!",
});

#define M_BEGIN 0
#define M_END_ME 1
#define M_END_HIM 2
#define M_END_OTHERS 3

void
create_spell() {
    spell_type = SP_OFFENSIVE;
    check_target = 1;
    duration = 2;
    dif = 30;
    lvl = 40;
    cost = 75;
    spell_damage_rate = 150.0;
    order_restriction = ({ 0 }); // only boccob can use
    dam_p = 20; // 80%.. full damage
    skills_used = ({SK_SERENITY, SK_SPIRITS, SK_FORCES});
    skill_p = ({30,35,35});
    funs = ({ #'begin_engulf, #'end_engulf });
}


void
begin_engulf(object who, mixed target)
{
    HERE->tell_here(&(messages[M_BEGIN]),0,0,0,who,target,0);
}

status
end_engulf(object who, mixed target)
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
    target->hit_player(dam, DT_FIRE, 7, who);
    return 1;
}

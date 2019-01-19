#include "priest_spell.h"

void begin_strangulation(object who, mixed target);
status end_strangulation(object who, mixed target);

static string *messages = ({
  ":%<me.capname>% clench%<me.ending_es>% %<me.possessive>% \
fists, chanting a prayer.",
  ":You tighten your grip and point at %<him.name>%, while \
still chanting the final words of the incantation. %<him.capname>% \
tries to scream, but gets no voice out as the air is drained out \
of %<him.possessive>% lungs!",
  ":As %<me.name>% tightens %<me.possessive>% grip, you can feel \
air leaving your lungs! You try to scream but in vain, some force \
seems to strangle you!",
  ":Suddenly %<me.name>% tightens %<me.possessive>% grip \
and points at %<him.name>%, making %<him.objective>% fall down on \
%<him.objective>% knees, begging for mercy.",
});

#define M_BEGIN 0
#define M_END_ME 1
#define M_END_HIM 2
#define M_END_OTHERS 3

void
create_spell() {
    spell_type = SP_OFFENSIVE;
    testing = 1;
    check_target = 1;
    dif = 30;
    lvl = 25;
    cost = 40;
    spell_damage_rate = 75.0;
    dam_p = 25; // 75%.. full damage
    skills_used = ({SK_SERENITY, SK_SPIRITS, SK_FORCES});
    skill_p = ({30,35,35});
    order_restriction = ({ -1, 0 }); // followers of seth can't use
    acolyte_spell = 1;
    funs = ({ #'begin_strangulation, #'end_strangulation });
}


void
begin_strangulation(object who, mixed target)
{
    HERE->tell_here(&(messages[M_BEGIN]),0,0,0,who,target,0);
}

status
end_strangulation(object who, mixed target)
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
    target->hit_player(dam, DT_MAGIC, 7, who);
    return 1;
}


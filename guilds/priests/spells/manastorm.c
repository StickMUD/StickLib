#include "priest_spell.h"

void begin_manastorm(object who, mixed target);
status end_manastorm(object who, mixed target);

static string *messages = ({
    ":The air around %<me.name>% begins to vibrate \
as %<me.pronoun>% speak%<me.ending_s>% out a single word pointing at \
%<him.name>%.",
    ":As %<me.capname>% release%<me.ending_s>% a burst of \
magical powers at %<him.name>%, a bright cloud of pure magical force envelopes \
%<him.objective>%, making %<him.objective>% scream in agony and feel the \
might of " GOD_NAME ".",
    ":The overwhelming force of %<me.gen_possessive>% prayer renders \
%<him.capname>% stunned.",
});

#define M_BEGIN 0
#define M_END 1
#define M_CRITICAL 2


void
create_spell() {
    spell_type = SP_OFFENSIVE;
    check_target = 1;
    dif = 40;
    lvl = 30;
    cost = 60;
    spell_damage_rate = 90.0;
    dam_p = 25; // 75%.. full damage
    skills_used = ({SK_SERENITY, SK_SPIRITS, SK_FORCES});
    skill_p = ({25, 20, 55});
    order_restriction = ({ -1, 0 }); // followers of seth can't use
    funs = ({ #'begin_manastorm, #'end_manastorm });
}

status
is_spell_caster(object who) {
    switch(who->query_guild()) {
    case GN_PRIEST:
#ifdef GN_MAGE
    case GN_MAGE:
#endif
#ifdef GN_NECROMANCER
    case GN_NECROMANCER:
#endif
#ifdef GN_HEALER
    case GN_HEALER:
#endif
        return 1;
    }
    return 0;
}

void
inform_surrounding_rooms(object here, string msg) {
    object ob, *inv, room;
    string *rooms, exit;
    int i,j,t;
    mapping m;
    
    m = here->query_exits();
    if(!mappingp(m)) return;
    
    rooms = m_values(m);
    i = sizeof(rooms);
    while(--i>=0) {
      if (!stringp(rooms[i]) || !(room = find_object(rooms[i])))
              continue;
        m = room->query_exits();
        // added a check 25-Apr-98 //Chopin
        if(!mappingp(m) || !sizeof(m)) break;
        t = member(m_values(m), file_name(here));
        if(t == -1) continue;
        
        exit = m_indices(m)[t];
        inv = all_inventory(room);
        j = sizeof(inv);
        while(--j>=0) {
            ob = inv[j];
            if(ob && interactive(ob) && is_spell_caster(ob))
                ob->tell_me(sprintf(msg, exit));
        }
    }
}

void
begin_manastorm(object who, mixed target)
{
    inform_surrounding_rooms(HERE, "You feel a surge of magical powers \
coming from the %s.");
    HERE->tell_here(&(messages[M_BEGIN]),0,0,0,who,target,0);
}

status
end_manastorm(object who, mixed target)
{
    int dam, old_sp, t, a, i, skill;
    object here, sym, *inv, ob;
    string str;
    
    sym = who->query_guild_object();
    here = HERE;
    
    who->add_sp(-cost);
    
    here->tell_here(&(messages[M_END]),0,0,0,who,target,0);
    
    skill = get_skill(sym);
    a = 1 + get_skill_rank(skill2pex(skill));
    inv = all_inventory(here);
    i = sizeof(inv);
    while(--i>=0) {
        ob = inv[i];
        if(ob && living(ob) && (ob!=who) && (!ob->query_coder_level()) &&
          (!ob->id("priest_angel")) ) {
            old_sp = ob->query_sp();
            t += old_sp - ob->add_sp(-3*a);
            ob->tell_me("You feel your mana being drained!");
        }
    }
    t /= 2;
    if(t>25) t = 25; // max bonus 25
    
    dam = get_damage(who,sym,interactive(target));
    dam = (100+t) * dam / 100;

#if 0
    if(critical_success) {
        target->add_condition(C_STUNNED, 3 + random(a/2));
        HERE->tell_here(&(messages[M_CRITICAL]),0,0,0,who,target,0);
    }
#endif
    who->attacked_by(target,1);
    inc_skills(who, sym, get_attack_spell_train(who, sym, target));
    target->hit_player(dam, DT_MAGIC, 5, who);
    
    return 1;
}

#include "priest_spell.h"

static string *messages = ({
  ":But %<him.name>% is unable to move already.",
  ":%<me.capname>% start%<me.ending_s>% chanting.",
  ":You cast the mighty paralyze spell at %<him.name>%, \
but %<him.pronoun>% seems to be unnaffected by it!",
  ":%<me.capname>% casts a spell at %<him.name>%, \
but %<him.pronoun>% resist%<him.ending_s>%!",
  ":%<me.capname>% casts a spell at %<him.name>%.",
  ":%<me.capname>% looks disappointed.",
});

#define M_BEGIN_FAIL 0
#define M_BEGIN 1
#define M_ALREADY_STUNNED_ME 2
#define M_ALREADY_STUNNED_OTHERS 3
#define M_SUCCESS 4
#define M_FAILURE 5

void begin_paralyze(object who, mixed victim);
status end_paralyze(object who, mixed victim);

void
create_spell() {
    spell_type = SP_OFFENSIVE;
    check_target = 1;
    dif = 0;
    lvl = 12;
    cost = 35;
    skills_used = ({SK_SERENITY, SK_LIFE, SK_SPIRITS});
    skill_p = ({30,40,30});
    order_restriction = ({ -1, 0 }); // followers of seth can't use
    funs = ({ #'begin_paralyze, #'end_paralyze });
}


void
begin_paralyze(object who, mixed victim)
{
    if(victim->query_condition(C_STUNNED)) {
	who->tell_me(&(messages[M_BEGIN_FAIL]),0,0,0,victim,0);
	victim = 0;
	return;
    }
    HERE->tell_here(&(messages[M_BEGIN]),0,0,0,who,0,0);
}


status
end_paralyze(object who, mixed victim)
{
    object here,sym;
    int myluck,eneluck;
    int mylvl,enelvl;
    int i,j,k;
    int dur, skill;

    sym = who->query_guild_object();

    here = HERE;
    if (!victim->attacked_by(who,0)) {
	who->tell_me("You somehow fail to paralyze here!");
	return 1;
    }
    who->add_sp(-cost);

    myluck = who->query_luck();
    eneluck = victim->query_luck();
    mylvl = who->query_level();
    enelvl = victim->query_level();

    skill = get_skill(sym);
    i = (myluck + mylvl) + skill/20;
    j = eneluck + enelvl;
    k = 5 + (mylvl/5); // Give break as level increases.

    inc_skills(who, sym, 50 + j);
    if(random(i) + k >= random(j)) {
	dur = BALANCE_D->duration(skill,GN_PRIEST,cost);
	if(interactive(victim)) dur /= 4;
	sym->inform_debugger("Duration: " + dur + ".");
	if(!victim->set_condition(C_STUNNED,dur)) {
	    who->tell_me(&(messages[M_ALREADY_STUNNED_ME]),0,0,0,victim,0);
	    here->tell_here(&(messages[M_ALREADY_STUNNED_OTHERS])
	      ,0,0,who,who,victim,0);
	} else {
	    who->tell_me("You cast the mighty paralyze spell.");
	    here->tell_here(&(messages[M_SUCCESS]),0,0,who,who,victim,0);
	}
	who->attacked_by(victim,0);
    }
    else {
	who->tell_me(GOD_NAME+" is against you, you fail in the spell!");
	here->tell_here(&(messages[M_FAILURE]),0,0,who,who,0,0);
    }
    return 1;
}

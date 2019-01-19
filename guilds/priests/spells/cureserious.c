#include "priest_spell.h"

static string* messages = ({
  ":%<me.capname>% starts chanting a prayer, closing \
%<me.possessive>% eyes and making a gesture with %<me.possessive>% left \
hand before %<me.possessive>% face.",
  ":You start chanting the mighty healing prayer, \
closing your eyes, and raising your left hand towards %<him.name>%.",
  ":%<me.capname>% start%<me.ending_s>% chanting \
a prayer, raising %<me.possessive>% left hand towards %<him.name>%.",
  ":You succeeded! %<cap(him.gen_possessive)>% wounds \
heal remarkably!",
  ":%<me.capname>% succeeds in %<me.possessive>% prayer, \
%<cap(him.gen_possessive)>% wounds heal remarkably!",
  ":%<me.capname>% ends %<me.possessive>% chanting, \
but looks rather disappointed." ,
});

#define M_BEGIN_1 0
#define M_BEGIN_2 1
#define M_BEGIN_3 2
#define M_END_ME 3
#define M_END_OTHERS 4
#define M_FAIL 5

void begin_cureserious(object who, mixed victim);
status end_cureserious(object who, mixed victim);

void
create_spell() {
    spell_type = SP_HEALING;
    check_target = 1;
    dif = 0;
    lvl = 15;
    cost = 40;
    finish_cost = 2*cost/3;
    skills_used = ({SK_SERENITY, SK_LIFE});
    skill_p = ({30,70});
    funs = ({ #'begin_cureserious, #'end_cureserious });
}

void
begin_cureserious(object who, mixed victim)
{
    if(who==victim) {
	who->tell_me("You start chanting the mighty healing prayer, closing \
your eyes and touching your forehead with your left hand's fingertips.");
	HERE->tell_here(&(messages[M_BEGIN_1]),0,0,who,who,0,0);
    }
    else {
	who->tell_me(&(messages[M_BEGIN_2]),0,0,who,victim,0);
	HERE->tell_here(&(messages[M_BEGIN_3]),0,0,0,who,victim,0);
    }
    // Now, 1/3 of total cost subtracted...
    who->add_sp(-(cost-finish_cost));
}


status
end_cureserious(object who, mixed victim)
{
    int amount, heal, old_hp_fp, skill;
    object here, sym;

    sym = who->query_guild_object();
    here = HERE;
    skill = get_skill(sym);

    if (skill_roll(sym, skill)>0) {
	who->tell_me(&(messages[M_END_ME]),0,0,0,victim,0);
	here->tell_here(&(messages[M_END_OTHERS]),0,0,who,who,victim,0);

	// And rest of the sp cost is used here...
	who->add_sp(-finish_cost);
	old_hp_fp = victim->query_hp() + victim->query_fp();
	amount = BALANCE_D->healing(skill,GN_PRIEST,cost);
	amount -= random(2*amount/7);

	if (critical_success) {
	    who->tell_me("You feel " GOD_NAME " is exceptionally \
pleased with you today!");
	    amount = amount * (105 + random(96)) / 100;
	}

	heal = (victim->add_hp(amount) + victim->add_fp(amount) - old_hp_fp) / 2;
	sym->inform_debugger(sprintf("Heal amount: %d, real heal: %d", amount, heal));

	// Should this be removed??
	if (victim != who && sym->is_valid_heal(victim))
	    who->add_exp(heal * 10);

	// no train if player already has full hp / fp
	inc_skills(who,sym,heal * 100 / amount);
    }
    else {
	who->tell_me("But you fail. " GOD_NAME " wasn't on your side \
this time...");
	here->tell_here(&(messages[M_FAIL]) ,0,0,who,who,0,0);
	inc_skills(who,sym,100);
    }
    return 1;
}

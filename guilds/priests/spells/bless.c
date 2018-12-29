#include "priest_spell.h"
#include <attack.h>
#include <curse.h>

#define MAX_SP_USAGE_FROM_POOL 1000
#define MAX_SP_BONUS 500
#define CRIT_FAIL_LIMIT -900
#define CRIT_SUCCESS_LIMIT 990

static string *messages = ({
    ({":%<me.capname>% succeed%<me.ending_s>% in %<me.possessive>% \
prayer! %<cap(it.name)>% glows in %<me.gen_possessive>% hands when \
%<me.name>% take%<me.ending_s>% it from the ground."}),
    ({":%<me.capname>% looks dissapointed."}),
    ({
      ":%<me.capname>% kneel%<me.ending_s>% down placing %<it.name>% \
on the ground and start%<me.ending_s>% chanting a prayer.",
      "You hear someone chanting a prayer.",
    }),
});

#define M_SUCCESS 0
#define M_FAILURE 1
#define M_BEGIN 2

void begin_bless(object who, mixed data);
void bless_it(object who, mixed *data);

void
create_spell() {
    spell_type = SP_SPECIAL;
    dif = 40;
    lvl = 24;
    cost = 100;
    duration = 5;
    skills_used = ({SK_SERENITY, SK_FORCES, SK_NATURE});
    skill_p = ({30,30,40});
    funs = ({ #'begin_bless, #'bless_it });
}


status bless_it(object who, mixed *data) {
    int t, success, max, cur;
    object room, sym;
    object ob;
    int skill;

    ob = data[0];
    skill = data[1];
    sym = who->query_guild_object();
    who->add_sp(-cost);
    inc_skills(who, sym, 100);
    if(environment(ob) != who) {
      who->tell_me("You lose our concentration.");
      return 0;
    }

    cur = ob->query_curse_level();
    max = ob->query_max_curse_level();
    if(max)
      t = 1000*cur/max;
    else {
      who->tell_me("You can't bless it.");
      return 0;
    }
    success = random(skill) - t;

    room = environment(who);
    t = 0;
    if(room->query_priest_moonwell() && (t = room->query_sp())) {
      if(t>MAX_SP_USAGE_FROM_POOL)
          t = MAX_SP_USAGE_FROM_POOL;
      room->add_sp(-t);
      write_file(PRIEST_DIR "log/moonwell",sprintf("%s: %s -%d sps.\n",
          ctime(time()), capitalize(who->query_real_name()), t));
    }
    success += random(t*MAX_SP_BONUS/MAX_SP_USAGE_FROM_POOL) - MAX_SP_BONUS;

    sym->inform_debugger(sprintf("Skill: %d, Success: %d, t: %d",
        skill, success, t));

    if(success > 0) {
      cur += 1;
      if(success > CRIT_SUCCESS_LIMIT) cur++;
      if(cur>max) cur = max;
      ob->set_curse(cur, CT_BLESSED);
      ob->add_identified_by(who);
      room->tell_here(
        messages[M_SUCCESS],
        ({LIV_CAN_SEE}),0,0,who,0,ob);
    } else {
      who->tell_me("You finish your chanting, but nothing happens.");
      room->tell_here(messages[M_FAILURE],
        ({LIV_CAN_SEE}),0,who,who,0,0);
      if(success < CRIT_FAIL_LIMIT) {
          cur -= 1+random(2);
          if(cur>0) ob->set_curse(cur, CT_BLESSED);
          else if(cur==0) ob->set_curse(0, CT_NONE);
          else {
              ob->set_curse(cur, CT_CURSED);
              // let's not inform player :P
          }
      }
    }

}


void
begin_bless(object who, mixed data) {
    object ob, sym;
    int skill;
    string str;

    str = data;
    data = 0;
    sym = who->query_guild_object();
    if(str == "") {
      who->tell_me("Bless what?");
      return;
    }

    if(!who->query_can_see()) {
      who->tell_me("You can't see anything!");
      return;
    }

    ob = present(lower_case(str),who);
    if(!ob) {
      who->tell_me("You don't have anything called that.");
      return;
    }

    skill = get_skill(sym);

    if(ob->is_cursed()) {
      who->tell_me("Cursed items can't be blessed.");
      return;
    }
    if(!ob->is_curseable() || !ob->query_max_curse_level()) {
      who->tell_me("You can't bless it.");
      return;
    }
    if(ob->is_weapon()) {
      if(ob->is_wielded()) {
          who->tell_me("Unwield it before blessing.");
          return;
      }

    } else if(ob->is_armour()) {
      if(ob->is_worn()) {
          who->tell_me("Remove it before blessing.");
          return;
      }
    } else {
      who->tell_me("Only armours and weapons can be blessed.");
      return;
    }

    HERE->tell_here(messages[M_BEGIN],
        ({ LIV_CAN_SEE, LIV_CAN_HEAR }),0,0,who,0,ob);
    data = ({ob, skill});
}

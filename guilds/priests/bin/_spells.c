#include "../priest.h"
#include "../skills.h"

#include "get_difficulty.c"

#define SK_STRS ({ "SE", "KL", "KN", "SP", "WF", "AL", "ST" })

mixed *spells;
// name, level, cost, difficulty, coder skill str, player skill str, damage rate


status
sort_by_level(mixed s1, mixed s2) {
    return s1[1] == s2[1] ? s1[0] > s2[0] : s1[1] > s2[1];
}

status
sort_by_name(mixed s1, mixed s2) {
    return s1[0] > s2[0];
}

void create()
{
    int n;
    object ob;
    string cost_str;
    string dif_str;
    int i,j,*skills_used,*skill_p, damage_rate;
    string *temp, s, coder_skill_str, player_skill_str;

    temp = get_dir(PRIEST_SP_DIR);
    spells = ({});
    i = sizeof(temp);
    while(--i>=0) {
	if(sscanf(temp[i], "%s.c", s) && (s+".c" == temp[i]) &&
	  (ob = find_object(PRIEST_SP_DIR + s))) {
	    skills_used = ob->query_skills_used();
	    if(ob->query_testing()) continue;
	    skill_p = ob->query_skill_p();

	    if(n = sizeof(skills_used)) {
		coder_skill_str = "(";
		player_skill_str = "";
		for(j=0;j<sizeof(skills_used);j++) {
		    coder_skill_str += sprintf(" %s:%d%%", SK_STRS[skills_used[j]],
		      skill_p[j]);
		    player_skill_str += sprintf(" %s", SK_STRS[skills_used[j]]);
		}
		coder_skill_str += " )";
		dif_str = get_difficulty_level(ob->query_difficulty());
	    }
	    else {
		dif_str = coder_skill_str = player_skill_str = "";
	    }

	    switch(ob->query_cost()) {
	    case -10000..-1:
		cost_str = "varies"; break;
	    case 0:
		cost_str = "-"; break;
	    default:
		cost_str = to_string(ob->query_cost());
	    }
	    damage_rate = to_int(ob->query_damage_rate());

	    spells += ({ ({ capitalize(s), ob->query_level(), cost_str,
		dif_str,
		coder_skill_str, player_skill_str, 
		(damage_rate ? to_string(damage_rate) : "")  }) });
	}
    }
    spells = sort_array(spells, #'sort_by_level);
}

status
spells_cmd(object who, object sym, string arg) {
    string str, sw;
    status is_coder;
    mixed spell, temp;
    int i, n;

    is_coder = who->query_coder_level();
    if(!arg) temp = spells;
    else {
	if(!sscanf(arg, "-%s", sw))
	    return notify_fail("No such option.\n"),0;
	switch(sw) {
	case "name": case "n":
	    temp = sort_array(spells, #'sort_by_name);
	    break;
	}
    }
    str = sprintf("%-16s%5s%8s%12s  %s", "Spell", "Level", "Cost", 
      "Difficulty", "Skills used");
    for(i=0,n=sizeof(temp); i<n; i++) {
	spell = temp[i];
	str += sprintf("\n%-16s%5d%8s%12s  %-25s%s",spell[0],spell[1],
	  spell[2], spell[3], spell[is_coder ? 4 : 5],
	  (is_coder ? spell[6] : ""));
    }
    who->more(explode(str,"\n"));
    return 1;
}

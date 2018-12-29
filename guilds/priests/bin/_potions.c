#include "../priest.h"
#include "../skills.h"

#include "get_difficulty.c"
 
mixed *potions;
// name, level, cost, difficulty, skill str


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
    object ob;
    int i,j,*skills_used,*skill_p;
    string s, skill_str, *t, *temp;

    temp = get_dir(PRIEST_ALC_DIR);
    potions = ({});
    i = sizeof(temp);
    while(--i>=0) {
        if(sscanf(temp[i], "%s.c", s) &&
        (ob = find_object(PRIEST_ALC_DIR + s))) {
            t = explode(s,"_");
            for(j=sizeof(t);--j>=0;)
                t[j] = capitalize(t[j]);
            s = implode(t," ");
         
            skills_used = ob->query_skills_used();
            skill_p = ob->query_skill_p();
            skill_str = "(";
            for(j=0;j<sizeof(skills_used);j++)
                skill_str += sprintf(" %d:%d%%", skills_used[j], skill_p[j]);
            skill_str += " )";
            potions += ({ ({ s, ob->query_level(), ob->query_cost(),
                get_difficulty_level(ob->query_difficulty()),
                skill_str }) });
        }
    }
    potions = sort_array(potions, #'sort_by_level);
}

status
potions_cmd(object who, object sym, string arg) {
    string str, sw;
    status is_coder;
    mixed potion, temp;
    int i, n;
    
    is_coder = who->query_coder_level();
    if(!arg) temp = potions;
    else {
        if(!sscanf(arg, "-%s", sw))
            return notify_fail("No such option.\n"),0;
        switch(sw) {
        case "name": case "n":
            temp = sort_array(potions, #'sort_by_name);
            break;
        }
    }
    str = sprintf("%-24s%5s%6s%12s", "Potion", "Level", "Cost", "Difficulty");
    for(i=0,n=sizeof(temp); i<n; i++) {
        potion = temp[i];
        str += sprintf("\n%-24s%5d%6d%12s",potion[0],potion[1],
            potion[2], potion[3]);
        if(is_coder)
            str += sprintf("    %s", potion[4]);
    }
    who->more(explode(str,"\n"));
    return 1;
}

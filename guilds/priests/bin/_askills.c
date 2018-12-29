#include "show_skills.c"

status
askills_cmd(object who, object sym, string s) {
    int i, *pexes;
    string str;
    string name;
    status is_coder;
    object ob;
    
    is_coder = who->query_coder_level() || who->query_testplayer();
    pexes = get_pexes(who, s);
    if(!pexes) return 0;
    str = "Your potion mixing skills:";
    for(i=7;i<N_SKILLS;i++)
        str += print_1_skill(i, pexes[i], is_coder);
    who->tell_me(str);
    return 1;
}

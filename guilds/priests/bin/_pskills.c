#include "show_skills.c"

string
get_faith_str(int faith) {
    if(faith<-8) return "very weak";
    if(faith<-3) return "weak";
    if(faith<3) return "moderate"; // invite something better
    if(faith<9) return "strong";
    return "very strong";
}

pskills_cmd(object who, object sym, string s) {
    int i, *pexes;
    string str;
    string name;
    status is_coder;
    object ob;
    
    is_coder = who->query_coder_level() || who->query_testplayer();
    pexes = get_pexes(who, s);
    if(!pexes) return 0;
    str = "";
    if(!is_coder || !s) {
        str+=sprintf("Your faith is %s.",get_faith_str(sym->query_faith()));
        if(is_coder) str += sprintf(" (%d)", sym->query_faith());
        str += "\n";
    }
    str += "\nYour priest skills:";
    for(i=0;i<7;i++)
        str += print_1_skill(i, pexes[i], is_coder);
    who->tell_me(str);
    return 1;
}

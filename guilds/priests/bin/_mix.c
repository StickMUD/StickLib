#include "../priest.h"
#include "../skills.h"

string *potions;

void create() {
    int i;
    string *temp, t;

    temp = get_dir(PRIEST_ALC_DIR);
    potions = ({});
    for(i=sizeof(temp); --i>=0;)
	if(sscanf(temp[i], "%s.c", t) == 1)
	    potions += ({ t });
}

status
mix_berry(object who, string berry_str, string potion_str) {
    object berry, potion;

    if(!(berry = present(berry_str, who)))
        return notify_fail("You don't have anything called '"+berry_str+"'!\n"),0;

    if(!(potion = present(potion_str, who)))
        return notify_fail("You don't have anything called '"+potion_str+"'\n"),0;

    if(!potion->is_priest_potion())
        return notify_fail("'"+potion_str+"' is not a priest potion.\n"),0;

    if(!berry->id("alepa berry"))
        return notify_fail("You don't know how to mix that with a potion.\n"),0;

    destruct(berry);
    potion->set_color("dark red");
    who->tell_me("You mix "+berry_str+" with "+potion_str+".");
    return 1;
}

status
mix_cmd(object who, object sym, string pot_name) {
    int i, found;
    string tmp, berry;

    if(!pot_name)
	return notify_fail("Mix what?"),0;

    if(sscanf(pot_name, "%s with %s", berry, tmp)==2) {
        return mix_berry(who, berry, tmp);
    }
    pot_name = lower_case(implode(explode(pot_name, " "), "_"));
    if(member(potions, pot_name) == -1) {
	i = sizeof(potions);
	found = 0;
	while(--i>=0) {
	    if(sscanf(potions[i], pot_name + "%s", tmp)) {
		pot_name = potions[i];
		found = 1; break;
	    }
	}
	if(!found) return notify_fail("No such potion.\n"),0;
    }
    return call_other(PRIEST_ALC_DIR + pot_name, "mix", who, sym);
}

#include "../../priest.h"
#include "../../skills.h"

string *potions;

create () {
    potions = get_dir(PRIEST_ALC_DIR);
}

status
make_potion_cmd(object who, object sym, string str) {
    int i, level;
    string tmp, pot_name;
    object pot;

    if(!who->query_coder_level() && !who->query_testplayer())
	return 0;
    if(!str || sscanf(str, "%s %D", pot_name, level)!=2)
	return notify_fail("Usage: make_potion <name> <level>\n"),0;

    pot_name = implode(explode(pot_name, " "), "_");
    i = sizeof(potions);
    while(--i>=0) {
	if(sscanf(potions[i], pot_name + "%s", tmp))
	{
	    sscanf(potions[i], "%s.c", pot_name);
	    break;
	}
    }
    if(member(potions, pot_name + ".c") == -1)
	return notify_fail("No such potion.\n"),0;

    pot = call_other(PRIEST_ALC_DIR + pot_name, "clone_potion", who, level);
    if(!pot)
	return 0;
    move_object(pot, who);
    who->tell_me("Ok.");
    return 1;
}

int
clean_up(int arg) {
    destruct(this_object());
    return 0;
}

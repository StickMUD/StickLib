#include "../priest.h"
#include "../skills.h"

string *spells;

void create() {
    int i;
    string *temp, t;

    temp = get_dir(PRIEST_SP_DIR);
    spells = ({});
    for(i=sizeof(temp); --i>=0;)
	if(sscanf(temp[i], "%s.c", t) == 1)
	    spells += ({ t });
}

status
invoke_cmd(object who, object sym, string str) {
    string spell, tmp, the_rest;
    status found;
    int i;

    if(!str)
	return notify_fail("Invoke what?\n"),0;

    str = lower_case(str);
    if(sscanf(str,"%s %s",spell,tmp)!=2) {
	spell = str;
	tmp = 0;
    }

    if(member(spells, spell) == -1) {
	found = 0;
	i = sizeof(spells);
	while(--i>=0) {
	    if(sscanf(spells[i], spell + "%s", the_rest)) {
		spell = spells[i];
		found = 1; break;
	    }
	}
	if(!found) return notify_fail("No such spell.\n"),0;
    }
    return call_other(PRIEST_SP_DIR + spell, "invoke", who, sym, tmp);
}

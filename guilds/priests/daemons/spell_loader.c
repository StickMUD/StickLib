#include "../priest.h"
#include "../skills.h"

status loaded;

create() {
    int size;
    if (sizeof(size = get_dir(PEXLOG, 2)) && size[0] > 20000) 
	catch(rename(PEXLOG, PEXLOG + ".old")); 
}


load_objs_in_dir(string dir) {
    int i;
    string *temp;

    temp = get_dir(dir);
    i = sizeof(temp);
    while(--i>=0) {
	if(sscanf(temp[i], "%s.c", temp[i]))
	    catch(call_other(dir + temp[i], "???"));
    }
}

varargs void
load_spells(status force)
{
    if(loaded && !force) return;
    loaded = 1;

    load_objs_in_dir(PRIEST_SP_DIR);
    load_objs_in_dir(PRIEST_ALC_DIR);
}

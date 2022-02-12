
#pragma save_types

#include "/include/config.h"

#define ROOT_UID "root"
#define BACKBONE_UID "backbone"

#include "/secure/simul_efun/amylaar.c"
#include "/secure/simul_efun/do_find_object.c"

int interactivep(object ob) { return interactive(ob); }

/*
 * Function name:   master_object
 * Description:     Returns the current master object
 * Returns:         master object
 */
object
master_object()
{
    return find_object(MASTER_OB);
}

/* Function name:   nature
 * Description:     Returns the current game clock and weather object
 * Returns:         nature object
 */
object
nature()
{
    catch(call_other("bin/daemons/nature_d", "lfun_not_exist"));
    return find_object("bin/daemons/nature_d");
}

/* Another addition.. will be put some day into the gd */

/*
 * Function: compile(file) 
 * Description: compile an object and return string of errors
 *              or 0 for success.
 */

string
compile(string path)
{
    if(find_object(path)) return "Already compiled\n"; /* destr? */
    return catch(call_other(path, "load_me_if_i_wasnt_hi_mom"));
}

#include "/secure/simul_efun/get_object.c"
#include "/secure/simul_efun/tilde_path.c"
#include "/secure/simul_efun/b_string.c"

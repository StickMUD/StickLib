#include <coder_levels.h>
#include <cmd.h>

#include <config.h>

#define PATH(OB) (PATH_USER + (string)(OB)->query_real_name()+"/XyZzY.c")
#define PATH2(OB) ("/open/eval."+(string)(OB)->query_real_name()+".c")

nomask status
lpc_cmd(string str)
{
    if (!CHECK) return 0;

/*
    if (this_player()->query_coder_level() < LVL_COADMIN)
	return 0;
*/

    if (str) {
	string file;
	object ob;
	string err;
	mixed ret;

	str = sprintf("fun(){ %s ;}\n", str);
	rm(file = PATH(this_player()));
	if (!write_file(file, str)) {
	    printf("Can't write file '%s'.\n", file);
	    rm(file = PATH2(this_player()));
	    if (!write_file(file, str)) {
		printf("Can't write file '%s'.\n", file);
		return 1;
	    }
	}

	if (ob = find_object( file )) destruct(ob);
	if (err = catch(ret = (mixed)file->fun())) {
	    printf("Error: %s", err);
	} else {
	    write("Returns: ");
	    printf("%O\n", ret);
	}
	rm(file);
	return 1;
    }
	notify_fail("Usage: "+query_verb()+" <some_lpc_code>\n");
}

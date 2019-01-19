
/**********************************

 NAME:    _fix.c

 PURPOSE: Logs all bugfixes done.

 AUTHOR:  Tron

***********************************/

#include <cmd.h>
#include <coder_levels.h>

#define LOGFILE  "/log/FIX_LOG"

static status fix;
static string prj_file;
static string report;

/*****************************

 Usage: fix [project file]

******************************/

nomask int
fix_cmd(string str) {

    int i;

    if(!CHECK) return 0;

    if(!IS_ADMIN_OB(TP)) return 0; /* (Co)Admin use only */

    if(fix) {
	notify_fail("Command busy by other (Co)Admin. Try later.\n");
	return 0;
    }

    report = "";

    if(str) {

	if(!(prj_file = (string)TP->absolute_path(str, 0))) {
	    notify_fail("Aborted.\n");
	    return 0;
	}

	if((i=file_size(prj_file)) < 0) {
	    if(i == -2) {
		notify_fail("File is a directory.\n");
		return 0;
	    }
	    write("Create project file: "+prj_file+" ? (y/n): ");
	    fix = 1;
	    input_to("y_n");
	    return 1;
	}
    }
    else prj_file = LOGFILE;

    write("File: "+prj_file+": '**' to finish, '--' to abort.\n");
    write("*) ");

    fix = 1;
    input_to("get_report");

    return 1;
}

static string
get_header() {

    return "$FIX: "+capitalize((string)TP->query_real_name())+" "+
    ctime(time())+"\n";
}

nomask void
y_n(string str) {

    if(!IS_ADMIN_OB(TP)) return;

    if(!str) str = "";

    str = lower_case(str);

    if(str != "y") {
	write("Aborted.\n");
	report = 0;
	fix = 0;
    }
    else {
	write(prj_file+": '**' to stop, '--' to abort.\n");
	write("*) ");
	input_to("get_report");
    }

    return;
}

nomask void
get_report(string str) {

    if(!IS_ADMIN_OB(TP)) return;

    if(!str) str = "";

    if(str == "--") {
	write("Aborted.\n");
	report = 0;
	fix = 0;
	return;
    }
    else if(str == "**") {

	report = get_header() + report;
	report += "\n";

	write_file(prj_file, report);

	report = 0;
	fix = 0;
	return;
    }
    else report += (str + "\n");

    write("*) ");
    input_to("get_report");

    return;
}

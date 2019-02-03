/***************************************************************************
 *
 * Guild help command, this handles all the headers for help files, stores
 * info about which commands can be read and so on.
 *
 ***************************************************************************/

// SYSTEM LIBRARIES ********************************************************

#include <daemons.h>

// GUILD DEFINITIONS *******************************************************
#include "../global.h"

#define TOPICS (GUILD_HELP_DIR + "TOPICS")

// GLOBAL VARIABLES ********************************************************

static string *helpFiles = ({});
static string *topics = ({});

/***************************************************************************
 *
 * void create(): Loads what helps we have. NOTE: to make a new help file,
 *                and publish it, you have to compile this.
 *
 ***************************************************************************/

void create()
{
    string tmp = "";
    helpFiles = get_dir(GUILD_HELP_DIR + "_*.help");
    foreach( string file : helpFiles )
    {
        sscanf(file, "_%s.help", tmp);
        tmp = regreplace(tmp, "_", " ", 1);
        topics += ({ tmp });
    }
}

/***************************************************************************
 *
 * status do_cmd(object master, object mark, string args)
 *
 ***************************************************************************/

status do_cmd(object master, object mark, string args)
{
    int length, i;
    string header, line, helpText, what, file, term;
    mapping ansi_codes;

    term = master->query_env("term");
    if( term == "ansi" || term == "vt100" )
    {
        ansi_codes = ANSI_D->query_ansi_codes();
    }
    else
    {
        ansi_codes = ([ "BOLD" : "", "RESET" : "" ]);
    }

    length = master->query_env("columns");
    if ( !length ) length = 80;

    length -= 3;

    line = "";
    for( i = 0; i < length; i++ ) line += "-";

    header = "," + line +".\n";
    what = "Guild help on ";

    if(args) args = lower_case(args);

    if(!args || args == "" || args == "topics")
    {
        args = "topics";
        helpText = read_file(TOPICS);
        helpText += "\n\n" + sprintf("%s", line[0..length-3]);
        helpText += "\n" + sprintf("%|*s", length - 2,
                                   "Available additional topics:");
        helpText += "\n" + sprintf("%s", line[0..length-3]);
        helpText += sprintf("\n%|*#s\n", length - 3, implode(topics, "\n"));
    }
    else
    {
        file = "_" + regreplace(args, " ", "_", 1) + ".help";
        if(member(helpFiles, file) < 0)
        {
            master->tell_me("Failed to find help for \"" + args + "\".\
 Consult \"guild help\" to see available topics.");
            return 1;
        }
        else
        {
            helpText = read_file(GUILD_HELP_DIR + file);
        }
    }

    i = sizeof(args);

    args = terminal_colour("%^BOLD%^" + args + "%^RESET%^",
                            ansi_codes);

    i = sizeof(args) - i;

    header += sprintf("|%|*s|\n", length + i, what + args);
    header += "`" + line +  "'\n";
    helpText = sprintf("\n  %-=*s", length - 2, helpText);
    master->binmsg(header);
    master->more( explode(helpText, "\n") );

    return 1;
}

/***************************************************************************
 *
 * This handles all the headers for help files, stores info about which
 * commands can be read and so on.
 *
 ***************************************************************************/

// SYSTEM LIBRARIES ********************************************************

#include <daemons.h>
#include <mud_name.h>

#define HELP_DIR        "/doc/text/help/"
#define TOPICS          HELP_DIR + "TOPICS"

// GLOBAL VARIABLES ********************************************************

static string *helpFiles = ({});
static string *topics = ({});

/***************************************************************************
 *
 * void create(): Loads what helps we have. NOTE: to make a new help file,
 *                and publish it, you have to compile this.
 *
 ***************************************************************************/

void create() {
    string tmp = "";

    helpFiles = get_dir(HELP_DIR + "*.txt");

    foreach (string file : helpFiles) {
        sscanf (file, "%s.txt", tmp);
        tmp = regreplace(tmp, "_", " ", 1);
        topics += ({ tmp });
    }
}

/***************************************************************************
 *
 * nomask int help2_cmd(string args, object user)
 *
 ***************************************************************************/

nomask int help2_cmd(string args, object user) {
    int length, i;
    string header, line, helpText, what, file, term;
    mapping ansi_codes;

    if(!user->query_coder_level())
       return notify_fail("This command can only be used by coders at the moment.\n"), 0;

    term = user->query_env("term");

    if (term == "ansi" || term == "vt100") {
        ansi_codes = ANSI_D->query_terminal_codes();
    } else {
        ansi_codes = ([ 0: "" ]);
    }

    length = user->query_env("columns");

    if (!length) length = 80;

    length -= 3;

    line = "";

    for (i = 0; i < length; i++) {
        line += "-";
    }

    header = "," + line +".\n";

    what = MUD_NAME " help on ";

    if (args) {
        args = lower_case(args);
    }

    if (!args || args == "" || args == "topics") {
        args = "topics";
        helpText = read_file(TOPICS);
        helpText += "\n" + sprintf("%s", line[0..length-3]);
        helpText += "\n" + sprintf("%|*s", length - 2,
          "Available additional topics:");
        helpText += "\n" + sprintf("%s", line[0..length-3]);
        helpText += sprintf("\n%-*#s\n", length - 3, implode(topics, "\n"));
    } else {
        file = regreplace (args, " ", "_", 1) + ".txt";

        if (member(helpFiles, file) < 0) {
            user->tell_me("Failed to find help for \"" + args + "\". \
Consult \"help\" to see available topics.");
            return 1;
        } else {
            helpText = read_file(HELP_DIR + file);
        }
    }

    i = sizeof(args);

    args = terminal_colour("%^bold%^" + args + "%^reset%^", ansi_codes);

    i = sizeof(args) - i;

    header += sprintf("|%|*s|\n", length + i, what + args);
    header += "`" + line +  "'\n";
    helpText = sprintf("\n  %-=*s", length - 2, helpText);
    user->binmsg(header, 1);
    user->more(explode(helpText, "\n"));

    return 1;
}

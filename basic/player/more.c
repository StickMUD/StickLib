/*
 * more.c - for user object
 */

// Fixed with a cheap kludge(s) to use tell_me in most crucial place.
// 26-may-95 -+ Doomdark +-

#define PAGESIZE (int)this_object()->query_env("rows")
#define SHOWLINE (int)this_object()->query_env("more_lines")

// The driver 'config.h' has READ_FILE_MAX_SIZE which stops reading
// a file after it is so many bytes in size. As of Oct 3, 1998, it
// is set to 75,000 bytes.  This means files with around 3000 lines
// will be too big to more -- if we set the size smaller, the end
// of the file will be truncated, but then at least something
// will appear.
#define MAXLINES 2400

static void even_more(string cmd);
private int load_more_file(int line);
private int more_search(string str);

#ifndef PLAYER_C
varargs void tell_me(string s, status x);
#endif

static private object more_caller;
static private string more_file, *more_text, more_regexp;
static private int    more_line, more_last, more_pos;

public int
more(mixed str)
{
    if (this_player() != this_object()) return 0;

    if (pointerp(str)) {
	more_file = 0;
	more_text = str;
	more_last = sizeof(more_text);
	more_pos = 0;
    } else if (stringp(str)) {
	if (str[0] != '/') str = "/" + str;
	more_file = str;
	more_last = more_pos = 0;
	if (!load_more_file(0)) {
	    printf("Can't read file '%s'\n", more_file);
	    return 0;
	}
    } else {
	notify_fail("Usage: more <file>\n");
	return 0;
    }
    more_caller = previous_object();
    more_line = 0;
    more_regexp = 0;
    even_more("");
    return 1;
}

static void
even_more(string cmd)
{
    string text;
    int show, page;

    page = PAGESIZE;
    if (!page || !intp(page)) page = 24;
    page -= 2;

    switch (cmd[0]) {
    case '1'..'9':
	sscanf(cmd, "%d", show);
	more_line = show - 1;
	write("skipping...\n");
	break;
    case 'b':
	more_line -= 2*page;
	show = 1;
	break;
    case ' ': case 0:
	show = 1;
	break;
    case 'q': case 'Q':
	more_line = more_last;
	break;
    case 'h': case '?':
	write("----\t-------------------------------\n\
<enter>\tnext page\n\
b\tprevious page\n\
=\tshow line number and filename\n\
h,?\tthis help\n\
q,Q\tquit more\n\
!<cmd>\texecute <cmd>\n\
#\tskip to line #\n\
/<r.e>\tsearch for <r.e> regular expression\n\
n\tsearch for next occurrance of last r.e\n\
----\t-------------------------------\n"
	);
	break;
    case '=':
	printf("File: '%s' Line: %d/%d\n",
	  more_file ? more_file : "--internal--",
	  more_line, more_last);
	break;
    case '/': case 'n':
	if (cmd[1])
	    more_regexp = cmd[1..<1];
	show = more_search(more_regexp);
	break;
    default:
	write("Unknown command.\n");
    }
    if (more_line < 0) more_line = 0;
    if (show) {
	int i, sz;

	if (more_file &&
	  (more_line < more_pos || more_line >= more_pos + MAXLINES)) {
	    show = load_more_file( (more_line / MAXLINES) * MAXLINES );
	}
	if (show) {
	    text = "";
	    for (i = page, sz = sizeof(more_text); i--;) {
		if (more_line - more_pos >= sz && !load_more_file(more_line)) {
		    more_line = more_last;
		    break;
		}
		text += more_text[(more_line++) - more_pos] + "\n";
	    }
	    tell_me(text, 1);
	}
    }
    if (more_line >= more_last) {
	more_text = 0;
	if (objectp(more_caller)) more_caller->more_done();
	return;
    }
    if (SHOWLINE) {
	printf("--More--(%d/%d)", more_line, more_last);
    } else {
	printf("--More--(%d%%)", more_line * 100 / more_last);
    }
    input_to("even_more", 0);
}

private int
load_more_file(int line)
{
    string tmp;

    if (!more_file || (more_last > 0 && line >= more_last))
	return 0;

    more_text = 0;
    // Kludge:
    if (!strstr(more_file, "//"))
	more_file = more_file[1..];

    if(!tmp = read_file(more_file, line+1))
    {
	if(file_size(more_file) >= 75000)
	    tell_me("***TRUNCATING FILE OVER 75,000 BYTES***");
	if(!tmp = read_file(more_file, line+1, MAXLINES)  ||  tmp == "")
	    return 0;
    }

    more_text = explode(":\n" + tmp, "\n");
    // Following line changed from -1 to -2  --Colt
    // Neat. And now it skips last line... :-p -+ Doomdark +-
    more_text = more_text[1..<1];
    more_pos = line;
    if (!more_last) {
	more_last = sizeof(more_text);
	if (more_last == MAXLINES) more_last = file_lines(more_file);
    }
    return 1;
}

private int
more_search(string str)
{
    if (str) {
	string *match;

	match = regexp(more_text[(more_line-more_pos)..sizeof(more_text)-1],
	  str);
	if (!match) {
	    write("Illegal pattern\n");
	    return 0;
	}
	if (sizeof(match)) {
	    more_line = member(
	      more_text[(more_line-more_pos)..sizeof(more_text)-1], match[0])
	    + more_line;
	    return 1;
	}
	if (more_file) {
	    int old_pos;

	    old_pos = more_pos;
	    while (!sizeof(match) && load_more_file(more_pos + MAXLINES)) {
		match = regexp(more_text, str);
	    }
	    if (sizeof(match)) {
		more_line = member(more_text, match[0]) + more_pos;
		return 1;
	    }
	    if (old_pos != more_pos) {
		if (!load_more_file(old_pos)) {
		    more_line = more_last = 1;
		}
	    }
	}
	write("Pattern not found\n");
	return 0;
    }
    write("No previous search pattern.\n");
}

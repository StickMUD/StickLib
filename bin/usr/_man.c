#include <daemons.h>

#define	MAN_DIR	"/doc/"
#define	MAN_DIR2	0

static mapping mans, full_mans, main_mans;

static string *not_docs = ({
".ACCESS"
});

nomask int
man_cmd(string str, object me, string mode)
{
mixed fname, s;
	if (!me && !(me = this_player())) return 0;

	if (!str || str == "?") {
		me->tell_me(
"With the man-command you can get man-pages a la Unix. The man-command\
 works much like 'help'-command for players.\n\
We currently have following general man-pages you can first begin reading:");
		me->tell_me(sprintf("'%s'", implode3(m_indices(main_mans),
			"', '", "' and '")));
		return 1;
	}
	switch (str) {
	case "topics":
	case "topic":
	case "list":
		me->tell_me("We have following general man-pages: %s.",
			implode3(m_indices(main_mans), ", ", " and "));
		return 1;
	}

	if (!(fname = full_mans[str])) {
		s = str;
		while (sscanf(s, "%s %s", fname, s) == 2)
			;
		if (!(fname = mans[s])) {
			if (s != str)
				me->tell_me(sprintf("No man-page for topic '%s',\
 nor any man-page regarding '%s' in other places.", str, s));
			else me->tell_me(sprintf("No man-page on topic '%s'.", str));
			return 1;
		}
		if (pointerp(fname)) {
			me->tell_me(sprintf("No man-page specifically on topic '%s';\
 but at least following possibly related man-pages: '%s'.", str,
implode3(fname, "', '", "' and '")));
			return 1;
		}
		fname = full_mans[fname];
	}
	this_player()->more(fname);
	return 1;
}

private static string
load_mans(string dir, string dir2, status main)
{
mixed *x, y;
int i;
string s, f, n;
	x = get_dir(dir, 0x23);
#if 1
	if (main) {
		if ((i = member_array("help", x)) >= 0) {
			x = x[0..i-1] + x[i + 2..<1];
		}
	}
#endif
	for (i = 0; i < sizeof(x); i += 2) {
		if (x[i + 1] < 0)
			load_mans(sprintf("%s%s/", dir, x[i]),
				(dir2 ? sprintf("%s %s", dir2, x[i]) :
				x[i]), 0);
		else {
		// Some files are never man-pages/documents:
			if (member(not_docs, x[i]) >= 0)
				continue;
		// ... especially the stupid backups emacs does:
			if (x[i][<1] == '~')
				continue;
			if (!sscanf(x[i], "%s.man", s)
			&& !sscanf(x[i], "%s.doc", s))
				s = x[i];
			if (!dir2) n = s;
			else n = sprintf("%s %s", dir2, s);
			f = sprintf("%s%s", dir, x[i]);
			full_mans[n] = f;
			if (!(y = mans[s])) {
				mans[s] = n;
			} else if (pointerp(y)) {
				mans[s] = y + ({ n });
			} else {
				mans[s] = ({ y, n });
			}

			if (main)
				main_mans[s] = f;
		}
	}
}

void
create()
{
	mans = ([ ]);
	full_mans = ([ ]);
	main_mans = ([ ]);
	load_mans(MAN_DIR, MAN_DIR2, 1);
}
debug() { return main_mans; }

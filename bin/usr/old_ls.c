#include <cmd.h>

#define LIB_GETOPT "/std/lib/getopt"
#define LIB_RESOLVE_PATH "/std/lib/resolve_path"

inherit LIB_GETOPT;
inherit LIB_RESOLVE_PATH;

/*
 * ls - command
 * Uses Amylaar get_dir => max 333 files
 * Depends on tab size to be 8 characters
 * Made by Val alias Tero.Pelander@utu.fi
 */

// Now it shows the dir we are listing. - Graah

#define DEF_WIDTH 80
#define LS_FLAGS "lsc;"
#define LS_l	 1
#define LS_s	 2
#define LS_c	 4

private void
ls_norm(string base, mixed *files, status s_flag, int tab_count)
{
    int i, j, k, max, maxlen, offset, total;
    string str, tabs;

    maxlen = 5;
    max = sizeof(files);
    do {
	str = files[i];
	switch (j = files[i + 1]) {
	  case -2:
	    total += j = 1;
	    str += "/";
	  case -1:
	    break;
	  default:
	    total += j = ((1023 + j) >> 10);
	    if (extract(str, -2, -1) == ".c"
		&& find_object(base + extract(str, 0, -3)))
	    {
		str += "*";
	    }
	}
	files[i] = s_flag ? sprintf("%4d %s", j, str) : str;
	if ((j = strlen(str)) > maxlen) maxlen = j;
    } while ((i += 3) < max);

    printf("%s : Total: %d\n", base, total);

    if ((maxlen = (maxlen + (s_flag ? (5+8) : 8) >> 3)) > tab_count)
	maxlen = tab_count;
    offset = (max / 3) / (tab_count / maxlen)
	+ ((max / 3) % (tab_count / maxlen) > 0);
    offset *= 3;
    tabs = sprintf("%'\t'*s", maxlen--, "");
    i = 0;
    do {
	k = strlen(str = files[j = i]);
	while ((j += offset) < max) {
	    str += tabs[(k>>3)..maxlen] + files[j];
	    k = strlen(files[j]);
	}
	write(str + "\n");
    } while ((i += 3) < offset);
}

private void
ls_long(string base, mixed *files) {
    int i, max, total, tmp, year1, year2;
    string name, date, size;

    date = ctime(time());
    year1 = date[22];
    year2 = date[23];

    max = sizeof(files);
    do {
	name = files[i++];
	switch (tmp = files[i++]) {
	  case -2:
	    size = "(dir)"; total++; break;
	  case -1:
	    size = "-N/A-"; break;
	  default:
	    size = "" + tmp;
	    total += (1023 + tmp) >> 10;
	}
	date = ctime(files[i++]);
	printf("%6s %s %s %s\n", size,
	    (extract(name, -2, -1) == ".c" && find_object(base + extract(name, 0, -3)))
		? "*" : " ",
	    (date[23] == year2 && date[22] == year1)
		? date[4..15] : (date[4..10] + date[19..23]),
	    name);
    } while (i < max);
    printf("Total: %d\n", total);
}

nomask int
ls_cmd(string path) {
    mapping opt;
    mixed  *list;
    int i;

    /* Resolve command line */
    opt = getopt(path, LS_FLAGS);
    if (opt[-2]) {
	write(opt[-2]);
	return 1;
    }
    path = resolve_path(opt[-1], this_player());
    if (path[0] != '/') {
	printf("%s is illegal path\n", opt[-1]);
	return 1;
    }

    /* Is argument directory? */
    if (extract(path, -1, -1) != "/" && file_size(path + "/") == -2)
	path += "/";

    if (!(list = get_dir(path, 7))) {
	printf("%s unreadable\n", path);
	return 1;
    }

    if (!sizeof(list)) {
	printf( (extract(path, -1, -1) == "/") ? "%s is empty\n" : "%s not found\n",
	    path);
	return 1;
    }

    /* Cut the base of path */
    i = strlen(path) - 1;
    while (path[i] != '/') i--;
    path = path[0..i];

    if (opt[0] & LS_l) {
	ls_long(path, list);
    } else {
	int tab_count;

	if (tab_count = opt[LS_c]) {
	    tab_count /= 8;
	    if (!tab_count) tab_count = 1;
	} else {
	    tab_count = DEF_WIDTH / 8;
	}
	ls_norm(path, list, opt[0] & LS_s, tab_count);
    }
    return 1;
}

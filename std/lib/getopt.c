/*
 * Created by Val (tero.pelander@utu.fi)
 * May not be used in muds other then Stickmud without permission from creator.
 */
/*
 * Function:	getopt
 * Description:	Getopt makes the option flags detection easier.
 * Arguments:	arg: (string | 0) the command line arguments.
 *		optlist: the option list.
 *			(string) one letter flags
 *			(string *) array of long switches
 *			succeeding ':' means need of string argument
 *			succeeding ';' means need of integer argument
 * Returns:	Mapping containing error messages(-2), resolved flags(0..) and
 *		the left over of arguments(-1). The mapping may contain any
 *		number of internal info that has negative indices (currently
 *		-3 and -4). If there is an error message the rest of the
 *		values may be what ever.
 */
/*
 * Example 1:
 * getopt("-lac 132 file", "alc;")
 *	=> ([ -1:"file", 0:(1 | 2 | 4), 4:132 ])
 * getopt("-acl 132 file", "alc;")
 *	=> ([ -2:"Flag -c must be succeeded by value.\n",
 *		** The rest is undefined ** ])
 *
 * Example 2:
 * getopt("-name 'big orc' -lev 20 -m", ({ "name:","level;","race:","move" }))
 * 	=> ([ -1:"", 0:"orc", 1:20, 3:-1 ])
 */

/* Prototypes */
private int getopt_short(string opt, string optlist, mapping ret);
private int getopt_long(string opt, string *optlist, mapping ret);

mapping getopt(string arg, mixed optlist) {
    mapping ret;

    /* Check for legal values */
    if (!arg) 
	return ([ -1:"" ]);
    if (!stringp(optlist) && !pointerp(optlist))
	return 0;

    /* Start flag separation from arguments */
    ret = ([ -1:arg ]);
    while (arg[0] == '-') {
	string opt;
	int    i;

	/* Separate flag(s) from the rest of arguments */
	if (!sscanf(arg, "-%s %s", opt, arg)) {
	    opt = arg[1..<1];
	    arg = "";
	}

	/* If "-" or "--" end flag interpretation */
	if (opt == "" || opt == "-") break;

	/* Interpret flag in subsystem and test if they need arguments */
	switch (pointerp(optlist) ?
	    getopt_long(opt, optlist, ret) :
	    getopt_short(opt, optlist, ret) )
	{
	  case -1: // Error in flag
	    break;

	  case ':': // Next argument must be string
	    if (arg == "") {
		ret[-2] = sprintf("Flag -%s needs a string argument.\n",
			ret[-4] );
		break;
	    }
	    if (sscanf(arg, "'%s'%s", opt, arg) ||
		sscanf(arg, "\"%s\"%s", opt, arg))
	    {
		if (arg[0] != 0 && arg[0] != ' ') {
		    ret[-2] = "Illegal ' or \" expression.\n";
		    break;
		}
		arg = arg[1..<1];
	    } else {
		if (!sscanf(arg, "%s %s", opt, arg)) {
		    opt = arg;
		    arg = "";
		}
	    }
	    ret[ret[-3]] = opt;
	    break;

	  case ';': // Next argument must be integer
	    if (!sscanf(arg, "%d%s", i, opt) ||
		(opt[0] != 0 && opt[0] != ' '))
	    {
		ret[-2] = sprintf("Flag -%s needs a number argument.\n",
			ret[-4] );
		break;
	    }
	    arg = opt[1..<1];
	    ret[ret[-3]] = i;
	}

	/* Save new value for non processed arguments */
	ret[-1] = arg;

	/* If error messages stop loop */
	if (ret[-2]) break;
    }

    return ret;
}

private int getopt_short(string opt, string optlist, mapping ret) {
    int i, len, foundopt;

    len = sizeof(opt);
    do {
	int tmp;

#if 0
	foundopt = member_array(opt[i], optlist);
#else
	tmp = opt[i];
	foundopt = sizeof(optlist);
	while (foundopt--)
	    if (tmp == optlist[foundopt]) break;
#endif
	if (foundopt == -1) {
	    ret[-2] = sprintf("Unknow flag -%s\n", opt[i..i]);
	    return -1;
	}

	/* Add flag to retun values */
	ret[0] |= 1 << foundopt;

	/* Check if there should be arguments behind flag */
	tmp = optlist[foundopt + 1];
	if (tmp == ':' || tmp == ';') {
	    if (i + 1 != len) {
		ret[-2] = sprintf("Flag -%s must preceed an argument.\n",
			optlist[foundopt..foundopt]);
		return -1;
	    }
	    ret[-3] = 1 << foundopt;
	    ret[-4] = optlist[foundopt..foundopt];
	    return tmp;
	}
    } while (++i < len);
}

private int getopt_long(string opt, string *optlist, mapping ret) {
    int i, foundopt;

    /* Search for flag */
    opt += "%s";
    foundopt = -1;
    i = sizeof(optlist);
    while (i--) {
	string tmp;

	if (sscanf(optlist[i], opt, tmp)) {
	    if (tmp == "") {
		foundopt = i;
		break;
	    }
	    if (foundopt != -1) {
		ret[-2] = sprintf("Unambiquous flag -%s.\n", opt[0..<3]);
		return -1;
	    }
	    foundopt = i;
	}
    }
    if (foundopt == -1) {
	ret[-2] = sprintf("Unknown flag -%s.\n", opt[0..<3]);
	return -1;
    }

    /* Check if there should be arguments behind flag */
    i = optlist[foundopt][sizeof(optlist[foundopt]) - 1];
    if (i == ':' || i == ';') {
	ret[-4] = optlist[foundopt][0..<2];
	ret[-3] = foundopt;
	return i;
    }
    ret[foundopt] = -1;
}

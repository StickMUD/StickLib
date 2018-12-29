/*
 * resolve_path
 * Make path name absolute.
 * 'player' is optional.
 */
#pragma strict_types

#define PATHERROR	"NONE"
#define XUSERPATH(name)	({ "players", name })
// #define XDOMAINPATH(name) ({ "d", name })
#define REAL_NAME(pl)	((string)pl->query_real_name())
#if 0
#define GET_PATH(pl)	((string)pl->query_path())
#else
#define	GET_PATH(pl) ((string)pl->query_env("cwd"))
#endif

nomask public varargs string resolve_path(string path, object player) {
    string *xpath, base;
    int    i, max;

    switch (path[0]) {
    /* Resolve tilde paths */
      case '~':
	if (sscanf(path, "~%s/%s", base, path) != 2) {
	    base = path[1..<1];
	    path = "";
	}
	switch (base[0]) {
	  case 0:
	    if (!player || !(base = REAL_NAME(player)))
		return PATHERROR;
	    /* fall through */
	  case 'a'..'z':
	    xpath = XUSERPATH(base);
	    break;
#ifdef XDOMAINPATH
	  case 'A'..'Z':
	    xpath = XDOMAINPATH(base);
	    break;
#endif
	  default:
	    return PATHERROR;
	}
	break;

    /* Check for '/' starting paths */
      case '/':
	xpath = ({ });
	break;

    /* Resolve "here" & "env" to player location */
      case 'e':
      case 'h':
	if (player && (path == "here" || path == "env")) {
	    if (environment(player)) {
		path = object_name(environment(player));
		sscanf(path, "%s#", path);
		return "/" + path + ".c";
	    }
	    return PATHERROR;
	}
	/* fall through */

    /* Use current working directory */
      default:
	if (player) base = GET_PATH(player);
	xpath = base ? explode(base, "/") : ({ });
    }


    /* Create the full path */
    i = sizeof(xpath);
    max = sizeof(xpath += explode(path + "/", "/")) - 1;
    while (i <= max) {
	switch (xpath[i]) {
	  case "":
	    if (i == max) {
		i++;
		break;
	    }
	  case ".":
	    xpath = xpath[0..i-1] + xpath[i+1..max];
	    max--;
	    break;
	  case "..":
	    xpath = xpath[0..i-2] + xpath[i+1..max];
	    if (i) {
		i--; max -= 2;
	    } else {
		max--;
	    }
	    break;
	  default:
	    i++;
	}
    }
    return "/" + implode(xpath, "/");
}

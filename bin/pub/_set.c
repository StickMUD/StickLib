#include <cmd.h>
#include <daemons.h>
#include <tell_me.h>

#define	TERM_LIST ({"vt100", "ansi", "dumb"})
#define SOUND_LIST  ({ "off", "msp" })

#define MIN_COL     20
#define MIN_ROW     10

static mapping messages = ([
  "say":                TELL_TYPE_SAY,
  "whisper":            TELL_TYPE_WHISPER,
  "tell":		TELL_TYPE_TELL,
  "shout":		TELL_TYPE_SHOUT,
  "party":		TELL_TYPE_PARTY,
  "hitby":		TELL_TYPE_HIT_BY,
  "missedby":           TELL_TYPE_MISSED_BY,
  "hitting":            TELL_TYPE_HITTING,
  "missing":            TELL_TYPE_MISSING,
  "death":		TELL_TYPE_DEATH,
  "roombrief":          TELL_TYPE_ROOM_BRIEF,
  //"roomdesc":         TELL_TYPE_ROOM_DESC,
  "smells":             TELL_TYPE_ROOM_SMELLS,
  "sounds":             TELL_TYPE_ROOM_SOUNDS,
  "roomitem":           TELL_TYPE_ROOM_ITEM,
  "inventory":          TELL_TYPE_INVENTORY,
  "invitem":            TELL_TYPE_INV_ITEM,
  "broadcast":          TELL_TYPE_BROADCAST,
  "info":               TELL_TYPE_INFO,
  "mort":               TELL_TYPE_MORT,
  "newbie":             TELL_TYPE_NEWBIE,
  "ooc":                TELL_TYPE_OOC,
  "lord":               TELL_TYPE_LORD,
  "guild":              TELL_TYPE_GUILD,
  "house":              TELL_TYPE_HOUSE,
  "faction":            TELL_TYPE_FACTION,
  "order":              TELL_TYPE_ORDER,
  "wiz":                TELL_TYPE_WIZ,
  "arch":               TELL_TYPE_ARCH,
  "admin":              TELL_TYPE_ADMIN,
]);

static mapping effects = ([
  "normal"      : "0",
  "reset"       : "0",
  "bold"        : "1",
  "dim"         : "2",
  "italic"      : "3",
  "underline"   : "4",
  "flash"       : "5",
  "reverse"     : "7",
  "black"       : "30",
  "red"         : "31",
  "green"       : "32",
  "yellow"      : "33",
  "blue"        : "34",
  "magenta"     : "35",
  "cyan"        : "36",
  "white"       : "37",
  "bgblack"     : "40",
  "bgred"       : "41",
  "bggreen"     : "42",
  "bgyellow"    : "43",
  "bgblue"      : "44",
  "bgmagenta"   : "45",
  "bgcyan"      : "46",
  "bgwhite"     : "47",
]);

void show_settings(object me);

nomask int
set_cmd(string str, object me)
{
    string s, t, tmp, *x;
    int v, i, j, msg;

    if (!me && !(me= this_player())) return 0;
    if (me != this_interactive()) return 0;

    if (str == "help" || str == "?") {
	notify_fail(
	  "For help about command \"set\" please use commands \"help set\"\n");
	return 0;
    }

    if (!str) {
	show_settings(me);
	return 1;
    }

    if (sscanf(str, "%s %s", s, t) < 2)
	return notify_fail("Try 'set <option> <value(s)>'\n"), 0;

    switch (s) {
    case "timezone":
	if (t=="0"||t=="default") {
	    me->set_env("timezone", 0);
	    me->tell_me("Ok.");
	    break;
	}
	if(!TIMEZONE_D->is_valid_timezone(t))
	    return notify_fail("Invalid timezone (check 'help uptime' for zones)\n"),0;
	me->set_env("timezone", upper_case(t));
	me->tell_me("Ok.");
	break;
    case "sound":
	if (member(SOUND_LIST, t) < 0) {
	    me->tell_me("Unknown sound option; try one of following types: "
	      + implode(SOUND_LIST, ", ", " or ")+".\n");
	    return 1;
	}
	else {
	    me->set_env("sound", t);
	    me->tell_me("Ok.");
	}

	break;
    case "term":
	if (member(TERM_LIST, t) < 0) {
	    me->tell_me(
	      "Unknown term type; try one of following types: "
	      +implode(TERM_LIST, ", ", " or ")+".\n");
	    return 1;
	} else {
	    me->set_env("term", t);
	    me->tell_me("Ok.");
	}
	break;
    case "columns":
	if(t == "default") {
	    me->set_env("columns", 0);
	    me->tell_me("Ok.");
	    break;
	}

	v = to_int(t);
	if (v < MIN_COL)
	    return me->tell_me("Minimum column length is: "+MIN_COL+"\n"), 1;

	me->set_env("columns", v);
	me->tell_me("Ok.");
	break;
    case "rows":
	me->set_env("rows", 0);  // obsolete
	if(t == "default") {
	    me->set_env("more_rows", 0);
	    me->tell_me("Ok.");
	    break;
	}

	v = to_int(t);
	if (v < MIN_ROW)
	    return me->tell_me("Minimum row length is: "+MIN_ROW+"\n"), 1;

	me->set_env("more_rows", v);
	me->tell_me("Ok.");
	break;
    case "message":
	if (sscanf(t, "%s to %s", s, t) < 2) {
	    me->tell_me(
	      "You have to specify both which message to set and which effects you want\
 to specify for the message (like \"set message lord to bold red\" for\
 example). For list of applicable effects, try \"help set\".");
	    return 1;
	}
	if (!(msg = messages[s])) {
	    me->tell_me(sprintf(
		"Unknown message type '%s'; available message types are: '%s'.",
		s, implode(sort_array(m_indices(messages), #'>), "', '", "' and '")));
	    return 1;
	}
	x = explode(t, " ");
	while (i < sizeof(x)) {
	    if (!sizeof(x[i])) {
		x[i] = 0;
		continue;	// Skip extra spaces...
	    }
	    if (!(t = effects[x[i]])) {
		me->tell_me(sprintf(
		    "Unknown text effect '%s'; skipping that effect.", x[i]));
		x[i] = 0;
	    } else {
		x[i] = t;
		j++;
	    }
	    i++;
	}
	if (!j) {	// Not a single "ok" effect:
	    me->tell_me(sprintf(
		"No single applicable effect; won't change the current text effect setting\
 for message type '%s'.", s));
	    return 1;
	}
	tmp = sprintf("%c[%sm", 27, efun::implode(x, ";"));
	me->set_text_effect(msg, tmp);
	me->tell_me("Ok.");
	break;
    default:
	notify_fail(sprintf(
	    "Unknown option ('%s') for set; try 'term' or 'message'\n(or 'help term'\
 to get help about\navailable options and their arguments).\n", s));
	return 0;
	break;
    }
    return 1;
}

void show_settings(object me)
{
    mapping envdata;
    string setenv;
    mixed value, tmp, tmp2;

    me->tell_me("Settings:");

    if(me->query_coder_level())
    {
	envdata = deep_copy(me->query_env()); // Coders may have additional data to see
	if (member(envdata, "columns"))   envdata["columns"]   = to_string(envdata["columns"]);
	if (member(envdata, "rows"))      envdata["rows"]      = to_string(envdata["rows"]);
	if (member(envdata, "more_rows")) envdata["more_rows"] = to_string(envdata["more_rows"]);
    }
    else
    {
	// We only care about these things for players
	envdata = ([]);
	envdata["columns"]   = to_string(me->query_env("columns"));
	envdata["rows"]      = to_string(me->query_env("rows"));
	envdata["more_rows"] = to_string(me->query_env("more_rows"));
	envdata["sound"]     = me->query_env("sound");
	envdata["term"]      = me->query_env("term");
    }

    if(!member(envdata, "sound") || !envdata["sound"] || envdata["sound"]=="")
	envdata["sound"] = "off";

    if(!member(envdata, "term") || !envdata["term"] || envdata["term"]=="")
	envdata["term"] = "dumb";

    if(!member(envdata, "columns") || !envdata["columns"] || envdata["columns"]=="")
	envdata["columns"] = "default";

    if(!member(envdata, "rows") || !envdata["rows"] || envdata["rows"]=="")
	envdata["rows"] = "default";

    if(!member(envdata, "more_rows") || !envdata["more_rows"] || envdata["more_rows"]=="")
	envdata["more_rows"] = "default";

    if(!member(envdata, "timezone") || !envdata["timezone"] || envdata["timezone"]=="")
	envdata["timezone"] = "default (AST)";

    foreach(setenv : sort_array(m_indices(envdata), #'>)) //'
    {
	if(setenv == "sound" || setenv == "term" || setenv == "rows" || setenv == "columns" || setenv == "timezone")
	    me->tell_me(sprintf("   %-10s: %s", capitalize(setenv), envdata[setenv]));
	else
	    me->tell_me(sprintf("   %-10s: %O", capitalize(setenv), envdata[setenv]));
    }

   me->tell_me("\nMessage colors:");

   foreach(setenv : sort_array(m_indices(messages), #'>)) //'
   {
      value = ANSI_D->unformat_ansi(me->query_text_effect(messages[setenv]));
      me->tell_me(sprintf("   %-10s: %s", capitalize(setenv), value), messages[setenv]);
   }
}

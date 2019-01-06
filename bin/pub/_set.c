#include <cmd.h>
#include <tell_me.h>

#define	TERM_LIST	({\
		"vt100", "ansi", "dumb"\
		})

mapping messages;
mapping effects;

void
create()
{
    messages = ([
      "tell"	: TELL_TYPE_TELL,
      "lord"	: TELL_TYPE_LORD_CHANNEL,
      "mud"	: TELL_TYPE_MUD_CHANNEL,
      "guild"	: TELL_TYPE_GUILD_CHANNEL,
      "hitby"	: TELL_TYPE_HIT_BY,
      "missedby": TELL_TYPE_MISSED_BY,
      "hitting" : TELL_TYPE_HITTING,
      "missing" :TELL_TYPE_MISSING,
      "party"	: TELL_TYPE_PARTY,
      "info"	: TELL_TYPE_INFO,
      "coder"	: TELL_TYPE_CODER_CHANNEL,
      "flord"	: TELL_TYPE_FLORD_CHANNEL,
      "liege"	: TELL_TYPE_LIEGE_CHANNEL,
      "lich"	: TELL_TYPE_LIEGE_CHANNEL,
      "shout"	: TELL_TYPE_SHOUT,
    ]);
    effects = ([
      "normal"		: "0",
      "bold"		: "1",
      "dim"		: "2",
      "italic"		: "3",
      "underlined"	: "4",
      "underline"	: "4",
      "reversed"	: "7",
      "colour0"		: "30",
      "colour1"		: "31",
      "colour2"		: "32",
      "colour3"		: "33",
      "colour4"		: "34",
      "colour5"		: "35",
      "colour6"		: "36",
      "colour7"		: "37",
      /* Let's allow using real colour names too... */
      "black"		: "30",
      "red"		: "31",
      "green"		: "32",
      "yellow"		: "33",
      "blue"		: "34",
      "purple"		: "35",
      "cyan"		: "36",
      "white"		: "37",
    ]);
}

nomask int
set_cmd(string str, object me)
{
    string s, t, tmp, *x;
    int i, j, msg;

    if (!me && !(me= this_player())) return 0;
    if (me != this_interactive()) return 0;

    if (str == "help" || str == "?") {
	notify_fail(
	  "For help about command \"set\" please use commands \"help set\"\n");
	return 0;
    }

    if (!str || sscanf(str, "%s %s", s, t) < 2)
	return notify_fail("Try 'set <option> <value(s)>'\n"), 0;

    switch (s) {
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
		s, implode(m_indices(messages), "', '", "' and '")));
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

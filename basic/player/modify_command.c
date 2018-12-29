/****************************************************************
*								*
* Module:							*
*	/basic/player/modify_command.c				*
* Description:							*
*	Modifies players' commands according to possible	*
*	command subsitutions and aliases. Based on original	*
*	modify_command.c written by Tron; this version was	*
*	originally create for MurderMud, later on ported 'back'	*
*	to StickMUD.						*
* Version:							*
*	1.0a 14-Jan-98						*
* Last change:							*
*	Doomdark 14-Jan-98					*
*								*
* NEW:								*
*								*
* 14-Jan-98: Now uses 'substitute_string()'-efun to do the	*
*	alias-substitutions. Should be pretty efficient. But	*
*	 let's hope it's reliable and won't crash us...		*
*								*
* 15-Jan-98, Doomdark: A fex fixes.				*
****************************************************************/

#ifndef PLAYER_C

#include <tell_me.h>

// Prototypes... Won't be needed when included.

varargs void tell_me(string s, mixed x, int f, object a, object b, object c);

#endif

varargs nomask string modify_command(string s, int recurse);

#define AUTH if (this_interactive() && this_interactive()!=this_object()) return
#define AUTH2 (this_interactive() && this_interactive() != this_object())

#define DELIM           "^#-+-#^"
#define HISTORY_SIZE	15
#define HISTORY_STR_LEN	5
#define MIN_ALIAS_LENGTH 2

#define COMMAND_SIZE    75
#define ALIAS_SIZE      32

#define MAX_RECURSION   8
#define MAX_LINES       10
#define MAX_MUL         10

// Notice that returning 1 from modify_command means we won't try to
// execute any command!
#define FAIL		1
#define	NOP		"nop"

#ifndef TRUE
#define	TRUE	1
#define	FALSE	0
#endif

// Stick-specific:
private static string   *queue;

private mapping commands = ([ ]);
private mapping aliases = ([ ]);
private string  delim = ";";

private static string	command_repeater;
private static string   *history = ({ });
private static string   lastHistory;
private static status   isBeat;
private static string *nocommand = ({
	"nop", "quit", "passwd", "password", "command", "alias", "cq"
});
private static string *dirs = ({
"n","e","s","w","u","d","ne","nw","se","sw","north","east","south","west",
"northeast","northwest","southwest","southeast","up","down","swim","climb"
});
private static string *_int_aliases, *_int_substs;
private static int _int_alias_min, _int_alias_max;

/****************************************************************
*								*
*			Query-function(s)			*
*								*
****************************************************************/

mixed
query_command_data(string s, string x)
{
	switch (s) {
	case "commands":
	case "command":
		if (x)
			return commands[x];
		return efun::copy(commands);
	case "aliases":
	case "alias":
		if (x)
			return aliases[x];
		return efun::copy(aliases);
	case "history":
		return history + ({ });;
	case "delimeter":
		return delim;
	}
}

varargs status
set_command_data(string which, mixed arg, mixed arg2)
{
int i;
mixed tmp, tmp2, cmd_tmp, tmpstr;

  if (!previous_object() || !(tmp = object_name(previous_object())))
	return FALSE;

/* Only commands from /bin/pub & /bin/usr can modify these... */

  // Do we get the leading slash or not? Let's play safe:
  if (strstr(tmp, "/bin/pub/") && strstr(tmp, "/bin/usr/") &&
  strstr(tmp, "bin/pub/") && strstr(tmp, "bin/usr/"))
	return FALSE;

/* And for added security, let's make sure it's us changing it.
 * (course, this is nothing like 100% security still but...)
 */
  if (this_interactive() != this_object())
	return FALSE;

  switch (which) {
	case "delimeter":
	case "delim":

	  if (!arg || !sizeof(arg))  {
		tell_me(sprintf("Your current command delimeter is '%s'.",
			delim));
		break;
	  }

		if (arg == DELIM) {
			tell_me(sprintf("Reserved delimiter: %s.\n\
Delimiter was not changed.", arg));
			return FALSE;
		}

		if (sizeof(commands)) {

		  tmp = m_indices(commands);
		  for (i = 0; i < sizeof(tmp); i++) {
			tmpstr = commands[tmp[i]];
			if (strstr(tmpstr, delim) >= 0) {
			  tmp2 = explode(tmpstr, delim);
			  commands[tmp[i]] = implode(tmp2, arg);
			}
		  } 
		}

	  delim = arg;
	  tell_me("Delimiter changed to: "+arg);
	  break;

	case "repeater":

	  command_repeater = arg;
	  tell_me(sprintf("Repeating with: '%s'. Stop with '**' command!!!",
		arg));
	  break;

	case "command":

	  if (!arg) {
		commands = ([]);
		tell_me("All commands removed!");
		return TRUE;
	  }
	  if (!arg2) {
		if (tmp = commands[arg]) {
		  m_delete(commands, arg);
		  tell_me(sprintf("Command (%s = %s) removed.", arg, tmp));
		} else {
		  tell_me(sprintf("No such command (%s) defined.",
			arg));
		}
	  } else {
      
		if (tmp = commands[arg]) {
			commands[arg] = arg2;
			tell_me(sprintf(
"Command changed from: %s = %s\n\
                  to: %s = %s", arg, tmp, arg, arg2));
		} else if (sizeof(commands) == COMMAND_SIZE) {
			tell_me(sprintf("Your command buffer is full\
 (%d commands)", COMMAND_SIZE));
			return FALSE;
		} else if(member(nocommand, arg) >= 0) {
			tell_me(sprintf("Command '%s' is a reserved word!",
				arg));
			return FALSE;
		} else {
			commands[arg] = arg2;
			tell_me(sprintf("Command added: %s = %s", arg, arg2));
		}
	  }
	  break;

	case "alias":

		if (!arg) {
			aliases = ([]);
			tell_me("All aliases removed!");
			break;
		}

		if (!arg2) {
			if (tmp = aliases[arg]) {
				m_delete(aliases, arg);
				tell_me(sprintf("Alias removed: %s = %s",
					arg, tmp));
			} else {
				tell_me("No such alias ('%s') defined.", arg);
				return FALSE;
			}
		} else {
			if (tmp = aliases[arg]) {
			  aliases[arg] = arg2;
			  tell_me(sprintf(
"Alias changed from: %s = %s\n\
                to: %s = %s", arg, tmp, arg, arg2));
			} else if (sizeof(aliases) == COMMAND_SIZE) {
				tell_me(sprintf("Your alias buffer is full\
 (%d aliases).", COMMAND_SIZE));
				return FALSE;
			} else {
			  if (sizeof(arg) < MIN_ALIAS_LENGTH) {
				tell_me(sprintf("Minimum length for alias:\
 %d.", MIN_ALIAS_LENGTH));
				return FALSE;
			  }
			  aliases[arg] = arg2;
			  tell_me(sprintf("Alias added: %s = %s", arg, arg2));
			}
		}
/* We'll do it easy way; ie. always invalidate the internal alias
 * arrays in case something is changed. :-/
 */
		_int_aliases = 0;

		break;

	default:
		return FALSE;
  }
  return TRUE;
}

#if 1
/*************************************************
 DESCRIPTION: Substitute alias strings to line.

 ARGS:        line      -Possible alias words.
	      recurse   -Recursion count.

 RETURN       Modiefied string

 NOTE: Sub-optimal method. Could be enhanced by
	recoding it in C... But let's see if there's
	need.
 *************************************************/

varargs static string
modify_alias(string line, int recurse)
{
int i, alias_found;
string str;
string *words;

	if (recurse > MAX_RECURSION) {
		tell_me("Quicktyper: Alias too recursed.");
		return line;
	}

	words = explode(line, " ");
	i = sizeof(words);
   
	while (i--) {
	 if (str = aliases[words[i]]) {
	   words[i] = modify_alias(str, recurse + 1);
	   alias_found++;
	 }
	}
	if (!alias_found) return line;

	if (sizeof(words) > 1)
		return implode(words, " ");
	return words[0];
}
#endif

/*************************************************
 DESCRIPTION: Substitute $* commands to the line.

 ARGS:        head    -Head of line.
	      tail    -Tail of line.
	      recurse -Recursion count.

 RETURN       Modified string
 *************************************************/

nomask varargs static string
modify_args(string head, string tail)
{
string left, right, *words;
int num, count, *tmp;
int i, j;

	while ((i = strstr(head, "$", j)) >= 0) {

		left = head[0..i - 1];
		right = head[i+1..];

		if (!sscanf(right, "%d%s", num, right)) {
// Let's just skip this dollar... might lead to problems with '$$'?
			j = i + 1;
			continue;
		}

		if (!count++) {
			words = (tail ? explode(tail, " ") : ({}));
			tmp = ({ });
		}
		if ((num > 0) && (num <= sizeof(words))) {
			num--;
			head = sprintf("%s%s%s", left, words[num], right);
			j = sizeof(left) + sizeof(words[num]);
			tmp += ({num});
		} else {
			if (left[<1] == ' ')
				head = sprintf("%s%s", left[0..<2], right);
			else
				head = sprintf("%s%s", left, right);
		}
	}

	if ((i = strstr(head, "$*")) >= 0) {

	  if (!count) {
		words = (tail ? explode(tail, " ") : ({}));
	  }
	  if (sizeof(tmp)) {
	    j = 0;
	    while (j < sizeof(tmp))
		words[tmp[j++]] = 0;
	  }

	  return sprintf("%s%s%s", head[0..i-1], implode(words, " "),
		head[i+2..]);
	}

	if (!count)
		if (!tail)
			return head;
		else
			return sprintf("%s %s", head, tail);
	return head;
}

/*************************************************
 DESCRIPTION: Modify the whole command line.

 ARGS:        s        -Command line.
	      recurse  -Recursion count.
	      no_queue -No queue adding.

 RETURN       Modified string
 *************************************************/

nomask varargs static mixed
modify(string s, int recurse, status multi)
{
string head, tail, tmp;
string  *list;
int     i, j, mul, siz;

	if(recurse > MAX_RECURSION) {
		tell_me("Quicktyper: Command too recursed.");
		return FAIL;
	}  

	if (multi) {

	  list = explode(s, delim);
	  i = sizeof(list);

	  if (i > MAX_LINES) {
		tell_me(sprintf("Quicktyper: Max command lines: %d;\
 truncating commands.\n", MAX_LINES), 0, TELL_NO_WRAP, 0, 0, 0);
		siz = MAX_LINES;
	  }

	  recurse++;
	  i = 0;
	  j = sizeof(list) - 1;
	  while (i < j) {
		s = list[i];
		sscanf(s, "%t%s", s); // Let's remove possible white space(s)
		if ((s = modify(s, recurse, 0)) != FAIL)
			command(s);
		i++;
	  }
	if (j >= 0) {
	  if (s = list[j])
	  	sscanf(s, "%t%s", s);
	  return modify(s, recurse, 0);
	 }
	}

	if(!s) return FAIL;

	if (command_repeater) {
		if (s == "**") {
			command_repeater = 0;
			tell_me("Repeating stopped.");
			return FAIL;
		}
		s = sprintf("%s %s", command_repeater, s);
	}
	sscanf(s, "%s%t%s", s, tail);
	while ((i = member(nocommand, s)) < 0 && (head = commands[s])) {
		s = modify_args(head, tail, recurse + 1);
	  // Urgh. And we may now have new delimeters:
		if (strstr(s, delim) >= 0) {
		  return modify(s, recurse+1, 1);
		}
		tail = 0;
		sscanf(s, "%s %s", s, tail);
	}
	// Even though we don't want alias substitutions, we do need
	// to do argument modifications:
	if (i >= 0) {
		if (tail)
			s = modify_args(s, tail, recurse + 1);
	  // Urgh. And we may now have new delimeters:
		if (strstr(s, delim) >= 0) {
		  return modify(s, recurse+1, 1);
		}
		tail = 0;
		sscanf(s, "%s %s", s, tail);
	} else {
	  // Multipliers have to be taken care of now:
	  if (i = sscanf(s, "%d%.0t%s", mul, s)) {
	    if (i == 1 || !sizeof(s)) {
		if (!(s = tail)) {
			tell_me(sprintf("Repeat what %d times?", mul));
			return FAIL;
		}
		tail = 0;
		sscanf(s, "%s%t%s", s, tail);
	    } else {
	    }
	    if (mul > MAX_MUL) {
		tell_me("Quicktyper: Command multiplier max value: "+MAX_MUL);
		mul = MAX_MUL;
	    }
	    if (mul < 2) {
		tell_me("Quicktyper: Command multiplier min value: 2");
		mul = 1;	// Better make sure it's at least 1...
	    } 
	    if (sscanf(s, "%d", j)) {
		tell_me("Quicktyper: Illegal to have multipliers inside\
 multipliers!");
		return FAIL;
	    }
	  }

	// Also, need to take "n", "e" and all such commands: */
	  if ((i = sizeof(s)) == 1) {
	    switch (s[0]) {
		case 'n': s = "north"; break;
		case 's': s = "south"; break;
		case 'e': s = "east"; break;
		case 'w': s = "west"; break;
		case 'd': s = "down"; break;
		case 'u': s = "up"; break;
		case 'l': s = "look"; break;
		case 'i': s = "inventory"; break;
	    }
	  } else if (i == 2) {
	    switch (s) {
		case "ne": s = "northeast"; break;
		case "nw": s = "northwest"; break;
		case "se": s = "southeast"; break;
		case "sw": s = "southwest"; break;
	    }
	  }

/* Now; the alias substitution... */
// (and thanks to us being in else-branch, we won't have probs with
// 'alias' or 'command'-commands)

	  if (tail && sizeof(aliases)) {
#if 1
		tail = modify_alias(tail, recurse + 1);
#else
		if (!_int_aliases) {
			_int_aliases = m_indices(aliases);
			_int_substs = allocate(sizeof(_int_aliases));
			_int_alias_min = _int_alias_max = sizeof(
			  _int_aliases[0]);
			_int_aliases = sort_array(_int_aliases, #'>);
			for (i = 0; i < sizeof(_int_aliases); i++) {
				if ((j = sizeof(_int_aliases[i])) <
				  _int_alias_min)
					_int_alias_min = j;
				else if (j > _int_alias_max)
					_int_alias_max = j;
				_int_substs[i] = aliases[_int_aliases[i]];
			}
		}
		tail = substitute_string(tail, _int_aliases, _int_substs,
			_int_alias_min, _int_alias_max);
#endif
	  }
	  if (mul > 0) {
		if (tail) s = sprintf("%s %s", s, tail);
		while (--mul)
			command(s);
		// Can it contain something weird?
		return modify(s, recurse+1, 1);
		//return s;
	  }
	}
	if (tail)
		return sprintf("%s %s", s, tail);
	return s;
}

/*************************************************
 DESCRIPTION: Handle history, command substitution,
	      multiple commands.
	      (party follow soon)

 ARGS:        s -Command line.

 RETURN       Modiefied string
 *************************************************/

nomask mixed
modify_command(string s)
{
string tmp, stmp;
int i, siz;
string *list;

	if (!s || s == "") return s;
	if (previous_object() && previous_object() != this_object())
		return 0;
   
// Let's check moving commands; not automatically converted by
// gamedriver any more! -+ Doomdark +-
// (as well as 'l'-> 'look' and 'i'->'inventory')
	if ((i = sizeof(s)) == 1) {
	switch (s[0]) {
	case 'n': return "north";
	case 's': return "south";
	case 'e': return "east";
	case 'w': return "west";
	case 'd': return "down";
	case 'u': return "up";
	case 'l': return "look";
	case 'i': return "inventory";
	}
	} else if (i == 2) {
	  switch (s) {
		case "ne": return "northeast";
		case "nw": return "northwest";
		case "se": return "southeast";
		case "sw": return "southwest";
	  }
	}

	if (s[0] == '\\')
		return s[1..];

#if 0
// This is a kludge. Ed and input_to should not
// even come to modify_command, or at least it
// should be notified in function argument.

	if (sscanf(s, "!%s", tmp)) {
		s = modify(tmp, 0);
		command(s ? s : FAIL);
		return "";
	}
#endif

	if (sscanf(s, "%s %s", tmp, stmp) < 2)
		tmp = s;
	if (member(nocommand, tmp) != -1)
		return s;

	if (s[0] == '%') {
	  sscanf(s, "%%%s", s);
	  siz = sizeof(history);
	  if (s == "" || s == "%") {
		if (siz == 0) {
			tell_me("No history.");
			return FAIL;
		}
		s = history[0];
	  } else {
	    if(sscanf(s, "%d", i)) {
		if (i < siz && i > -1) s = history[i];
		else {
			tell_me("No such history reference.");
			return FAIL;
		}
	    } else {
		stmp = s +"%s";
		i = 0;
		while(i < siz) {
		  if (sscanf(history[i], stmp, tmp)) {
			s = history[i]; break;
		  }
		  i++;
		}
		if(i == siz) {
			tell_me("History reference not found.");
			return FAIL;
		}
	    }
	  }
	}

	if (sizeof(s) > HISTORY_STR_LEN) {
	  if (siz = sizeof(history)) {
		if (s != history[0]) {
			history = ({s}) + history;
		}
		if (siz >= HISTORY_SIZE)
			history = history[0..HISTORY_SIZE-1];
	  } else history = ({s});
	}

	s = modify(s, 0, strstr(s, delim) >= 0);
	return s ? s : FAIL;
}

/* This shouldn't be needed but... */
nomask static void
command_queue() {
int siz, i;
string cmd;
string *tmp_queue;

   if(!sizeof(queue) || !queue) return;

   if(!interactive(this_object())) {
      queue = ({});
      return;
   }

   tmp_queue = queue;
   queue = ({});

   siz = sizeof(tmp_queue);

   while(i < siz) {
      cmd = modify(tmp_queue[i], 0);
      if(!cmd) cmd = NOP;

      if(sizeof(queue)) {
	 if(i+1 < siz) queue += tmp_queue[i+1..siz-1];
	 break;
      }

      if(i && member(dirs, cmd) != -1) {
	 queue += ({cmd});
      }
      else command(cmd, this_object());

      i++;
   }

}

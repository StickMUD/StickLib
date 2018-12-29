// Module that contains functions to handle strings used as arguments
// that are formatted in SSF; Stickmud String Format.
//
// Version 2.0 31-sep-96 by Doomdark

// 9-Feb-98 Chopin: Took part of code from /basic/player/tell_me and put it
//   to interpret_format_string function so that rooms and monsters can use
//   it when needed.

// Flags to be used on 2nd array of an SSF format descriptor:

#ifndef _DAEMONS_H
#include <daemons.h>
#endif


// First, objects used:
#define     SSF_ME                  0x1
#define     SSF_HIM                 0x2
#define     SSF_IT                  0x3

#define     SSF_OBJECTS             0xFF

// Then, data to be obtained:
#define		SSF_NAME                0x100
#define		SSF_CAP_NAME		0x200
#define		SSF_REAL_NAME		0x300
#define		SSF_SHORT_DESC          0x400
#define		SSF_LONG_DESC           0x500
#define		SSF_PRONOUN             0x600
#define		SSF_OBJECTIVE           0x700
#define		SSF_POSSESSIVE          0x800
#define		SSF_RACE                0x900
#define		SSF_GUILD		0xA00
#define		SSF_GEN_POSSESSIVE	0xB00
#define		SSF_ENDING_S		0xC00
#define		SSF_ENDING_ES		0xD00
#define		SSF_ENDING_IES		0xE00
#define		SSF_ANSI_DATA		0xF00

#define     SSF_DATA                0xFF00

// Then, functions to be used to modify strings so far got:
#define     SSF_CAPITALIZE          0x10000
#define     SSF_LOWER_CASE          0x20000
#define     SSF_UPPER_CASE          0x30000

#define     SSF_FUNCTIONS           0xFF0000

// And finally, miscellaneous data we may have...

#define		SSF_MISC_MASK		0x0F000000
#define		SSF_MISC_SHIFT		24

// And finally, internal flag(s) we have to use when using
// 'shared' strings, for example:
#define     SSF_SHARED_COPY         (1 << 28)
#define     SSF_SHARED_ORIGINAL     (1 << 29)
#define     SSF_SHARED_MASK         0xFFFF

// This function is used to 'parse' format string; returns an array
// with 2 elements; 1st is array containing substrings in format string;
// constant strings and 'empty slots' for string generated when
// forming the 'real' string to be used.
// 2nd is an array containing integers that are used as bitfields
// to decide what to do to strings in 1st array; and how to generate
// strings for 'empty slots' in it.

// For example "<%me:name%> hits <%him:name%> with the <%it:name%>."
// is a valid SSF to be parsed. Returned array would contain 2
// arrays, each with size of 5.

static mapping _ansi_codes;

mixed
make_format_string(string x)
{
string *tmp, s, func;
int i, j, bit_flags, func_flags;
mixed misc, *tmp2;

	if (!stringp(x) || !sizeof(x))
		return x;
	if (x[0] == ':') x = x[1..];
	tmp = explode(x, "%");

// If this isn't a SSF, let's return the original string!
	if ((i = sizeof(tmp)) == 1 && tmp[0][0] != '<')
		return x;

// Now let's remove empty strings, if there are some.
	while (i--)
        if (!sizeof(tmp[i])) {
            tmp[i] = 0;
            bit_flags++;
        }
	if (bit_flags)
		tmp -= ({ 0 });

	tmp2 = allocate(sizeof(tmp));

// Now, the main parse loop!
    i = -1;
    while (++i < sizeof(tmp)) {
        if (tmp[i][0] != '<' || !sscanf(tmp[i], "<%s>", s))
		continue;
	tmp[i] = 0;
// Because of the syntax, we have to parse the functions first:
        bit_flags = 0;
        while (sscanf(s, "%s(%s)", func, s) == 2) {
            switch (func) {
                case "cap":
                    bit_flags |= SSF_CAPITALIZE; break;
                case "lower":
                    bit_flags |= SSF_LOWER_CASE; break;
                case "upper":
                    bit_flags |= SSF_UPPER_CASE; break;
            }
        }

	if (sscanf(s, "%d", misc)) {
		bit_flags = misc << (SSF_MISC_SHIFT);
	} else {
// *sigh* This can still be a 'non-SSF'...
	        if (sscanf(s, "%s.%s", func, s) < 2) continue;

// Now, let's check which object we're referencing:
	  switch (func) {
            case "me":
                bit_flags |= SSF_ME; break;
            case "him":
                bit_flags |= SSF_HIM; break;
            case "it":
                bit_flags |= SSF_IT; break;
	    case "ansi":
/* Hmmh. With ansi, we need to do some other nasty stuff as well... */
// Let's kludge in the codes...
		if (!_ansi_codes) {
			_ansi_codes = (mapping) ANSI_D->query_ansi_codes();
		}
// Are there more than 1 ANSI-code to be used?
		if (member(s, '+') >= 0) {
			misc = explode(s, "+");
			for (j = 0; j < sizeof(misc); j++)
			  misc[j] = _ansi_codes[misc[j]];
#if 0
			tmp2[i] = sprintf("\e[%sm", implode(misc, ";"));
#else
			tmp2[i] = sprintf("%c[%sm", 0x1B, implode(misc, ";"));
#endif
		} else {
			if (misc = _ansi_codes[s])
#if 0
				tmp2[i] = sprintf("\e[%sm", misc);
#else
				tmp2[i] = sprintf("%c[%sm", 0x1B, misc);
#endif
			else
				tmp2[i] = 0;	// -> erroneous code or something...
		}
// ... and there it was, ANSI-handling.
		bit_flags = 0;
	  }

/* This line is only used for ANSIs for now; later on it may be used for other
 * things as well...
 */
	  if (!bit_flags) continue;

// And, finally, which data item we want to obtain:
          switch (s) {
		case "name":
			bit_flags |= SSF_NAME; break;
		case "capname":
			bit_flags |= SSF_CAP_NAME; break;
		case "real_name":
			bit_flags |= SSF_REAL_NAME; break;
		case "short_desc":
			bit_flags |= SSF_SHORT_DESC; break;
		case "long_desc":
			bit_flags |= SSF_LONG_DESC; break;
		case "pronoun":
			bit_flags |= SSF_PRONOUN; break;
		case "objective":
			bit_flags |= SSF_OBJECTIVE; break;
		case "possessive":
			bit_flags |= SSF_POSSESSIVE; break;
		case "race":
			bit_flags |= SSF_RACE; break;
		case "guild":
			bit_flags |= SSF_GUILD; break;
		case "gen_possessive":
			bit_flags |= SSF_GEN_POSSESSIVE; break;
		case "ending_s":
			bit_flags |= SSF_ENDING_S; break;
		case "ending_es":
			bit_flags |= SSF_ENDING_ES; break;
		case "ending_ies":
			bit_flags |= SSF_ENDING_IES; break;
		default:
			raise_error("invalid MSS-string");
	  }
	}

// Now, let's set this descriptor (though we may have to change this later!).
        tmp2[i] = bit_flags;

// And, to be more efficient when 'executing' the SSF; let's check if we could
// use a 'shared' string!!! (reduces amount of call_others considerably; only
// 1 call_other / query_name / object etc.)

        func_flags = bit_flags & SSF_FUNCTIONS;
        bit_flags &= (SSF_OBJECTS | SSF_DATA);
        if (!bit_flags)
		continue;       // _Shouldn't_ happen... but just in case.

	j = -1;
	while (++j < i) {
            if ((tmp2[j] & (SSF_OBJECTS | SSF_DATA | SSF_MISC_MASK)) == bit_flags) {
// Gotcha! Now let's mark this one uses "shared string"...
// Also, we need to mark the original string; when processing SSF, we have to
// take it into account (because of upper/lower_case-funcs and such)
                tmp2[i] = (j & SSF_SHARED_MASK) | SSF_SHARED_COPY | func_flags;
                tmp2[j] |= SSF_SHARED_ORIGINAL;
                break;
            }
        }
    }

    return ({ tmp, tmp2 });
}


// Function that combines data made with make_format_string() to an
// understandable string.

// parameters:
//	format_string	Formatted SSF "string"
//	effects_on 		Do we want to use ansi?
//	ansi_count		How many ansi codes was found (reference!)
//	ext_data		Same as tell_me()'s tell_flags parameter
//	me, him, it		See tell_me()

string
interpret_format_string(mixed format_string, int effects_on, int ansi_count,
string* ext_data, object me, object him, object it)
{
	int i, j, bit_flags;
	mixed strs, flags;
	object ob;

	strs = format_string[0];
	flags = format_string[1];
	
	i = -1;
	while (++i < sizeof(strs)) {
		if (!(bit_flags = flags[i])) continue;
		if (!intp(bit_flags)) {
			// Ah-ha. So we have an ANSI-code string here...
			if (effects_on) {
				strs[i] = bit_flags;
					ansi_count++;
			}
			flags[i] = 0;
			continue;
		}

// First let's check if this is a "shared string" copy; if so, we can do this
// in fast way:

		if (bit_flags & SSF_SHARED_COPY) {
			bit_flags &= (~SSF_SHARED_COPY);
			strs[i] = strs[bit_flags & SSF_SHARED_MASK];
			continue;
		}

// First we have to decide which object's data is needed:
		switch (bit_flags & SSF_OBJECTS) {
			case SSF_ME:	ob = me; break;
			case SSF_HIM:	ob = him; break;
			case SSF_IT:	ob = it; break;
			default:		ob = 0;
		}
		
// If we do use "external" data we'll do:
		if (!ob) {
			j = (bit_flags & SSF_MISC_MASK) >> SSF_MISC_SHIFT;
			if(pointerp(ext_data) && sizeof(ext_data) > j)
				strs[i] = ext_data[j];
			else
				strs[i] = "ERROR";
			continue;
		}

#if defined(PLAYER_C) || defined(NPC_C)
		if (ob == this_object())
		switch (bit_flags & SSF_DATA) {
			case SSF_NAME:			strs[i] = "you"; break;
			case SSF_CAP_NAME:		strs[i] = "You"; break;
			case SSF_SHORT_DESC:	strs[i] = query_short(0, this_object()); break;
			case SSF_LONG_DESC:		strs[i] = query_long(0, this_object()); break;
			case SSF_REAL_NAME:		strs[i] = query_real_name(); break;
			case SSF_PRONOUN:		strs[i] = "you"; break;
			case SSF_OBJECTIVE:		strs[i] = "you"; break;
			case SSF_POSSESSIVE:	strs[i] = "your"; break;
			case SSF_GEN_POSSESSIVE: strs[i] = "your"; break;
			case SSF_RACE:			strs[i] = query_race(); break;
			case SSF_GUILD:			strs[i] = query_guild(); break;
			case SSF_ENDING_S:
			case SSF_ENDING_ES:		strs[i] = ""; break;
			case SSF_ENDING_IES:	strs[i] = "y"; break;
		}
		else
#endif
		switch (bit_flags & SSF_DATA) {
			case SSF_NAME:
                                strs[i] = (string) ob->query_name(0, this_object(), 1); break;
    // New: let's try to use non-capitalized version of the name!
    // Chopin 10-May-98
			case SSF_CAP_NAME:
				strs[i] = capitalize((string) ob->query_name(0, this_object())); break;
			case SSF_SHORT_DESC:
				strs[i] = (string) ob->query_short(0, this_object()); break;
			case SSF_LONG_DESC:
				strs[i] = (string) ob->query_long(0, this_object()); break;
			case SSF_REAL_NAME:		strs[i] = (string) ob->query_real_name(); break;
			case SSF_PRONOUN:		strs[i] = (string) ob->Pronoun(); break;
			case SSF_OBJECTIVE:		strs[i] = (string) ob->Objective(); break;
			case SSF_POSSESSIVE:	strs[i] = (string) ob->Possessive(); break;
			case SSF_GEN_POSSESSIVE:
				strs[i] = (string) ob->query_name(0, this_object());
				if (strs[i]) strs[i] += "'s";
				break;
			case SSF_RACE:			strs[i] = (string) ob->query_race(); break;
			case SSF_GUILD: 		strs[i] = (string) ob->query_guild(); break;
			case SSF_ENDING_S:		strs[i] = "s"; break;
			case SSF_ENDING_ES: 	strs[i] = "es"; break;
			case SSF_ENDING_IES:	strs[i] = "ies"; break;
		}
	}

// Since shared originals can't have functions applied to them earlier than 
// here, it's not a great spead-up to perform functions in the previous loop
// for other strings either. So let's just do another loop here where
// all functions are performed.

	i = sizeof(strs);
	while (--i>=0) {
		if(bit_flags = (flags[i] & SSF_FUNCTIONS))
			switch (bit_flags) {
			case SSF_CAPITALIZE:
				strs[i] = strs[i] ? capitalize(strs[i]) : "ERROR"; break;
			case SSF_LOWER_CASE:
				strs[i] = strs[i] ? lower_case(strs[i]) : "ERROR"; break;
			case SSF_UPPER_CASE:
				strs[i] = strs[i] ? upper_case(strs[i]) : "ERROR"; break;
			}
	}
	
	return implode(strs, "");
}



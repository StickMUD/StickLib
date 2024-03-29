/*
 * This function is used to tell every living thing in the room a
 * message. Since it uses tell_me in living.c, one doesn't have to worry
 * about line-wrapping. For this reason, this should be used instead of
 * tell_room and say where possible. Note however, that calling this is
 * quite costy if there are many livings in the room, so use just 1
 * tell_here / action, if you have multiple messages, combine them into
 * 1 string and call then tell_here.
 *
 * The second optional argument defines what livings shouldn't be told
 * the message. It may be either an object-pointer, or an array of
 * object-pointers. If you have only 1 such living, use rather object-
 * pointer than array.
 * 
 * If, for some reason, you don't want lines to be wrapped, use third
 * optional argument so that linewrapping is not used.
 *
 * -Doomdark, 14-apr-94.
 */
/* Although the code may look rather clumsy, it should be fast.
 * Effectiveness over elegancy...
 * -Doomdark.
 */

#include "/basic/misc/fstring.c"

#ifndef ROOM_C
#include "/basic/room/room_private.h"

static int Flags;
#endif

#define	TELL_X(x)	{ x->tell_me(&s, sense, flags, me, him, it); \
		i++; }\
		if (x) x = next_inventory(x);

#define	CALL_CATCH_TELL (Flags & F_ROOM_CATCH_TELL_USED)

// Prototype:
void catch_tell(string s);


varargs int
tell_here(mixed s, mixed sense, mixed flags, mixed dont_tell_these,
  object me, object him, object it)
{
    object x;
    object a, b;
    int i;
    mixed tmp;

    /* First a compatibility section: */
    if (objectp(sense) || (pointerp(sense) && objectp(sense[0]))) {
	dont_tell_these = sense;
	sense = 0;
    }
    x = first_inventory(this_object());
    if (!dont_tell_these) {
	while(x) {
	    if (living(x)) TELL_X(x);
	}
    } else if(!pointerp(dont_tell_these)) {
	while(x) {
	    if (living(x) && x != (object) dont_tell_these) TELL_X(x);
	}
    } else switch(sizeof(dont_tell_these)) {
    case 0: /* Shouldn't actually happen... */
	while (x) {
	    if(living(x)) TELL_X(x);
	}
	break;
    case 1:
	a = (object) dont_tell_these[0];
	while(x) {
	    if (living(x) && x != a) TELL_X(x);
	}
	break;
    case 2:
	a = (object) dont_tell_these[0];
	b = (object) dont_tell_these[1];
	while(x) {
	    if (living(x) && x != a && x != b) TELL_X(x);
	}
	break;
    default:
	while(x) {
	    if (living(x) && member(dont_tell_these, x) < 0) TELL_X(x);
	}
    }
    if (CALL_CATCH_TELL) {
	if(pointerp(sense)) {
	    // Let's just choose the first message
	    tmp = s[0];
	} else tmp = s;

	if(stringp(tmp) && tmp[0] == ':')
	    tmp = make_format_string(tmp[1..]);

	if(pointerp(tmp) && sizeof(tmp) > 1)
	    tmp = interpret_format_string(tmp, 0, 0, flags, me, him, it);

	if(stringp(tmp))
	    catch_tell(tmp);
    }
    return i;
}

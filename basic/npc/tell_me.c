/****************************************************************
*								*
* Module: tell_me.c / NPC-version.				*
* Version: 1.00							*
* Last modification: 24-feb-96, Doomdark			*
* Description:							*
*	Light-weight version of tell_me.c for NPCs. Reason for	*
*	this is that while we've been adding features to	*
*	tell_me.c for players to use, little is needed for NPCs.*
*	Thus, we can optimize efficiency by using stripped-down	*
*	version for NPCs...					*
* Changes:							*
* 9-Feb-98, Chopin: SSF strings are now handled correctly. Added
*   F_NPC_CATCH_TELL_USED flag too.				*
*								*
****************************************************************/

#include "/basic/misc/fstring.c"

#ifndef NPC_C

#include "/basic/npc/npc_private.h"
#include <tell_me.h>

void catch_tell(string s);

int npc_Flags;

#endif


public varargs int
tell_me(mixed str, mixed sense_list, mixed tell_flags,
  mixed me, mixed him, mixed it)
{
    // NPCs generally have no use for line-wrapping; if they're contolled by
    // coders/players, the controller already uses line-wrap, so there's
    // little to be gained... Thus, this is _very_ simple. Also, we can
    // further by-pass tell_object and call catch_tell directly!

    mixed s;

    if(!(npc_Flags & F_NPC_CATCH_TELL_USED))
	return TELL_RETURN_NONE_USED;

    if (!str || !this_object()) return 0;

    /* Compatibility section: */
    if (sense_list && intp(sense_list)) {
	tell_flags = sense_list;
	sense_list = 0;
    }

    if(pointerp(sense_list)) {
	// Let's just choose the first message
	s = str[0];
    } else s = str;

    if(stringp(s) && s[0] == ':')
	s = make_format_string(s[1..]);

    if(pointerp(s) && sizeof(s) > 1) {
	s = interpret_format_string(s,0,0,tell_flags,me,him,it);
    }

    if (!stringp(s)) return TELL_RETURN_NONE_USED;

    catch_tell(s);
    return 0;
}


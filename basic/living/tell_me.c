#ifndef	LIVING_C

#include "/basic/living/living_private.h"

static int liv_Flags;

#endif

static int s_columns, s_rows;	/* Screen dimensions. */
static string s_wrap;		/* String sprintf uses in tell_me. */

// Tell us something. Should replace the whole tell_object() actually...
// Uses line-wrap now. -Doomdark, 14-apr-94. 

// Modified by Tron 15-apr-94. 
// The columns is saved as integer from now on --Val 25-jul-94

public varargs void
tell_me(string str, status no_wrap, status update_wrap) {

    if ((liv_Flags & F_LIV_IS_NPC) && s_columns < 2) {

	// Let's check s_columns so monsters can use line-wrap, but usually won't.
	// If wrapping is needed (coder controlling monster etc.), set the value
	// of s_wrap to desired value and update the s_wrap string, or call this
	// with udpate_wrap on.

	if (!str || !this_object()) return;
	if (no_wrap) tell_object(this_object(), str);
	else tell_object(this_object(), str + "\n");
	return;
    }
    if (s_columns < 2 || !s_wrap || update_wrap) {
	mixed data;

	if (data = (mixed)this_object()->query_env("columns")) {
	    if (stringp(data)) {
		s_columns = 0;
		sscanf(data, "%d", s_columns);
		this_object()->set_env("columns", s_columns);
		/* Hack the "rows" into integer too. */
		if (stringp(data = (mixed)this_object()->query_env("rows"))) {
		    int i;
		    sscanf(data, "%d", i);
		    this_object()->set_env("rows", i);
		}
	    } else {
		s_columns = (intp(data) && data) ? data : 80;
	    }
	}
	if(s_columns < 2) s_columns = 80;
	s_wrap = "%-=" + (s_columns - 1) + "s\n";
    }

    if (!str) return;
    if (no_wrap) {
	tell_object(this_object(), str);
	return;
    }

    /* Kludge, but has to do 'til tell_me's won't use linefeeds anymore. */
    if (sizeof(str) > 1 && str[<1] == '\n')
	str = str[0..<2];

    if(s_wrap) tell_object(this_object(), sprintf(s_wrap, str));
    else tell_object(this_object(), str+"\n"); 
}

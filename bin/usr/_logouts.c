//
// /bin/usr/_logouts.c
// Last modified: 17-feb-98 by Doomdark
//
// This command displays a summary list of coders/admins
// and when they last logged out.
//
#include <daemons.h>
#include <tell_me.h>
#include <mud_name.h>
#include "/secure/coders.h"

#define LOGGED_IN	-1
#define LINKDEAD	-2

static closure compare_cl;

nomask string show_info(string who, int ptime);

nomask status
logouts_cmd(string str, object me)
{
int i,j, x, tt;
string nn, tmp;
mixed names;
object ob;

    if (!me && !me=this_player()) return 0;

    if ( str ) tt=str[0];
    if ( tt=='-' && sizeof(str)>1) tt=str[1];

    if ( tt )
    switch ( tt )
     {
	case 'c': names = CODER_LIST;
		  str = "Coder";
		  break;
	case 'a': names = ADMIN_LIST + COADMIN_LIST;
		  str = "Admin";
		  break;
	default:
		notify_fail("Usage: logouts [a|c|p]\n");
		return 0;
     }

	if ( !names) {
	   names = ADMIN_LIST + COADMIN_LIST + CODER_LIST;
	   str = "Immortal";
	}

	j = sizeof(names);

	for ( i=0; i<j; i++ ) {
	  if (ob = find_player(names[i])) {
		if (ob == me) {
			names[i] = 0;
			continue;
		}
		if (interactive(ob))
			names[i] = ({ LOGGED_IN, names[i] });
		else {
			names[i] = ({ LINKDEAD, names[i] });
		}
	  } else names[i] = ({ (int) FINGER_D->query_last_save(names[i])
		, names[i] });
	}

	names -= ({ 0 });
	j = sizeof(names);

	me->tell_me(sprintf("%s %ss:\n", MUD_NAME, str), 0, TELL_NO_WRAP, 0, 0, 0);
	names = sort_array(names, compare_cl);
	for ( i=0; i < j; i++ ) {
		if ((tt = names[i][0]) == LOGGED_IN) {
			me->tell_me(sprintf("%-16sLogged in.", names[i][1]));
		} else if ((tt = names[i][0]) == LINKDEAD) {
			me->tell_me(sprintf("%-16sLinkdead.", names[i][1]));
		} else {
			x = time() - tt;
			if (x >= 86400) {
				str = sprintf("%dD,", x / 86400);
				x %= 86400;
			} else str = "";
			if (x >= 3600) {
				str = sprintf("%s%dH,", str, x / 3600);
				x %= 3600;
			}
			x /= 60;
			me->tell_me(sprintf("%-16sLeft %s (%s%dM\
 ago).", names[i][1], ctime(tt), str, x));
		}
	}
	return 1;
}

void
create()
{
	compare_cl = lambda(({ 'x, 'y }), ({ #'?,
                ({ #'<, ({ #'[, 'x, 0 }), ({ #'[, 'y, 0 }) }),
                        0, 1 })
        );
}

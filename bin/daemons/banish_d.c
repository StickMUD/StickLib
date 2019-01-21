/* This is the new banishement daemon, which seems not to be used
 * (yet?). Perhaps it should replace banish_d.c... :-)
 */

// secure/banish.c
//
// Banishments and freezings
//
// level
//
//  -3  Permanent banishment (due crimes or such)
//  -2  Name is reserved for a former hero who is permanently dead
//  -1  Name is otherwise reserved
//
//  >0  Value of time() when a frozen player can play again
//
// New banish system:
//
// Can now ban whole clusters, for examples (just examples :)
//	*.se
//	*.helsinki.fi
//	*.freenet.hut.fi
// Or whole ips:
//	123.*
//	321.456.*
//	432.456.789.*

#include "/sys/interactive_info.h"

#include <coder_levels.h>

#pragma strict_types

#define SAVE_FILE "/data/d/banish_d/new_banish_save"

/* Prototypes */
nomask string is_banished(string name);

private string *banished;
private int *level;
private string *ip_banished;
private string *ip_stopped; // IPs that can't even try logging in.
private string *allowed; // Allow certain people through banned ip's too.
private string *guest_ip_banished;

nomask void save_me() { save_object(SAVE_FILE); }

nomask void
create()
{
    banished = ({}); level = ({});
    ip_banished = ({}); allowed = ({});
    ip_stopped = ({});
    guest_ip_banished = ({});
    restore_object(SAVE_FILE);
}

nomask void purge() {
    string *tmp1;
    int *tmp2,i;

    tmp1 = ({}); tmp2 = ({});
    for (i = sizeof(banished) - 1; i >= 0; i--) {
	if (level[i] && level[i] > time() || (level[i] < 0 && level[i] > -4)) {
	    tmp1 += ({banished[i]}); tmp2 += ({level[i]});
	}
    }
    banished = tmp1; level = tmp2;
}

/* Return a string of form "99 D 99 h 99 m 99 s " from no of seconds */
static string time_desc(int s) {
    string tmp;
    int d, y;

    tmp = "";
    if(s/86400) {
	d = (s/86400);
	y = (d / 365);
	if (y > 0) tmp += y + " a ";
	tmp += (d - (y * 365)) + " D ";
	s -= d*86400;
    }
    if(s/3600)  { tmp +=  (s/3600) + " h "; s -=  (s/3600)*3600; }
    if(s/60)    { tmp +=    (s/60) + " m "; s -=    (s/60)*60; }
    if(s)         tmp +=         s + " s ";

    return tmp;
}

nomask
string list_banished(string arg)
{
    int i,t;
    string tmp;

    if(!arg) return "Check /log/BANISH for complete list.\n";
    tmp = is_banished(lower_case(arg));
    if(!tmp) return "Name " + arg + " is not banished.\n";
    return tmp;

    /* This was a bit too much with long list...
       purge();

       tmp = "";
       for(i = sizeof(banished); i >= 0; i--) {
	  tmp += (banished[i] + "                   ")[0..19] + "(";
	  if(level[i] == -1) tmp += "reserved name";
	  else if(level[i] == -2) tmp += "former hero";
	  else if(level[i] == -3) tmp += "permanent banishment";
	  else if(level[i] < -3) tmp += "very strange banishment";
	  else tmp += "is still frozen for " + time_desc(level[i] - time())
	    + " (Until " + ctime(level[i]) + ")";
	  tmp += ")\n";
       }
       save_me();
       return tmp;
    */
}

/* Return 0 if not, otherwise tell us how */
nomask string
is_banished(string name)
{
    string tmp;
    int x;

    if(!name) return 0;
    name = lower_case(name);
    purge();
    x = member(banished, name);
    if(x == -1) return 0;

    tmp = capitalize(name) + " is ";
    x = level[x];
    if(x == -1) tmp += "a reserved name.\n";
    else if(x == -2) tmp += "a former hero and permanently dead.\n";
    else if(x == -3) tmp += "permanently banished from this mud.\n";
    else if(x < 1) tmp += "banished in a very strange way.\n";
    else tmp += "still frozen for " + time_desc(x - time()) +
	"(Until " + ctime(x) + ")\n";

    return tmp;
}

string *query_ip_banished() { return ip_banished; }
string *query_ip_stopped() { return ip_stopped; }
string *query_allowed() { return allowed; }
string *query_guest_ip_banished() { return guest_ip_banished; }

string list_allowed() { return implode(allowed, ", ", " and "); }

nomask status
query_ip_banned(string ip)
{
    return (member(ip_banished, ip) != -1);
}

// Check if "who"'s ip number is banished. Might allow certain people.
nomask status
is_ip_banished(object who, string n)
{
    string ipno, ipname, *x, tmp;
    int b, s;

    if (!who || !n) return 0;
    if (member(allowed, n) != -1) return 0;
    ipname = interactive_info(who, II_IP_NAME);
    ipno = interactive_info(who, II_IP_NUMBER);

    if (stringp(ipname))
    {
	x = explode(lower_case(ipname), ".");
	s = sizeof(x);

	if (s > 3)
	{
	    tmp = "*." + x[<1];
	    if (query_ip_banned(tmp)) return 1;
	    tmp = "*." + x[<2] + "." + x[<1];
	    if (query_ip_banned(tmp)) return 1;
	    tmp = "*." + x[<3] + "." + x[<2] + "." + x[<1];
	    if (query_ip_banned(tmp)) return 1;
	}
	else if (s > 2)
	{
	    tmp = "*." + x[<2] + "." + x[<1];
	    if (query_ip_banned(tmp)) return 1;
	}

	if (query_ip_banned(ipname)) return 1;
    }

    if (stringp(ipno))
    {
	x = explode(ipno, ".");
	s = sizeof(x);

	if (s > 3)
	{
	    tmp = x[0] + ".*";
	    if (query_ip_banned(tmp)) return 1;
	    tmp = x[0] + "." + x[1] + ".*";
	    if (query_ip_banned(tmp)) return 1;
	    tmp = x[0] + "." + x[1] + "." + x[2] + ".*";
	    if (query_ip_banned(tmp)) return 1;
	}

	if (query_ip_banned(ipno)) return 1;
    }

    return 0;
}

nomask status
query_ip_stop(string ip)
{
    return (member(ip_stopped, ip) != -1);
}

// Stop an IP?
nomask status
is_ip_stopped(object who)
{
    string ipno, ipname, *x, tmp;
    int b, s;

    if (!pointerp(ip_stopped)) return 0;
    if (!who) return 0;
    ipname = interactive_info(who, II_IP_NAME);
    ipno = interactive_info(who, II_IP_NUMBER);

    if (stringp(ipname))
    {
	x = explode(lower_case(ipname), ".");
	s = sizeof(x);

	if (s > 3)
	{
	    tmp = "*." + x[<1];
	    if (query_ip_stop(tmp)) return 1;
	    tmp = "*." + x[<2] + "." + x[<1];
	    if (query_ip_stop(tmp)) return 1;
	    tmp = "*." + x[<3] + "." + x[<2] + "." + x[<1];
	    if (query_ip_stop(tmp)) return 1;
	}
	else if (s > 2)
	{
	    tmp = "*." + x[<2] + "." + x[<1];
	    if (query_ip_stop(tmp)) return 1;
	}

	if (query_ip_stop(ipname)) return 1;
    }

    if (stringp(ipno))
    {
	x = explode(ipno, ".");
	s = sizeof(x);

	if (s > 3)
	{
	    tmp = x[0] + ".*";
	    if (query_ip_stop(tmp)) return 1;
	    tmp = x[0] + "." + x[1] + ".*";
	    if (query_ip_stop(tmp)) return 1;
	    tmp = x[0] + "." + x[1] + "." + x[2] + ".*";
	    if (query_ip_stop(tmp)) return 1;
	}

	if (query_ip_stop(ipno)) return 1;
    }

    return 0;
}

nomask status
query_guest_ip_banned(string ip)
{
    return (member(guest_ip_banished, ip) != -1);
}

// Can Guest be used from ip?
nomask status
is_guest_ip_banished(object who, string n)
{
    string ipno, ipname, *x, tmp;
    int b, s;

    if (!who || !n) return 0;

    ipname = interactive_info(who, II_IP_NAME);
    ipno = interactive_info(who, II_IP_NUMBER);

    if (stringp(ipname))
    {
	x = explode(ipname, ".");
	s = sizeof(x);

	if (s > 3)
	{
	    tmp = "*." + x[3];
	    if (query_guest_ip_banned(tmp)) return 1;
	    tmp = "*." + x[2] + "." + x[3];
	    if (query_guest_ip_banned(tmp)) return 1;
	    tmp = "*." + x[1] + "." + x[2] + "." + x[3];
	    if (query_guest_ip_banned(tmp)) return 1;
	}
	else if (s > 2)
	{
	    tmp = "*." + x[1] + "." + x[2];
	    if (query_guest_ip_banned(tmp)) return 1;
	}

	if (query_guest_ip_banned(ipname)) return 1;
    }

    if (stringp(ipno))
    {
	x = explode(ipno, ".");
	s = sizeof(x);

	if (s > 3)
	{
	    tmp = x[0] + ".*";
	    if (query_guest_ip_banned(tmp)) return 1;
	    tmp = x[0] + "." + x[1] + ".*";
	    if (query_guest_ip_banned(tmp)) return 1;
	    tmp = x[0] + "." + x[1] + "." + x[2] + ".*";
	    if (query_guest_ip_banned(tmp)) return 1;
	}

	if (query_guest_ip_banned(ipno)) return 1;
    }

    return 0;
}


nomask int banish(string str, int lvl)
{
    string tmp;
    int x;

    if(!str) return 0;

    if (lvl < -3) return 0;

    /* It must come from a real player */
    if (!interactive_info(this_player(), II_IP_NUMBER)) return 0;

    /* Only player object can call this... It does checking. -+ Doomdark +- */
    if (!interactive(previous_object())) return 0;

    // Only senior coders & higher! And Police.
    if ((int)previous_object()->query_coder_level() < LVL_SENIOR &&
      (string)previous_object()->query_real_name() != "police") return 0;

    log_file("BANISH",this_player()->query_real_name() + " banished " +
      str + " to time/lvl " + lvl + " at " + ctime(time()) + "\n");

    x = member(banished, str);
    if (x == -1) {
	banished += ({str}); level += ({lvl});
    } else level[x] = lvl;

    save_me();
    return 1;
}


nomask int
banish_ip(string str)
{
    string tmp;
    int x;

    if (!str) return 0;

    if (!interactive_info(this_player(), II_IP_NUMBER)) return 0;
    if (!interactive(this_player())) return 0;
    if (this_player()->query_coder_level() < LVL_COADMIN) return 0;
    if (member(ip_banished, str) != -1) return 0;

    log_file("BANISH_IP",this_player()->query_real_name() + " banished IP " +
      str + " at " + ctime(time()) + "\n");

    ip_banished += ({ str });

    save_me();
    return 1;
}

nomask int
stop_ip(string str)
{
    string tmp;
    int x;

    if (!str) return 0;
    if (!interactive_info(this_player(), II_IP_NUMBER)) return 0;
    if (!interactive(this_player())) return 0;
    if (this_player()->query_coder_level() < LVL_COADMIN) return 0;
    if (!pointerp(ip_stopped)) ip_stopped = ({});
    if (member(ip_stopped, str) != -1) return 0;

    log_file("BANISH_IP",this_player()->query_real_name() + " _STOPPED_ IP " +
      str + " at " + ctime(time()) + "\n");

    ip_stopped += ({ str });

    save_me();
    return 1;
}

nomask int
banish_guest_ip(string str)
{
    string tmp;
    int x;

    if (!str) return 0;

    if (!interactive_info(this_player(), II_IP_NUMBER)) return 0;
    if (!interactive(this_player())) return 0;
    if (this_player()->query_coder_level() < LVL_COADMIN) return 0;
    if (member(guest_ip_banished, str) != -1) return 0;

    log_file("BANISH_IP",this_player()->query_real_name() +
      " banished Guest from IP " +
      str + " at " + ctime(time()) + "\n");

    guest_ip_banished += ({ str });

    save_me();
    return 1;
}

nomask int
allow_ip(string str)
{
    string tmp;
    int x;

    if (!str) return 0;

    if (!interactive_info(this_player(), II_IP_NUMBER)) return 0;
    if (!interactive(this_player())) return 0;
    if (this_player()->query_coder_level() < LVL_COADMIN) return 0;
    if (member(ip_banished, str) == -1) return 0;

    log_file("BANISH_IP",this_player()->query_real_name() + " allowed IP " +
      str + " at " + ctime(time()) + "\n");

    ip_banished -= ({ str });

    save_me();
    return 1;
}

nomask int
unstop_ip(string str)
{
    string tmp;
    int x;

    if (!str) return 0;

    if (!interactive_info(this_player(), II_IP_NUMBER)) return 0;
    if (!interactive(this_player())) return 0;
    if (this_player()->query_coder_level() < LVL_COADMIN) return 0;
    if (!pointerp(ip_stopped)) return 0;
    if (member(ip_stopped, str) == -1) return 0;

    log_file("BANISH_IP",this_player()->query_real_name() + " Un-STOPPED IP " +
      str + " at " + ctime(time()) + "\n");

    ip_stopped -= ({ str });

    save_me();
    return 1;
}

nomask int
allow_guest_ip(string str)
{
    string tmp;
    int x;

    if (!str) return 0;

    if (!interactive_info(this_player(), II_IP_NUMBER)) return 0;
    if (!interactive(this_player())) return 0;
    if (this_player()->query_coder_level() < LVL_COADMIN) return 0;
    if (member(guest_ip_banished, str) == -1) return 0;

    log_file("BANISH_IP",this_player()->query_real_name() +
      " allowed Guest from IP " +
      str + " at " + ctime(time()) + "\n");

    guest_ip_banished -= ({ str });

    save_me();
    return 1;
}

nomask int
allow_name(string str)
{
    string tmp;
    int x;

    if (!str) return 0;

    if (!interactive_info(this_player(), II_IP_NUMBER)) return 0;
    if (!interactive(this_player())) return 0;
    if (this_player()->query_coder_level() < LVL_COADMIN) return 0;
    str = lower_case(str);
    if (member(allowed, str) != -1) return 0;
    // Easier to not have this here...
    // if (!master_object()->exists_player(str)) return 0;

    log_file("BANISH_IP",this_player()->query_real_name() + " allowed name " +
      str + " at " + ctime(time()) + "\n");

    allowed += ({ str });

    save_me();
    return 1;
}

nomask int
disallow_name(string str)
{
    string tmp;
    int x;

    if (!str) return 0;

    if (!interactive_info(this_player(), II_IP_NUMBER)) return 0;
    if (!interactive(this_player())) return 0;
    if (this_player()->query_coder_level() < LVL_COADMIN) return 0;
    str = lower_case(str);
    if (member(allowed, str) == -1) return 0;

    log_file("BANISH_IP",this_player()->query_real_name() + " disallowed name " +
      str + " at " + ctime(time()) + "\n");

    allowed -= ({ str });

    save_me();
    return 1;
}

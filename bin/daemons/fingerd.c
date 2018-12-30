#include "/sys/interactive_info.h"

#include <coder_levels.h>
#include <guild.h>
#include <config.h>

/*
 * Finger daemon
 */

#define LASTTIME 60  /* How much time until reread the savefile */

/* To stop multiple reads of same file. */
private static int last_finger;

int     gender;
string  race;
int     money;
string  name;
int     level;
int     experience;
int     age;
int     frog, ghost, dead;
string  auto_load; /* "/u/rincewind/dictionary:^!" */
int     alignment;
int     Str, Int, Con, Dex;
int	luck;
int    *conditions;
string  title, al_title;
string  desc;
int     scar;
string  called_from_ip_name, called_from_ip;
int     last_saved;
string  quests;
string  mailaddr;
string  liege;
int     coder_level;
string guild;

/*
 * Function:    do_load
 * Description: Load data from a player savefile.
 * Returns:     True if load succesfull.
 */
nomask int
do_load(string str) {
    int i;

    /* Check for validity of name */
    if (!stringp(str)) return 0;
    str = lower_case(str);
    for (i = 0; i < sizeof(str); i++) {
        if (str[i] < 'a' || str[i] > 'z') return 0;
    }

    /* Restore data */
	if (restore_object(PATH_FOR_PLAYER_SAVE(str))) {
	last_finger = time();
	return 1;
    }
}

nomask private int
show_name(string str) {
    /* Check if data is loaded or load it */
    if (!stringp(str)) return 0;
    if ((str != name || time() > last_finger + LASTTIME) && !do_load(str)) return 0;

    /* Show name, title, race, liege, level or coder status and age */
    write("Name  : " + capitalize(name) + " " + title + "\n");
    str = "Status: " + ({"neuter", "male", "female" })[gender]
        + (race ? (" " + race) : " *human*");
    if (liege) str += ", servant of " + capitalize(liege);
    switch (coder_level) {
	case -1:  str += ", testplayer (" + level + ")"; break;
        case 0:   str += ", level " + level; break;
        case 10:  str += ", novice coder"; break;
	case 50: str += ", junior coder"; break;
        case 100: str += ", coder"; break;
        case 200: str += ", senior coder"; break;
        case 300: str += ", co-admin"; break;
        case 400: str += ", admin"; break;
        default: str += ", coder level(" + coder_level + ")";
    }
    write(str + "  (" + age/43200 + " d " + (age/1800)%24 + " h)\n");
    return 1;
}

/*nomask private*/ void show_new_mail(string str) {
    string tmp;

    tmp = (string)"/bin/daemons/maild"->query_mail(str);
    if( tmp  && (strstr(tmp, "NEW") != -1) )
      write("There is unread mail.\n");
    else
      write("No unread mail.\n");
}

nomask private void
show_login() {
    object ob;
    string str;
    int    i;

    if (ob = find_player(name)) {
        if (interactive(ob)) {
	    i = interactive_info(ob, II_IDLE) / 60;
	    write("Logged in." + (i ? (" Idle " + i + " min.\n") : "\n"));
	} else
	    write("Link dead.\n");
    } else {
        if (last_saved) {
	    if ((i = time() - last_saved) >= 86400)
	        str = (i = i/86400) + " day";
	    else if (i >= 3600)
		str = (i = i/3600) + " hour";
	    else
	        str = (i = i/60) + " minute";
	    if (i != 1) str += "s";
	    write("Logged out " + str + " ago.\n");
	} else {
	    write("Logged out.\n");
	}
    }
}

/*
 * Function:    show_mortal
 * Description: Show the data on a player. No loading if data already in
 *              memory. This shows MORTAL finger.
 * Returns:     True is succesfull.
 */
nomask int
show_mortal(string str, object who) {
    string *xstr, tmp1, tmp2, tmp3;
    int    i;

    if (!show_name(str))
	return 0;
    tmp3 = str;
    if (stringp(called_from_ip_name)) {
        xstr = explode(called_from_ip_name, ".");
	if (sizeof(xstr) && !sscanf(str = xstr[sizeof(xstr) - 1], "%d", i)) {
	    printf("Called from: .%s\n", str);
	}
    }

/*
#ifdef GN_THIEF
    /* Show guild if both are thieves. */
/*
	if (sscanf(auto_load, "%s/thief_mark%s", tmp1, tmp2) == 2 &&
	who && (string)who->query_guild() == GN_THIEF)
        write("Member of Guild of Thieves\n");
#endif
*/

    show_new_mail(tmp3);

    /*  We don't show information about coders to mortals //Frobozz
	Yes we do. But not when they have logged out since.
	Exception with coders that haven't logged in for 10 days. --Val */
    if (!coder_level || (time()-last_saved > 86400 * 10 && !find_player(name)))
	show_login();
    return 1;
}

/*
 * Description: Show extended finger for coders.
 */
nomask int
show_coder(string str) {
    if (!show_name(str)) return 0;

    /* Extended finger informantion */
    write(sprintf("Stats : %2d str  %2d dex  %2d int  %2d con\n",
        Str, Dex, Int, Con));
    if (luck)
	printf("Other : %d exp  %d $  (luck:%d)\n", experience, money, luck);
    else
	printf("Other : %d exp  %d $\n", experience, money);
    write(sprintf("Quests  : %-=69s\n",
        (quests ? implode(explode(quests, "#"), " ") : "none") ));
    write(sprintf("Autoload: %-=69s\n",
        (auto_load ? implode(explode(auto_load, "^!"), "\n") : "none") ));
     write("Member of "+guild+" guild.\n");
    if(this_interactive()->query_coder_level() > 100)
      write("Email: "+ mailaddr+"\n");
    write("Called from: " + called_from_ip_name
        + ((called_from_ip_name != called_from_ip)
            ? (" (" + called_from_ip + ")\n") : "\n") );
    show_new_mail(str);
    show_login();
    return 1;
}

nomask int clean_up() { destruct(this_object()); }

int
query_last_save(string who)
{
	if (who && who != name)
		do_load(who);
	return last_saved;
}

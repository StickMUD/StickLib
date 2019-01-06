#include "/sys/interactive_info.h"

#include <config.h>
#include <daemons.h>
#include <std_lib>
#include <room_defs.h>
#include <coder_levels.h>
#include <invis_levels.h>

#define READ_DELAY 10

#define BAR \
"=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n"

int gender, level, coder_level, age, founder, last_saved;
string name, race, kingdom, clan, spouse, location, rlname, birthday, 
    mailaddr, icq, called_from_ip_name, webpage;
private static int last_read;

string process_status()
{
    return capitalize(({"neuter", "male", "female"})[gender]) + " " + ((race) ?
        race : "human");
}

string process_icq()
{
    object ob;

    if (!icq)
        return "None";
    else if (ob = find_player(name))
        return ob->query_icq();
    else
        return icq;
}
string process_position()
{
    object ob;

    if (!coder_level)
    {
        if (ob = find_player(name))
            return "Mortal [" + (int)ob->query_level() + "]";
        else
            return "Mortal [" + level + "]";
    }
    else if (coder_level < 0)
        return "Test character";
    else if (coder_level < LVL_APPRENTICE)
        return "Divided";
    else if (coder_level < LVL_NOVICE)
        return "Apprentice Wizard";
    else if (coder_level < LVL_CODER)
        return "Novice Wizard";
    else if (coder_level < LVL_SENIOR)
        return "Wizard";
    else if (coder_level < LVL_ELDER)
        return "Senior Wizard";
    else if (coder_level < LVL_ARCH)
        return "Elder Wizard";
    else if (coder_level < LVL_COADMIN)
        return "Arch Wizard";
    else if (coder_level < LVL_ADMIN)
        return "Co-Administrator";
    else
        return "Administrator";
}

string process_kingdom()
{
    if (!coder_level)
        return (kingdom) ? capitalize(kingdom) : "None";

    if (member(({"fire", "wind", "water", "earth", "mudlib"}), kingdom) != -1)
        return capitalize(kingdom);
    else
        return "Immortal";
}

string process_clan()
{
    if (!coder_level)
        return (clan) ? capitalize(clan) : "None";
    else
        return "Coders";
}

string process_age()
{
    int i;
    string tmp;
    object ob;

    if (ob = find_player(name))
        return secs2sstring((int)ob->query_age() * 2);

    return secs2sstring(age * 2);
}

string process_login()
{
    int i;
    string tmp;
    object ob;

    if (ob = find_player(name))
    {
        if (interactive(ob))
        {
            i = interactive_info(ob, II_IDLE) / 60;
            if (i)
            {
                tmp = " (idle "+ i; 
                tmp += (i > 1) ? " mins)" : " min)";
            }
            else
                tmp = "";
            return "Connected" + tmp;
        }
        else
            return "Linkdead";
    }
    else
    {
        if (last_saved)
        {
           tmp = "";
           if ((i = time() - last_saved) >= 86400)
               tmp = (i = i / 86400) + " day";
           else if (i >= 3600)
               tmp = (i = i / 3600) + " hour";
           else
               tmp = (i = i / 60) + " minute";
           if (i != 1) tmp += "s";
           return "Left " + tmp + " ago";
        }
        else
            return "Not connected";
    }
}

string process_mudmail()
{
    if ((string)MAIL_D->query_mail(name) && strstr((string)MAIL_D->query_mail(name), "NEW") != -1)
        return "New messages waiting";
    else
        return "No new messages";
}
    
nomask status load_data(string arg)
{
    if (!stringp(arg))
        return 0;

    if (restore_object(PATH_FOR_PLAYER_SAVE(lower_case(arg))))
    {
        last_read = time();
        return 1;
    }

    return 0;
}

nomask mixed show_data(string arg)
{
    string tmp;

    if (!stringp(arg))
        return 0;

    if ((arg != name || time() > last_read + READ_DELAY) && !load_data(arg))
        return 0;

    tmp = BAR;
    tmp += sprintf("%|73s\n", capitalize(implode(explode(name, ""), "  ")));
    tmp += BAR;
    tmp += sprintf("%10s: %-25s %10s: %-25s\n", "Status", process_status(),
        "Kingdom", process_kingdom());
    tmp += sprintf("%10s: %-25s %10s: %-25s\n", "Position", process_position(),
        "Clan", process_clan());
    tmp += sprintf("%10s: %-25s %10s: %-25s\n\n", "Spouse", (spouse) ? 
        capitalize(spouse) : "Single", "Age", process_age());
    tmp += sprintf("%10s: %-25s %10s: %-25s\n", "Location", (location) ?
        capitalize(location) : "Unknown", "Name", (rlname) ? capitalize(rlname)
: "Unknown");
 
   if (this_player()->query_coder_level() >= LVL_ARCH)
    {
        tmp += sprintf("%10s: %-25s\n", "Birthday", (birthday) ? 
            birthday : "Unknown");
        tmp += sprintf("%10s: %s\n", "ICQ", process_icq());
        tmp += sprintf("%10s: %s\n", "Mail", mailaddr);
        tmp += sprintf("%10s: %s from %s\n", "Login", process_login(),
            called_from_ip_name);
    }
    else
    {
        tmp += sprintf("%10s: %-25s %10s: %-25s\n", "Birthday", (birthday) ? 
            birthday : "Unknown", "Login", process_login());
        tmp += sprintf("%10s: %s\n", "ICQ", process_icq());
        tmp += sprintf("%10s: %s\n", "Mail", (mailaddr[0..0] == "#") ?
            "Private" : mailaddr);
    }
    tmp += sprintf("%10s: %s\n\n", "Mudmail", process_mudmail());
    tmp += sprintf("%10s: %s\n", "Webpage", !webpage ? "None" : webpage);
    tmp += BAR;
    
    this_player()->tell_me(tmp);
    return 1;
}

int query_last_save(string who)
{
    if (who && who != name)
        load_data(who);
    return last_saved;
}

nomask int sort_em(object ob1, object ob2)
{
    int x1, x2;
 
    if (!objectp(ob2)) return 1;
    if (!objectp(ob1)) return -1;
 
    x1 = (int)ob1->query_real_name();
    x2 = (int)ob2->query_real_name();
    if (x1 > x2) return 1;
    if (x1 < x2) return -1;
 
    return 0;
}

nomask string finger_list(object me)
{
    string who_list;
    object *who_member, *who;
    int i, num, il, me_coder, me_level;
    status isolated;

    who = sort_array(users(), #'sort_em, this_object());
    who_member = ({});

    if (me && environment(me))
        isolated = (status)environment(me)->query(ROOM_ISOLATED);
    else isolated = 0;

    if (!isolated)
    {
        for(i = 0; i < sizeof(who); i++)
            if (who[i] && interactive(who[i]))
                who_member += ({ who[i] });
    }
    else who_member = ({ me });

    who_list = "";

   who_list += sprintf("       Name Lvl Kingdom Clan          Birthday      Location\n");

    me_coder = (int)me->query_coder_level();
    me_level = (int)me->query_level();

    num = 0;

    for (i = 0; i < sizeof(who_member); i++)
    {
        il = (int)who_member[i]->query_invis();

/* We're broken to non coders.....
        if (me_coder || (!environment(who_member[i])->query(ROOM_ISOLATED)
*/
	if (me_coder || ((environment(who_member[i]) && !environment(
	who_member[i])->query(ROOM_ISOLATED))
            && (!il || (il < IL_TOTALLY_INVISIBLE && me_level > 21))))
        {
            who_list += sprintf("%11s %3s %-7s %-13s %-13s %s\n",
                capitalize((string)who_member[i]->query_real_name()),
                !(int)who_member[i]->query_coder_level() ?
                    sprintf("%d", (int)who_member[i]->query_level()) : "wiz",
                who_member[i]->query_kingdom() != 0 ?
                    (string)who_member[i]->query_kingdom() : "",
                who_member[i]->query_clan() != 0 ?
                    (string)who_member[i]->query_clan() : "",
                who_member[i]->query_birthday() != 0 ?
                    (string)who_member[i]->query_birthday() : "",
                who_member[i]->query_location() != 0 ?
                    (string)who_member[i]->query_location() : "");
            num++;
        }
    }

    who_list += "Total number of people visible: " + num + "\n";

    return who_list;
}

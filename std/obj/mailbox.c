//****************************************************************
// Standard mailbox object.                                     //
// Version 1.0g                                                 //
//                                                              //
// Created by Germ 10-jun-96                                    //
//                                                              //
// NEW:                                                         //
//      The mailbox now masks catch_event(EVENT_LOOKED_AT,...)  //
//      for implementing the long desc.                         //
//                                                              //
//      18-Sep-96, Germ: The alias list is now derived from     //
//              the defines in /secure/coders.h                 //
//      16-Oct-96, Germ: Implemented read <next/previous>       //
//      17-Oct-96, Germ: Updated the input_to functionality     //
//      12-apr-97, Doomdark: Fixed "read"-command not to catch  //
//              every read command -> can now read other objects//
//              in the room too. *grin*                         //
//      05-jun-97, Germ: removed strstr() from "help"           //
//              command processing.                             //
// 16-Jan-98, Doomdark: Finally made mail box use EVENT_READ    //
//      like bulletin boards.                                   //
// 19-Sep-98, Kieve: Modified it for StockMUD                   //
//                                                              //
//****************************************************************

#include "/sys/object_info.h"

#include <mail_defs.h>
#include <daemons.h>
#include "/secure/coders.h"

inherit "/basic/id";

#define TP this_player()
#define _MAIL_DEST 0    // message mapping index
#define _MAIL_SUBJ 1    // ""
#define _MAIL_CC   2    // ""
#define _MAIL_MESS 3    // ""

#define _FUNC_SHORT     0
#define _FUNC_LONG      1

#define DEBUG(xx) if(find_player("kieve")) \
                      find_player("kieve")->tell_me(xx);

#define _PLAYER_OBJ ( previous_object()\
        && (sscanf(object_name( previous_object()),"/std/player#%s",temp_cc)==1)\
        && me == previous_object())

static mapping message; // Mapping of messages that are in the
// process of being written.
static status _temporary;

void input_mail_data(string str, int flag, mixed temp_data);

/****************************************************************
*                                                               *
*                       Query-functions:                        *
*                                                               *
****************************************************************/

status query_prevent_shadow() { return 1; }     // No shadowing this object

/****************************************************************
*                                                               *
*                       Set-functions:                          *
*                                                               *
****************************************************************/
/****************************************************************
*                                                               *
*                       Standard functions:                     *
*                                                               *
****************************************************************/

void
create(mixed arg)
{
    set_name("mailbox");
    set_id( ({ "box", "mail", "next" }) );
    set_short("A broken mailbox");
    set_long("This is a mailbox, which unfortunately appears to be "+
      "non-functional.");
}

void init()
{
    add_action("post_command", "save");
    add_action("post_command", "delete");
    add_action("post_command", "mail");
    add_action("post_command", "reply");
    add_action("post_command", "greply");
    add_action("post_command", "forward");
    add_action("post_command", "help");
    add_action("read_cmd", "read");
}

int get() { return 0; }

/* We'll use a small kludge to get rid of unnecessary mail box created by
 * test players:
 */
void reset()
{
    object x;
    if (!_temporary || !environment())
	return;
    x = first_inventory(environment());
    while (x) {
	if (object_info(x, OI_ONCE_INTERACTIVE))
	    return;
	x = next_inventory(x);
    }
    environment()->tell_here(":%<it.capname>% suddenly disappears in "+
      "a purple puff of smoke!", 0, 0, 0, 0, 0, this_object());
    destruct(this_object());
}

/****************************************************************
*                                                               *
*               Event-handling functions:                       *
*                                                               *
****************************************************************/

string query_long()
{
    string desc;

    if ( this_player()->query_real_name() == "guest" )
	return "Guests are not permitted to use the post office.";

    if ( !MAIL_D->post_command(CMD_MAIL_HEADERS, 0, &desc, TP))
	return "There's a large gaping hole in the side of your mailbox!";

    return sprintf("Type 'help mailbox' to see the mailbox commands.\n%s\n",
      desc);
}

status read_cmd(string str)
{
    string return_text;
    mixed *temp, mnum;
    object me;
    int num;

    me = this_player();

    if ( me != this_interactive())
	return 0;

    // We prevent Guest from sending mail, presumably to prevent players
    // from harrassing people anonymously.
    //
    if ( me->query_real_name() == "guest" )
    {
	me->tell_me("Guests are not permitted to use the post office.");
	return 1;
    }

    if ( !str ) {
	me->tell_me("Read what from mail box?");
	return 1;
    }
    if (intp(str))
	mnum = str;
    else if ( sscanf(str,"%d", mnum)!=1) {
	if (str != "next" && str != "previous") {
	    me->tell_me("Can't see any such thing in the "+
	      "mail box.");
	    return 1;
	}
	mnum = str;
    }
    if ( !temp = MAIL_D->query_info(mnum, me )) return 0;
    if ( stringp(temp)) {
	me->tell_me(temp);
	return 1;
    } else if ( !mappingp(temp))
	return 0;
    me->tell_me( sprintf(
	"From:\t\t%s\nSubject:\t%s\nDate:\t\t%s\nCC:\t\t%s\n",
	temp[INFO_MAIL_SENDER], temp[INFO_MAIL_SUBJECT],
	ctime(temp[INFO_MAIL_TIME])[4..9],
	temp[INFO_MAIL_CC]));
    me->more(explode(temp[INFO_MAIL_MESSAGE], "\n"));
    return 1;
}

/****************************************************************
*                                                               *
*               And then the other functions:                   *
*                                                               *
****************************************************************/


//
// Cleans the message mapping by removing the player specified
// by who, as well as any other players that are not here.
// Link-dead players are not removed.
// Note: we are looping thru the mapping in order to eliminate
// any players who are not currently mailing. This is not real
// efficient but seems like the best way. :)
//
void
clean_user_mapping(object who)
{
    int i;
    object *key;
    mapping tmp;


    if ( !message || !key = m_indices(message)) return;
    tmp = m_allocate(sizeof(key),4);
    for ( i=0; i<sizeof(key); i++ )
    {
	if ( key[i]==who ) continue;
	if ( !interactive(key[i]) || present(key[i],environment()))
	    tmp += ([ key[i] : message[key[i],0]; message[key[i],1];
	      message[key[i],2]; message[key[i],3] ]);
}
message = tmp;
}


//
// Called via closure by the short desc for this object.
// Allows the short desc to contain variable info.
//
string
query_short()
{
    object me;
    int num;

    me = this_player();

    if ( !me ) return "A broken mailbox";
    if ( me->query_real_name() == "guest" )
	return "Guests are not permitted to use the post office";

    if ( num = (int)MAIL_D->query_mail( me,1))
	return sprintf("Your mailbox is here, containing %d message%s",
	  num, ((num>1)?"s":""));

    return "Your mailbox is here, looking empty";
}

status
post_command(string str, object me )
{
    string return_text;
    mixed *temp, mnum;
    int num;
    if (!me && !me = this_player()) return 0;
    if ( me != this_interactive()) return 0;

    //    if ( !_PLAYER_OBJ ) return 0;
    //
    // We prevent Guest from sending mail, presumably to prevent players
    // from harrassing people anonymously.
    //
    if ( me->query_real_name() == "guest" )
    {
	me->tell_me("Guests are not permitted to use the post office.");
	return 1;
    }

    switch ( query_verb() )     // Do the user commands and stuff:
    {
    case "save" :
	if (!MAIL_D->post_command(CMD_MAIL_SAVE, str, &return_text, me))
	    return 0;
	me->tell_me(return_text);
	return 1;

    case "delete" :
	if (!MAIL_D->post_command(CMD_MAIL_DELETE, str, &return_text, me))
	    return 0;
	me->tell_me(return_text);
	return 1;

    case "mail" :
	if ( !str || str=="" ) return 0;
	if ( !str = MAIL_D->query_valid_name(str))
	{
	    me->tell_me("No such player.");
	    return 1;
	}
	input_mail_data( str, _MAIL_DEST,
	  ([ INFO_MAIL_SENDER  : str,
	    INFO_MAIL_SUBJECT : "None",
	    INFO_MAIL_CC      : "",
	    INFO_MAIL_MESSAGE : "" ]));
	return 1;

    case "forward" :
	if ( !str || sscanf(str,"%d %s",num,str )!=2) return 0;
	if ( !str = MAIL_D->query_valid_name(str))
	{
	    me->tell_me("No such player.");
	    return 1;
	}
	if ( !temp=MAIL_D->query_info(num,me)) return 0;
	if (stringp(temp))
	{
	    me->tell_me(temp);
	    return 1;
	}
	if ( !mappingp(temp) ) return 0;
	MAIL_D->send_mail(TP, str,
	  sprintf("%s (fwd:%s)",  temp[INFO_MAIL_SUBJECT],
	    temp[INFO_MAIL_SENDER]),
	  temp[INFO_MAIL_CC],
	  temp[INFO_MAIL_MESSAGE] );
	return 1;

    case "reply" :
    case "greply" :
	if ( !str || sscanf(str,"%d",num)!=1) return 0;
	if ( !mappingp(temp)
	  && !temp = MAIL_D->query_info(num,me)) return 0;
	if (stringp(temp))
	{
	    me->tell_me(temp);
	    return 1;
	}
	else
	if ( !mappingp(temp) ) return 0;
	me->tell_me(sprintf("Replying to: %s",temp[INFO_MAIL_SENDER]));
	if ( !temp[INFO_MAIL_CC] || query_verb()!="greply" )
	    temp[INFO_MAIL_CC] = "";
	temp[INFO_MAIL_SUBJECT] = sprintf("Re: %s",temp[INFO_MAIL_SUBJECT]);
	input_mail_data( str, _MAIL_DEST, temp );
	return 1;

    case "help" :
	if ( !str || (str!="box" && str[0..3]!="mail" )) return 0;
	me->tell_me(sprintf("Mailbox commands are:\n\n\
        read <n/next/prev> - Read a message in the mailbox.\n\
        save <n>           - Save message <n> (Coders only).\n\
        delete <n>         - Delete message <n>.\n\
        mail <name/alias>  - Send a message to <name> or <alias>.\n\
        reply <n>          - Reply to the author of message <n>.\n\
        greply <n>         - Reply to the author and CC of message <n>.\n\
        forward <n> <name> - Forward message <n> to <name>.\n\n\
Valid aliases are:\n\   admins  : %s\n\ coadmins: %s\n",
	    implode(ADMIN_LIST,", "),
	    implode(COADMIN_LIST,", ")));
	return 1;
    }
    return 0;
}

void
input_mail_data(string str, int flag, mixed temp_data)
{
    string *cc_temp;
    object me;

    if ( !me=this_player() ) return;

    switch ( flag )
    {
    case _MAIL_DEST :
	if ( !mappingp(temp_data)) return;
	if ( !message ) message = ([ ]);
	if (!member(message,me)) message +=
	    ([ me : temp_data[INFO_MAIL_SENDER];
	      temp_data[INFO_MAIL_SUBJECT];
	      temp_data[INFO_MAIL_CC]; ""
	    ]);
	else
	{
	    message[me,_MAIL_DEST] = temp_data[INFO_MAIL_SENDER];
	    message[me,_MAIL_SUBJ] = temp_data[INFO_MAIL_SUBJECT];
	    message[me,_MAIL_CC]   = temp_data[INFO_MAIL_CC];
	    message[me,_MAIL_MESS] = "";
	}
	write(sprintf("Subject (%s): ",temp_data[INFO_MAIL_SUBJECT]));
	input_to("input_mail_data",0,_MAIL_SUBJ,0);
	return;

    case _MAIL_SUBJ :

	if ( str && str != "" ) message[TP,_MAIL_SUBJ]=str;
	if ( message[TP,_MAIL_CC]!="" )
	{
	    TP->tell_me(sprintf("CC: %s",message[TP,_MAIL_CC]));
	    write("Send also to (separate names with comma):");
	}
	else
	    write("Send copies to (separate names with comma): ");
	input_to("input_mail_data",0,_MAIL_CC,0);
	return;

    case _MAIL_CC :

	if (str && str != "") message[TP,_MAIL_CC] += str;
	write("Give message.  Finish message with '**', or '~q' to cancel.\n]");
	message[TP,_MAIL_MESS] = "";
	input_to("input_mail_data",0,_MAIL_MESS,0);
	return;

    case _MAIL_MESS :
	//
	// This is the main loop for writing mail.  Each new line is added
	// to the message, and when the end-char is found, the complete
	// message is sent to the post daemon.
	//
	if (str == "~q")
	{
	    TP->tell_me("Aborted.\n");
	    clean_user_mapping(TP);
	    return;
	}
	if (str == "**" || str == ".")
	{
	    MAIL_D->send_mail(TP,
	      message[TP,_MAIL_DEST],
	      message[TP,_MAIL_SUBJ],
	      message[TP,_MAIL_CC],
	      message[TP,_MAIL_MESS] );
	    clean_user_mapping(TP);
	    return;
	}
	message[TP,_MAIL_MESS] = sprintf("%s%s\n",message[TP,_MAIL_MESS],str);
	write("]");
	input_to("input_mail_data",0,_MAIL_MESS,0);
	return;
    } // switch
}

/* This shouldn't be needed except as long as we have that kludgy
 * 'mailbox'-command:
 */
void
set_temporary(status on)
{
    _temporary = on;
}

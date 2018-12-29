/****************************************************************
*								*
* Module:							*
*	/std/obj/bboard.c					*
* Description:							*
*	Standard bulletin board object for StickLib. Based on	*
*	the bulletin board of MurderMud (not StickMud). Makes	*
*	use of the bulletin board demon, which takes care of	*
*	storing and restoring the data (notes on the board).	*
*  NOTE! The security isn't really waterproof here; do not	*
*	count on b.boards to be really secure. If you need a	*
*	board that is _really_ non-accessible by others, either	*
*	develop security of this object or implement a better	*
*	one...							*
* Version:							*
*	1.0a (for StickLib), Aug-98				*
* Last update:							*
*	28-Aug-98 by Doomdark					*
*       03-Oct-98 by Kieve                                      *
*           Added closures to the filter mapping so coders can  *
*           have much more flexibility with the board           *
*								'
* Modifications:						*
*								*
****************************************************************/

#include <bboard_defs.h>
#include <coder_levels.h>
#include <daemons.h>

inherit "/basic/id";

#pragma strict_types

#define _MAX_SIZE_DEFAULT 150
#define _BOARD_SUBJ	0		// message mapping index
#define _BOARD_MESS	1		// ""
#define	_CACHE_SIZE	5

static mapping _message;	// Mapping of messages that are in the
				// process of being written.

static mapping _topfive;	// Five most recent messages
static string *_headers;	// Headers for all the messages.
static string *_filenames;	// Handles to the real savefile
static string _board_name;	// bboard directory name
static int _max_size;		// max number of messages on boad
static mapping _access;		// Filters for reading/writing
static mapping _last_read;	// Last-read info
static int _board_flags;	// Possible flags that may be set for this board
static object _last_room;	// Last room we have been in, if any

// Function prototypes
//
string board_desc_short(int mode, object me);
string board_desc_long(string str, object me);
void clean_user_mapping(object who);
int do_read(string str);
status update_board(object me);

/****************************************************************
*								*
*			Query-functions:			*
*								*
****************************************************************/

int
query_board_flag(int flags)
{
	return _board_flags & flags;
}

//
// This function implements the user-access system, and is called
// before any of the user's commands are processed. It if the
// mapping _access is defined, it compares the various elements
// with the caller as follows:
//	BB_READ_CODER  : Prevents reading by users with coderlvl
//			 less than what is defined here.
//	BB_WRITE_CODER : Prevents writing by users with coderlvl
//			 less than what is defined here.
//
status
query_access(object me, int write_op)
{
   int i, cl;
   closure func;
   string *temp;
 
   if ( !me || !objectp(me) ) return 0;
 
   if ( mappingp(_access) ) {
/* Hmmh. No need for looping, if we just have 2 options... */
 
        cl = (int) me->query_coder_level();
 
        if (write_op)
        {
            if(i = _access[BB_WRITE_CODER]) {
                if (cl < i) {
                     me->tell_me("Sorry, you cannot write on this board.");
                     return 0;
                }
            }
            if(func = _access[BB_WRITE_CLOSURE])
                // returns 1 if valid access
                return funcall(func, me);
        }
        else
        {
            if (i = _access[BB_READ_CODER]) {
                if (cl < i) {
                     me->tell_me("Sorry, you cannot access this board.");
                     return 0;
               }
           }
           if(func = _access[BB_READ_CLOSURE])
               // returns 1 for valid read.
               return funcall(func, me);
        }
    }
    return 1;
}

/****************************************************************
*								*
*			Set-functions:				*
*								*
****************************************************************/

// Mode; 0 -> clear, 1 -> set, -1 -> toggle
// Returns previous value of the flag(s)
int
set_board_flag(int flags, int mode)
{
int i;
	i = _board_flags & flags;
	if (!mode)
		_board_flags &= (~flags);
	else if (mode < 0)
		_board_flags ^= flags;
	else _board_flags |= flags;
	return i;
}

//
// Must be called after the board is cloned, and defines the save directory
// for this board, and the maximum number of messages.
//
void
set_board(int max, mapping filter, string boardname, object me )
{
	if ( max ) _max_size = max;
	_board_name = boardname;
	_access = filter;
	if (!me)
		update_board(this_object());
	else update_board(me);
}

/****************************************************************
*								*
*			Standard functions:			*
*								*
****************************************************************/

void
create()
{
	set_name("bulletin board");
	set_id(({ "board", "full board" }));
	set_short(#'board_desc_short);
	set_long(#'board_desc_long);
	_max_size = _MAX_SIZE_DEFAULT;
	_last_read = ([ ]);
}

void
init()
{
	add_action("do_read", "read");

	add_action("board_cmd", "help");
	add_action("board_cmd", "write");
	add_action("board_cmd", "list");
	add_action("board_cmd", "remove");
}

status
do_read(string str)
{
    string temp_str;
 mixed temp;
    int num,i,j;
    object me;

	if ( !str)
		return 0;

	me = this_player();
	if (sscanf(str, "%d", num)) {
		num--;
	} else {
	 num = (member(_last_read,me) < 0)? 1 : _last_read[me];
	 if (str == "next") {
		num++;
	 } else if (str == "prev" || str == "previous") {
		num--;
	 } else return 0;
	}

    if ( !_headers || !_topfive )
	update_board(this_object());

	if ( !query_access(me, 0))
		return 0;

	if (num >= sizeof(_filenames) || num < 0) {
		me->tell_me(sprintf("Only %d notes on the board.",
			sizeof(_filenames)));
		return 1;
	}
	if (temp = _topfive[num])
		;
	else if (!temp &&  !(temp=(mixed)BBOARD_D->query_board(
	BB_INFO_MESSAGE, _filenames[num], _board_name, me )))
		return 0;
	if (!stringp(temp)) {
		me->tell_me("Error when trying to read a note. Please\
 inform the administrators.");
		return 1;
	}
	_last_read[me] = num;
        if(stringp(temp))
            temp = explode((string)temp, "\n");
        me->more( temp, me);

	/* If StickLib supported 'last read note' thing, we'd
	 * set it here...
	 */
	if (!(_board_flags & F_BBOARD_NO_NOTIFY)) {
		//me->set_player_feature(FEAT_PLR_LAST_NOTE,_board_name,
		//_filenames[num]);
	}
	return 1;
}

/****************************************************************
*								*
*		And then the other functions:			*
*								*
****************************************************************/

//
// Gets a new header list, and updates the 5 most recent messages.
// This is called whenever anything changes on the board.
//
status
update_board(object me)
{
mixed tmp;
int i, size;

	if ( !pointerp(tmp=(mixed)BBOARD_D->query_board(
	(_board_flags & F_BBOARD_MISC_NOTES) ? BB_INFO_ALL2 : BB_INFO_ALL,
	0, _board_name,me)))
		return 0;
	_filenames = tmp[0];
	_headers = tmp[1];
	i = sizeof(_filenames) - _CACHE_SIZE;
	if (i < 0)
		i = 0;
	_topfive = m_allocate(i, _CACHE_SIZE);
	for (; i < sizeof(_filenames); i++)
	  _topfive[i] = (string) BBOARD_D->query_board(BB_INFO_MESSAGE,
		_filenames[i], _board_name, me );
	return 1;
}   

// Generic short description for bboard
//
string
board_desc_short(int mode, object me)
{
    if ( !_headers )
	update_board(this_object());
    return sprintf("A bulletin board with %d notes",sizeof(_headers));
}

string
board_desc_long(string id, object me)
{
    int i,j,num;
    string text, *m_arr;

    if (!me)
	me = this_player();

    if ( !query_access(me, 0) )
	return "You see nothing on it.";

    if ( !_headers )
	update_board(this_object());

	if (id == "full board") {
		m_arr = allocate(sizeof(_headers) + 1);
		m_arr[0] = sprintf("The bulletin board contains %d messages.\n",
			sizeof(_headers));
		for (i = 0; i < sizeof(_headers); i++)
		  m_arr[i + 1] = sprintf("%3d. %s",i+1,_headers[i]);
		return implode(m_arr, "\n");
	}

    num = sizeof(_headers);
    text = sprintf("The bulletin board contains %d messages.\
 Type 'help board' for commands.\n", num);
    if ( num <10 ) j=0;
    else
	{
	  j = num-10;
	  text = sprintf("%sListing the last 10 messages.\
 'look at full board' to see them all.\n",text);
	}
	   
// Generate and format the headers into the form:
//  num (date) Name       subject 
//
    for ( i=j; i<num; i++ )
	text = sprintf("%s\n%3d. %s",text,i+1,_headers[i]);
    return text;
}

//
// General command handler for the bboard.
//
status
board_cmd(string str, object me )
{
    string temp_str;
    mixed *temp;
    int num,i,j;
    if (!me && !(me = this_player())) return 0;
    if ( !_headers || !_topfive )
	update_board(this_object());

    if ( (string)me->query_real_name() == "guest")
       {
	 me->tell_me("Guest is not permitted to do that.");
	 return 1;
       }

    // Changed from str == "write" -- silly mistake --Kieve
    if ( !query_access(me, (query_verb() == "write")))
	return 1; 

    switch ( query_verb() )     // Do the user commands and stuff:
       {
// List 10 headers from the specified number.
//
	     case "list" :
		if ( !str || sscanf(str,"%d",j)!=1) return 0;
		num = sizeof(_headers);
		if ( j > num ) j = num-10;
		if ( --j < 0 ) j=0;
		if ( num > j+10 ) num = j+10;
		temp_str = sprintf("The bulletin board contains %d messages.\
 Type 'help board' for commands.\nListing messages %d thru %d:\n",
	 		sizeof(_headers),j+1,num);
		for (i=j; i<num;i++)
		   temp_str = sprintf("%s\n%3d. %s",temp_str,i+1,_headers[i]);		
		me->tell_me( sprintf("%s\n",temp_str));
		return 1;

// Write a note or post a file on the board.
// Files to be posted must be in the coders home directory tree.

	     case "write" :

		if ( !str || str=="" ) return 0;
		if ( sizeof(_headers) >= _max_size )
		  {
		     me->tell_me("Sorry, the board is full.");
		     return 1;
		  }
		if (sscanf(str,"note%t%s",temp_str)) {
		     if ( !me->query_coder_level())
			{
			  me->tell_me("Only coders may post a file.");
			  return 1;
			}
		     if (!(int)BBOARD_D->board_command(CMD_BB_POST,0,
			temp_str,_board_name, me, _board_flags)) {
			  me->tell_me(sprintf("Failed to post %s.",temp_str));
			  return 1;
			}
		     me->tell_me(sprintf("File posted to the board.",temp_str));
		     update_board(me);
		     return 1;
		  }
		if ( str != "note" )
		  {
		     me->tell_me("Usage: write note {[filename] [subject]}");
		     return 1;
		  }
		if (!_message ) _message = ([ me : "None";"" ]);
		else
		if ( member(_message,me))
		  {
		    _message[me,_BOARD_SUBJ] = "None";
		    _message[me,_BOARD_MESS] = "";
		  }
		else _message += ([ me : "None";"" ]);
		write("Subject: ");
		input_to("user_input_subject");
		return 1;

// Delete a note from the board.
// Players less than co-admin can only remove their own notes.

	     case "remove" :

		if ( !str || sscanf(str,"%d", num)!=1)
			return 0;
		if ( !(int)BBOARD_D->board_command(CMD_BB_REMOVE,
		num,0,_board_name, me, _board_flags))
		  {
			me->tell_me(sprintf("Failed to remove message %d.",num));
			return 1;
		  }
		me->tell_me(sprintf("Message %d removed.",num));
		update_board(me);
		return 1;

// Display board help info. 

	     case "help" :
                if (!str || (str && str != "board" && str != "bboard"))
                     return 0;
		me->tell_me("\nValid bulletin board commands are:\n\n\
  list [num]		- Lists 10 message headers, beginning with [num].\n\n\
  read [num/next/prev]	- Read note [num] on the board.\n\n\
  remove [num]		- Delete note [num] from the board.\n\
			  You can only delete your own notes\n\
			  unless you are an admin.\n\n\
  write note [filename] [subject]\n\
			- Write a note on the board. If filename is\n\
			  specified, that file will be posted.\n\
			  (The file must be in coders home dir tree).");
	}
     return 1;
}

// Called by input_to in board_cmd()
//
void
user_input_subject(string str)
{
    if ( str && str != "" ) _message[this_player(),_BOARD_SUBJ]=str;
    write("Give message.  Finish message with '**', or '~q' to cancel.\n]");
    _message[this_player(),_BOARD_MESS] = "";
    input_to("user_input_note");
}

//
// Called by input_to in user_input_subject().
// This is the main loop for writing a note.  Each new line is added
// to the message, and when the end-char is found, the complete
// message is sent to the post daemon.
//
void
user_input_note(string str)
{
    string tmp;
    if (str == "~q")
      {
	this_player()->tell_me("Aborted.\n");
	clean_user_mapping(this_player());
	return;
      }
    if (str == "**" || str == ".")
      {
	tmp = sprintf("Title: %s\nName: %s\nDate: %s\n\n%s",
		(string)_message[this_player(), _BOARD_SUBJ],
                capitalize((string)this_player()->query_real_name()),
		ctime(time())[4..9],
		(string)_message[this_player(), _BOARD_MESS]);
	if ( !BBOARD_D->board_command(CMD_BB_WRITE,0,tmp,
	_board_name,this_player(),
		_board_flags))
	  this_player()->tell_me("Failed to add message to the board.");
	else
	  this_player()->tell_me("Message added to the board.");
	clean_user_mapping(this_player());
	update_board(this_player());
	return;
      }
    _message[this_player(),_BOARD_MESS] = sprintf("%s%s\n",
	_message[this_player(),_BOARD_MESS],str);
    write("]");
    input_to("user_input_note");
}


//
// Cleans the message mapping by removing the player specified
// by who, as well as any other players that are not here.
// Link-dead players are not removed.
// Note: we are looping thru the mapping in order to eliminate
// any players who are not currently writing. This is not real
// efficient but I'm not sure how else to do it. :)
//
void
clean_user_mapping(object who)
{
int i;	
object ob, *key;

	if ( !_message || !(key = m_indices(_message)))
		return;
	for ( i=0; i<sizeof(key); i++ ) {
		ob = key[i];
		if (ob != who && (
		 !interactive(key[i]) || present(key[i],environment())))
			continue;
		m_delete(_message, ob);
	  }
}

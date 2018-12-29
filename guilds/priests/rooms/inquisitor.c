#include "../priest.h"
inherit PRIEST_ROOM_DIR "officer_room";
#include <living_defs.h>

/* Doh. Pleeeeease. No such warped names: */
#if 0
#define BOOK_FILE PRIEST_SAVE_DIR "inquisitor's_diary"
#else
#define	BOOK_FILE	PRIEST_SAVE_DIR "inq_diary"
#endif
#define SHOW_LINES 20

static int book_open;
static object writer;
static string* message;


string book_desc() {
    if(book_open) return "An old, black book. It's open.";
    return "An old, black book with some white runes on the cover. \
There is an eerie white light glowing from the runes.";
}

void create_room(){
    ::create_room();
    officer_title = "Inquisitor";
    set_short("The Office of the Inquisitor");
    set_long(
      "A small cosy room with some old candles giving out a dim, gloomy light. \
The smell of incense fills the air. Carved, black oak shelves circle the \
chamber with books and scrolls lying about. There is a large mahogany table \
in the centre of the room. On a black marble pedal there is an old black \
book with some runes written on the cover. The sounds of the Cathedral are \
almost fully silenced by the thick granite walls. Being in this room gives \
you a feeling of absolute peace and serenity.");

    set_items(([
	"shelves": "They are made of black painted oak. There are some \
ancient carvings on them.",
	"table": "It's a large mahogany table. On the table there are \
some candles and several other items, such as feather pens, ink, \
scrolls, and some other necessary things.",
	"pedal" : "This pedal is made of black marble. As you look closer \
you notice some engravings on it. You get a strange feeling there is \
more to it than just stone. On the pedal there is a large black book. \
You also notice an ancient rosarious hanging from chain on the pedal.",
	"book": #'book_desc,
	"rosarious": "This is a silver, medallion-like object usually worn \
by the inquisitor. There is a skull-shaped diamond in the middle of the \
rosarious, and some black pearls on the edges. A black pearl is added \
to the rosarious each time a fallen has repented her sins, and been \
forgiven by Boccob.",
      ]));
    add_exit("southeast", PRIEST_ROOM_DIR "basement");
    book_open = 0;

    add_command("write", "write_cmd");
}

string get_date() {
    int day;
    string endung;

    day = nature()->query_day();
    if (day>10 && day<20) endung = "th";
    else switch(day%10) {
    case 1: endung = "st"; break; case 2: endung = "nd"; break;
    case 3: endung = "rd"; break; default: endung = "th";
    }

    return sprintf("%s the %d%s in the Year of the Lord %d",
      nature()->query_month_s(),
      day, endung, nature()->query_year());
}

status read_cmd(string what) {
    int i, from_beginning;

    if(what != "book") {
	if(what != "book from beginning") return 0;
	from_beginning = 1;
    }
    if(!book_open) return notify_fail("But it is closed.\n"),0;
    if(this_player()->query_guild() != GN_PRIEST) return
	notify_fail("You try to read the book, but are unable to understand \
a single word of it.\n"),0;
    i = file_lines(BOOK_FILE);
    if(!i) return notify_fail("The book seems to be empty.\n"),0;
    i -= SHOW_LINES-1;
    if(from_beginning || (i<1))
	this_player()->more(explode(read_file(BOOK_FILE), "\n"));
    else
	this_player()->more(explode(read_file(BOOK_FILE,
	      i, SHOW_LINES), "\n"));
    return 1;
}

status open_cmd(string what) {
    if(what != "book") return 0;
    if(book_open) return notify_fail("But it is already open.\n"),0;
    book_open = 1;
    tell_here(({":%<me.capname>% open%<me.ending_s>% the book."}),
      ({LIV_CAN_SEE}),0,0,this_player(),0,0);
    return 1;
}

status close_cmd(string what) {
    if(what != "book") return 0;
    if(!book_open) return notify_fail("But it's already closed.\n"),0;
    book_open = 0;
    tell_here(({":%<me.capname>% close%<me.ending_s>% the book."}),
      ({LIV_CAN_SEE}),0,0,this_player(),0,0);
    return 1;
}

status write_cmd(string what) {
    notify_fail("Write what?\n");
    if(!what) return 0;
    if(what != "note") return 0;
    if(writer) return notify_fail(sprintf("You can't, because %s is already \
writing.\n", writer->query_real_name())),0;
    if(!book_open) return notify_fail("Open the book first.\n"),0;
    if(this_player()->query_guild() != GN_PRIEST ||
      ((!this_player()->query_coder_level()) &&
	this_player()->query_guild_object()->query_rank() != IG_RANK))
	return notify_fail("You are too afraid to touch the book.\n"),0;

    write("Write your message and terminate with '**' or '.'. \
To quit type '~q'.\n");
    write("-> ");
    writer = this_player();
    message = ({ sprintf("On %s, %s wrote:",
	get_date(), capitalize(writer->query_real_name())) });
    input_to("write_func");
    return 1;
}

void write_func(string str) {
    switch(str) {
    case "**": case ".":
	if(sizeof(message) == 1) write(
	      "Your message was empty. It wasn't added to the book.\n");
	else {
	    write_file(BOOK_FILE, implode(message, "\n") + "\n\n");
	    write("Ok, your message was added to the book.\n");
	}
	writer = 0;
	break;

    case "~q":
	write("Cancelling your message.\n");
	writer = 0;
	break;

    default:
	message += ({ str });
	write("-> ");
	input_to("write_func");
    }
}

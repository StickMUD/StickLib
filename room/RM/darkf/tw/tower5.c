#include "tower.h"

void
create_room ()
{
    set_short ("Fifth level of the tower");
    set_long (
"The fifth level of the Wizard's Tower seems to be some kind of a library. " +
"Dusty bookshelves carry rows of even dustier books. The lights here are so "+
"dim you wonder how anyone could possibly read a book in a place like this. " +
"The winding stairs lead up and down.");
    SET_LIGHTS();
    add_exit ("up", PATH + "tower6");
    add_exit ("down", PATH + "tower4");

    add_item (({"shelf","bookshelf","shelves","bookshelves"}),
"Dusty bookshelves contain probably the most honourable collection " +
"of books you have ever seen, all in direct (dusty though) rows, in " +
"some strict order. Too bad it's not the ABC-order, anyone but the " +
"owner of this place would probably have hard time finding a book " +
"he or she wants. But maybe you could read a book by random.");
    add_item ("book", "You randomly pick a book and stare at it. Honestly, " +
"you could get much better results by reading it.");
    add_item ("books", "Uh oh, there are many! Why not to try reading one?");
    add_item ("dust", "It almost makes your cough, yack!");
    add_object (PATH + "mirror");
}


void
init_room ()
{
    add_action ("read_", "read");
}



int
read_ (string s)
{
    string *b;
    if (s != "book") {
        notify_fail ("Read what?\n");
        return 0;
    }

    if (this_player()->query_invis())
        say ("A book flows off from the shelf, opens, and after a moment\n" +
        "flows back into the shelf.\n");
    else
        say (this_player()->query_name() + " reads a book from the shelves.\n");
    write ("You pick a book by random and start reading..\n");
    b = explode (read_file (PATH + "library_data"), "**NEXT");
    write (b [random (sizeof(b))] + "\n");
    b = 0;
    return 1;
}



#include <treasure.h>

string looker;
int cracked;

void reset(int x) { if(x) this_object()->reset_object(); else this_object()->create_object(); }


void
create_object ()
{
    looker = cracked = 0;
    set_id ("mirror");
    set_short ("A man-sized mirror");
    set_value (1000);
    set_weight (50); /* this needs a bit work */
}

void
long (string arg)
{
    if (environment (this_player()) == this_object ()) {
        /* so we are in the mirror now */
        return 1;
    }
    if (cracked == 2) {
        /* someone's in there */
    return 1;
    }
    if (cracked) {
        write ("A man-sized, very heavy and expensive-looking mirror. It is completely\n" +
        "broken, you feel sorry for the one with seven years of unluck.\n");
        return;
    }
    if (this_player()->query_invis()) {
        write ("Strange, you look in it but see no reflection from yourself.\n");
        return;
    }
    if (looker == (string)this_player()->query_name()) {
        /* Oh no! */
        write ("ohnou\n");
        return 1;
    }
    looker = (string)this_player()->query_name();
    write (
"A man-sized, very heavy and expensive-looking mirror. You look in it\n" +
"and smile to your mirror image. Somehow the image smiling back just\n" +
"gives you the creeps.\n");
}

#include <cmd.h>

#define ASDOL 36
#define ASN 78
#define ASO 79

nomask int
clone_cmd(string str) {
    object ob;
    string err;
// Next 2 lines added for message handling -- Bull
    string message, tmp;
    int i, ie, j;

    if (!CHECK) return 0;

    if (!str) {
        notify_fail("Clone what object?\n");
	return 0;
    }

    str = (string)TP->absolute_path(str,0);

    if (!str || str[0] != '/') {
        notify_fail("Invalid file.\n");
	return 0;
    }

/*  This makes no sense, it's too easy to make your own
    clone and pass by these restrictions. //Tron

    x = file_size(str + ".c");

    if (x < 0) {
        notify_fail("No such file.\n");
	return 0;
    }
*/

    str = catch(ob = clone_object(str));  // Better error information ++Tron

    if (!ob) {
        notify_fail("Cloning failed: "+str+"\n");
	return 0;
    }

    /* Todo: setting of messages...who cares though */
// How about doing it this way? (And I do care!) -- Bull
    tmp = (string)this_player()->query_env("MCLONE");
    if(tmp) {
	message ="";
      for(i=0, ie=strlen(tmp)-1, j=0 ; i < ie ; i++) {
        if(tmp[i] == ASDOL && tmp[i+1] == ASN) {
          if( i == 0 ) message += (string)this_player()->query_name();
          else message += tmp[j..i-1] + (string)this_player()->query_name();
          i++; j = ++i;
        }
        else if(tmp[i] == ASDOL && tmp[i+1] == ASO) {
          if( i == 0 ) message += (string)ob->query_short();
          else message += tmp[j..i-1] + (string)ob->query_short();
          i++; j = ++i;
        }
      }
      if(i > j) message += tmp[j..i];
    }
    else
    message = (string)TP->query_name() +
              " fetches something from another dimension.";

    say( message+"\n" );

    /* Weight? Coders should perhaps be able to set unlimited
       carrying mode. */
    if (ob->get() && TP->add_weight((int)ob->query_weight()))
        move_object(ob, TP);
    else
        move_object(ob, environment(TP));

    write("Cloned "+object_name(ob)+".\n");
    return 1;
}

#include "../priest.h"

status
advice_cmd(object who, object sym, string str)
{
   int index;
   string *dir, fname;
   object ob;

   if(!str)
      fname = "topics";
   else {
      str = implode(explode(lower_case(str), " "), "_");
      dir = get_dir(PRIEST_SP_DIR);
        if(member(dir, str + ".c") >= 0) {
            ob = find_object(PRIEST_SP_DIR + str);
            if(ob && (ob->query_level() > who->query_level()))
                return notify_fail("You can't use that spell yet.\n"),0;
        }
      dir = get_dir(PRIEST_HELP_DIR);
      fname = str + ".help";
      index = member(dir, fname);
      if(index==-1 && (sym->query_rank() || who->query_coder_level())) {
         fname = str + ".ofc";
         index = member(dir, fname);
      }
      if(index==-1) {
         notify_fail("No help on that spell. Maybe you should check \
the library for more information.\n");
         return 0;
      }
   }
   fname = PRIEST_HELP_DIR + fname;
   who->more(explode(read_file(fname), "\n"));
   return 1;
}

int
clean_up(int arg) {
    destruct(this_object());
    return 0;
}

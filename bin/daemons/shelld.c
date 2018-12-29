
/******************************************
 NAME:    shelld.c

 PURPOSE: StickMUD coder command parsing.
     
 AUTHOR:  Tron
 ******************************************/

private status silent;

nomask static void
snd_error(object me, string s) {

   if(!me || !s) return;

   if(!silent) me->tell_me(s);
}

nomask static string *
get_blocks(string s, string by) {
string *list;
string head, tail, last;
int i, j;
   
   if(!s || !by) return 0;
 
   if(!sizeof(by)) return 0;
   if(!(j = sizeof(s))) return 0;

   while(sscanf(s, "%s"+by+"%s", head, tail) == 2) {
       if(head[sizeof(head)-1] == '\\') {
          if(sizeof(head) > 1) head = head[0..-2];
          else head = "";
          head += by;
          last = (last ? last + head : head);
       }
       else {
          if(last) head = last + head;
          list = (list ? list + ({head}) : ({head}));
          last = 0;
       }
       s = tail;
   }
   list = (list ? list + ({s}) : ({s}));
   return list;
}

nomask static object
get_object(string a, object env, object me) {
int len;
object obj;

   if(!a) return 0;

   len = sizeof(a);

   if(!env) {
      if(a == "me") return me;
      if(a == "here") return environment(me);
   }
   else {
      if(a == "..") return environment(env);
      if(a[0] == '#') {
        object *list;
        int i;

        if(!sscanf(a, "#%d", i)) {
           snd_error(me, "Parse: Illegal argument to '#' operator.");
           return 0;
        }
        if(i < 1) {
           snd_error(me, "Parse: Bad argument to '#' ("+i+")");
           return 0;
        }

        list = all_inventory(env);
        if(!list || sizeof(list) < i) {
           snd_error(me, "Parse: '#' argument out of range.");
           return 0;
        }
        return list[i-1];
      }
   }
   if(a[0] == '\\') sscanf(a, "\\%s", a);
   if(!a) return 0;
       
   if(env)
      if(obj = present(a, env)) return obj;
   else {
      obj = present(a, me);
      if(!obj) obj = present(a, environment(me));
      if(obj) return obj;
   }
   
   obj = find_player(a);
   if(!obj) obj = find_living(a);
   if(obj) return obj;

   if(a[0] == '/' && len > 1) a = a[1..-1];
   else if(a[0] != '/') a = (string)me->absolute_path(a, 0);
   else {
      snd_error(me, "Parse: Invalid file name ("+a+")");
      return 0;
   }
   obj = find_object(a);
   if(!obj) {
     if(catch(call_other(a, "?_?_?")))
       snd_error(me, "Parse: Could not load object.");
     obj = find_object(a);
   }
   return obj;
}

nomask varargs object
shelld_parse_object(string s, object me, status sil) {
string *list;
int i, j;
object ob, env;

   if(!s || !me) return 0;

   silent = sil;

   list = get_blocks(s, ":"); //Object scope 
   if(list && (j = sizeof(list))) {
        while(i < j) {
            if(!(ob = get_object(list[i], env, me))) {
                snd_error(me, "Parse: Failed to find object. ("+list[i]+")");
                return 0;
            }
            env = ob;
            i++;
        }
   }
   else return 0;

   return env;
}



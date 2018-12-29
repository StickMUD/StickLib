#include <config.h>

#define SPC35 "                                   "
#define SPC20 "                    "

status
calls_cmd(string str, object me)
{
   mixed *call_array;
   string s, s1, s2, *res, tmp;
   int i, num;

   if(!(call_array = call_out_info()))
      return notify_fail("#Calls: got 0 from call_out_info().\n"),0;
   i = 0;
   num = sizeof(call_array);
   res = ({"#Calls: Pending call_outs:\nObject"+extract(SPC35, 6)+
	" Function"+extract(SPC20, 8)+" Delay\t Argument(s)"});
   while(i < num) {
      s = "";
      if(!call_array[i]) {
	 if(!str)
	    res += ({"#Calls: item "+i+" lost."});
	 i++;
	 continue;
      }
      if(call_array[i][0]) tmp = object_name(call_array[i][0]);
      else tmp = "NA";
      if(str && sscanf(tmp,"%s"+str+"%s",s1,s2) < 2) {
         i++;
         continue;
      }
	if (!strstr(tmp, PATH_USER_FNAME))
		tmp = "~" + tmp[sizeof(PATH_USER_FNAME)..];
      s += sprintf("%-35s", tmp)+" ";
      s += sprintf("%-20s", (tmp = (string)call_array[i][1]))+" ";
      s += call_array[i][2]+"\t ";
      if(sizeof(call_array[i]) == 4) {
         if(intp(call_array[i][3]) || stringp(call_array[i][3]))
            s += call_array[i][3];
         else if(objectp(call_array[i][3]))
	    s += "(Object)";
         else if(mappingp(call_array[i][3]))
	    s += "(Mapping)";
         else
	    s += "(Array)";
      } else
         s += "0";
      res += ({s});
      i++;
   }
   res += ({"#Calls: total of "+num+" pending call_outs."});
   this_player()->more(res);
   return 1;
}

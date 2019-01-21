/**
 * Module:
 *      /bin/daemons/ansid.c
 *
 * Description:
 *      A standard daemon that knows which ANSI-codes we support. For
 *      efficiency, this daemon is not used for formatting text, it
 *      just provides the mapping used in translation.
 *
 * ChangeLog:
 *
 **/

#include <tell_me.h>

private static mapping ansi_codes = ([
        "normal"      : "0",
        "reset"       : "0",
// Some basic text effects
        "bold"        : "1",
        "dim"         : "2",
        "italic"      : "3",
        "underline"   : "4",
        "flash"       : "5",
        "reverse"     : "7",
// Let's allow using real colour names too...
        "black"       : "30",
        "red"         : "31",
        "green"       : "32",
        "yellow"      : "33",
        "blue"        : "34",
        "magenta"     : "35",
        "cyan"        : "36",
        "white"       : "37",
// Background color codes too...
        "bgblack"     : "40",
        "bgred"       : "41",
        "bggreen"     : "42",
        "bgyellow"    : "43",
        "bgblue"      : "44",
        "bgmagenta"   : "45",
        "bgcyan"      : "46",
        "bgwhite"     : "47",
]);

private static mapping terminal_codes  = ([
// Default fallback, just strip out the code that's not recognized
        0             : "",
// Resetting codes
        "normal"      : "\e[0m",
        "reset"       : "\e[0m",
// Some basic text effects
        "bold"        : "\e[1m",
        "dim"         : "\e[2m",
        "italic"      : "\e[3m",
        "underline"   : "\e[4m",
        "flash"       : "\e[5m",
        "reverse"     : "\e[7m",
// Let's allow using real colour names too...
        "black"       : "\e[30m",
        "red"         : "\e[31m",
        "green"       : "\e[32m",
        "yellow"      : "\e[33m",
        "blue"        : "\e[34m",
        "magenta"     : "\e[35m",
        "cyan"        : "\e[36m",
        "white"       : "\e[37m",
// Background color codes too...
        "bgblack"     : "\e[40m",
        "bgred"       : "\e[41m",
        "bggreen"     : "\e[42m",
        "bgyellow"    : "\e[43m",
        "bgblue"      : "\e[44m",
        "bgmagenta"   : "\e[45m",
        "bgcyan"      : "\e[46m",
        "bgwhite"     : "\e[47m",
]);

static string *colors = ({ "black", "red", "yellow", "green",
    "cyan", "blue", "magenta", "white" });

/* Returns the normal ansi codes for normal sprintf escapes done
 * with for instance the SSF strings.
 */
mapping query_ansi_codes() { return ansi_codes; }

/* Returns full escaped codes for use with something else, like
 * terminal_colour() efun, or sprintfing them directly and passing
 * to binmsg() applied fun in player.
 *
 * These shouldn't be mixed with ACTUAL terminal codes, which contain
 * much more than just text formatting.
 */
mapping query_terminal_codes() { return terminal_codes; }

//
// Formats a series of ansi descriptions into ansi terminal codes.
// For example, format_ansi("bold underline blue",me) returns the equivalent
// ansi terminal code string.
// Called by /bin/pub/_set.c and /bin/pub/_colors

varargs string format_ansi(string t, object me)
{
   if (!stringp(t)) return 0;
   string *x = explode(t, " ");

   int i, j=0;
   while (i < sizeof(x))
   {
       if (!sizeof(x[i])) {
          x[i] = 0;
          continue;   // Skip extra spaces...
       }

       if (!(t = ansi_codes[x[i]])) {
          if (me) me->tell_me(sprintf("Unknown text effect '%s'; skipping that effect.", x[i]));
          x[i] = 0;
       }
       else {
          x[i] = t;
          j++;
       }
       i++;
   }
   return j ? sprintf("%c[%sm", 27, efun::implode(x, ";")) : 0;
}

//
// Unformats an ansi terminal code string into an ansi description.
// Called by /bin/pub/_set.c

string unformat_ansi(string val)
{
  string tmp, tmp2, *arr;

  if (!val || val == "" || sscanf(val[1..], "[%sm", tmp) !=1
         || !tmp || !(arr = explode(tmp, ";")))
      return "normal";

  string ret = 0;
  foreach(tmp2:ansi_codes) {
      if (tmp2=="reset") continue;
      if (member(arr, ansi_codes[tmp2]) != -1) {
          if (!ret) ret = tmp2;
          else if(to_int(ansi_codes[tmp2]) < 30) ret = tmp2+" "+ret;
          else ret += " "+tmp2;
      }
  }
  return ret;
}

//
// Displays str to the specified obs in rainbow colors
// - obs can be either an object or an array of objects
// - flags is checked for TELL_NO_WRAP
//
// Called by /bin/usr/_rainbow.c
//
varargs status rainbow(mixed obs, string str, int flags)
{
   if (!stringp(str)) return 0;

   object *arr;

   if (objectp(obs)) arr = ({ obs });
   else if (pointerp(obs)) arr = obs;
   else return 0;

   foreach(object ob : arr)
   {
     int i = random(sizeof(colors));
     foreach(string c : explode(str,"")) {
        if (!objectp(ob)||!living(ob)) continue;
        ob->set_text_effect(TELL_TYPE_RESERVED, format_ansi(colors[i], ob));
        ob->tell_me(c, 0, TELL_TYPE_RESERVED|TELL_NO_WRAP);
        if (++i>=sizeof(colors)) i=0;
     }
     ob->set_text_effect(TELL_TYPE_RESERVED, format_ansi("normal", ob));
     if (!(flags&TELL_NO_WRAP)) ob->tell_me("");
   }
   return 1;
}

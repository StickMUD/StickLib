/* Enhanced soul programmed by Graah (with native mode typing)
 * Excellent idea comes from Erriond & Plura Inc.
 *
 * Command: feeling [how] [at] [target]
 *
 * Examples:
 *
 * frown   ->   "You frown."
 * smile me   ->   "You smile merrily."
 * cackle it zert   ->   "You cackle ironically at Zert."
 * fart ru at max   ->   "You fart rudely at Max."
 *
 */

#pragma strict_types

inherit "basic/id";

#define Me ((string)owner->query_name())

/* Prototypes */
void init_feels();
int feel_help();

static string *feels,*feel_adj;
static object owner;

string
query_auto_load()
{
string s;
	s = object_name();
	sscanf(s, "%s#", s);
	return s + ":";
}

int get() { return 1; }
int drop() { return 1; }

void
create()
{
    set_name("enchanced feelings");    
    set_id(({"feelings", "enhanced", "funfeel"}));
}

varargs string
query_short(object who) { return "Enhanced feelings"; }

varargs string
query_long(string arg, object who) {
    return "Type \"feelhelp\" (it's LONG!) to know how to use this.";
}

void
init()
{
    if (!owner && environment() && living(environment()))
        owner = environment();
    if (!feels) init_feels();
    if (owner != this_player()) return;
    if ((owner->query_level() >= 20) || (owner->query_coder_level())) {
	add_action("feel_driver", "", 1);
	owner->tell_me("You have enhanced feelings.");
    }
}

int find_how(string arg) {
   int x;

   /* Choose a random one */
   if(arg == "rnd") return random(sizeof(feel_adj))/2 * 2;

   x = member(feel_adj, arg);
   if(x != -1) return x/2 * 2; else return -1;
}

object find_target(string arg) {
   object tmp;

   tmp = present(arg,environment(owner));
   if(!tmp) { notify_fail("No one by that name here.\n"); return 0; }
   if(!living(tmp)) { notify_fail("That's not even alive.\n"); return 0; }
   return tmp;
}

int feel_driver(string arg) {
   int feel,how;
   string who_s,how_s,does;
   object target;

   if(query_verb() == "feelhelp") return feel_help();

   if(!arg) return 0; /* Use the regular soul if no argument */
   if((feel = member(feels, query_verb())) == -1) return 0;

   does = feels[feel];
   if(does == "cry") does = "cries";
   else if(does == "blush") does = "blushes";
   else does += "s";

   if(!arg || arg == "") {
      write("You " + feels[feel] + ".\n");
      say(Me + " " + does + ".\n");
      return 1;
   }
   if(sscanf(arg,"%s at %s",how_s,who_s) != 2 &&
      sscanf(arg,"%s %s",how_s,who_s) != 2) {
      if((how = find_how(arg))!=-1) {
         write("You " + feels[feel] + " " + feel_adj[how] + ".\n");
         say(Me + " " + does + " " + feel_adj[how] + ".\n");
	 return 1;
      }
      if(!(target = find_target(arg))) return 0;
      write("You " + feels[feel] + " at " + target->query_name() + ".\n");
      say(Me + " " + does + " at " +
	 target->query_name() + ".\n",target);
      tell_object(target,Me + " " + does + " at you.\n");
      return 1;
   }
   if((how = find_how(how_s))==-1) {
      notify_fail("Excuse me?\n");
      return 0;
   }
   if(!(target = find_target(who_s))) return 0;
   write("You " + feels[feel] + " " + feel_adj[how] + " at " +
      target->query_name() + ".\n");
   say(Me + " " + does + " " + feel_adj[how] + " at " +
      target->query_name() + ".\n",target);
   tell_object(target,Me + " " + does + " " +
      feel_adj[how] + " at you.\n");
   return 1;
}

void init_feels() {
 feels = ({
   "applaud","blush","bounce","bow","burp","cackle","chuckle",
   "clap","cough","cringe","cry","curtsey","daydream","fart",
   "frown","gasp","giggle","glare","grin","groan","growl",
   "hiccup","hiss","laugh","moan","nod","ponder","pout","puke","purr",
   "scream","shiver","shrug","sigh","sing",
   "smile","smirk","snap","snarl","sneeze","snicker","sniff",
   "snore","snort","spit","stare","struggle","strut","sulk","walk",
   "wave","whine","whistle","wink","yawn"
  });

 feel_adj = ({
   "amusedly","amu","angrily","ang","awfully","awf",
   "in ashtonishment","ash",
   "badly","bad","bitterly","bit","briefly","bri",
   "broadly","bro","brutally","bru",
   "carelessly","ca","carefully","car","casually","cas",
   "charmingly","cha",
   "chaotically","chao",
   "cheerfully","che","childishly","chi","coldly","col",
   "contemptously","con","contendly","cont",
   "in a cool manner","coo","dangerously","dan",
   "deadly","dea","in deep thought","deep",
   "deeply","dee",
   "deliciously","del",
   "demonically","dem",
   "dirtily","dir","dreadfully","dr","dreamily","dre",
   "like a dwarf","dwa",
   "like an elf","elf",
   "evilly","evi","fascinatedly","fas","fatherly","fat",
   "firmly","fir","friendly","fri","furiously","fur",
   "greatly","grea",
   "gleefully","gle","goofily","goo","gracefully","gra",
   "greedily","gre",
   "like a half-orc","hal",
   "happily","hap","hedonistically","hed",
   "holocaustically","hol",
   "like a human","hum",
   "superbly","sup",
   "sodomistically","sod","necrophilically","nec",
   "sadistically","sad","masochistly","mas",
   "helplessly","hel",
   "like a hobbit","hob",
   "hopefully","hop","hornily","hor","hungrily","hun",
   "impatiently","imp","importantly","impr",
   "impotently","impo",
   "inanely","ina","insanely","ins",
   "innocently","inn","inwardly","inw",
   "ironically","iro","knowingly","kno","longingly","lon",
   "lawfully","law",
   "logically","log",
   "like only a Lady could","lady",
   "like only a Lord could","lord",
   "loudly","lou",
   "lovingly","lov","madly","mad","merrily","mer",
   "motherly","mot","naturally","nat","naughtily","nau",
   "nicely","nic","noisily","noi","officially","off",
   "like an orc","orc",
   "patiently","pat",
   "peacefully","pea","pervertly","per","pervectly","perv",
   "politely","pol",
   "proudly","pro","questioningly","que","quickly","qui",
   "quietly","quie","rapidly","rap","respectfully","res",
   "rudely","rud","sadly","sad","seductively","sed",
   "seriously","ser","sexily","sex",
   "shamelessly","sha","silly","sil",
   "slowly","slo","softly","sof","solemnly","sol",
   "stupidly","stu","suggestively","sug",
   "temporarily","tem","tenderly","ten",
   "in terror","ter",
   "thirsty","thi",
   "tightly","tig","weakly","wea",
   "wholeheartedly","who",
   "widely","wid",
   "wickedly","wic","wormily","worm",
   "in that lovely way you just can't resist","irr"
  });
}

int feel_help() {
   int i,s;
   string tmp;

   write("Usage: <feeling>\n       <feeling> <how>\n"+
      "       <feeling> <how> [at] <target>\n" +
      "\n(if <how> is \"rnd\" you'll get a random one.)\n");
   write("\n<feeling>s available with this item:\n\n");

   s = 0;
   for(i=0;i<sizeof(feels);i++) {
      s += strlen(feels[i]) + 1; if(s > 70) { s = 0; write("\n"); }
      write(feels[i] + " ");
   }

   write("\n\n<how>s available, with abbreviations:\n\n");

   i = 0; s = 0;
   while(i < (sizeof(feel_adj)-1)) {
      tmp = (feel_adj[i] + "....................")[0..16] +
        "." + feel_adj[i+1] + "         ";
      tmp = tmp[0..21];
      write("  " + tmp);
      s++; if(s > 2) { write("\n"); s = 0; }
      i += 2;
   }
   write("\n");
   return 1;
}

void
goppala()
{
  int i;

for (i = 0; i < sizeof(feel_adj); i+=2)
write("\"" + feel_adj[i+1] + "\", \"" + feel_adj[i] + "\",\n");
}

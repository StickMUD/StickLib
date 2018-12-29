/* Standard smokable object coded by Mclaud for StickMUD in January 1996

   Max is the amount of tobacco in the pouch.
   Fuel is the amount that is taken when the pipe is filled.
   Strength is the strength of the tobacco.
   Brand ofcorse is the brand of the tobacco.

   Example:

   ob = clone_object("/obj/smoke");
   ob->set_brand("dirty socks");
   ob->set_fuel(8); <- How many times can we fill the pipe.
   ob->set_strength(2); <- Don't make'em too strong!!
   ob->set_value(10) <- this means that the pouch costs (10 * fuel) coins...
                        
   If you have any problems using this... Mail me :)

   Mclaud. */


  /* need this... --colt */
inherit "/basic/id";
inherit "/basic/object/dimensions";
#define CLEAN_TIME 120
#define RESET_USED   1
#define INIT_USED    2
#define MAX         12

int fuel, value, strength, max;

string brand;

private static int functions_used;

/* Query Functions */

int query_strength()    { return strength; }
int query_fuel()        { return fuel; }
string query_brand()    { return brand; }
status is_smoke()       { return 1; }
int query_value() {
        if (fuel > 0) return ((value * fuel * 2) / 3);
        return 0;
        }
string query_name() {
    if (brand) return brand;
    return "tobacco";
    }
    
/* Set Functions */

void set_strength(int s) { strength = s; }
void set_fuel(int x) { if (x <= MAX) fuel = max = x; else fuel = max = MAX; }
void set_value(int v) { if (v > 0 && v < 100) value = v; else value = 100; }
void set_brand(string b) { brand = b; }


varargs status
id(string s, object who)
{
    return s == "pouch" || ::id(s) || s == "tobacco pouch" || (fuel &&
          (s == "tobacco"));
    }

void
init()
{
        if (functions_used & INIT_USED) this_object() -> init_smoke();
        }

void
reset(int arg)
{
    if (arg) {
        if (functions_used & RESET_USED) this_object() -> reset_smoke();
        }
    else {
        fuel = max = 1;
        set_weight(1);
         if (function_exists("reset_smoke")) functions_used |= RESET_USED;
         if (function_exists("init_smoke")) functions_used |= INIT_USED;
        this_object() -> create_smoke();
        }
    }


varargs string
query_long(string s, object who)
{
int d;
mixed txt;
string n;

    txt = ::query_long(s, who);

    if (brand) n = brand;
    else n = query_name();

    if (fuel > 0) {
        if (fuel >= max) txt += "\nIt is full of " + n;
        else {
            d = ((100 * fuel) / max);
            if (d > 80) txt += "\nIt is almost full of " + n;
            else if (d > 60) txt += "\nIt is more than a half filled with " + n;
            else if (d > 40) txt += "\nIt is about half full of " + n;
            else if (d > 20) txt += "\nIt is less than a half filled with " + n;
            else txt += "\nThere's only a small amount of " + n + " left";
            }
        txt += ".";
	}
    else txt += "\nIt's empty.";
    return txt;
    }

varargs string                           
query_short(string s, object who)
{
    int d;
    mixed txt;

    txt = "";

    if (fuel > 0) {
        if (fuel >= max) txt = "Full";
        else {
            d = ((100 * fuel) / max);
            if (d > 80) txt += "Allmost full";
            else if (d > 60) txt += "More that half filled";
            else if (d > 40) txt = "Half filled";
            else if (d > 20) txt = "Less than half filled";
            else txt = "Allmost empty";
            }
        txt += " tobacco pouch";
	}
    else txt += "Empty tobacco pouch.";
    return txt;
    }

varargs status
took_some()
{
    if (!fuel || !max) return 0;
    fuel --;
   /* should be == not =  --Colt */
   if (fuel == 0) call_out("clean", CLEAN_TIME);
    return 1;
    }

status get() { return 1; }

void clean() {
    if (environment())
        environment()->tell_me("Empty tobacco pouch falls in pieces.");
    destruct(this_object());
}

/*
 * Clothing - a mapping of data. For easier saveability.
 * They can be made objects when stripped off. But they won't get
 * saved if not "worn".
 *
 */
#include <daemons.h>

#ifndef _CLOTH_DEFS_H
#include <cloth_defs.h>
#endif

#define TOBJ this_object()
#define TNAME (string)TOBJ->query_name()

mapping clothes;

static void init_clothes() {
    clothes = ([ ]);
}

mapping
query_clothes()
{
    if (!clothes) init_clothes();

    return clothes;
}

void
set_clothes(mapping c)
{
    if (!clothes) init_clothes();

    if (mappingp(c)) clothes = c;
    /* Added this so we won't set other values than mappings. */
}

/*
 * Function name: wear_clothes
 * Description	: Wear a piece of clothing and write message about it.
 * Arguments	: 
 */
int
wear_clothes(string cn, string s, int value, int weight)
{
    if (!clothes) init_clothes();

    if (clothes[cn]) {
	TOBJ->tell_me("You are already wearing " + clothes[cn][0] + ".");
	return 0;
    }

    TOBJ->tell_me("You wear " + s + ".");
    environment(TOBJ) -> tell_here(TNAME +
      " wears " + s + ".", TOBJ);

    clothes[cn] = ({ s, value, weight });

    return 1;
}

int
strip_clothes(string cn)
{
    object ob;
    string tmp;

    if (!clothes) init_clothes();

    tmp = lower_case(cn);
    if ((tmp == "verkkopaita") || (tmp == "paita")) tmp = "shirt";
    else if ((tmp == "verkkarit") || (tmp == "suomi-verkkarit")) tmp = "dress";

    if (!clothes[cn])
    {
	if (!clothes[tmp])
	{
	    notify_fail("You aren't wearing any " + cn + ".\n");
	    return 0;
	}

	cn = tmp;
    }

    ob = clone_object(CLOTHING_FILE);

    if (!ob) {
	notify_fail("You can't, it's cursed!\n");
	return 0;
    }

    TOBJ->tell_me("You take off " + clothes[cn][0] + ".");
    environment(TOBJ) -> tell_here(TNAME +
      " takes off " + clothes[cn][0] + ".", TOBJ);

    ob->set_name(cn);
    ob->set_short(clothes[cn][0]);
    ob->set_cloth_type(cn);
    ob->set_long("It's an used " + clothes[cn][0] + ".");
    ob->set_id(({ "cloth", cn, explode(clothes[cn][0], " ") }));
    ob->set_value(clothes[cn][1]);
    ob->set_weight(clothes[cn][2]);

    // Kludge these buggy clothes
    if (clothes[cn][0] == "Verkkopaita")
	ob->add_id(({ "verkkopaita", "paita" }));
    else if (clothes[cn][0] == "Suomi-verkkarit")
	ob->add_id(({ "suomi-verkkarit", "verkkarit" }));

    if (!TOBJ->add_weight(clothes[cn][2])) {
	TOBJ->tell_me("Oops, it's too heavy. You have to drop it.");
	environment(TOBJ) -> tell_here(TNAME +
	  " drops " + cn + ".\n", TOBJ);
	move_object(ob, environment(TOBJ));
    } else move_object(ob, TOBJ);

    clothes = m_delete(clothes, cn);

    return 1;
}

void
describe_clothing()
{
    int i, s;
    string *ind;

    if (!clothes) return;

    ind = m_indices(clothes);
    s = sizeof(ind);

    if (s < 1) {
	//        write("Nothing.\n");
	return;
    }

    for (i = 0; i < s; i++)
	write(capitalize(clothes[ind[i]][0]) + " (worn).\n");
}

string
query_clothing_text()
{
    int i, s;
    mixed c;
    string txt;
    if (!clothes ||
      !(c = m_values(clothes)) ||
      !(s = sizeof(c))
      //           ) return "Nothing.";
    ) return "";

    for (i = 0, txt = ""; i < s; i++)
	txt += c [i] [0] + " (worn).\n";
    if ((i = sizeof(txt)) > 1)
	return txt[0..i-2];
}

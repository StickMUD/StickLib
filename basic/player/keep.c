// Module to be used by "keep"-command. Included by player.c
//
// Version 1.1 31-aug-95 by Doomdark
//
// Rather clumsy way to do it... But has to do for now.

static object *keepItems;

status
add_keep_item(mixed ob, status is_add)
{
    if (!ob && !is_add) {
	keepItems = ({ });
	return 1;
    }
    if (!keepItems)
	keepItems = ({ });
    else {
	keepItems -= ({ 0 });
	if (is_add && member(keepItems, ob) >= 0)
	    return 0;
    }
    if (is_add) {
	if (objectp(ob))
	    keepItems += ({ ob });
	else if (pointerp(ob)) {
	    keepItems -= ob;
	    keepItems += ob;
	} else return 0;
    } else {
	int i;
	i = sizeof(keepItems);
	if (objectp(ob))
	    keepItems -= ({ ob });
	else if (pointerp(ob))
	    keepItems -= ob;
	else return 0;
	if (i == sizeof(keepItems))
	    return 0;
    }
    return 1;
}

varargs mixed
query_keep_items(object x)
{
    if (!keepItems)
	keepItems = ({ });

    if (objectp(x))
	return (member(keepItems, x) >= 0);

    return keepItems;
}

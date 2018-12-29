// --Val (94-8)
// Get target for coders object handling commands.
// Ex. "val:env:i1"	- find first object in Val's environment
//     "item#123"	- find object "/current_directory/item#123"

varargs object
get_object( string str, object player )
{
    string *obpath;
    object what;
    mixed tmp;

    if (!str) return 0;
    if (!player) player = this_player();

    if (sizeof(obpath = explode(str, ":")) > 1) {
	str = obpath[0];
    } else {
	obpath = ({ str });
    }
	
    // Check the first argument
    do {
	if (player) {
	    if (str == "me") { what = player; break; }
	    if (what = present(str, player)) break;
	    if (what = environment(player)) {
		if (str == "here" || str == "env") break;
		if (what = present(str, what)) break;
	    }
	}

//	if (what = present(str, previous_object())) break;
 
	if (what = find_player(str)) break;
	if (what = find_living(str)) break;

	if (player) {
	    str = (string)player->absolute_path(str);
	    if (!str) return 0;
	}

	if (what = find_object(str)) break;
	if (!sscanf(str, "%s#", str)
	    && (file_size(str) >= 0 || file_size(str + ".c") >= 0))
	{
	    catch( call_other( str, "??" ) );
	    what = find_object( str );
	}
    } while (0);

    // Check the "atribute" arguments
    while (what && sizeof(obpath) > 1) {
	int i;

	obpath = obpath[1..sizeof(obpath)-1];
	str = obpath[0];
	if (str == "e" || str == "env") {
	    what = environment(what);
	    continue;
	}
	if ((sscanf(str, "i%d", i) || sscanf(str, "%d", i))
	    && i >= 1 && sizeof(tmp = all_inventory(what)) >= i)
	{
	    what = tmp[i-1];
	    continue;
	}
	what = present(str, what);
    }

    return what;
}

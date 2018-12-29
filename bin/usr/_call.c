// /bin/usr/_call.c
 
#include <cmd.h>

mixed handle_arg(mixed arg);
mapping build_mapping(mixed arg);
mixed build_number(mixed arg);
 
nomask static int
usage()
{
    notify_fail("Usage: call <object>;<function>;[arg1];[arg2]...\n");
    return 0;
}
 
status call_cmd(string arg)
{
    object ob, o2;
    mixed *a, ret;
    string err;
    float f;
    int i;
 
    if (!CHECK  ||  !TP->query_coder_level())
        return 0;
 
    if (!arg || !stringp(arg)) return usage();
 
    a = explode(arg, ";");
 
    if (sizeof(a) < 2) return usage();
 
    if (!(ob = get_object(a[0])))
    {
        if (!(ob = TP->locate_object(a[0], 1))  &&
            !(ob = find_player(a[0])))
        {
            TP->tell_me("Can't find object \"" + a[0] + "\"!");
            return 1;
        }
    }
 
    if (!function_exists(a[1], ob))
    {
        TP->tell_me(sprintf("No function \"%s\" in %O.", a[1], ob));
        return 1;
    }
 
    // TODO: Check arguments and make them use correct type.
    // So that we can use:
    // call blahblah;set_all_props;(["name":"blahblah","weight":20000])
    // for example.
 
    // Can this be done with some clever closure or something...?
    // Just handles integer and float numbers at the moment.
    for (i = sizeof(a) - 1; i >= 2; i--)
    {
        if (a[i] == "0")
            a[i] = 0;
        else if (a[i] == "0.0")
            a[i] = 0.0;
        else if ((sscanf(a[i], "%d", ret) == 1)  &&
            (ret != 0))
            a[i] = ret;
        else if ((f = to_float(a[i]))  &&
            (f != 0.0))
        {
            a[i] = f;
            TP->tell_me("Yep, it thinks there was a float...");
            printf("%O\n", f);
        }
        else if ((a[i][0..3] == "OBJ(") && (a[i][<1] == ')'))
        {
            o2 = find_object(a[i][4..<2]);
            if (!o2)
                TP->tell_me("Can't find object " + a[i] + ".");
            a[i] = o2;
        }
    }
 
    err = catch(ret = apply(#'call_other, ob, a[1..<1]));
 
    if (err)
        TP->tell_me("Error: " + err);
    else
        TP->tell_me(sprintf("Returned:\n%O", ret));
 
    return 1;
}

//
//me func;15;"15";15.0;({args});([ args : args, args : args ]);
//
// This function does not do anything to handle white spaces.
//
// This function cannot deal with a colon inside a mapping that
// does not directly relate to the mapping.  That is, no mappings
// within mappings, and no strings in mappings with a colon.
//
mixed handle_arg(mixed arg)
{
    // arg is an array of args.
    // Each arg can be int, float, string, array, or mapping.
    // when one arg is identified, it is exploded to an array and
    // sent to this function.
    int i;
    for(i=0 ; i < sizeof(arg) ; i++)
    {
        // first, let's see if we can instantly identify what type of
        // argument this is -- arrays and mappings are easy -- So are
        // explicit strings
        if(arg[i][0..1] == "({")
        {
            // array
            if(arg[i][<2..<1]  == "})")
            {
                // deffinatly an array
                // explode the array, and process each arg
                arg[i] = explode(arg[i][2..<3], ",");
                arg[i] = handle_arg(arg[i]);
                continue;
            }
            else
            {
                // Hmm guess it was a string -- let's make it explicit
                arg[i] = "\""+arg[i]+"\"";
            }
        }
        if(arg[i][0..1] == "([")
        {
            // mapping
            if(arg[i][<2..<1] == "])")
            {
                // For sure a mapping
                // this will take another loop to build the mapping
                // properly
                arg[i] = build_mapping(arg[i]);
                continue;
            }
            else
            {
                // Hmm guess it was a string -- let's make it explicit
                arg[i] = "\""+arg[i]+"\"";
            }
        }
        if(arg[i][0] == '"')
        {
            // string
            if(arg[i][<1] == '"')
                arg[i] = sprintf("%s", arg[i][1..<2]);
            // else it's fine already
        }
        else
        {
            // We have an int or float, or string
            // build number will figure it out, and return the
            // correct type
            arg[i] = build_number(arg[i]);
        }
    }
    // Now that we've modified all the args, return them
    return arg;
}
 
mapping build_mapping(mixed arg)
{
    // the arg is a mapping string, so we need to check each arg type
    // and build up a mapping;
    mapping built;
    int i;
 
    built = ([ ]);
 
    arg = explode(arg[2..<3], ",");
    if(!arg)
        return built;
 
    for(i=0 ; i < sizeof(arg) ; i++)
    {
        if(!arg[i]  ||  arg[i] == "")
            continue;
 
        arg[i] = explode(arg[i], ":");
        if(sizeof(arg[i])  != 2)
            // This means it wasn't like ':' ?
            // I guess this means we can't have a ':' in a mapping
            // ie a : in a string, or a mapping in a mapping.
            continue;
        arg[i][0] = handle_arg( ({ arg[i][0] }) );
        arg[i][1] = handle_arg( ({ arg[i][1] }) );
        built[arg[i][0][0]] = arg[i][1][0];
    }
 
    return built;
}
 
mixed build_number(mixed arg)
{
    int i, period;
 
    // What we need to do here, is figure out if it is all numbers
    // or all numbers with a single '.' in it.
 
    period = 0;
    for(i=0 ; i < sizeof(arg) ; i++)
    {
        if(arg[i] < '0'  ||  arg[i] > '9')
        {
            if(arg[i] == '.'  &&  !period)
                period++;
            else
            {
                // There's already one period in here, or the character
                // is not a period, and therefore this is a string!
                period = 2;
                break;
            }
        }
    }
    if(!period)
        arg = to_int(arg);
    else if(period == 1)
        arg = to_float(arg);
//    else
        // everthing is fine
 
    return arg;
}

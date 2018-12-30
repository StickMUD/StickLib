/* Amylaar simul_efuns */

/* Most of this is unneccessary? */

#pragma strong_types
#pragma save_types

#define MASTER "secure/master"

#include "/sys/interactive_info.h"
#include "/sys/object_info.h"
#include "/sys/wizlist.h"

#define MAX_LOG_SIZE 50000

#define BACKBONE_WIZINFO_SIZE 5

#define LIVING_NAME 3
#define NAME_LIVING 4

static int RebootTime;

#include "/secure/simul_efun/cat.c"
#include "/secure/simul_efun/tail.c"
#include "/secure/simul_efun/set_connection_charset.c"
#include "/secure/simul_efun/set_combine_charset.c"
#include "/secure/simul_efun/log_file.c"
#include "/secure/simul_efun/file_lines.c"
#include "/secure/simul_efun/implode.c"
#include "/secure/simul_efun/implode3.c"
#include "/secure/simul_efun/shout.c"
#include "/secure/simul_efun/present4.c"
#include "/secure/simul_efun/mkdir.c"

void show_fortune()
{
  write("Fortune is not available at the moment.\n");
}

void localcmd() {
    string *verbs;
    int i,j;

    verbs = query_actions(this_player());
    for (i=0, j = sizeof(verbs); --j >= 0; i++) {
	write(verbs[i]+" ");
    }
    write("\n");
}

string version() {
    return __VERSION__;
}

#if 0
// Nope. Let's let it be like it should.
mapping m_delete(mapping m, mixed key) {
    return efun::m_delete(copy(m), key);
}
#endif

/* Hmmh. An old outdated simulated efun. Is there any use
 * for it?
 */
void add_worth(int value, object ob) {
    mixed old;
#ifdef COMPAT_FLAG
    switch (explode(object_name(previous_object()), "/")[0]) {
#else
    switch (explode(object_name(previous_object()), "/")[1]) {
#endif
	case "bin":
	case "lib":
      case "std":
      case "room":
	break;
    }
    if (!ob) {
	if ( !(ob = previous_object(1)) )
	    return;
    }
    if (intp(old = get_extra_wizinfo(ob)))
        set_extra_wizinfo(ob, old + value);
}

#if 0
void wizlist(string name) {
    int i, pos, total_cmd;
    int *cmds;
    mixed *a;
    mixed *b;

    if (!name) {
        name = this_player()->query_real_name();
        if (!name)
            return;
    }
    a = transpose_array(wizlist_info());
    cmds = a[WL_COMMANDS];
    a[WL_COMMANDS] = a[0];
    a[0] = cmds;
    a = order_alist(a);
    cmds = a[0];
    a[0] = a[WL_COMMANDS];
    a[WL_COMMANDS] = cmds;
    if ((pos = member(a[WL_NAME], name)) < 0 && name != "ALL")
        return;
    b = allocate(sizeof(cmds));
    for (i = sizeof(cmds); i;) {
        b[<i] = i;
        total_cmd += cmds[--i];
    }
    a = transpose_array(a + ({b}) );
    if (name != "ALL") {
        if (pos + 18 < sizeof(cmds)) {
            a = a[pos-2..pos+2]+a[<15..];
        } else if (pos < sizeof(cmds) - 13) {
            a = a[pos-2..];
        } else {
            a = a[<15..];
        }
    }
    write("\nWizard top score list\n\n");
    if (total_cmd == 0)
        total_cmd = 1;
    for (i = sizeof(a); i; ) {
        b = a[<i--];
        printf("%-15s %5d %2d%% (%d)\t[%4dk,%5d] %6d %d\n",
          b[WL_NAME], b[WL_COMMANDS], b[WL_COMMANDS] * 100 / total_cmd, b[<1],
          b[WL_EVAL_COST] / 1000,
          b[WL_HEART_BEATS], b[WL_EXTRA], b[WL_ARRAY_TOTAL]
        );
    }
    printf("\nTotal         %7d     (%d)\n\n", total_cmd, sizeof(cmds));
}
#else
varargs void wizlist(string name)
{
    int i, pos, total_cmd;
    int *cmds;
    mixed *a;
    mixed *b;

    if (!name) {
        name = this_player()->query_real_name();
        if (!name)
        {
            write("Need to provide a name or 'ALL' to the wizlist function.\n");
            return;
        }
    }
    a = transpose_array(wizlist_info());
    cmds = a[WL_COMMANDS];
        a[WL_COMMANDS] = a[0];
    a[0] = cmds;

    a = unmkmapping(apply(#'mkmapping, a));
    cmds = a[0];
    a[0] = a[WL_COMMANDS];
    a[WL_COMMANDS] = cmds;

    if ((pos = member(a[WL_NAME], name)) < 0 && name != "ALL")
    {
        write("No wizlist info for '"+name+"' found.\n");
        return;
    }
    b = allocate(sizeof(cmds));
    for (i = sizeof(cmds); i;) {
        b[<i] = i;
        total_cmd += cmds[--i];
    }
    a = transpose_array(a + ({b}) );
    if (name != "ALL") {
        if (pos + 18 < sizeof(cmds)) {
            a = a[pos-2..pos+2]+a[<15..];
        } else if (pos < sizeof(cmds) - 13) {
            a = a[pos-2..];
        } else {
            a = a[<15..];
        }
    }
    write("\nWizard top score list\n\n");
    if (total_cmd == 0)
        total_cmd = 1;
    for (i = sizeof(a); i; ) {
        b = a[<i--];
        if (b[WL_GIGACOST] > 1000)
            printf("%-15s %5d %2d%% (%d)\t[%d%4dk,%5d] %6d %d\n",
              b[WL_NAME], b[WL_COMMANDS],
              b[WL_COMMANDS] * 100 / total_cmd, b[<1],
              b[WL_GIGACOST] / 1000,
              b[WL_COST] / 1000 + (b[WL_GIGACOST] % 1000) * 1000000000,
              b[WL_HEART_BEATS], b[WL_EXTRA], b[WL_ARRAY_TOTAL]
            );
    else
            printf("%-15s %5d %2d%% (%d)\t[%4dk,%5d] %6d %d\n",
              b[WL_NAME], b[WL_COMMANDS],
              b[WL_COMMANDS] * 100 / total_cmd, b[<1],
              b[WL_COST] / 1000 + (b[WL_GIGACOST] % 1000) * 1000000000,
              b[WL_HEART_BEATS], b[WL_EXTRA], b[WL_ARRAY_TOTAL]
            );
    }
    printf("\nTotal         %7d     (%d)\n\n", total_cmd, sizeof(cmds));
}
#endif
/*
 * Function name: exclude_array
 * Description:   Deletes a section of an array
 * Arguments:     arr: The array
 *		  from: Index from which to delete elements
 *		  to: Last index to be deleted.
 * Returns:
 */
public mixed *
exclude_array(mixed *arr, int from, int to)
{
    return arr[0..from-1] + arr[to+1..sizeof(arr)];
}

/*
 * Function name: exclude_element
 * Description:   deletes an element from an array
 * Arguments:     arr: the array
 *                from: index of the element to remove from the array
 * Returns:       first element of the array, or -1 if failed.
 * Warning:       elements in the array will have been reshuffled somewhat.
 */
public mixed * exclude_element (mixed *arr, int index) {
        arr[index] = arr[0];
        // return slice_array (arr, 1, sizeof (arr));
        return arr[1..sizeof (arr)];
}

/*************************************************************************/

/* Efuns: break_string() and update_actions()
*/

static int gCol;

/*
 * Function name: break_string
 * Description:   Breaks a continous string without newlines into a string
 *		  with newlines inserted at regular intervalls replacing spaces
 *		  Each newline separeted string can be indented with a given
 *		  number of spaces.
 * Arguments:     str: Original message
 *		  width: The total maximum width of each line.
 *		  indent: (optional) How many spaces to indent with.
 * Returns:       A string with newline separated strings
 */
public varargs string
break_string(string str, int width, int indent)
{
    mixed *brkarg;
    string indstr;

    if (indent) {
	indstr = "                                                                                ";
	indstr = indstr[0..indent-1];
    }
    else
	indstr = "";
    gCol = indent;
    brkarg = ({ width, indstr });
  
    return indstr + 
	implode(map(explode(str+" "," "),"brk",this_object(),brkarg)," ");
}

static string brk(string word, mixed *width_indstr)
{
    int width;
    string indstr;

    width = width_indstr[0];
    indstr = width_indstr[1];
    if ((sizeof(word)+gCol) > width) {
	gCol = sizeof(indstr) + sizeof(word) + 1;
	return "\n" + indstr + word;
    }
    else {
	gCol += sizeof(word) + 1;
	return word;
    }
}

/*
 * Function name:   update_actions
 * Description:     Updates this_objects actions in all living objects
 */
public void
update_actions()
{
  int objs;

  if (environment(previous_object()))
    previous_object()->move(environment(previous_object()),1);
  filter(all_inventory(previous_object()),"move_inv",previous_object());
}

static void move_inv(object ob) { if (living(ob)) ob->move(previous_object(),1); }

/*************************************************************************/


/* Functions below are useful but should probably not be made into efuns
*/

#if 0
string creator(object ob) { return (string)SECURITY->creator_object(ob); }
string domain(object ob) { return (string)SECURITY->domain_object(ob); }
#endif


static void dump_elem(mixed sak, string tab);

/*
 * Function name: dump_array
 * Description:   Dumps a variable with write() for debugging purposes.
 * Arguments:     a: Anything including an array
 */
public varargs void 
dump_array(mixed *a, string tab)
{
  int n,m;

  if (!tab) tab = "";
  if (!pointerp(a)) {
    write(a); return;
  }
  else write("\n");
  m=sizeof(a);
  n=0;
  while (n<m) {
    write(tab+"["+n+"] = "); dump_elem(a[n],tab); n+=1;
  }
}

status is_clone(mixed ob) {
    return objectp(ob) && member(object_name(ob), '#') >= 0;
}

static void dump_elem(mixed sak, string tab)
{
  if (pointerp(sak)) {
    write("Array: \n"); dump_array(sak,tab+"   ");
  }
  else write(sak);
  write("\n");
}

int file_time(string path)
{
    mixed *v;

    set_this_object(previous_object());
    if (sizeof(v=get_dir(path,4))) return v[0];
    return 0;
}

#ifdef COMPAT_FLAG
mixed snoop(mixed snoopee) {
    int result;

    if (snoopee && interactive_info(snoopee, II_SNOOP_NEXT)) {
        write("Busy.\n");
        return 0;
    }
    result = snoopee ? efun::snoop(this_player(), snoopee)
                     : efun::snoop(this_player());
    switch (result) {
	case -1:
	    write("Busy.\n");
	case  0:
	    write("Failed.\n");
	case  1:
	    write("Ok.\n");
    }
    if (result > 0) return snoopee;
}
#endif

string query_host_name() {
    return __HOST_NAME__;
}

#ifndef _MASTER_C
nomask void efun308() {}
nomask void set_this_player(object x) { }
#endif

mapping living_name_m, name_living_m;

#ifndef _MASTER_C
void start_simul_efun() {
    mixed *info;

    if ( !(info = get_extra_wizinfo(0)) )
	set_extra_wizinfo(0, info = allocate(BACKBONE_WIZINFO_SIZE));
    if (!(living_name_m = info[LIVING_NAME]))
	living_name_m = info[LIVING_NAME] = m_allocate(0, 1);
    if (!(name_living_m = info[NAME_LIVING]))
	name_living_m = info[NAME_LIVING] = m_allocate(0, 1);
    if (find_call_out("clean_simul_efun") < 0)
	call_out("clean_simul_efun", 1800);
    this_object()->start_simul_efun_dr();
}

static void clean_name_living_m(string *keys, int left) {
    int i, j;
    mixed a;

    if (left) {
	if (pointerp(a = name_living_m[keys[--left]]) && member(a, 0)>= 0) {
	    i = sizeof(a);
	    do {
		if (a[--i])
		    a[<++j] = a[i];
	    } while (i);
	    name_living_m[keys[left]] = a = j > 1 ? a[<j..] : a[<1];
	}
	if (!a)
	    efun::m_delete(name_living_m, keys[left]);
	call_out("clean_name_living_m", 1, keys, left);
    }
}

static void clean_simul_efun() {
    /* There might be destructed objects as keys. */
    m_indices(living_name_m);
    remove_call_out("clean_simul_efun");
    if (find_call_out("clean_name_living_m") < 0) {
	call_out(
	  "clean_name_living_m",
	  1,
	  m_indices(name_living_m),
	  sizeof(name_living_m)
	);
    }
    call_out("clean_simul_efun", 3600);
}

/* disable symbol_function('set_living_name, SIMUL_EFUN_OBJECT) */
#if 0
protected void set_living_name(string name) {
    string old;
    mixed a;
    int i;

    if (old = living_name_m[previous_object()]) {
	if (pointerp(a = name_living_m[old])) {
	    a[member(a, previous_object())] = 0;
	} else {
	    efun::m_delete(name_living_m, old);
	}
    }
    living_name_m[previous_object()] = name;
    if (a = name_living_m[name]) {
	if (!pointerp(a)) {
	    name_living_m[name] = ({a, previous_object()});
	    return;
	}
	/* Try to reallocate entry from destructed object */
	if ((i = member(a, 0)) >= 0) {
	    a[i] = previous_object();
	    return;
	}
	name_living_m[name] = a + ({previous_object()});
	return;
    }
    name_living_m[name] = previous_object();
}
#else
void set_living_name(string name)
{
    string old;
    mixed a;
    int i;

    if (old = living_name_m[previous_object()]) {
        if (pointerp(a = name_living_m[old])) {
            a[member(a, previous_object())] = 0;
        } else {
            efun::m_delete(name_living_m, old);
        }
    }
    living_name_m[previous_object()] = name;
    if (a = name_living_m[name]) {
        if (!pointerp(a)) {
            name_living_m[name] = ({a, previous_object()});
                        return;
        }
    /* Try to reallocate entry from destructed object */
        if ((i = member(a, 0)) >= 0) {
            a[i] = previous_object();
            return;
        }
    name_living_m[name] = a + ({previous_object()});
        return;
    }
    name_living_m[name] = previous_object();
}
#endif

object find_living(string name) {
    mixed *a, r;
    int i;

    if (pointerp(r = name_living_m[name])) {
	if ( !living(r = (a = r)[0])) {
	    for (i = sizeof(a); --i;) {
		if (living(a[<i])) {
		    r = a[<i];
		    a[<i] = a[0];
		    return a[0] = r;
		}
	    }
	}
	return r;
    }
    return living(r) && r;
}

object find_player(string name) {
    mixed *a, r;
    int i;

    if (pointerp(r = name_living_m[name])) {
	if ( !(r = (a = r)[0]) || !object_info(r, OI_ONCE_INTERACTIVE)) {
	    for (i = sizeof(a); --i;) {
		if (a[<i] && object_info(a[<i], OI_ONCE_INTERACTIVE)) {
		    r = a[<i];
		    a[<i] = a[0];
		    return a[0] = r;
		}
	    }
	    return 0;
	}
	return r;
    }
    return r && object_info(r, OI_ONCE_INTERACTIVE) && r;
}
#endif

void
cindent(string name)
{
  return;
#if 0
    string valid;

    valid = funcall(
	bind_lambda(#'call_other, previous_object()),
	__MASTER_OBJECT__,
	"valid_write",
	name,
#if __EFUN_DEFINED__(geteuid)
	geteuid(previous_object()),
#else
	0,
#endif
	"cindent", previous_object()
    );
    if (valid)
    send_erq(ERQ_FORK, "indent_wrapper " + (stringp(valid) ? valid : name));
#endif
}

// Added 30-nov-95 by Doomdark:
int
query_uptime()
{
	if (!RebootTime) RebootTime = (int) MASTER->query_reboot_time();
	return time() - RebootTime;
}

int
set_hide(object x)
{
  return 0;
}

int
clear_hide(object x)
{
  return 0;
}

int
query_next_reset(object z)
{
	return random(1800);	// Ain't we evil!!! -+ Doomdark +-
}
/* TODO as efuns! */
string upper_case(string str) { return str; }

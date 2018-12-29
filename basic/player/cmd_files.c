/****************************************************************
*                                                               *
* Module:                                                       *
*       /basic/player/cmd_files.c                               *
* Description:                                                  *
*       Part of the standard player object (inherited by        *
*       player.c that implements operations related to file     *
*       handling; access control and so on. This is a security  *
*       sensitive module, so take care not to open up any new   *
*       security holes... :-)                                   *
*       Also, this module currently contains some file handling *
*       commands. They shouldn't probably be here, but due to   *
*       some security concerns the are...                       *
* Version:                                                      *
*       1.0a (for StickLib), Aug-98.                            *
* Last update:                                                  *
*	06-Nov-98, Doomdark					*
*                                                               *
* Modifications:                                                *
*       06-Sep-98 by Kieve                                      *
*           Cleaned up the _file_cmd() function, and pulled out *
*           each individual function                            *
*	06-Nov-98, Doomdark: Fixed probs with multiple args	*
*		to cd ("cd foo bar").				*
*                                                               *
****************************************************************/
 
#pragma strict_types
 
// These are defined so that the module can be test-compiled
// stand-alone, before trying to compile the player object:
#ifndef PLAYER_C
 
#include <config.h>
 
varargs void tell_me(string s, status a, status b);
string query_real_name();
string query_env(string env);
int set_env(string env, string arg);
int query_coder_level();
#endif
 
/* for absolute_path */
#define PATHERROR "NONE"
#define XUSERPATH(name)  ({ DIR_USER, name })
 
#define MAXFILES        80
 
/* for 'cd' command */
#define USERDIR(name)  sprintf("/u/%s", name)
 
static string we_are_editing;   /* File we are editing */
 
static string *rdirs;
 
// Necros 10-6-96
static nomask
void process(string arg, string pre)
{
  string *dir;
  int i;
 
  if (!arg) return;
  if (pre) arg = pre + arg;
  if (file_size(arg) != -2) return;
  if (member(rdirs, arg) >= 0) return;
  rdirs += ({ arg });
  dir = get_dir(arg+"/");
  for (i = 0; i < sizeof(dir); i++)
    process(dir[i], arg+"/");
  return;
}
 
nomask string *
recursive_dir(string arg)
{
  rdirs = ({});
  if (arg[strlen(arg)-1] == '/') arg = arg[0..strlen(arg)-2];
  process(arg, 0);
  return rdirs;
}
 
 
string
absolute_path(string path, int check_special)
{
    string *xpath;
    int    i, j;
    string cwd;

    // First, we'll get rid of any white space
    while((j = member(path, ' ')) != -1)
        path = path[0..j-1] + path[j+1..<1];
    while((j = member(path, '\t')) != -1)
        path = path[0..j-1] + path[j+1..<1];

    if(strstr(path, "...") != -1)
        return PATHERROR;
 
    /* Resolve "here" to player location */
    if (check_special && path == "here") {
        if (environment()) {
            path = object_name(environment());
            sscanf(path, "%s#", path);
            return sprintf("/%s.c", path);
        }
        return PATHERROR;
    }
 
    /* Resolve tilde paths */
    if (path && path[0] == '~') {
        string base;
 
        if (sscanf(path, "~%s/%s", base, path) != 2) {
            base = path[1..<1];
            path = "";
        }
        if (member(({ "", ".", ".." }), base) != -1) {
            base = query_real_name();
        }
        xpath = XUSERPATH(base);
    }
    /* Check for '/' starting paths */
    else if (path && path[0] == '/') {
        xpath = ({ });
    } else {
        /* Use current working directory */
        cwd = query_env("cwd");
        if (cwd == "")
            xpath = ({ });
         else {
             xpath = explode(query_env("cwd"), "/");
         }
    }
 
    /* Create the full path */
    i = sizeof(xpath);
    if (i > 0)
      i--;
    xpath += explode(path, "/");
    for (; i < sizeof(xpath); i++) {
        switch (xpath[i]) {
            case "": case ".":
                //xpath = xpath[0..i-1] + xpath[i+1..max];
                //max--;
                xpath[i] = 0;   // Implode skips non-string elements...
                break;
          case "..":
                xpath[i] = 0;
                j = i;
                while (--j >= 0) {
                    if (stringp(xpath[j])) {
                        xpath[j] = 0;
                        break;
                    }
                }
                //xpath = xpath[0..i-2] + xpath[i+1..max];
                //if (i) { i--; max -= 2;
                //} else { max--; }
                break;
        }
    }
    return "/" + efun::implode(xpath, "/");
}
 
static int
ed_wizcmd(string file)
{
    string tmp_file;
    string *files;
    object *u;
    int    i;
    if(!file) {
        ed(); /* Can't check here if we are in ed because of GD */
        return 1;
    }
    file = absolute_path(file,1);
    if (sizeof(files = get_dir(file))) {
        file = sprintf("/%s/%s", implode(explode(file, "/")[0..<2], "/"),
                    files[0]);
    }
/*
    file = valid_write(file);
*/
    if(!file) {
        tell_me("You can only edit your own files.");
        return 1;
    }
    /* Check if someone else is editing at the same time. //Starbyt */
    u = users();
    for(i = 0; i < sizeof(u); i++)
        if((int)u[i]->query_in_ed(file))
            write("WARNING: " + capitalize((string)u[i]->query_real_name())
                + " is editing it already.\n");
    we_are_editing = file;
    ed(file, "after_ed");
    return 1;
}
 
#if 0
string query_path() { return query_env ("cwd"); }
 
/* return old path type: "player/val", this might change in future
   to type: "player/val/" */
string query_current_path() { return query_env ("cwd"); }
 
/*
  New functions that handle multiple files with single command
  Commands: cp,mv,rm
  Made by Val
*/
#endif
 
private static int copy(string p1, string p2) {
    string buf;
 
    if (!(buf = read_file(p1))) return 0;
    switch (file_size(p2)) {
        case -2: return 0;
        case -1: case 0: break;
        default: if (!rm(p2)) return 0;
    }
    if (!write_file(p2,buf)) return 0;
    return 1;
}
 
private static int do_multi_file(int operation, string p1, string p2) {
    string tmp;
    string *list, *list2;
    int i,c,d;
 
    p1 = absolute_path(p1,0);
    list = get_dir(p1);
    if (!list || !sizeof(list)) {
        write("File(s) not found.\n");
        return 0;
    }
    list = list - ({ ".", ".." });
    if (sizeof(list) > MAXFILES) {
        write("Selected over "+MAXFILES+" files.\n");
        return 0;
    }
 
    /* Cut file name from p1; /ftp/*.c => /ftp/ */
    i = strlen(p1)-1;
    while (p1[i] != '/') --i;
    p1 = p1[0..i];
 
    /* Is p2 directory or a file name */
    if (p2) {
        p2 = absolute_path(p2,0);
        if (file_size(p2+"/") != -2) {
            if (sizeof(list) != 1) {
                write("Destination must be a directory for multiple copy/move.\n");
                return 0;
            }
            list2 = ({ "" });
        } else {
            p2 = p2+"/";
            list2 = list;
        }
    }
 
    /* Loop for real work */
    for (d=c=i=0; i<sizeof(list) && i==(c+d); ++i) {
        tmp = p1+list[i];
        if (list[i]==".." || file_size(tmp) < 0) d++;
        else {
            switch (operation) {
            case 0: c += copy(tmp,p2+list2[i]); break;
#ifndef COMPAT_FLAG
            case 1: c += rename(tmp,p2+list2[i]); break;
#else
            case 1: if (!copy(tmp,p2+list2[i])) break;
#endif
            case 2: c += rm(tmp); break;
            }
        }
    }
    if (c+d != sizeof(list)) write("Failed at file '"+tmp+"'.\n");
    write(({"Copied","Moved","Removed"})[operation]+" "
        +c+" file"+((c==1)?"":"s")+".\n");
    return 1;
}
 
varargs int
query_in_ed(string file)
{
    if (file)
        return we_are_editing == file;
    return we_are_editing != 0;
}
 
void after_ed() { we_are_editing = 0; }
 
 
 
/********************************************************/
 
 
static int _cp_cmd(string arg)
{
mixed p1;
string p2;
    if (!arg || (sscanf(arg,"%s %s",p1,p2) < 2)) {
        notify_fail("Usage: cp <from> <to>\n");
        return 0;
    }
    do_multi_file(0,p1,p2);
    return 1;
}
 
static int _mv_cmd(string arg)
{
mixed p1;
string p2;
    if (!arg || (sscanf(arg,"%s %s",p1,p2) < 2)) {
        notify_fail("Usage: mv <from> <to>\n");
        return 0;
    }
    do_multi_file(1,p1,p2);
    return 1;
}
 
static int _rm_cmd(string arg)
{
    if (!arg) {
        notify_fail("Usage: rm <files>\n");
        return 0;
    }
    do_multi_file(2,arg,0);
    return 1;
}
 
static int _pwd_cmd(string arg)
{
    tell_me(sprintf("/%s", query_env("cwd")));
    return 1;
}
 
static int _mkdir_cmd(string arg)
{
mixed p1;
    if (!arg) {
        notify_fail("Usage: mkdir <directory>\n");
        return 0;
    }
 
    // Mkdir() creates .ACCESS file
    if (mkdir(p1 = absolute_path(arg, 0))) {
        tell_me(sprintf("Ok, created directory '%s'.", p1));
    } else
        tell_me(sprintf("Failed to create directory '%s'!", p1));
    return 1;
}
 
static int _rmdir_cmd(string arg)
{
mixed p1;
string p2;
    if (!arg) {
        notify_fail("Usage: rmdir <directory>\n");
        return 0;
    }
    p1 = absolute_path(arg, 0);
    switch (file_size(p1)) {
        case -2: break;
        case -1: tell_me("No such file or not accessible by you.");
                 return 1;
        default: tell_me("It is not a directory but a normal file.");
                 return 1;
    }
    p2 = sprintf("%s/.ACCESS", p1);
    if (file_size(p2) >= 0 && !rm(p2)) {
        tell_me("Can't remove .ACCESS-file from the directory!");
        return 1;
    }
    if (rmdir(p1))
        tell_me(sprintf("Ok, removed directory '%s'.", p1));
    else
        tell_me(sprintf("Failed to remove directory '%s' (probably "+
            "still files in the directory?)", p1));
    return 1;
}
 
static int _tail_cmd(string arg)
{
    if (!arg) {
        notify_fail("Usage: tail <file>\n");
        return 0;
    }
    if (!tail(absolute_path(arg, 1)))
        tell_me("No such file (or no access).");
    return 1;
}
 
 
static int _cat_cmd(string arg)
{
    if (!arg) {
        tell_me("Usage: cat <file>.");
        return 1;
    }
    if (!cat(absolute_path(arg, 1)))
        tell_me("No such file (or no access).");
    return 1;
}
 
static int _cd_cmd(string arg)
{
mixed p1;
string p2, p3, *files;
int i, j, max, found, nsize;
    if (!arg || arg == "") {
        p1 = PATH_FOR_USER(query_real_name());
    } else {
        p1 = absolute_path(arg, 0);
        if (p1[<1] != '/')
            p1 = p1 + "/";
        if (file_size(p1) != -2) {
            if (!sizeof(files = get_dir(p1))) {
                // hmmm Let's see if we can do a '*' subst before giving
                // an error
                p1 = explode(p1, "/");
                for(i=0 ; i < sizeof(p1) ; i++) {
                    if(p1[i][<1] == '*') {
                        p2 = implode(p1[0..i-1], "/") + "/";
                        files = get_dir(p2);
                        max = sizeof(files);
                        found = 0;
                        while(j < max  &&  !found) {
                            if(file_size(p2 + files[j]) == -2) {
                                if(p1[i][0..<2] == 
                                    files[j][0..strlen(p1[i][0..<2])-1])
                                    found = 1;
                            }
                            j++;
                        }
                        if(found)
                            p1[i] = files[j-1];
                    } // if *
                } // for
                p1 = implode(p1, "/");
                if(file_size(p1) != -2) {
                    notify_fail("No such directory (or no access to it).\n");
                    return 0;
                }
            }
            else if (p1 != "/")
                p1 = sprintf("%s/%s", implode(explode(p1, "/")[0..<2], "/"),
                    files[0]);
            //p1 = absolute_path(p1, 0);
        }
    }
    if (file_size(p1) != -2) {
        tell_me("Cannot read or no such directory.");
        return 1;
    }
    set_env("cwd", p1[1..<1]);
    tell_me(p1);
    return 1;
}
 
static int _chown_cmd(string arg)
{
mixed p1;
string p2, cmd, *files;
int i, j;
    if (!arg || sscanf(arg, "%s%.1t%s", p1, p2) < 2) {
        tell_me("Usage: chown <new_owner> directory.");
        return 1;
    }
    if (member(p2, ' ') >= 0) {
        tell_me("Can only use chown for single directory; except "+
            "when using wildcards ('chown Foobar /u/mydir/dir*').");
        return 1;
    }
    p1 = lower_case(p1);
    p2 = absolute_path(p2, 0);
    if (sizeof(files = get_dir(p2))) {
        if (p2 != "/")
            p2 = implode(explode(p2, "/")[0..<2], "/");
        for (i = 0; i < sizeof(files); i++) {
            if (files[i] == "." || files[i] == "..")
                continue;
            cmd = sprintf("%s/%s", p2, files[i]);
            if (file_size(cmd) != -2) {
                continue;       // Let's just skip non-dirs...
            }
            j++;
            if (MASTER_OB->set_access_data(cmd, p1, "owner"))
                tell_me(sprintf("Changed owner of directory %s to %s.",
                    cmd, p1));
            else
                tell_me(sprintf("Failed to change owner of directory %s "+
                    "to %s.", cmd, p1));
        }
    }
    if (!j) {
        tell_me("No such directory.");
    }
    return 1;
}
 
static int _chmod_cmd(string arg)
{
mixed p1;
string p2, p3, cmd, msg, *files;
int i, j;
    if (!arg || sscanf(arg, "%s%.1t%s%.1t%s", p1, cmd, p2) < 2) {
        if(!arg || (sscanf(arg, "%s %s", cmd, p2) != 2 ||  cmd != "-r")) {
            tell_me("Usage: chmod <coder/coder level> <access> directory\n"+
                "Recursive Usage: chmod -r directory\n"+
                " For example 'chmod Foobar r /u/foo' to add read access "+
                "for Foobar to directory /u/foo. 'chmod -r /u/foo' will "+
                "copy the access in /u/foo to all subdirectories in "+
                "/u/foo/.");
            return 1;
        }
        else p1 = "";
    }
    if (member(p2, ' ') >= 0) {
        tell_me("Can only use chown for single directory; except when "+
            "using wildcards ('chmod Foobar +w /u/mydir/dir*').");
        return 1;
    }
    if (p1[0] >= '0' && p1[0] <= '9')
        p1 = to_int(p1);
    else
        p1 = lower_case(p1);
    p2 = absolute_path(p2, 0);
    if (cmd != "-" && cmd != "r" && cmd != "w" &&
        cmd != "rw" && cmd != "wr" && cmd != "-r") {
        tell_me("Strange access to set; allowed access values are 'r' "+
            "(read access), 'w' (write access), 'rw'/'wr' (both), '-' "+
            "(no access), and '-r' (recursive).");
        return 1;
    }
    if (cmd == "wr") cmd = "rw";
    else if (cmd == "-") cmd = 0;
 
// The recursive call to make the access in the subdirs the same as the
// specified dir. The easy way to do this is to copy the .ACCESS file
// then manually tell the master to load each dir.  So, we must fail if
// there is no .ACCESS file =)
// (Kieve)
    if (cmd == "-r") {
        if (!files = recursive_dir(p2))
            return 1;
        msg = "";
        j = sizeof(files);
        for(i=0 ; i < j ; i ++) {
            p3 = files[i];
            p1 = copy(p2+"/.ACCESS", p3+"/.ACCESS");
            if(!p1) {
                msg += "Failed access copy in "+p3+"\n";
                continue;
            }
            if(!MASTER_OB->load_access_data(p3)) {
                msg += "Failed loading access in "+p3+"\n";
                continue;
            }
            msg += sprintf("Access to directory %s modified as "+
                "specified.\n", p3);
        }
        this_object()->more(explode(msg, "\n"));
        // We're done. Now return 1
        return 1;
    }
 
    if (sizeof(files = get_dir(p2))) {
        if (p2 != "/")
            p2 = implode(explode(p2, "/")[0..<2], "/");
        for (i = 0; i < sizeof(files); i++) {
            if (files[i] == "." || files[i] == "..")
                continue;
            p3 = sprintf("%s/%s", p2, files[i]);
            if (file_size(p3) != -2) {
                continue;       // Let's just skip non-dirs...
            }
            j++;
            if (MASTER_OB->set_access_data(p3, p1, cmd))
                tell_me(sprintf("Access to directory %s modified as "+
                    "specified.", p3));
            else
                tell_me(sprintf("Failed to modify the access to "+
                    "directory %s.", p3));
        }
    }
    if (!j) {
        tell_me("No such directory.");
    }
    return 1;
}
 
 
// Let's just make one function... Less functions helps with call cache.
// Yes, and it makes it UGLY. With a P-II, a few nano-seconds won't matter
static int
_file_cmd(string arg, string cmd)
{
    if (query_coder_level() < 1) return 0;
    switch (cmd) {
        case "cp":
            return _cp_cmd(arg);
            break;
        case "mv":
            return _mv_cmd(arg);
            break;
        case "rm":
            return _rm_cmd(arg);
            break;
        case "pwd":
            return _pwd_cmd(arg);
            break;
        case "mkdir":
            return _mkdir_cmd(arg);
            break;
        case "rmdir":
            return _rmdir_cmd(arg);
            break;
        case "tail":
            return _tail_cmd(arg);
            break;
        case "cat":
            return _cat_cmd(arg);
            break;
        case "cd":
            return _cd_cmd(arg);
            break;
        case "chown":
            return _chown_cmd(arg);
            break;
        case "chmod":
            return _chmod_cmd(arg);
            break;
        default:
            return 0;
    }
    return 1;
}

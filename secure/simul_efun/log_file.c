#ifndef MAX_LOG_SIZE
#define	MAX_LOG_SIZE	50000
#endif

void log_file(string file,string str)
{
    string file_name;
    int *st;

    file_name = "/log/" + file;
#ifdef COMPAT_FLAG
    if (sizeof(regexp(({file}), "/")) || file[0] == '.' || strlen(file) > 30 )
    {
        write("Illegal file name to log_file("+file+")\n");
        return;
    }
#endif
    if ( sizeof(st = get_dir(file_name,2) ) && st[0] > MAX_LOG_SIZE) {
	catch(rename(file_name, file_name + ".old")); /* No panic if failure */
    }
// No no no. Needs to have master access, but only to /log;
// otherwise we'd need to code specific access system to
// prevent coders from editing log files.
#if 0
    set_this_object(previous_object());
#else
	set_this_object(this_object());
#endif
    write_file(file_name, str);
}

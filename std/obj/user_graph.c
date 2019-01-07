/*
 * user_graph.c
 * Made by Val (Tero.Pelander@utu.fi)
 * Last change: 29.3.1996
 */
#pragma strict_types

#define VERSION         "v1.11C"
#define DEST            "/areas/tristeza/adv_guild/adv_guild"
#define SAVENAME        "/open/user_graph/graph_save"
#define FIRST_UPD       30    /* 30 s */
#define UPD_TIME        60    /* 60 s */
#define SAVETIME        900   /* 15 min = 900 s */
#define IS_WIZARD(ob)   ((int)ob->query_coder_level() > 0)
/* #define SHORTGRAPH	2 */ /* For compressed graph when over 55 users */

/*
 * How many hours do you want to group together?
 * TIMEGROUP * TIMEPARTS = 24
 */
#define TIMEGROUP 2
#define TIMEPARTS 12
#define TIMESTRINGS\
    ({"24-02","02-04","04-06","06-08","08-10","10-12",\
      "12-14","14-16","16-18","18-20","20-22","22-24"})
#define DAYS 7


string  *dlist;                 /* date list, list of date strings       */
mixed   *list;                  /* arrays list = list of last seven days */
static  int     save_time;      /* next time when Graph is to be saved   */


/*
 * The slow "heart_beat" of graph.
 */
nomask void
hb()
{
    mixed *tmp;
    object *us;
    int    i, w, p;
    string time_str;

    call_out("hb", UPD_TIME);

    /* Test for change of day and update lists if needed */
    time_str = ctime(time());
    sscanf(time_str[11..12], "%d", p);  /* Let's use p as temporary variable */
    if(time_str[4..9] != dlist[DAYS - 1])
    {
	tmp = allocate(i = TIMEPARTS);
	while(i--) tmp[i] = allocate(4);
	list  = list[1..(DAYS-1)]  + ({ tmp });
	dlist = dlist[1..(DAYS-1)] + ({ time_str[4..9] });
    }

    /* Count wizards/mortals present and update list */
    tmp = list[DAYS - 1][p/TIMEGROUP];
    i = p = sizeof(us = users());
    while (i--) if (us[i] && IS_WIZARD(us[i])) ++w;
    tmp[0] += w;
    tmp[1] += p;
    if (tmp[2] < p) tmp[2] = p;
    tmp[3]++;

    /* Check if the graph has to be saved */
    if (time() >= save_time)
    {
	save_object(SAVENAME);
	save_time = time() + SAVETIME;
    }
}

status
id(string s)
{
    return member(({"graph","user graph","week","week list"}), s) != -1;
}

string
short()
{
    return "User graph hanging on the wall";
}

void
create(int x)
{
    if (save_time) return;
    move_object(this_object(), DEST);
    if (!restore_object(SAVENAME))
    {
	int i, j;

	list = allocate(DAYS);
	dlist = allocate(DAYS);
	for (i = 0; i < DAYS; ++i)
	{
	    dlist[i] = "--- --";
	    list[i] = allocate(TIMEPARTS);
	    for(j = 0; j < TIMEPARTS; ++j)
		list[i][j] = allocate(4);
	}
    }
    save_time = time() + SAVETIME;
    call_out("hb", FIRST_UPD);
}

void
long(string arg)
{
    int line, i, w,p,c,m;
    int *tmp;
    string str;

    write("Values from last seven days. Local time: "+ctime(time())+".\n");
    write("C = average of coders, m = average of mortals, . = max players.\n");

    /* Look at week/week list */
    if (arg=="week" || arg=="week list")
    {
	string *str_list;

	write("Day      " + implode(dlist, "    ") + "\n");
	write("         C  m  .   C  m  .   C  m  .   C  m  .   C  m  .   C  m  .   C  m  .\n");
	str_list = allocate(DAYS);
	do
	{
	    for(i = 0; i < DAYS; ++i)
	    {
		tmp = list[i][line];
		if (c = tmp[3])
		{
		    w = (c/2+tmp[0])/c;
		    p = (c/2+tmp[1])/c-w;
		    m = tmp[2];
		    str_list[i] = sprintf("%2d%3d%3d", w, p, m);
		} else
		    str_list[i] = " -  -  -";
	    }
	    write(sprintf("%s:%@10s\n", TIMESTRINGS[line], str_list));
	}
	while (++line < TIMEPARTS);
	return;
    }

    /* Look at graph/user graph */
#ifndef SHORTGRAPH
    write("        123456789.123456789.123456789.123456789.123456789.12345  C   M Tot max\n");
#else
    /* For SHORTGRAPH == 2 */
    write("        2468.2468:2468.2468:2468.2468:2468.2468:2468.2468:2468.  W   M Tot max\n");
#endif
    write("------------------------------------------------------------------------------\n");
    do
    {
	for (w = m = c = i = 0; i < DAYS; ++i)
	{
	    tmp = list[i][line];
	    w += tmp[0]; p += tmp[1]; c += tmp[3];
	    if (tmp[2]>m) m = tmp[2];
	}
	if (c) { w = (c/2+w)/c; p = (c/2+p)/c; } else { w = p = m = 0; }
	write(sprintf("%s:  %-55.55s%3d%4d%4d%4d\n",
	    TIMESTRINGS[line],
#ifndef SHORTGRAPH
	    (m ? sprintf("%*'C's%*'m's%*'.'s ",
		w, "", p-w, "", m-p, "")  : "-"),
#else
#define SG SHORTGRAPH
#define SM (SHORTGRAPH-1)
	    (m ? sprintf("%*'C's%*'m's%*'.'s ",
		(w+SM)/SG, "", (p+SM)/SG-(w+SM)/SG, "", (m+SM)/SG-(p+SM)/SG, "")
	      : "-"),
#undef SG
#undef SM
#endif
	    w, p-w, p, m));
    }
    while (++line < TIMEPARTS);
    write("------------------------------------------------------------------------------\n");
    write(" Made by Val "+VERSION+". 'look at week list' for separate days.\n");
}

void
remove()
{
    save_object(SAVENAME);
    destruct(this_object());
}

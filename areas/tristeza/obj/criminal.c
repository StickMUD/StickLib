/*
 * Criminal status (in Main Town)
 *
 * /basic/living/criminal.c
 *
 *	Id	Crime
 *
 *	1	theft
 *	2	burglary
 *	3	assault
 *	4	manslaughter
 *	5	murder
 *
 */
#pragma strict_types

int crime_time, crime_level;

string
query_auto_load()
{
string tmp, s;
int i;
	tmp = file_name(this_object());
	if (sscanf(tmp, "%s#%d", s, i)) tmp = s;
	return tmp + ":"+crime_time+"#"+crime_level;
}

void
init_arg(string arg)
{
    sscanf(arg, "%d#%d", crime_time, crime_level);
}

static void
check_criminal()
{
    if (!crime_time) {
	crime_time = crime_level = 0;
    }
}

int
query_crime_level()
{
    check_criminal();

    if (time() < crime_time) return crime_level;

    crime_level = 0;

    tell_object(environment(), "You are no longer hunted for your crimes.\n");
}

int query_crime_time() { check_criminal(); return crime_time; }
void set_crime_time(int t) { check_criminal(); crime_time = t; }
void set_crime_level(int l) { check_criminal(); crime_level = l; }

string
query_crime_status()
{
    check_criminal();

    if (crime_level < 1) return 0;

    switch (crime_level) {
	case 1: return "theft"; break;
	case 2: return "burglary"; break;
	case 3: return "assault"; break;
	case 4: return "manslaughter"; break;
	case 5: return "murder"; break;
	default: return 0; break;
    }
}

// Module containing description handling for players...

string title;                   // Our official title. Wizs can change it.
string desc;                    // Optional extra_look set by player

#ifndef PLAYER_C
#include <party.h>

int partyId;

varargs void tell_me(string s, status x);
mixed query_party(int x);
#endif

int title_wizcmd(string s);

static string desc_set;
static int desc_ln;

// Query-functions:

string query_desc() { return desc; }

string
query_title()
{
string s;
#if 0
	if (partyId && (s = query_party(PARTY_TITLE)))
		return sprintf("%s [Party: %s]", title, s);
#endif
	return title;
}

// Set-functions:

void set_desc(string d) { desc = d; }

static status
desc_cmd(string str)
{
    if (str=="clear" || str=="none" || str=="remove") {
        tell_me("Description cleared.");
        desc = 0;
        return 1;
    }

    tell_me("Your description is:\n" + (desc ? desc : "none"));

    if (str != "set")
       tell_me(
"Use command 'desc set' to set your description, and 'desc clear' to\
 get rid of it.");
    else {
       tell_me(
"Set your description (max. 5 lines). Type ** to end, and ~q to cancel.\n"+
"Note that each line may have a maximum of 79 characters.");
	tell_me("]",1);
	desc_set = "";
	desc_ln = 0;
	input_to("descset",0);
    }

    return 1;
}

static void
descset(string str)
{
    if (str == "~q") {
       tell_me("Canceled desc set.");
       return;
    }

    if (str == "**" || desc_ln > 5) {
	tell_me("Ok.");
	desc = desc_set;
	desc_cmd(0); return;
    }

    if (strlen(str) > 79) {
       tell_me("Too long line - it was truncated.");
       str = str[0..79];
    }


    desc_ln++;

    desc_set += str + "\n";

    tell_me("]", 1);
    input_to("descset",0);
    return;
}

// Had to remove control characters - even lords started using beeping titles.
int
set_title(string t)
{
int i;
string tit;

	if (!t) {
		tell_me("Your title is " + title + ".");
		return 1;
	}
	if (t == "empty" || t == "none" || t == "clear") {
		title = 0;
		return 1;
	}

	if (strlen(t) > 60)
		t = t[0..59];	// Max 60 characters

	tit = "";
	for (i = 0; i < strlen(t); i++)
	if (lower_case(t[i..i]) >= " " && lower_case(t) <= "z")
		tit += t[i..i];

	title = tit;
	return 1;
}

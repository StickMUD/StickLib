/************************************************************************
*									*
* Module name:								*
*		id ("/basic/id.c")					*
*									*
* Version:								*
*		V1.1a							*
*									*
* Last changes:								*
*		17-May-98 / Graah					*
*									*
* - id_list not private any more. *sigh* -+ Doomdark +-			*
* 17-May-98 / Graah: Added function add_id().				*
*************************************************************************
*
* NEW:
*
* - 'query_short', 'query_long' and 'id' all accept now new arguments;
*   'who' is the object who wants to get our long/short_desc or test
*   our id, 'id' is the string with which we got the idea of getting
*   the long_desc of this object (usually with 'examine'/'look at'
*   command). Look below why they are added.
* - save_variables & restore_variables; used by reboot safes.
* - 'set_invis' and 'query_invis'; allows invisibility levels to be
*   added to all items that inherit this module (almost every object).
*   Invisibility level is used by other functions as well, like
*   'query_name', 'query_short' etc.
* - Long_desc & short_desc can now be closures! -+ Doomdark 26-mar.96 +-
*
* HOW TO USE THIS MODULE?
*
* - Almost all standard objects (exception being room.c) inherit this module;
*   so you usually use this module in every object you code (excl. rooms).
*   Some standard objects like weapon.c and armour.c inherit this module
*   indirectly through /basic/curse.c (which inherits this module).
*   Usually you first set the name of the object with 'set_name', then
*   synonyms your object accepts with 'set_id', then sometimes short
*   description with set_short (note that set_name also sets short
*   description so that it's the same as the name plus the article
*   in front of it (set_name("red ball"); -> short_desc == "a red ball"),
*   and last, set the long description of the object (the text player gets
*   when he/she examines the object. So, you might use something like this:
*
* set_name("blue test object");
* set_id(({ "object","test object","blue object" }));
* // You may leave next line off, it's not needed in this case!
* set_short("a blue test object");
* set_long("It's a usual blue test object to show coders how\
*  to set up normal objects with functions found in /basic/id.c.");
* // Note: NO LINEFEEDS THERE!!! Also, if your description doesn't fit in
* // one line, use "\" in the end of the line to concatenate the lines
* // (instead of using "+"-operator; no big deal, just bit more efficient).
*
* So, here you go. Things to note:
*
* o No linefeeds! Mudlib handles linewrapping so that players can have
*   different linelenghts (settable by players), so you shouldn't
*   decide how long lines they should get... If you specifically want
*   a sentence to start at the beginning of the line, use a linefeed
*   before that sentence. Otherwise, there's no need for them.
* o No need for set_short if your short description is just the
*   name of the object preceded by the article.
* o When texts (like long_desc) don't fit in one line, concatenate
*   the lines with "\".
**************************************************************************/

#include <invis_levels.h>

// Global variables.

nomask static string id_name, *id_list;
nomask static mixed short_desc, long_desc;
int invis_level;

/****************************************************************
*								*
*			Set-functions.				*
*								*
****************************************************************/

// Every object should have a name. A name starts with small letter,
// no article ('a' or 'an') and no location mentioned.
// Examples: orc, blue cube, can of tunafish
// (ILLEGAL: an orc, blue cube is here)

void
set_name(string str)
{
    if (stringp(str)) {
      string foo;
	id_name = str;
	    if (member( ({ 'a', 'e', 'i', 'o', 'u' }), str[0]) < 0 ||

// Words beginning with these letters are (usually) pronounced like beginning
// with consonants, so they will have an "a" as an article.

	    sscanf(str, "eu%s", foo) || sscanf(str, "uni%s",foo)
	    || sscanf(str, "us%s",foo)) short_desc = "A "+str;
	    else short_desc = "An "+str;
    } else id_name = 0;

	if (!id_list) id_list = ({ });
}

// Set the id of an item. Should be an array. For compatibility reasons
// can be a string too. Argument 0 removes all ids.

void
set_id(mixed arg)
{
	if (pointerp(arg))
		id_list = arg;
	else if (arg) {
		if (id_list) id_list += ({ arg });
		else id_list = ({ arg });
	} else {
		id_list = ({ });
		return;
	}
// This should not be needed; 'set_name' should be called before set_id.
	if (!id_name) id_name = id_list[0];
}

// Description	: This function only adds more ids, unlike set_id().
void
add_id(mixed arg)
{
  int s, i;

  if (!pointerp(id_list)) id_list = ({});

  // Adding many ids at once
  if (pointerp(arg))
    {
      // Remove duplicate ids
      s = sizeof(id_list);
      if (s > 0)
	for (i = (s - 1); i >= 0; i--)
	  if (member(arg, id_list[i]) >= 0) arg -= ({ id_list[i] });

      if (sizeof(arg) > 0) id_list += arg;
      return;
    }

  if (stringp(arg))
    if (member(id_list, arg) == -1) id_list += ({ arg });
}

// Avoid returning THE id_list - don't ask why :)  /Graah
string query_id_str() { return implode(id_list, "#"); }

// MUST be static! Object internal use only. /Graah
// (Well, actually, who cares, this could be non-static...)
static nomask
string *query_id_list()
{
  return id_list;
}

void
set_short(mixed str)
{
	short_desc = str;
}

void
set_long(mixed str)
{
  long_desc = str;
}

void
set_invis(int i)
{
	if (intp(i)) invis_level = i;
	else if (i) invis_level = 1;
}

/****************************************************************
*								*
*			Query-functions.			*
*								*
****************************************************************/

varargs public string
query_name(int mode, object who)
{
	if (invis_level && (!intp(mode) || mode < invis_level)) return "something";
// Living.c has its own query_name, so this doesn't have to worry about
// living things.
	return id_name;
}

varargs string
query_short(int mode, object who)
{
	if (invis_level && (!intp(mode) || mode < invis_level)) return 0;
	if (invis_level) {
		if (invis_level >= IL_INVISIBLE) {
		  if (invis_level >= IL_TOTALLY_INVISIBLE)
			return sprintf("%s <totally invisible>",
                          funcall(short_desc, mode, who));
		  else return sprintf("%s <invisible>",
                          funcall(short_desc, mode, who));
		} else if (invis_level >= IL_HIDING) {
		  if (invis_level >= IL_HIDDEN)
		     return sprintf("%s <hidden>",
                          funcall(short_desc, mode, who));
		  else return sprintf("%s <hiding>",
                          funcall(short_desc, mode, who));
		} else return sprintf("%s <sneaking>",
                          funcall(short_desc, mode, who));
	}
	return funcall(short_desc, mode, who);
}

varargs string
query_long(string id, object who)
{
	if (long_desc) return funcall(long_desc, id, who);
	else return "You see nothing unusual.";
}

int query_invis() { return invis_level; }

/****************************************************************
*								*
*			And then all the rest.			*
*								*
****************************************************************/

// Called by game driver (=GD).
varargs public status
id(string s, int invis_to_see, object who)
{
	if (invis_level && intp(invis_to_see) && invis_to_see < invis_level)
		return 0;
	return s == id_name || (id_list && member(id_list, s) >= 0);
}

// save_variables() and restore_variables() for saving items over reboot.

mapping
save_variables()
{
	return ([
		"id_name" : id_name
// We have to take care that mappings won't be saved... :-/
		,"short_desc" : (stringp(short_desc) ? short_desc : 0)
		,"long_desc" : (stringp(long_desc) ? long_desc : 0)
		,"id_list" : id_list
		,"invis_level" : invis_level
	]);
}

void
restore_variables(mapping var)
{
	id_name = var["id_name"];
	short_desc = var["short_desc"];
	long_desc = var["long_desc"];
	id_list = var["id_list"];
	invis_level = var["invis_level"];
}

/******* compatibility functions ******/
// void set_alias(string str) { if (str) set_id(str); }
// void set_alt_name(string str) { if (str) set_id(str); }

/*** Default short and long ***/
mixed short(mixed x, mixed y) { return query_short(x,y); }
mixed long(mixed x, mixed y) { return query_long(x,y); }

/*** Can now ask light from all objects ***/
int query_light() { return set_light(0); }

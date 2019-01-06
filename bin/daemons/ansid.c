/************************************************************************
*									*
* Module:								*
*	/bin/daemons/ansid.c		       				*
* Description:								*
*	A standard daemon that knows which ANSI-codes we support. For	*
*	efficiency, this daemon is not used for formatting text, it	*
*	just provides the mapping used in translation.			*
* Version:								*
*	1.0a 12-aug-96.							*
* Changes:								*
*	-								*
*									*
* NEW:									*
*	-								*
************************************************************************/

static mapping _codes;

mapping
query_ansi_codes()
{
    return _codes;
}

void
create()
{
    _codes = ([
      "normal"		: "0",
      "reset"		: "0",
      "bold"		: "1",
      "dim"		: "2",
      "italic"		: "3",
      "underlined"	: "4",
      "reversed"	: "7",
      // These pseudonames might be useful occasionally?
      "colour0"	: "30",
      "colour1"	: "31",
      "colour2"	: "32",
      "colour3"	: "33",
      "colour4"	: "34",
      "colour5"	: "35",
      "colour6"	: "36",
      "colour7"	: "37",
      // Let's allow using real colour names too...
      "black"		: "30",
      "red"		: "31",
      "green"		: "32",
      "yellow"		: "33",
      "blue"		: "34",
      "purple"		: "35",
      "cyan"		: "36",
      "white"		: "37",
    ]);
}

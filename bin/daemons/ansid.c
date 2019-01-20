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
        "normal"      : "0",
        "reset"       : "0",
        "bold"        : "1",
        "dim"         : "2",
        "italic"      : "3",
        "underline"   : "4",
        "flash"       : "5",
        "reverse"     : "7",
        "black"       : "30",
        "red"         : "31",
        "green"       : "32",
        "yellow"      : "33",
        "blue"        : "34",
        "magenta"     : "35",
        "cyan"        : "36",
        "white"       : "37",
        "bgblack"     : "40",
        "bgred"       : "41",
        "bggreen"     : "42",
        "bgyellow"    : "43",
        "bgblue"      : "44",
        "bgmagenta"   : "45",
        "bgcyan"      : "46",
        "bgwhite"     : "47",
    ]);
}

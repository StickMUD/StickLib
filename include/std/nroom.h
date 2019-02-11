/*
 * nroom.h
 * The Standard Room
 *
 * THIS VERSION IS BUT A KLUDGE. BUT IT SHOULD BE UPWARDS COMPATIBLE AND
 * USERS NOTICE NO DIFFERENCE ANYHOW.
 *
 */

#include <room_defs.h>
#pragma combine_strings

#if !defined(NO_INHERIT) && !defined(NROOM_NO_INHERIT)
inherit ROOM_FILE;
#endif

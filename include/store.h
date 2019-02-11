#ifndef STORE_H
#define STORE_H

/* Let's not inherit /lib/room.c. It's in /lib/store.c */
#include <room_defs.h>

#if !defined(NO_INHERIT) && !defined(STORE_NO_INHERIT)
inherit STORE_FILE;
#endif

#endif /* STORE_H */

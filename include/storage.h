#ifndef STORAGE_H
#define STORAGE_H

/* Let's not inherit /lib/room.c. It's in /lib/store.c */
#include <room_defs.h>

#if !defined(NO_INHERIT) && !defined(STORAGE_NO_INHERIT)
inherit STORAGE_FILE;
#endif

#endif /* STORAGE_H */

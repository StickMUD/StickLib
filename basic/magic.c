/*
 * /basic/magic.c
 *
 * Magical "plusses" of an object. If you make magical weapons or armours
 * remember that their ac or wc should not be too high (i.e. ac 5 is still
 * maximum for armour suits, wc max. 18 for non-unique weapons)
 *
 * Magic items are much harder to damage, more valuable, and make their owner
 * feel so great and mighty.
 *
 * "Shining +5 magic adamantite full plate mail (worn)"...
 * (That would be a funny item because its actual ac would have to be
 *  set as 0. But since its ac is 0, its weight could be for example 2)
 *
 * This module also defines function "saving_throw" which returns 1 if
 * the item saves (normal items have 5% change to save), 0 if save fails.
 *
 * Copyright (c) 1993 The Stickmud Project,
 * All rights reserved.
 *
 */
#pragma strict_types

int magic;

void
set_magic(int x) {
    if (x < 0 || x > 5) {
	write("There is too much magic around here!\n");
	return;
    }

    magic = x;
}

int
query_magic()
{
    return magic;
}

/* A magic item resists more punishment than other items */
status
saving_throw()
{
    if (random(107) < (5 + 20 * magic)) return 1;

    return 0;
}

#pragma strict_types
/*
 * This file in inherited by the player object. No need for monsters to have
 * this.
 * Busy flag - to prevent players from doing two things on one round
 * (like zapping two wands, or casting spells, attacking and throwing
 * things at the same time).
 *
 * Stupid new variable and more call_others...but it would be even
 * slower to add properties.
 *
 * Now only one new function (plus the variable) 27.7.1995 //Frobozz
 */

static int busy_flag;

int
add_busy_flag(int flag)
{
    return (busy_flag += flag);	// Kludged even more by Doomdark.
}

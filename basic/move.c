#include <move.h>

public int
move(mixed dest) {
int x;
	if (x = (int) dest -> receive(this_object())) return x;
/*
 * This means move can't be done. The type of 'error' depends on the value,
 * values are defined in <move.h>.
 */
	move_object(this_object(), dest);
	return 0;
/* Move made succesfully, we'll return 0. */
}

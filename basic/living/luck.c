/* Luck  - a "new" hidden ability. */

/* Luck range */
#define MIN_LUCK (-15)
#define MAX_LUCK (10)

int luck;
static int temp_luck, item_luck;

void
set_luck(int x)
{
    if (x < MIN_LUCK) luck = MIN_LUCK;
    else if (x > MAX_LUCK) luck = MAX_LUCK;
    else luck = x;
}

void set_temp_luck(int x) {
    if (x < MIN_LUCK) temp_luck = MIN_LUCK;
    else if (x > MAX_LUCK) temp_luck = MAX_LUCK;
    else temp_luck = x;
}

/* The items themselves have to update the value when the item is dropped! */
/* 2.1.1994  //Elena */
void add_item_luck(int x) { item_luck+=x; }

int query_luck() { return luck; }
int query_temp_luck() { return temp_luck; }
int query_item_luck(int x) { return item_luck; }

/* Call this function to get a random luck adjustment */
int
query_random_luck()
{
    int l, itm;

    /* Luck has an effect only 35% of the time */
    if (random(100) < 65) return 0;

    /* Check max/min values for lucky items */
    itm = item_luck;
    if (itm < MIN_LUCK) itm = MIN_LUCK;
    else if (itm > MAX_LUCK) itm = MAX_LUCK;

    l = (luck + temp_luck + itm);

    /* Added luck needs to be in range, too.   //Elena */
    if (l < MIN_LUCK) l = MIN_LUCK*2;
    else if (l > MAX_LUCK) l = MAX_LUCK*2;

    /* Let's be paranoid with this. Game has crashed with this stuff. */
    /* Unlucky? */
    if (l < 0) {
        l = -l;
	return (-(random(l)));
    }
    /* Lucky? */
    else if (l > 0) return random(l);

    return 0;
#if 0
    return (1 - random(3)); /* Luck zero - actual adjustment -1, 0 or 1 */
#endif
}

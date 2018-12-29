/*
 * Every daemon (and command) must inherit this object.
 */
int clean_up(int ref) { destruct(this_object()); }

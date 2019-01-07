#if 0
// Old one:
void shout(string s) {
    filter_array(users(), lambda(({'u}),({#'&&,
	  ({#'environment, 'u}),
	  ({#'!=, 'u, ({#'this_player})}),
	  ({#'tell_object, 'u, s})
	})));
}
#else
void shout(string s) {
    object *u, ob;
    int i;
    u = users();
    ob = this_player();
    set_this_object(previous_object());
    efun::set_this_player(previous_object());
    i = sizeof(u);
    while (--i >= 0) {
	if (u[i] != this_player())
	    u[i]->catch_shout(s);
    }
    if (ob)
	efun::set_this_player(ob);
}
#endif

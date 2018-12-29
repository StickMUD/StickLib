/* Moved to /bin/pub 22-sep-94. */
/* Graah 18-Jan-96: room extra_move_object() function :( */

#include <cmd.h>

varargs nomask status
put_cmd(string str, object me)
{
  int i;
  string item, s, container;
  object item_o;
  object container_o;

  if (!me && !(me = this_player())) return 0;
  if (!str) return notify_fail("Put what?\n"), 0;
	if (!me->query_can_move())
		return 0;
	i = (int) me->test_dark();
	if (i && i > -4 && i < 4)
		return 0;

  if(sscanf(str, "%s in %s", item, container) != 2)
    return
      notify_fail("Put what (\"put <object> in <container>\")?\n"), 0;
  container = lower_case(container);
  if (sscanf(str, "the %s", s)) str = s;
  if (!(container_o = present(container, me)))
    container_o = present(container, environment(me));
  if (!container_o) {
    me -> tell_me("You can't see any \"" + container + "\" here!");
    return 1;
  }
  if (!container_o -> can_put_and_get()) {
    me -> tell_me("You can't do that.");
    return 1;
  }
  item = lower_case(item);
  if (!(item_o = present(item, me))) {
    me -> tell_me("You don't have any \"" + item + "\".");
    return 1;
  }
  if (item_o == container_o)
    return notify_fail("You can't put any object inside itself!\n"), 0;
  if (item_o -> prevent_insert(me, container_o))
    return 1;
  if (item_o -> drop(0)) return 1;

  /* NEW: containers can refuse from taking an item 10.6.1994 //Frobozz */

  if (container_o -> refuse_item(item_o, me)) return 1;

  i = (int)item_o->query_weight();

  if(container_o -> add_weight(i)) {
    /* Remove the weight from the previous container. */
    environment(item_o) -> add_weight(-i);
    move_object(item_o, container_o);
    call_other(environment(me), "extra_move_object", item_o, container_o);

    environment(me) -> tell_here(
(string) me -> query_name(0, me)+ " puts the " + item + " in the "
+ (string) container_o-> query_name(0, me) + ".", me);
    me -> tell_me("Ok.");
    return 1;
  }
  me -> tell_me("There is no room for more.");
  return 1;
}

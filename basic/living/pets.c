/*
 * Pets/slaves (monsters) or party followers (players)
 *
 */

static object master; /* Whose pet we are? */
static status follow_master; /* Do we follow? (default: yes) */
static status defend_master; /* Do we defend (default: yes) */
static object *pets; /* Our pets/slaves/party */

object query_master() { return master; }
object *query_pets() { return pets; }
status query_follow_master() { return follow_master; }
status query_defend_master() { return defend_master; }

status
set_master(object ob)
{
  if (!ob)
    {
      if (master) master->remove_pet(this_object());
      master = 0;
      return 0;
    }

  if (ob == this_object()) return 0; /* Can't be its own master */

  /* Don't allow loops! They jam attacking and moving. */
  if (ob && ((object)ob->query_master() == this_object()))
    {
      /* There isn't any proper error message efun for these... */
      write("Monster tried to be a master of its own master!\n");
      return 0;
    }

  master = ob;
  follow_master = 1;
  defend_master = 1;

  return (status)master->add_pet(this_object());
}

status
add_pet(object ob)
{
  int i;
  object *p;

  if (!ob) return 0;
  if (!pointerp(pets)) pets = ({});
  if (member(pets, ob) == -1) pets += ({ ob });

  p = ({});

  for (i = sizeof(pets) - 1; i >= 0; i--)
    {
      if (pets[i] && living(pets[i])) p += ({ pets[i] });
    }

  pets = p;

  return 1;
}

status
remove_pet(object ob)
{
  if (!pointerp(pets)) return 0;
  if (member(pets, ob) == -1) return 0;
  pets -= ({ ob });
  return 1;
}

void set_follow_master(status x) { follow_master = x; }
void set_defend_master(status x) { defend_master = x; }

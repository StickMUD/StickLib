#include <daemons.h>

int finger2_cmd(string str)
{
  if(!str)
  {
    notify_fail("Usage: finger <player>\n");
    return 0;
  }

  if(!FINGER_D2->show_data(str))
  {
      notify_fail("No such player.\n");
      return 0;
  } 

  return 1;
}

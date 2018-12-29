// Version 1.2 12-jan-96 by Doomdark 
// Peace added (to PK query) 20-Jan-96 by Graah
// 17-Jul-96 by Graah: ROOM_ISOLATED flag for totally "isolated"
//  rooms. (Empty who list, no channels, tells, shouts, where etc...)

#ifndef ROOM_C

#include <room_defs.h>
#include <daemons.h>

string night_long_desc;
mapping Items;
mapping Exits;
mixed *room_effects;
static string short_desc, long_desc, short_exit_list;

#endif

#include "/basic/room/room_private.h"

static int Flags, outdoorness;

mixed
query(int x)
{
  status n;

  if (x > 0) n = 1;
  else x = -x;

  switch (x)
    {
    case SKIP_EXITS:
      return n ? (Flags & F_ROOM_EXITS_SKIPPED) :
	!(Flags & F_ROOM_EXITS_SKIPPED);
    case PREVENT_TELEPORT:
      return n ? (Flags &
		  (F_ROOM_ISOLATED |
		   F_ROOM_PREVENT_TELEPORT_TO |
		   F_ROOM_PREVENT_TELEPORT_FROM)) :
		     !(Flags & (F_ROOM_ISOLATED |
				F_ROOM_PREVENT_TELEPORT_TO |
				F_ROOM_PREVENT_TELEPORT_FROM));
    case NO_CLEANUP:
      return n ? (Flags & F_ROOM_NO_CLEANUP) : !(Flags & F_ROOM_NO_CLEANUP);
    case NO_PK:
      return n ? ((Flags & F_ROOM_NO_PK) || PEACE_D->query_peace()) :
	!((Flags & F_ROOM_NO_PK) || PEACE_D->query_peace());
    case IN_CITY:
      return n ? (outdoorness & WD_CITY) : !(outdoorness & WD_CITY);
    case PREVENT_SHOUT:
      return n ? (Flags & (F_ROOM_ISOLATED | F_ROOM_PREVENT_SHOUT)) :
	!(Flags & (F_ROOM_ISOLATED | F_ROOM_PREVENT_SHOUT));
    case PREVENT_EMOTE:
      return n ? (Flags & F_ROOM_PREVENT_EMOTE) :
	!(Flags & F_ROOM_PREVENT_EMOTE);
    case PREVENT_TELEPORT_TO:
      return n ? (Flags & (F_ROOM_PREVENT_TELEPORT_TO | F_ROOM_ISOLATED)) :
	!(Flags & (F_ROOM_PREVENT_TELEPORT_TO | F_ROOM_ISOLATED));
    case PREVENT_TELEPORT_FROM:
      return n ? (Flags & (F_ROOM_PREVENT_TELEPORT_FROM | F_ROOM_ISOLATED)) :
	!(Flags & (F_ROOM_PREVENT_TELEPORT_FROM | F_ROOM_ISOLATED));
    case ROOM_WD:
      return outdoorness;
    case ROOM_PREVENT_WHERE:
      return n ? (Flags & (F_ROOM_PREVENT_WHERE | F_ROOM_ISOLATED)) :
	!(Flags & (F_ROOM_PREVENT_WHERE | F_ROOM_ISOLATED));
    case ROOM_ISOLATED:
      return n ? (Flags & F_ROOM_ISOLATED) :
	!(Flags & F_ROOM_ISOLATED);
    case ROOM_GUILD:
      return n ? (Flags & F_ROOM_GUILD_ROOM) : !(Flags & F_ROOM_GUILD_ROOM);
    case ROOM_NIGHT_LONG_DESC:
      return night_long_desc;
    case ROOM_LONG_DESC:
      return long_desc;
    case ROOM_SHORT_DESC:
      return short_desc;
    case ROOM_SHORT_EXIT_LIST:
      return short_exit_list;
    }

  if (x == ROOM_FLAGS) return Flags;
}

// NEW!
status
add(int what, mixed arg1, mixed arg2)
{
  string t;
  int x;

  switch (what) {
    // New; room effect. Message to get printed with chance of arg1%;
    // message is arg2. So, something like:
    // add(ROOM_EFFECT, 10, "You hear howling of wolves in distance.");
    // (this would print message "You hear..." every 10th hb (hb is done
    // every 2nd seconds) in average.
    //
  case ROOM_EFFECT:
    if (!room_effects || !sizeof(room_effects)) {
      room_effects = ({ arg1, arg1, arg2 });
      Flags |= F_ROOM_HAS_EFFECTS;
    } else {
      room_effects += ({ arg1, arg2 });
      room_effects[0] += arg1;
    }
    return 1;

  case ROOM_EXIT:
    if (!arg2) {
      if (Exits)	
	Exits = m_delete(Exits, arg1);
    }
    else if (!Exits) Exits = ([ arg1 : arg2 ]);
    else Exits[arg1] = arg2;
    Flags &= (~F_ROOM_EXIT_LISTS_UPDATED);
    return 1;

  case ROOM_ITEM:
    if (!arg2) return 0;
    if (stringp(arg1))
      if (!Items) Items = ([ arg1 : arg2 ]);
      else Items[arg1] = arg2;
    else if (pointerp(arg1)) {
      if (!Items) Items = ([ arg1[0] : arg2 ]);
      else Items[arg1[0]] = arg2;

      x = sizeof(arg1);
      t = "=" + (string*) arg1[0];

      while (--x) Items[(string*) arg1[x]] = t;
    }
    else return 0;
    return 1;
  }

  return 0;
}

varargs status
set(int x, mixed y)
{
  status n;

  if (x > 0) n = 1;
  else x = -x;
  switch (x) {
  case SKIP_EXITS:
    Flags = n ? (Flags | F_ROOM_EXITS_SKIPPED) : (Flags & (~F_ROOM_EXITS_SKIPPED));
    break;
  case PREVENT_TELEPORT:
    Flags = n ? (Flags | F_ROOM_PREVENT_TELEPORT_TO | F_ROOM_PREVENT_TELEPORT_FROM) :
      (Flags & (~(F_ROOM_PREVENT_TELEPORT_TO | F_ROOM_PREVENT_TELEPORT_FROM)));
    break;
  case NO_CLEANUP:
    Flags = n ? (Flags | F_ROOM_NO_CLEANUP) : (Flags & (~F_ROOM_NO_CLEANUP));
    break;
  case NO_PK:
    Flags = n ? (Flags | F_ROOM_NO_PK) : (Flags & (~F_ROOM_NO_PK));
    break;
  case IN_CITY:
    outdoorness = n ? (outdoorness | WD_CITY) : (outdoorness & (~WD_CITY));
    break;
  case PREVENT_SHOUT:
    Flags = n ?(Flags | F_ROOM_PREVENT_SHOUT) : (Flags & (~F_ROOM_PREVENT_SHOUT));
    break;
  case PREVENT_EMOTE:
    Flags = n ? (Flags | F_ROOM_PREVENT_EMOTE) : (Flags & (~F_ROOM_PREVENT_EMOTE));
    break;
  case PREVENT_TELEPORT_TO:
    Flags = n ? (Flags | F_ROOM_PREVENT_TELEPORT_TO) :
      (Flags & (~F_ROOM_PREVENT_TELEPORT_TO));
    break;
  case PREVENT_TELEPORT_FROM:
    Flags = n ? (Flags | F_ROOM_PREVENT_TELEPORT_FROM) :
      (Flags & (~F_ROOM_PREVENT_TELEPORT_FROM));
    break;
  case ROOM_WD:
    outdoorness = y;
    break;
  case ROOM_PREVENT_WHERE:
    Flags = n ? (Flags | F_ROOM_PREVENT_WHERE) :
      (Flags & (~F_ROOM_PREVENT_WHERE));
    break;
  case ROOM_ISOLATED:
    Flags = n ? (Flags | F_ROOM_ISOLATED) :
      (Flags & (~F_ROOM_ISOLATED));
    break;
  case ROOM_GUILD:
    Flags = n ? (Flags | F_ROOM_GUILD_ROOM) : (Flags & (~F_ROOM_GUILD_ROOM));
    break;
  case ROOM_NIGHT_LONG_DESC:
    night_long_desc = y; break;
  case ROOM_LONG_DESC:
    long_desc = y; break;
  case ROOM_SHORT_DESC:
    short_desc = y; break;
  default:
    if (x == ROOM_FLAGS && intp(y)) {
      Flags = y;
      return 1;
    }
    return 0;
  }
  return 1;
}

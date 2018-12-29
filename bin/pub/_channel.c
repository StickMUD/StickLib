// /bin/pub/_channel.c
// Copyright � 1997 Niilo Paasivirta

#include <channels.h>
#include <coder_levels.h>
#include <daemons.h>

#define WRITE me->tell_me

#ifndef TP
#define TP this_player()
#endif

nomask int
usage()
{
  TP->tell_me("Usage: " + query_verb() +
	      " <channel name/check> <on/off/send/togglelist/hist/more/ban> [message/name]\n");
  return 1;
}

nomask int
channel_cmd(string arg, object me)
{
  object ob;
  string ch, cmd, msg, tmp, *tmp2;
  int i, s, z, x, p, npc;

  p = (int)me->query_coder_level();
  npc = (int)me->is_npc();

#if 0
  if (!p && !me->query_testplayer())
    {
      WRITE("Sorry, this command is still under development. If you wanted \
the lord channel log, the command for that is now 'channellog'.");
      return 1;
    }
#endif

  if (!arg || lower_case(arg) == "list")
    {
      tmp2 = (string *)CHANNEL_D->query_channel_names();
      s = sizeof(tmp2);
      for (i = 0; i < s; i++)
	{
	  z = (int)me->query_channel_status(tmp2[i]);
	  x = CHANNEL_D->can_read_channel(TP, tmp2[i]);

	  if (!x && !p) { tmp2[i] = 0; continue; }

	  tmp2[i] = sprintf(" %-8s : %8s  %s   %s",
			    tmp2[i],
			    ((z & CHANNEL_BAN) ? " Banned " :
			     (!x || (z & CHANNEL_OFF))
			     ? "   Off  " : "   On   "),
			    (x ? "Available" : "Unavailable"),
			    (tmp2[i] == "flord" ?
			     "{FLord} is the finnish lord channel." :
			     ""));
	}

      tmp2 -= ({ 0 });

      if (sizeof(tmp2) < 1)
	{
	  WRITE("There are no channels available for you.");
	  return 1;
	}

      tmp2 = ({ " The current channels are:",
	sprintf(" %-8s : %8s  %s", "Channel", " Status ", "Availability"),
	"---------------------------------",
      }) + tmp2;
      me->more(tmp2);
      return 1;
    }

  if (arg == "check")
    {
      CHANNEL_D->check_channels(me);
      WRITE("Ok. Channels checked.");
      return 1;
    }

  if ((sscanf(arg, "%s %s %s", ch, cmd, msg) != 3) &&
      (sscanf(arg, "%s %s", ch, cmd) != 2))
    return usage();

  cmd = lower_case(cmd);

  if (member(({ "ban", "on", "off", "toggle",
			"send", "list", "who",
			"hist", "tail", "more" }), cmd) == -1)
    {
      WRITE("Only these channel commands are available at the moment: \
on, off, toggle, send, list, who, hist, tail, more and ban.");
      return 1;
    }

  if (cmd == "ban")
    {
      if (!me->query_coder_level() &&
	  !CHANNEL_D->query_is_channel_admin(me, ch))
	{
	  WRITE("You are not privileged to ban anyone from channel \"" +
		ch + "\".");
	  return 1;
	}

      ob = find_player(msg);
      if (!ob)
	{
	  WRITE("There is no player \"" + msg + "\" logged in.");
	  return 1;
	}

      i = ((int)ob->query_channel_status(ch) & CHANNEL_BAN);
      ob->set_channel_ban(ch, !i);

      WRITE("Ok. " + (!i ? "Banned" : "Unbanned") + " " +
	    capitalize(msg) + " from channel \"" + ch + "\".");
      return 1;
    }

  if (!CHANNEL_D->can_read_channel(me, ch) ||
      me->query_channel_ban(ch))
    {
      WRITE("You can't access channel \"" + ch + "\".");
      return 1;
    }

  if (cmd == "list" || cmd == "who")
    {
      me->more((string *)CHANNEL_D->list_channel(ch, me));
      return 1;
    }

  if (cmd == "more")
    {
      me->more((string *)CHANNEL_D->more_channel(ch));
      return 1;
    }

  if (cmd == "hist" || cmd == "tail")
    {
      CHANNEL_D->tail_channel(ch, me);
      return 1;
    }

  if (cmd == "on")
    {
      if (me->is_on_channel(ch))
	{
	  WRITE("You already are on channel \"" + ch + "\".");
	  return 1;
	}

      CHANNEL_D->join_channel(ch, me);
      me->channel_on(ch);
      WRITE("Ok. You are on channel \"" + ch + "\".");
      return 1;
    }

  if (cmd == "toggle")
    {
      i = me->is_on_channel(ch);

      if (i) me->channel_off(ch);
      else me->channel_on(ch);

      if (i && npc) CHANNEL_D->remove_from_channel(ch, me);

      WRITE("Ok. You are " + (i ? "off" : "on") +
	    " channel \"" + ch + "\".");
      return 1;
    }

  if (cmd == "off")
    {
      me->channel_off(ch);
      if (npc) CHANNEL_D->remove_from_channel(ch, me);
      WRITE("Ok. You are off channel \"" + ch + "\".");
      return 1;
    }

  if (!me->is_on_channel(ch))
    {
      WRITE("You are not on the channel called \"" + ch + "\".");
      return 1;
    }

  if (sizeof(msg) < 1)
    {
      // Empty msg: list channel.
      me->more((string *)CHANNEL_D->list_channel(ch, me));
      return 1;
    }

  if (stringp(tmp = (string)CHANNEL_D->valid_channel_message(msg)))
    {
      WRITE(tmp);
      return 1;
    }

  if (!CHANNEL_D->send_channel_message(me, ch, msg))
    WRITE("Couldn't send message!");

  return 1;
}

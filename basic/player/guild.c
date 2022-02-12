/****************************************************************
*								*
*	Part of the living.c handling all the data that has to	*
*	do with living belonging to a guild; guild name,	*
*	possible guild object, guild save and alike.		*
*								*
*	Version 2.2 24-jan-96					*
*								*
*	Last changes 22-Jun-97 by Graah				*
*								*
*	NEW:							*
*								*
* - Guild object can also give list of commands for command	*
*   driver; they will be called by command driver without need	*
*   for add_actions. Please _do_ use this method as it allows	*
*   nice things later on... Also, this is more efficient of two.*
*								*
* - The Guildless Year is coming... /Graah			*
*								*
****************************************************************/



#include <guild.h>

#ifndef PLAYER_C
#ifndef LIVING_C

string guild;
static object guild_object;
static int GuildFlags;
static mapping GuildHooks;
static closure _busy;

varargs void tell_me(string s, status a, status b);
#endif
#endif

// Some variables defined in /basic/living/guild.c!
string guild_object_name;	// Filename of our guild object.
// This is cloned when player logs in.
static mapping guild_commands;	// Commands guild object has.
string guild_save;		// Data guild object wants to save.
// Can be other than string too.
static int guild_melee_damage_rate;	// How well this guild does damage...
static int guild_ac_rate;		// Does this guild add to ac rate?

/****************************************************************
*								*
*			Query-functions.			*
*								*
****************************************************************/

public mixed
query_guild_save()
{
    return guild_save;
}

public object
query_guild_object()
{
    return guild_object;
}

public string
query_guild_object_name()
{
    return guild_object_name;
}

mapping
query_guild_commands()
{
    return guild_commands;
}

/****************************************************************
*								*
*			Set-functions.				*
*								*
****************************************************************/

public void
set_guild(string g)
{
    if (!(guild = g)) {
	guild_melee_damage_rate = guild_ac_rate = guild_object
	= guild_save = guild_object_name = GuildFlags =
	GuildHooks = 0;
    }
}

public void
set_guild_save(mixed s)
{
    guild_save = s;
}

public void
set_guild_object(object ob)
{
    guild_object = ob;
}

public void
set_guild_object_name(string s)
{
    guild_object_name = s;
}

void
set_guild_commands(mapping m)
{
    if (!m || mappingp(m))
	guild_commands = m;
}

// With this you can set either a single hook or all hooks!
// (set_guild_hook(G_HOOK_QUIT, #'member_quit_func) is example of first)
varargs void
set_guild_hook(mixed a, mixed b)
{
    int i, *x;
    mixed *y;

#ifdef GUILDLESS
    // Testplayers and coders are still in guild.
    if (!this_object()->query_coder_level() &&
      !this_object()->query_is_testplayer())
	return;
#endif

    if (mappingp(a)) {
	GuildHooks = ([ ]);
	x = m_indices(a);
	y = m_values(a);
	i = sizeof(x);
	while (--i >= 0) {
	    if (y[i]) {
		GuildFlags |= (1 << x[i]);
		GuildHooks[x[i]] = y[i];
	    } else {
		GuildFlags &= (~(1 << x[i]));
		GuildHooks[x[i]] = 0;
	    }
	}
    } else {
	if (!intp(a)) return;
	if (!a) {
	    GuildFlags = GuildHooks = 0;
	    return;
	}
	if (!b)
	    GuildFlags &= (~(1 << a));
	else
	    GuildFlags |= (1 << a);
	GuildHooks[a] = b;
    }

    // We need to update this too.  -+ 10-mar-95, Doomdark +-
    _busy = GuildHooks[G_HOOK_PREVENT_ATTACK];
}

/****************************************************************
*								*
*	Functions called by player.c to determine various	*
*	things about our guild...				*
*								*
****************************************************************/

// Guild damage bonus/penalty with melee combat, percent.
// 100: none, min. 50, max 150

public int
guild_melee_damage_rate(int dmg)
{
    object gob;
    int r;

#ifdef GUILDLESS
    // Testplayers and coders are still in guild.
    if (!this_object()->query_coder_level() &&
      !this_object()->query_is_testplayer())
	return dmg;
#endif

    if (guild_melee_damage_rate) return guild_melee_damage_rate * dmg / 100;
    return dmg;
}

/****************************************************************
*								*
*		Guild object initialization and saving.		*
*								*
****************************************************************/

void
initialize_guild()
{
    string s;

    if (!stringp(guild_object_name)) return;
    if ((s = catch(guild_object = clone_object(guild_object_name))) ||
      !guild_object) {
	tell_me(
	  "Error in initializing your guild object. Please report this to either\
 your guild coder, or (co)admins."+(s ? ("\nError code: \""+s+"\"") : ""));
    } else {
	// UGH! Have to be paranoid for buggy init() and initialize_guild()s... :-/
	if (catch(move_object(guild_object, this_object())) ||
	  catch(guild_object->initialize_guild(guild_save))) {
	    tell_me("ERROR: Problems with initializing guild object!");
	} else {
	    guild_commands = (mapping) guild_object -> query_guild_commands();
	}
    }
}

void
save_guild(int msg)
{
    if (guild_object) {
	guild_save = (string) guild_object -> query_guild_save();
	if (msg) tell_me("Saving guild data.");
    }
}

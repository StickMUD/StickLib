/****************************************************************
*								*
*			Guild daemon				*
*								*
* Made by: Doomdark						*
* Version 1.0 by Doomdark 12-nov-95.				*
* Last change:	28-nov-95					*
*								*
* This daemon handles saving/restoring additional guild specific*
* data for players. In practice, this might be saving old guild	*
* skills for player when he/she resigns from the guild or	*
* perhaps saving backup data occasionally.			*
* Note that this should _NOT_ be used for 'normal' guild data	*
* saving; these should be saved along with player object via	*
* functions "set_guild_data" (in player.c) and "initialize_guild"
* (in guild objects).						*
*								*
* IDEAS:							*
*								*
* - Should internal cache help in efficiency? Perhaps not?	*
* - How about changing "whose_data" to non-static (ie. saveable)*
*   variable? This way, we could have a simple method of	*
*   checking whether this is the "right" savefile?		*
*								*
****************************************************************/

// Directory, to where savefiles should be located:
#define	GUILD_SAVE	"/data/guild/all/"

// This mapping contains data to be saved; index is the name of
// the guild of which data is to be saved and the value can be
// any LPC-datatype (of course, objects won't get saved etc).
// Note that if some guild wants to have more than 1 distinct
// save items, it should use indexes like "priest:1", "priest:2"
// etc. (just recommendation). If one data item is enough,
// use guild names defined in <guild.h> (/include/guild.h);
// GN_NINJA (currently, "ninja"), GN_PRIEST (- " - "priest")
// and so on.

mapping save_data;

static string whose_data;

status
save_guild_data(string plr_name, string guild, mixed data)
{
	if (!guild || !plr_name) return 0;
	if (whose_data != plr_name) {
		save_data = 0;
		whose_data = plr_name;
		restore_object(GUILD_SAVE + plr_name);
	}
	if (!save_data) save_data = ([ guild : data ]);
	else save_data[guild] = data;
	save_object(GUILD_SAVE + plr_name);
}

mixed
restore_guild_data(string plr_name, string guild)
{
	if (whose_data != plr_name) {
		whose_data = plr_name;
		restore_object(GUILD_SAVE + plr_name);
	}
	if (!save_data) return 0;
	return save_data[guild];
}

// Basic version of the guild module. Used by NPCs; player.c overrides
// some of the definitions.
// If you need complete version, include it from /basic/player.
// In that case, you need player version of command_driver.c too,
// most probably.

string guild;
static object guild_object;

string
query_guild()
{
    return guild;
}

void
set_guild(string s)
{
    guild = s;
}

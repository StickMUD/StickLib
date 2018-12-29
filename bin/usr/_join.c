/*****************************************************************************
  join <guild> 
  Coded by: Germ
  Last modified: 12/13/97 //Chopin

*****************************************************************************/
#include <guild.h>

mapping guild_marks;

/*************************************************/

void
create() {
    guild_marks = ([
#ifdef GN_PRIEST
	GN_PRIEST: PRIEST_MARK_PATH,
#endif
#ifdef GN_MAGE
	GN_MAGE: MAGE_MARK_PATH,
#endif
#ifdef GN_THIEF
	GN_THIEF: THIEF_MARK_PATH,
#endif
#ifdef GN_NINJA
	GN_NINJA: NINJA_MARK_PATH,
#endif
#ifdef GN_FIGHTER
	GN_FIGHTER: FIGHTER_MARK_PATH,
#endif
#ifdef GN_NECROMANCER
	GN_NECROMANCER: NECRO_MARK_PATH,
#endif
#ifdef GN_HEALER
	GN_HEALER: HEALER_MARK_PATH,
#endif
#ifdef GN_WARRIOR
	GN_WARRIOR: WARRIOR_MARK_PATH,
#endif
   ]);
}


status join_cmd( string guild )
{
    string old_guild;
    object mark;

    if(!this_player()->query_coder_level()) 
	return 0;

    if(!member(guild_marks, guild)) {
	notify_fail(sprintf("Valid guilds are: %s\n",
	  implode(m_indices(guild_marks), " ")));
	return 0;
    }

    /** resign current guild **/

    old_guild = this_player()->query_guild();
    
    //No autoresign for Fighters!
#ifdef GN_FIGHTER
    if(old_guild == GN_FIGHTER) {
	notify_fail("You have to leave the Fighters' guild first.\n");
	return 0;
    }
#endif

    if(mark = this_player()->query_guild_object()) {
	this_player()->tell_me("You leave the "+old_guild+" guild."); 
	destruct(mark);
    }

    this_player()->set_guild_commands( 0 );
    this_player()->set_guild_save(0);
    this_player()->set_guild_hook( ([]) );


    /** join new guild **/

    this_player()->set_guild(guild);
    this_player()->set_guild_object_name(guild_marks[guild]);
    this_player()->initialize_guild();
    this_player()->tell_me("You join the "+guild+" guild.");

    return 1;
}

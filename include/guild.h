#ifndef _GUILD_H
#define	_GUILD_H

/* Note that as the StickLib-distribution only contains one of
 * the guilds StickMud has; the priest guild. Thus, most
 * guild defines here are commented out. They are left in comments
 * here though just to show what could be in here, if some old
 * code should need them.
 */

// Define to begin a Guildless Time /Graah
#undef GUILDLESS

// The standard header-file defining the names of the guilds
// (used by set_guild & query_guild).

//#define GN_NINJA	"ninja"
#define	GN_PRIEST	"priest"
//#define GN_MAGE		"mage"
//#define GN_FIGHTER	"fighter"
#define GN_THIEF	"thief"
//#define GN_NECROMANCER	"necromancer"
//#define GN_HEALER	"healer"
// Dancers are Shadowchaser specific and should not be included in sticklib, as
// StickMUD has never heard of them- Starks
#define GN_DANCER	"dancer"
/* Grin. Sometimes we just need to set _some_ guild... */
#define	GN_ANY		"priest"
/* And we might also need to define a 'special' "no-guild" entry,
 * but so that it won't be considered as "someone forgot to set
 * the guild variable". Thus:
 */
#define	GN_NONE		"no-guild"

//#define GUILD_PATH_NINJA	"/guilds/ninja/"
#define	GUILD_PATH_PRIEST	"/guilds/priests/"
//#define GUILD_PATH_MAGE		"/guilds/mage/"
//#define GUILD_PATH_FIGHTER	"/guilds/fighter/"
#define GUILD_PATH_THIEF	"/guilds/thieves/"
//#define GUILD_PATH_NECROMANCER	"/guilds/necromancer/"
//#define GUILD_PATH_HEALER	"/guilds/healer/"

// Hooks guilds can set:

#define	G_HOOK_QUIT		0
#define	G_HOOK_DEATH		1
#define	G_HOOK_GO_LD		2
#define	G_HOOK_RECOVER_FROM_LD	3
#define	G_HOOK_HIT_PLAYER	4
#define	G_HOOK_GO_MORT		5
#define	G_HOOK_ADD_EXP		6
#define	G_HOOK_PREVENT_ATTACK	7
#define	G_HOOK_LWIELD_WEAPON 	8
#define	G_HOOK_WIELD_WEAPON	9
#define	G_HOOK_REMOVE_WEAPON	10
#define	G_HOOK_PRIMARY_ATTACK	11
#define	G_HOOK_SECONDARY_ATTACK	12

/** locations of guild marks **/

#define PRIEST_MARK_PATH "/guilds/priests/symbol"
//#define NINJA_MARK_PATH "/u/ninja/mark_new"
#define THIEF_MARK_PATH "/guilds/thieves/thief_mark_new"
//#define NECRO_MARK_PATH "/guilds/necromancer/amulet"
//#define FIGHTER_MARK_PATH "/u/frobozz/F/mark"
//#define MAGE_MARK_PATH "/u/mage/mark"
//#define HEALER_MARK_PATH "/u/healers/symbol"

#endif

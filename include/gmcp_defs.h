#ifndef GMCP_DEFS_H
#define GMCP_DEFS_H

// This is used in the login and player objects.
#define GMCP_CLIENT_LIST_IAC_GA                         ({"Mudlet", "Grapevine"})

#define GMCP_PKG_CLIENT_GUI                             "Client.GUI"
#define GMCP_KEY_CLIENT_GUI_VERSION                     "1"
#define GMCP_KEY_CLIENT_GUI_URL                         "url"
#define GMCP_VALUE_CLIENT_GUI_URL                       "https://raw.githubusercontent.com/StickMUD/StickMUDMudletGUI/master/StickMUD.mpackage"
#define GMCP_PKG_CLIENT_MAP                             "Client.Map"
#define GMCP_KEY_CLIENT_MAP_URL                         "url"
#define GMCP_VALUE_CLIENT_MAP_URL                       "https://raw.githubusercontent.com/StickMUD/StickMUDOfficialmap/master/maps/map.xml"
#define GMCP_PKG_CORE					"Core"
#define GMCP_PKG_CORE_HELLO				"Core.Hello"
#define GMCP_KEY_CORE_HELLO_CLIENT			"client"
#define GMCP_KEY_CORE_HELLO_VERSION			"version"
#define GMCP_PKG_CORE_SUPPORTS				"Core.Supports"
#define GMCP_PKG_CORE_SUPPORTS_SET			"Core.Supports.Set"
#define GMCP_PKG_CORE_SUPPORTS_ADD			"Core.Supports.Add"
#define GMCP_PKG_CORE_SUPPORTS_REMOVE			"Core.Supports.Remove"
#define GMCP_PKG_CORE_KEEPALIVE				"Core.KeepAlive"
#define GMCP_PKG_CORE_PING				"Core.Ping"
#define GMCP_PKG_CORE_GOODBYE				"Core.Goodbye"
#define GMCP_VALUE_CORE_GOODBYE				"Goodbye, adventurer"
#define GMCP_PKG_EXTERNAL_DISCORD_GET			"External.Discord.Get"
#define GMCP_PKG_EXTERNAL_DISCORD_HELLO			"External.Discord.Hello"
#define GMCP_PKG_EXTERNAL_DISCORD_INFO			"External.Discord.Info"
#define GMCP_KEY_EXTERNAL_DISCORD_INFO_INVITEURL	"inviteurl"
#define GMCP_KEY_EXTERNAL_DISCORD_INFO_APPLICATIONID	"applicationid"
#define GMCP_VALUE_EXTERNAL_DISCORD_INFO_APPLICATIONID	"<bunch of numbers>" // Update!
#define GMCP_PKG_EXTERNAL_DISCORD_STATUS		"External.Discord.Status"
#define GMCP_KEY_EXTERNAL_DISCORD_STATUS_GAME		"game"
#define GMCP_VALUE_EXTERNAL_DISCORD_STATUS_GAME		"Kandawen"
#define GMCP_KEY_EXTERNAL_DISCORD_STATUS_STARTTIME	"starttime"
#define GMCP_VALUE_EXTERNAL_DISCORD_INFO_INVITEURL	"https://discord.gg/<fill in>" // Update
#define GMCP_PKG_CHAR					"Char"
#define GMCP_PKG_CHAR_LOGIN				"Char.Login"
#define GMCP_KEY_CHAR_LOGIN_NAME			"name"
#define GMCP_KEY_CHAR_LOGIN_PASSWORD			"password"
#define GMCP_PKG_CHAR_NAME				"Char.Name"
#define GMCP_KEY_CHAR_NAME_NAME				"name"
#define GMCP_KEY_CHAR_NAME_FULLNAME			"fullname"
#define GMCP_PKG_CHAR_VITALS				"Char.Vitals"
#define GMCP_KEY_CHAR_VITALS_HP				"hp"
#define GMCP_KEY_CHAR_VITALS_MAXHP			"maxhp"
#define GMCP_KEY_CHAR_VITALS_SP				"sp"
#define GMCP_KEY_CHAR_VITALS_MAXSP			"maxsp"
#define GMCP_KEY_CHAR_VITALS_FP				"fp"
#define GMCP_KEY_CHAR_VITALS_MAXFP			"maxfp"
#define GMCP_KEY_CHAR_VITALS_MP				"mp"
#define GMCP_KEY_CHAR_VITALS_MAXMP			"maxmp"
#define GMCP_KEY_CHAR_VITALS_EP				"ep"
#define GMCP_KEY_CHAR_VITALS_MAXEP			"maxep"
#define GMCP_KEY_CHAR_VITALS_WP				"wp"
#define GMCP_KEY_CHAR_VITALS_MAXWP			"maxwp"
#define GMCP_KEY_CHAR_VITALS_NL				"nl"
#define GMCP_KEY_CHAR_VITALS_STRING			"string"
#define GMCP_PKG_CHAR_STATS				"Char.Stats"
#define GMCP_KEY_CHAR_STATS_CON				"con"
#define GMCP_KEY_CHAR_STATS_INT				"int"
#define GMCP_KEY_CHAR_STATS_DEX				"dex"
#define GMCP_KEY_CHAR_STATS_STR				"str"
#define GMCP_KEY_CHAR_STATS_FAT				"fat"
#define GMCP_PKG_CHAR_MAXSTATS				"Char.MaxStats"
#define GMCP_KEY_CHAR_MAXSTATS_MAXCON			"maxcon"
#define GMCP_KEY_CHAR_MAXSTATS_MAXINT			"maxint"
#define GMCP_KEY_CHAR_MAXSTATS_MAXDEX			"maxdex"
#define GMCP_KEY_CHAR_MAXSTATS_MAXSTR			"maxstr"
#define GMCP_KEY_CHAR_MAXSTATS_MAXFAT			"maxfat"
#define GMCP_PKG_CHAR_TRAINING_LIST			"Char.Training.List"
#define GMCP_KEY_CHAR_TRAINING_LIST_SKILL		"skill"
#define GMCP_KEY_CHAR_TRAINING_LIST_NAME		"name"
#define GMCP_KEY_CHAR_TRAINING_LIST_RANK		"rank"
#define GMCP_KEY_CHAR_TRAINING_LIST_PERCENT		"percent"
#define GMCP_PKG_CHAR_SESSION_TRAINING			"Char.Session.Training"
#define GMCP_KEY_CHAR_SESSION_TRAINING_SKILL		"skill"
#define GMCP_KEY_CHAR_SESSION_TRAINING_NAME		"name"
#define GMCP_KEY_CHAR_SESSION_TRAINING_PERCENT		"percent"
#define GMCP_PKG_CHAR_STATUS				"Char.Status"
#define GMCP_PKG_CHAR_STATUSVARS			"Char.StatusVars"
#define GMCP_KEY_CHAR_STATUS_NAME			"name"
#define GMCP_KEY_CHAR_STATUS_FULLNAME			"fullname"
#define GMCP_KEY_CHAR_STATUS_AGE			"age"
#define GMCP_KEY_CHAR_STATUS_SESSION_LOGIN		"session_login"
#define GMCP_KEY_CHAR_STATUS_SESSION_CONNECTED		"session_connected"
#define GMCP_KEY_CHAR_STATUS_SESSION_XP			"session_xp"
#define GMCP_KEY_CHAR_STATUS_SESSION_KILLS		"session_kills"
#define GMCP_KEY_CHAR_STATUS_SESSION_ROOMS		"session_rooms"
#define GMCP_KEY_CHAR_STATUS_RACE			"race"
#define GMCP_KEY_CHAR_STATUS_SPECIALISATION		"specialisation"
#define GMCP_KEY_CHAR_STATUS_LEVEL			"level"
#define GMCP_KEY_CHAR_STATUS_XP				"xp"
#define GMCP_KEY_CHAR_STATUS_XPRANK			"xprank"
#define GMCP_KEY_CHAR_STATUS_CLASS			"class"
#define GMCP_KEY_CHAR_STATUS_CITY			"city"
#define GMCP_KEY_CHAR_STATUS_HOUSE			"house"
#define GMCP_KEY_CHAR_STATUS_ORDER			"order"
#define GMCP_KEY_CHAR_STATUS_LESSONS			"lessons"
#define GMCP_KEY_CHAR_STATUS_EXPLORERRANK		"explorerank"
#define GMCP_KEY_CHAR_STATUS_GOLD			"gold"
#define GMCP_KEY_CHAR_STATUS_BANK			"bank"
#define GMCP_KEY_CHAR_STATUS_QP				"qp"
#define GMCP_KEY_CHAR_STATUS_UNREAD_MSGS		"unread_msgs"
#define GMCP_KEY_CHAR_STATUS_UNREAD_NEWS		"unread_news"
#define GMCP_KEY_CHAR_STATUS_GENDER			"gender"
#define GMCP_KEY_CHAR_STATUS_ENEMY			"enemy"
#define GMCP_KEY_CHAR_STATUS_ENEMY_HEALTH		"enemy_health"
#define GMCP_KEY_CHAR_STATUS_TITLE			"title"
#define GMCP_KEY_CHAR_STATUS_NL				"nl"
#define GMCP_KEY_CHAR_STATUS_DEAD			"dead"
#define GMCP_KEY_CHAR_STATUS_BRAVE			"brave"
#define GMCP_KEY_CHAR_STATUS_DRUNK			"drunk"
#define GMCP_KEY_CHAR_STATUS_HUNGER			"hunger"
#define GMCP_KEY_CHAR_STATUS_THIRST			"thirst"
#define GMCP_KEY_CHAR_STATUS_DISGUISE			"disguise"
#define GMCP_KEY_CHAR_STATUS_ALIGN			"align"
#define GMCP_KEY_CHAR_STATUS_STATE			"state"
#define GMCP_KEY_CHAR_STATUS_SIT			"sit"
#define GMCP_KEY_CHAR_STATUS_REST			"rest"
#define GMCP_KEY_CHAR_STATUS_FROG			"frog"
#define GMCP_KEY_CHAR_STATUS_INVIS			"invis"
#define GMCP_KEY_CHAR_STATUS_SUMMONABLE			"summonable"
#define GMCP_KEY_CHAR_STATUS_PKABLE			"pkable"
#define GMCP_KEY_CHAR_STATUS_DRAINCON			"draincon"
#define GMCP_KEY_CHAR_STATUS_DRAININT			"drainint"
#define GMCP_KEY_CHAR_STATUS_DRAINDEX			"draindex"
#define GMCP_KEY_CHAR_STATUS_DRAINSTR			"drainstr"
#define GMCP_KEY_CHAR_STATUS_AMULET			"amulet"
#define GMCP_KEY_CHAR_STATUS_ARMOUR			"armour"
#define GMCP_KEY_CHAR_STATUS_BELT			"belt"
#define GMCP_KEY_CHAR_STATUS_BOOTS			"boots"
#define GMCP_KEY_CHAR_STATUS_BRACERS			"bracers"
#define GMCP_KEY_CHAR_STATUS_CLOAK			"cloak"
#define GMCP_KEY_CHAR_STATUS_GIRDLE			"girdle"
#define GMCP_KEY_CHAR_STATUS_GLOVES			"gloves"
#define GMCP_KEY_CHAR_STATUS_HELMET			"helmet"
#define GMCP_KEY_CHAR_STATUS_LEGGINGS			"leggings"
#define GMCP_KEY_CHAR_STATUS_MASK			"mask"
#define GMCP_KEY_CHAR_STATUS_RING			"ring"
#define GMCP_KEY_CHAR_STATUS_RING2			"ring2"
#define GMCP_KEY_CHAR_STATUS_SHIELD			"shield"
#define GMCP_KEY_CHAR_STATUS_NECKLACE			"necklace"
#define GMCP_PKG_CHAR_WORTH				"Char.Worth"
#define GMCP_PKG_CHAR_CONDITIONS			"Char.Conditions"
#define GMCP_KEY_CHAR_CONDITIONS_NAME			"name"
#define GMCP_KEY_CHAR_CONDITIONS_DESC			"desc"
#define GMCP_PKG_CHAR_CONDITIONS_LIST			"Char.Conditions.List"
#define GMCP_PKG_CHAR_CONDITIONS_ADD			"Char.Conditions.Add"
#define GMCP_PKG_CHAR_CONDITIONS_REMOVE			"Char.Conditions.Remove"
#define GMCP_PKG_CHAR_RESISTANCES			"Char.Resistances"
#define GMCP_KEY_CHAR_RESISTANCES_NAME			"name"
#define GMCP_KEY_CHAR_RESISTANCES_DESC			"desc"
#define GMCP_PKG_CHAR_RESISTANCES_LIST			"Char.Resistances.List"
#define GMCP_PKG_CHAR_RESISTANCES_ADD			"Char.Resistances.Add"
#define GMCP_PKG_CHAR_RESISTANCES_REMOVE		"Char.Resistances.Remove"
#define GMCP_PKG_CHAR_AFFLICTIONS			"Char.Afflictions"
#define GMCP_KEY_CHAR_AFFLICTIONS_NAME			"name"
#define GMCP_KEY_CHAR_AFFLICTIONS_CURE			"cure"
#define GMCP_KEY_CHAR_AFFLICTIONS_DESC			"desc"
#define GMCP_PKG_CHAR_AFFLICTIONS_LIST			"Char.Afflictions.List"
#define GMCP_PKG_CHAR_AFFLICTIONS_ADD			"Char.Afflictions.Add"
#define GMCP_PKG_CHAR_AFFLICTIONS_REMOVE		"Char.Afflictions.Remove"
#define GMCP_PKG_CHAR_DEFENCES				"Char.Defences"
#define GMCP_KEY_CHAR_DEFENCES_NAME			"name"
#define GMCP_KEY_CHAR_DEFENCES_DESC			"desc"
#define GMCP_PKG_CHAR_DEFENCES_LIST			"Char.Defences.List"
#define GMCP_PKG_CHAR_DEFENCES_ADD			"Char.Defences.Add"
#define GMCP_PKG_CHAR_DEFENCES_REMOVE			"Char.Defences.Remove"
#define GMCP_PKG_CHAR_ITEMS				"Char.Items"
#define GMCP_KEY_CHAR_ITEMS_LOCATION			"location"
#define GMCP_VALUE_CHAR_ITEMS_LOCATION_INV		"inv"
#define GMCP_VALUE_CHAR_ITEMS_LOCATION_ROOM		"room"
#define GMCP_VALUE_CHAR_ITEMS_LOCATION_CONTAINER	"%d"
#define GMCP_KEY_CHAR_ITEMS_ITEM			"item"
#define GMCP_KEY_CHAR_ITEMS_ITEMS			"items"
#define GMCP_KEY_CHAR_ITEMS_ITEMS_ID			"id"
#define GMCP_KEY_CHAR_ITEMS_ITEMS_NAME			"name"
#define GMCP_KEY_CHAR_ITEMS_ITEMS_ICON			"icon"
#define GMCP_KEY_CHAR_ITEMS_ITEMS_ATTRIB		"attrib"
#define GMCP_PKG_CHAR_ITEMS_CONTENTS			"Char.Items.Contents"
#define GMCP_PKG_CHAR_ITEMS_INV				"Char.Items.Inv"
#define GMCP_PKG_CHAR_ITEMS_ROOM			"Char.Items.Room"
#define GMCP_PKG_CHAR_ITEMS_LIST			"Char.Items.List"
#define GMCP_PKG_CHAR_ITEMS_ADD				"Char.Items.Add"
#define GMCP_PKG_CHAR_ITEMS_REMOVE			"Char.Items.Remove"
#define GMCP_PKG_CHAR_ITEMS_UPDATE			"Char.Items.Update"
#define GMCP_PKG_CHAR_SKILLS				"Char.Skills"
#define GMCP_KEY_CHAR_SKILLS_GROUP			"group"
#define GMCP_KEY_CHAR_SKILLS_NAME			"name"
#define GMCP_KEY_CHAR_SKILLS_RANK			"rank"
#define GMCP_KEY_CHAR_SKILLS_LIST			"list"
#define GMCP_KEY_CHAR_SKILLS_SKILL			"skill"
#define GMCP_KEY_CHAR_SKILLS_DESC			"desc"
#define GMCP_KEY_CHAR_SKILLS_INFO			"info"
#define GMCP_PKG_CHAR_SKILLS_GET			"Char.Skills.Get"
#define GMCP_PKG_CHAR_SKILLS_GROUPS			"Char.Skills.Groups"
#define GMCP_PKG_CHAR_SKILLS_LIST			"Char.Skills.List"
#define GMCP_PKG_CHAR_SKILLS_INFO			"Char.Skills.Info"
#define GMCP_PKG_COMM_CHANNEL				"Comm.Channel"
#define GMCP_KEY_COMM_CHANNEL_NAME			"name"
#define GMCP_KEY_COMM_CHANNEL_CAPTION			"caption"
#define GMCP_KEY_COMM_CHANNEL_COMMAND			"command"
#define GMCP_KEY_COMM_CHANNEL_CHANNEL			"channel"
#define GMCP_KEY_COMM_CHANNEL_CHANNELS			"channels"
#define GMCP_KEY_COMM_CHANNEL_TALKER			"talker"
#define GMCP_KEY_COMM_CHANNEL_TEXT			"text"
#define GMCP_PKG_COMM_CHANNEL_PLAYERS			"Comm.Channel.Players"
#define GMCP_PKG_COMM_CHANNEL_ENABLE			"Comm.Channel.Enable"
#define GMCP_PKG_COMM_CHANNEL_LIST			"Comm.Channel.List"
#define GMCP_PKG_COMM_CHANNEL_START			"Comm.Channel.Start"
#define GMCP_PKG_COMM_CHANNEL_END			"Comm.Channel.End"
#define GMCP_PKG_COMM_CHANNEL_TEXT			"Comm.Channel.Text"
#define GMCP_PKG_GROUP					"Group"
#define GMCP_KEY_GROUP_GROUPNAME			"groupname"
#define GMCP_KEY_GROUP_LEADER				"leader"
#define GMCP_KEY_GROUP_CREATED				"created"
#define GMCP_KEY_GROUP_STATUS				"status"
#define GMCP_KEY_GROUP_COUNT				"count"
#define GMCP_KEY_GROUP_KILLS				"kills"
#define GMCP_KEY_GROUP_EXP				"exp"
#define GMCP_KEY_GROUP_MEMBERS				"members"
#define GMCP_KEY_GROUP_MEMBERS_NAME			"name"
#define GMCP_KEY_GROUP_MEMBERS_INFO			"info"
#define GMCP_KEY_GROUP_MEMBERS_INFO_HP			"hp"
#define GMCP_KEY_GROUP_MEMBERS_INFO_MAXHP		"maxhp"
#define GMCP_KEY_GROUP_MEMBERS_INFO_SP			"sp"
#define GMCP_KEY_GROUP_MEMBERS_INFO_MAXSP		"maxsp"
#define GMCP_KEY_GROUP_MEMBERS_INFO_FP			"fp"
#define GMCP_KEY_GROUP_MEMBERS_INFO_MAXFP		"maxfp"
#define GMCP_KEY_GROUP_MEMBERS_INFO_LEVEL		"lvl"
#define GMCP_KEY_GROUP_MEMBERS_INFO_HERE		"here"
#define GMCP_PKG_ROOM					"Room"
#define GMCP_KEY_ROOM_INFO_NUM				"num"
#define GMCP_KEY_ROOM_INFO_NAME				"name"
#define GMCP_KEY_ROOM_INFO_AREA				"area"
#define GMCP_KEY_ROOM_INFO_ENVIRONMENT			"environment"
#define GMCP_KEY_ROOM_INFO_COORDS			"coords"
#define GMCP_KEY_ROOM_INFO_MAP				"map"
#define GMCP_KEY_ROOM_INFO_DETAILS			"details"
#define GMCP_KEY_ROOM_INFO_EXITS			"exits"
#define GMCP_PKG_ROOM_INFO				"Room.Info"
#define GMCP_PKG_ROOM_WRONGDIR				"Room.WrongDir"
#define GMCP_PKG_ROOM_PLAYERS				"Room.Players"
#define GMCP_PKG_ROOM_ADDPLAYER				"Room.AddPlayer"
#define GMCP_PKG_ROOM_REMOVEPLAYER			"Room.RemovePlayer"
#define GMCP_PKG_REDIRECT				"Redirect"
#define GMCP_PKG_REDIRECT_WINDOW			"Redirect.Window"

#define GMCP_PACKAGES 				{\
						}

#endif /* GMCP_DEFS_H */

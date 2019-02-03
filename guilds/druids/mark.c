/**
 * Druid guild object, Version 1.
 *
 * Changes: 02/06/2005 Tamarindo: Started this file from a shell created
 *                                by Fafnir.
 */

#ifndef _GUILD_OBJECT_C
#define _GUILD_OBJECT_C

// SYSTEM DEFINITIONS ******************************************************

#include <treasure.h>
#include <guild.h>

// OWN DEFINITIONS *********************************************************

#include "include/dirs.h"
#include "include/daemons.h"
#include "include/consts.h"

// GLOBAL VARIABLES ********************************************************

private nosave object debugger;        // Is someone debugging us?
private nosave object master;          // Owner of this object
private nosave object pretitle;        // Pretitle object to mask pretitle

// Shared static variables, same for each object
static mapping guild_commands; // Guild commands

// PROTOTYPES **************************************************************

void inform_debugger(string str);

// SET, ADD and QUERY -LIKE FUNCTIONS **************************************

mapping query_guild_commands() {
    return guild_commands;
}

string query_pretitle() {
    string pretitle;
    int office;

    office = GUILD_OFFICER_D->my_rank(master->query_real_name());
    pretitle = GUILD_OFFICER_D->get_office(office);

    if (pretitle) {
	return pretitle;
    } else {
	return "";
    }
}

// Sets, or resets debugging object
int set_debugger(object who) {
    if(debugger) {
        debugger = 0;
        return 0;
    } else {
        debugger = who;
    }

    return 1;
}

// GUILD HOOKS *************************************************************

void hook_quit(object player) {
    inform_debugger("Hook G_HOOK_QUIT called.");

    // Skill backup
}

void hook_link_death(object player) {
    inform_debugger("Hook G_HOOK_GO_LD called.");
}

void hook_link_recover(object player) {
    inform_debugger("Hook G_HOOK_RECOVER_FROM_LD called.");
}

void hook_died(object player, object killer) {
    inform_debugger("Hook G_HOOK_DEATH called.");
}

int hook_moved(object player) {
    inform_debugger("Hook G_HOOK_MOVE_PLAYER called.");

    return 0;
    return ({ 1, -1 })[random(2)];
}

int hook_wield_weapon(object player, object weapon, closure wc, closure hit, closure extra_hit) {
    inform_debugger("Hook G_HOOK_WIELD_WEAPON called.");

    return 1;
}

int hook_lwield_weapon(object player, object weapon, closure wc, closure hit, closure extra_hit) {
    inform_debugger("Hook G_HOOK_LWIELD_WEAPON called.");

    return 1;
}

void hook_remove_weapon(object player, object weapon) {
    inform_debugger("Hook G_HOOK_REMOVE_WEAPON called.");
}

int hook_primary_attack(object player, object enemy, object right_weapon, object left_weapon) {
    inform_debugger("Hook G_HOOK_PRIMARY_ATTACK called.");

    return 0;
}

int hook_secondary_attack(object player, object enemy, object right_weapon, object left_weapon) {
    inform_debugger("Hook G_HOOK_SECONDARY_ATTACK called.");

    return 0;
}

int hook_prevent_attack() {
    inform_debugger("Hook G_HOOK_PREVENT_ATTACK called.");

    return 0;
}

int hook_hit_player(int damage, int damage_type, int hc, object attacker) {
    inform_debugger("Hook G_HOOK_HIT_PLAYER called.");

    return damage;
}


// INITIALIZATIONS *********************************************************

void initialize_guild(string arg) {
    master = environment();

    if (!master || !living(master)) {
	return;
    }

    // Let's set up the guild.
    master->set_guild(GN_DRUID);

    master->set_guild_hook(([
        G_HOOK_QUIT		: #'hook_quit,
        G_HOOK_DEATH            : #'hook_died,
        G_HOOK_GO_LD            : #'hook_link_death,
        G_HOOK_RECOVER_FROM_LD  : #'hook_link_recover,
        G_HOOK_MOVE_PLAYER      : #'hook_moved,
        G_HOOK_WIELD_WEAPON     : #'hook_wield_weapon,
        G_HOOK_LWIELD_WEAPON    : #'hook_lwield_weapon,
        G_HOOK_REMOVE_WEAPON    : #'hook_remove_weapon,
	G_HOOK_HIT_PLAYER	: #'hook_hit_player,
        G_HOOK_PRIMARY_ATTACK   : #'hook_primary_attack,
        G_HOOK_SECONDARY_ATTACK : #'hook_secondary_attack,
	G_HOOK_PREVENT_ATTACK   : #'hook_prevent_attack,
    ]));

    master->set_guild_commands(guild_commands);
}

// GUILD_COMMAND HANDLERS **************************************************

int guild_command(string str) {
    string cmd = "";
    string args = "";

    if (!str) {
	return notify_fail("Usage: guild <command> [arg(s)].\n"), 0;
    }

    if (sscanf(str, "%s %s", cmd, args) != 2) {
	cmd = str;
    }

    inform_debugger(sprintf("Calling command %s with args %s.",
                            cmd, args));

    return GUILD_COMMAND_D->do_cmd(master, this_object(), cmd, args);
}

// LFUNS *******************************************************************

void create_treasure() {
    // Initialization of private variables
    debugger = 0;
    master = 0;
    pretitle = 0;

    if (member_array('#', file_name(this_object())) < 0) {
	return;
    }

    set_name(GUILD_MARK_NAME);
    set_id(GUILD_MARK_IDS);
    set_short(GUILD_MARK_SHORT);
    set_long(GUILD_MARK_LONG);
    set_weight(0);
    set_value(0);

#ifdef GUILDLESS
    if(this_object()->query_coder_level()
	|| this_object()->query_testplayer()) {
#endif

	guild_commands = ([
	    "guild" : #'guild_command /*'*/
	    ]);

#ifdef GUILDLESS
    }
#endif
}

// No getting
int get() {
    return 0;
}

// No dropping
int drop() {
    return 1;
}

// No shadowing
nomask int query_prevent_shadow() {
    return 1;
}

// REST OF FUNS ************************************************************

// Easy way to handle debugging
void inform_debugger(string str) {
    if (debugger) {
	debugger->tell_me(sprintf("DEBUG: [%s] %s",
	    master->query_real_name(), str));
    }

    return;
}

#endif //_GUILD_OBJECT_C

// EOF *********************************************************************

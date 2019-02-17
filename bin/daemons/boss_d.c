/* Boss Daemon - Author: Tamarindo
 *
 * 09-Oct-09, Germ: added generate_items() which can be used to add items to
 *      any object such as a treasure chest
 * 05-Oct-11, Fafnir: Takes use of the lib includes instead of defining
 *      file operation constants locally. No need to do backing up to old
 *      logs. log_file() simul efun does it for us.
 * 25-Nov-18, Tamarindo: Added GMCP.
 *
 */
#include "/sys/files.h"

#include <daemons.h>
#include <sound.h>
#include <boss.h>
#include <player_defs.h>

#define LOG_FILE "SPECIAL_EQUIPMENT"
#define PATH "/std/obj/special"

private int directory_filter(string str);
private void log_equipment(string message);

void create() {}

private int directory_filter(string str) {
    return (strstr(str, ".") == -1
      && file_size(sprintf("%s/%s/", PATH, str)) == FSIZE_DIR) ? 1 : 0;
}

/* Log special EQ cloned, prepend with date */
private void
log_equipment(string message)
{
    log_file(LOG_FILE, sprintf("%s: %s\n", ctime(time()), message));
}

private void
send_gmcp(object equipment, object room)
{
    foreach (object you : filter(all_inventory(room), (: $1->query(LIV_IS_PLAYER) :)))
    {
	if (you->query_env("gmcp"))
	{
	    TELOPT_D->send_char_items_add(you, "room", equipment);
	}
    }
}

public void generate_special_equipment(int chance, object room)
{
    string *equipment_files, *directories;
    object equipment;

    if (random(99) + 1 <= 100 - chance) return;
    equipment_files = ({ });
    directories = get_dir(sprintf("%s/*", PATH));

    foreach (string directory : filter(directories, #'directory_filter))  //'
    {
        string *files = get_dir(sprintf("%s/%s/*.c", PATH, directory), GETDIR_PATH);
        foreach (string file : files) {
                equipment_files += ({ file[0..<3] });
        }
    }

    int size = sizeof(equipment_files);
    if (!size) return;
    while (--size && !equipment) {
        int index = random(sizeof(equipment_files));
        string err = catch(equipment = clone_object(equipment_files[index]));

        if (err != 0) {
                log_equipment(err);
        } else if (!equipment || !equipment->query_unique()) {
                equipment_files -= ({ equipment_files[index] });
        } else {
                log_equipment(sprintf("%s cloned to %s", equipment_files[index], object_name(room)));
                move_object(equipment, room);
		send_gmcp(equipment, room);
        }
    }
}

private void generate_drop(int chance, string file, object room) {
    object ob;

    if (random(101) <= chance) {
        move_object((ob = clone_object(file)), room);
	send_gmcp(ob, room);
    }
}

public void generate_items(object env, int level)
{
    int i;
    if ( !objectp(env)) return;
    switch (level) {
    case 250..349:
        generate_drop(20, LEAF_COCA, env);
        generate_drop(10, MAGIC_LOCKPICK, env);
        generate_drop(10, SCROLL_OF_TIME_DISTORTION, env); // was 5
        generate_drop(10, SCROLL_OF_LOCATION, env); // was 15
        generate_drop(10, SCROLL_OF_ABJURATION, env);
        generate_drop(15, SCROLL_OF_RECALL, env);
        generate_drop(15, SCROLL_OF_HEALING, env);
        generate_drop(15, SCROLL_ENCHANT_WEAPON, env);
        generate_drop(15, SCROLL_ENCHANT_ARMOUR, env);
        generate_drop(10, POTION_DETECT_MAGIC, env);  // was 5
        generate_drop(20, POTION_REGENERATION, env);
        generate_drop(100, POTION_HEALING, env); // Was 20
        generate_drop(35, POTION_REFRESHING, env); // was 30 thden 20
        generate_drop(35, POTION_MANA, env); // new then 20
        generate_special_equipment(30, env);

        // Falls through...
    case 150..249:
        generate_drop(15, LEAF_COCA, env);
        generate_drop(5, MAGIC_LOCKPICK, env);
        generate_drop(5, SCROLL_OF_TIME_DISTORTION, env);
        generate_drop(5, SCROLL_OF_LOCATION, env);  // was 10
        generate_drop(5, SCROLL_OF_ABJURATION, env);
        generate_drop(10, SCROLL_OF_RECALL, env);
        generate_drop(10, SCROLL_OF_HEALING, env);
        generate_drop(10, SCROLL_ENCHANT_WEAPON, env);
        generate_drop(10, SCROLL_ENCHANT_ARMOUR, env);
        generate_drop(5, POTION_DETECT_MAGIC, env);
        generate_drop(15, POTION_REGENERATION, env); // was 20
        generate_drop(25, POTION_HEALING, env); // was 20 then 15
        generate_drop(25, POTION_REFRESHING, env);  // was 30 then 15
        generate_drop(25, POTION_MANA, env);  // was 30 then 15
        generate_special_equipment(20, env);
        // Falls through...
    case 100..149:
        generate_drop(10, LEAF_COCA, env);
        generate_drop(5, SCROLL_OF_HEALING, env);
        generate_drop(5, SCROLL_ENCHANT_WEAPON, env);
        generate_drop(5, SCROLL_ENCHANT_ARMOUR, env);
        generate_drop(10, POTION_REGENERATION, env);
        generate_drop(15, POTION_HEALING, env); // was 10
        generate_drop(15, POTION_REFRESHING, env);  // was 20 then 10
        generate_drop(15, POTION_MANA, env);   // was 20 then 10
        generate_special_equipment(10, env);
        // Falls through...
    default:
        generate_drop(5, LEAF_COCA, env);  // added
        generate_drop(5, POTION_REFRESHING, env);  // added
        generate_drop(5, POTION_MANA, env);  // was 100
        generate_drop(5, POTION_HEALING, env);  // added
        generate_drop(100, POTION_REGENERATION, env);
    }
}

// Called by monster_died() in various boss monsters
public void random_drop(object npc) {
    if (!objectp(npc)) return;
    object env = environment(npc);
    generate_items(env, (int)npc->query_level());
    env->sound(SD_SOUND_LIGHTNING);
    env->tell_here(sprintf("As %s falls, there is a bright flash!", npc->query_name()));
}

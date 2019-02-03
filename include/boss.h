#ifndef _BOSS_H
#define _BOSS_H

#ifndef _AREAS_H
#include <areas.h>
#endif

#ifndef _GENERIC_ITEMS_H
#include <generic_items.h>
#endif

#define POTION_DETECT_MAGIC             AREA_MALL + "obj/p_detm"
#define POTION_REGENERATION             AREA_MALL + "obj/p_regen"
#define POTION_REFRESHING               AREA_MALL + "obj/p_refr"
#define POTION_MANA                     AREA_MALL + "obj/p_mana"
#define POTION_HEALING                  AREA_MALL + "obj/p_heal"

#define SCROLL_ENCHANT_ARMOUR           AREA_MALL + "obj/earm_scroll"
#define SCROLL_ENCHANT_WEAPON           AREA_MALL + "obj/ewpn_scroll"
#define SCROLL_ENCHANT_INSTRUMENT       AREA_MALL + "obj/eins_scroll"
#define SCROLL_OF_HEALING               "/std/obj/scroll_of_healing"
#define SCROLL_OF_RECALL                GENERIC_SCROLL_OF_RECALL
#define SCROLL_OF_TIME_DISTORTION       AREA_TRISTEZA + "special/reset_scroll"
#define SCROLL_OF_LOCATION              AREA_TRISTEZA + "special/locate_scroll"
#define SCROLL_OF_ABJURATION            AREA_TRISTEZA + "special/abjure_scroll"
#define SCROLL_OF_POISON_DETECTION      AREA_REALMS   + "shared/item/scroll_of_poison_det"
#define SCROLL_OF_CONFUSION             AREA_REALMS   + "shared/item/scroll_of_confusion"

#define LEAF_COCA                       AREA_TRISTEZA + "special/coca"
#define ROOT_MARUNA                     AREA_ISLANDS  + "herbs/root"
#define EGG_INVULN                      AREA_ISLANDS  + "obj/egg_invul.c"
#define MAGIC_LOCKPICK                  "/guilds/thieves/objs/magic_lockpick"
#define PLANT_JOJOJOPO                  AREA_FARAWAY  + "obj/jojojopo"

#endif /* _BOSS_H */

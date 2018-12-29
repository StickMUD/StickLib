// Updated 09-Sep-98, Doomdark

#ifndef ARMOUR_H
#define ARMOUR_H

#ifndef _SERVERS_H
#include <servers.h>
#endif

#define	ARM_SP_BOOST_STAT		(1 << 8)
#define	ARM_SP_DRAIN_STAT		(1 << 9)
#define	ARM_SP_CAUSE_CONDITION		(1 << 10)
#define	ARM_SP_RESIST_CONDITION		(1 << 11)

#define	ARMOUR_FILE	"/lib/armour"

#define F_ARMOUR_TYPE_NAME(x)\
  ({"amulet", "armour", "belt", "boot", "bracers",\
    "cloak", "gloves", "helmet", "leggings", "mask", "ring",\
    "shield", "necklace"}) [((x <= 12) ? x : 1)]

#define	ARMOUR_TYPE_AMULET	"amulet"
#define	ARMOUR_TYPE_BODY_ARMOUR	"armour"
#define	ARMOUR_TYPE_BELT	"belt"
#define	ARMOUR_TYPE_BOOTS	"boot"
#define	ARMOUR_TYPE_BRACERS	"bracers"
#define	ARMOUR_TYPE_CLOAK	"cloak"
#define	ARMOUR_TYPE_GLOVES	"gloves"
#define	ARMOUR_TYPE_HELMET	"helmet"
#define	ARMOUR_TYPE_LEGGINGS	"leggings"
#define	ARMOUR_TYPE_MASK	"mask"
#define	ARMOUR_TYPE_RING	"ring"
#define	ARMOUR_TYPE_SHIELD	"shield"
#define	ARMOUR_TYPE_NECKLACE	"necklace"

/* Do not use the following definitions any more: */

#define T_AMULET        0
#define T_ARMOUR        1
#define T_ARMOR         1
#define T_BELT          2
#define T_GIRDLE        2
#define T_BOOTS         3
#define T_BRACERS       4
#define T_CLOAK         5
#define T_GLOVES        6
#define T_GAUNTLETS     6
#define T_HELMET        7
#define T_HELM          7
#define T_LEGGINGS      8
#define T_MASK          9
#define T_RING          10
#define T_SHIELD        11
#define T_NECKLACE      12

#endif

#ifndef _TELL_ME_H
#define	_TELL_ME_H

#define	TELL_NO_WRAP			1
#define	TELL_TYPE_MASK	0xFFFF00

#define	TELL_TYPE_TELL			(1 << 8)
#define	TELL_TYPE_LORD_CHANNEL		(2 << 8)
#define	TELL_TYPE_GUILD_CHANNEL		(3 << 8)
#define	TELL_TYPE_HIT_BY		(4 << 8)
#define	TELL_TYPE_MISSED_BY		(5 << 8)
#define	TELL_TYPE_HITTING		(6 << 8)
#define	TELL_TYPE_MISSING		(7 << 8)
#define	TELL_TYPE_PARTY			(8 << 8)
#define	TELL_TYPE_INFO			(9 << 8)
#define	TELL_TYPE_CODER_CHANNEL		(10 << 8)
#define TELL_TYPE_RESERVED		(11 << 8)
#define TELL_TYPE_FLORD_CHANNEL		(12 << 8)
#define	TELL_TYPE_LIEGE_CHANNEL	(13 << 8)
#define	TELL_TYPE_SHOUT		(14 << 8)
#define	TELL_TYPE_ROOM_DESC	(15 << 8)
#define	TELL_TYPE_ROOM_ITEM	(16 << 8)
#define	TELL_TYPE_INVENTORY	(17 << 8)
#define	TELL_TYPE_INV_ITEM	(18 << 8)
#define TELL_TYPE_MUD_CHANNEL   (19 << 8)

#define	TELL_BEGIN		0x20000000
#define	TELL_END		0x40000000

#define	TELL_MIN_X		20
#define	TELL_MIN_Y		10
#define	TELL_STD_X		80
#define	TELL_STD_Y		25

#define	TELL_UPDATE			-1
#define	TELL_RETURN_NONE_USED		-1
#define	TELL_RETURN_CLIENT_IN_USE	-2

#endif

/****************************************************************
*								*
* Module: party_handling.c					*
* Used by: standard player object, /obj/player.c.		*
* Description: contains code handling parties; groups made by	*
*	players; allow some "party command" that are found	*
*	from /bin/pub/_party.c.					*
* Version: 1.00 26-sep-95					*
* Changes:							*
*								*
****************************************************************/

#ifndef	PLAYER_C

#include <party.h>
#include <daemons.h>
#include <invis_levels.h>

int coder_level;

void add_exp(int x);
void tell_me(string s);
int query_invis();

#endif

private static int partyId;		// Party ID
private static int partyFlags;			// Party member status, leader/member/etc...
private static int partyExp;		// Party exp gathered.
private static string partyTitle;	// Party title.

nomask mixed
query_party(int data)
{
    if (!data) return partyId;
    switch (data) {
	case PARTY_DO_LEAD:
		return (partyFlags & (PARTY_LEADER | PARTY_LEAD))
		== (PARTY_LEADER | PARTY_LEAD)
		&& !(query_invis() & (~IL_SNEAKING));
	case PARTY_TITLE:
		return partyTitle;
	case PARTY_MEMBER:
	case PARTY_LEADER:
	case PARTY_FOLLOW:
	case PARTY_LEAD:
		return partyFlags & data;
	case PARTY_ID:
		return partyId;
	case PARTY_EXP:
		return partyExp;
    }
}

nomask int
set_party(int stat, mixed data)
{
    switch (stat) {
	case PARTY_TITLE:
		partyTitle = data;
		break;
	case PARTY_MEMBER:
	case PARTY_LEADER:
	case PARTY_FOLLOW:
	case PARTY_LEAD:
		partyFlags = stat;
		break;
	case PARTY_ID:
		partyId = data;
		break;
	default:
		return 0;
    }
    return 1;
}

nomask int
add_party(int stat, mixed data)
{
    switch (stat) {
	case PARTY_DO_FOLLOW:
		if (!coder_level && (partyFlags & PARTY_FOLLOW)) {
			tell_me("[Party follow]: " + data);
			command( data);
			return 1;
		}
		return 0;
	case PARTY_MEMBER:
	case PARTY_LEADER:
	case PARTY_FOLLOW:
	case PARTY_LEAD:
		if (data < 0)
			partyFlags ^= stat;
		else if (!data) {
			partyFlags &= (~stat);
			if (stat == PARTY_LEADER)
				stat &= (~PARTY_LEAD);
			else if (stat == PARTY_MEMBER)
				stat &= (~PARTY_FOLLOW);
		} else {
			partyFlags |= stat;
			if (stat == PARTY_LEADER)
				stat &= (~(PARTY_MEMBER | PARTY_FOLLOW));
			else if (stat == PARTY_MEMBER)
				stat &= (~(PARTY_LEADER | PARTY_LEAD));
		}
		return partyFlags & stat;
	case PARTY_EXP:
		add_exp(data);
		return (partyExp += data);
    }
    return 0;
}

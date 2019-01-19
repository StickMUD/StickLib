/*
 * WRATH (was: purify)
 *
 * Purify: a spell used mainly for defense by Priests of Boccob.
 * It does damage in the casters room, plus in rooms surrounding caster.
 */

/* I think this spell does way too much damage because it hurts all the
monsters in the current room and surrounding rooms. But maybe 100 sp cost
is enough to limit its use. /Chopin
*/

#include "priest_spell.h"

void begin_wrath(object who, mixed data);

void
create_spell() {
    spell_type = SP_OFFENSIVE;
    dif = 40;
    lvl = 27;
    set_casting_flag = 1;
    cost = 75;
    spell_damage_rate = 90.0;
    skills_used = ({SK_SERENITY, SK_FORCES, SK_NATURE});
    skill_p = ({30,45,25});
    funs = ({ #'begin_wrath, 0 });
}

static string here_begin_message = 
":%<me.capname>% raises %<me.possessive>% arms over \
%<me.possessive>% head, making fists. In a loud voice, %<me.name>% calls \
upon " GOD_NAME "'s aid. A faint blue light begins to surround \
%<me.gen_possessive>% hands and grow down %<me.possessive>% arms. You feel \
the air around you begin to crackle and vibrate, as if it was being stretched \
too tight. Suddenly, %<me.name>% slams %<me.possessive>% fists together. The \
blue nimbus contracts to a pinpoint over %<me.possessive>% head. All \
strength in your body drains out as the world goes deathly silent.";

void
begin_wrath(object who, mixed data)
{
    mixed *rooms, filter;
    mapping tmp_rooms;
    object here,sym;
    int i, dmg, skill;

    who->add_sp(-cost);
    sym = who->query_guild_object();
    inc_skills(who,sym,100);
    here = HERE;
    filter = (data=="pk") ? #'pk_filter : #'mk_filter;

    /* I think we can decide the damage here, because it should be faster
    this way. The bad thing is, that wrath pk does less damage against
    monsters than ordinary wrath, but who cares. */
    dmg = get_damage(who, sym, data=="pk");
    if(strstr("/"+object_name(HERE), PRIEST_ROOM_DIR)>=0) {
	dmg = 120 * dmg / 100; // more damage when defending the guild
	sym->inform_debugger(sprintf("Max guild defencing damage: %d", dmg));
    }

    tmp_rooms = (mapping)here->query_exits();
    rooms = mappingp(tmp_rooms) ? m_values(tmp_rooms) : ({});

    for (i=sizeof(tmp_rooms);--i>=0;) {
	if(!stringp(rooms[i])||(!file_lines(rooms[i])&&!file_lines(rooms[i]+".c"))) {
	    rooms[i] = 0;
	    continue;
	}
	if(!find_object(rooms[i])) {
	    // have to do this because all_inventory works only on objects
	    rooms[i]->query_short();
	}
	rooms[i] = find_object(rooms[i]);
	rooms[i]->tell_here(
	  "Suddenly, you feel the air tingling, as if it is being stretched too \
thin over a hot flame. The breath is sucked from your chest and your \
vision seems to almost catch a wrongness in the ether surrounding you. \
You are suddenly terrified as a deadly calm settles in.");
    }

    who->tell_me(
      "You raise your arms over your head. Extending them upward and outward, \
making a fist. In a strong voice, you call upon " GOD_NAME " to lend his \
aid in your cause. A faint blue light begins to pulsate from around your \
clenched fists and down your arms. You suddenly slam your fists together \
above your head. The blue nimbus contracts suddenly into a pinpoint over \
your now joined hands.");

    here->tell_here(&here_begin_message,0, 0, who, who, 0, 0);

    call_out("wrath2",2, who, filter, here, dmg, critical_success);
    call_out("wrath3",3, who, filter, dmg, rooms);
    data = 1;
}

void
wrath2(object who, closure filter, object here, int dmg, status was_crit)
{
    object *victims, sym;
    int i, yes_hit;
    status outd, pk_not_allowed;

    victims = filter_array(all_inventory(here), filter);
    outd = here->query(ROOM_WD) & WD_OUTDOORS;
    pk_not_allowed = 0;
    if(outd) dmg /= 2;
    for (i=sizeof(victims);--i>=0;) {
	// Extra check for existence added by Doomdark 03-Jan-98
	// But let's not set pk_not_allowed flag if the victim doesn't exits //Chopin
	if(!victims[i]) continue;
	if(!victims[i]->attacked_by(who,-1)) {
	    pk_not_allowed = 1;
	    victims[i] = 0;
	} else {
	    yes_hit=victims[i]->hit_player(dmg-random(dmg/2),DT_AIR,10,who);
	    if(yes_hit&&victims[i]) {
		if(was_crit)
		    victims[i]->add_condition(C_DEAF, 5+random(10)); 
		if (outd)
		    victims[i]->tell_me(
		      "You open your mouth, yet are unable to scream as the air around seems to \
rend itself in two. Multicolored sparks begin sizzling in the fragmented \
world about you and you vaguely feel it as your body is crushed beneath \
blow of the expanding shock wave caused by the self-disintegrating air. \
The only good thing is that here in the outdoors, the effect is not \
as lethal as in indoors.");
		else
		    victims[i]->tell_me(
		      "You open your mouth, yet are unable to scream as the air around seems to \
rend itself in two. Multicolored sparks begin sizzling in the fragmented \
world about you and you vaguely feel it as your body is crushed beneath \
blow of the expanding shock wave caused by the self-disintegrating air.");
	    }
	    else victims[i] = 0;
	}
    }

    here->tell_here(
      "With a deafening crack, the air around you splits into its primal \
components. The wind is sucked out of you and you begin to fall. A \
warm, protective healing envelopes you and you feel as thought your \
body is being cradled gently in a shielding embrace.",
      0, 0, victims, 0, 0, 0);

    if(pk_not_allowed)
	if (who)
	    who->tell_me("You tried to kill players in a place where it's not \
allowed! You feel ashamed.");
}

void
wrath3(object who, closure filter, int dmg, object *rooms)
{
    int i, j, prec_flag, l_dmg, guild_rank, yes_hit;
    object *victims, sym;
    status outd;

    sym = who->query_guild_object();
    sym->set_casting(0);
    guild_rank = sym->query_rank();
    if (guild_rank == PH_RANK || guild_rank == PT_RANK)
	prec_flag=1;

    dmg = 2 * dmg/(4+random(2)-prec_flag);
    // Preceptors get 2/3 to 1/2 original damage. Others get 1/2 to 2/5.

    for(i=sizeof(rooms);--i>=0;) {
	if (!objectp(rooms[i]))
	    continue;

	victims = filter_array(all_inventory(rooms[i]),filter,TO);
	outd = rooms[i]->query(ROOM_WD) & WD_OUTDOORS;
	l_dmg = outd ? dmg/2 : dmg;
	l_dmg /= 3; // TAMARINDO *growl*

	for(j=sizeof(victims);--j>=0;) {
	    if(!victims[j]) continue;
	    if (!victims[j]->attacked_by(who,-1))
		victims[j] = 0;
	    else {
		yes_hit=victims[j]->hit_player(l_dmg-random(l_dmg/2),DT_AIR,10,who);
		if(yes_hit&&victims[j]) {
		    if (outd)
			victims[j]->tell_me(
			  "You see a boiling rent in the air around you slowly swell and overtake \
you. The force of the deafening concussion knocks you to the ground and \
the air is sucked from your lungs. Fortunately, you are in outdoors, \
and the force wasn't as lethal as it could have been.");
		    else
			victims[j]->tell_me(
			  "You see a boiling rent in the air around you slowly swell and overtake \
you. The force of the deafening concussion knocks you to the ground and \
the air is sucked from your lungs. Slowly, you become aware of a trickle \
of blood running from your ears and nose.");
		}
		else victims[j] = 0;
	    }
	}

	rooms[i]->tell_here(
	  "The air around you begins to vibrate in a palpable manner. You suddenly \
see the air split and a strong gust of wind buffets you to your knees. For \
some reason you feel as if you have been spared a great injury somehow.",
	  0, 0, victims, 0, 0, 0);
    }
}

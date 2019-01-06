/* The module containing these ugly old global-spells.
 * Included in /obj/player.c.
 */

//ARE WE SURE WE WANT TO DEFINE THESE HERE? //Frobozz with Caps Lock
// YES. -+ Doomdark +-
// Otherwise this won't compile (for test purposes)
// note that defines are used to ensure these _won't_ be
// declared for player.c including these.

#ifndef PLAYER_C
#ifndef LIVING_C
// Double-check for strange changes... DD
// :) //F

int experience;
int spell_points;

status attacked_by(object ob, status s);
varargs void tell_me(string s, status a, status b);
varargs string query_name(int mode, object who);
varargs status attack_object(object x, status y);

#endif
#endif

static string spell_name;
static int spell_cost, spell_dam;

void
zap_object(object ob)
{
    ob -> attacked_by(this_object());
    environment() -> tell_here(query_name(0,this_object())
      + " summons a flash from the sky.",this_object());
    tell_me("You summon a flash from the sky.");
    experience += (int) ob -> hit_player(100000);
    tell_me("There is a big clap of thunder!\n\n");
}

void
missile_object(object ob)
{
    if (spell_points < 10) {
	tell_me("Too low on power.");
	return 0;
    }

    if (!attack_object(ob, 1)) {
	tell_me("Can't cast magic missile!");
	return 0;
    }
    spell_name = "magic missile";
    spell_dam = random(20);
    spell_cost = 10;
}

void
shock_object(object ob)
{
    if (spell_points < 15) {
	tell_me("Too low on power.");
	return 0;
    }

    if (!attack_object(ob, 1)) {
	tell_me("Can't cast shock!");
	return 0;
    }

    spell_name = "shock";
    spell_dam = random(30);
    spell_cost = 15;
}

void
fire_ball_object(object ob)
{
    if (spell_points < 20) {
	tell_me("Too low on power.");
	return 0;
    }

    if (!attack_object(ob, 1)) {
	tell_me("Can't cast fireball!");
	return 0;
    }
    spell_name = "fire ball";
    spell_dam = random(40);
    spell_cost = 20;
}

/* Hope this works: one can now add more combat spells easily.
   This is used by the lord's crown for example. \\Graah
   No, it isn't! //Frobozz
*/
void
initiate_spell(object ob, string spn, int spd, int spc)
{
    if (!ob) return;
    if (spell_points < spc) {
	tell_me("Too low on power.");
	return;
    }
    if (!attack_object(ob, 1)) return 0;
    spell_name = spn;
    spell_dam = spd;
    spell_cost = spc;
}

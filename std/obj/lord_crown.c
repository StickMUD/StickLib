// 21-Jul-96 / Graah: Isolated rooms disable channel
// 26-Sep-96 / Graah: Lords now start from level 25!

inherit "/basic/id";

#include <lord.h>
#include <lights.h>
#include <npc_defs.h>
#include <invis_levels.h>
#include <room_defs.h>
#include <tell_me.h>

// These two macros allow easy changing between 'owner' and 'this_player()'
// I _think_ it's safe to assume every command comes from the owner, though.

#define	TELL_ME(x)	owner->tell_me(x)
#define	OWNER		owner
#define	IS_LORD(x)	(x->query_lord())
#define	CROWN_ID	"lord_player_object"
#define ECHO		"CHANNEL_ECHO"

static object *serv_req;	// Array of those wanting to serve
object owner;			// Player holding crown
int lchan;			// Is the channel on or off?
static closure sort_lambda;

private int owner_level;		// Owner's level.
private status is_coder;	// Is owner a coder?
private string owner_name;	// Owner's real name.

static string *badstrs;

int check();

nomask status query_prevent_shadow() { return 1; }

string query_auto_load()
{
    string fname;
    sscanf(object_name(this_object()), "%s#%~d", fname);
    return fname+":";
}

void
reset()
{
    if (owner)
	owner_level = (int) owner -> query_level();

    // Not 100% reliable solution, but has to do for now...
    // So, if lord has gained/lost level, it will take effect in crown's reset.
}

void
create()
{
    if ((int)this_player()->query_gender() == G_FEMALE) {
	set_name("lady's crown");
	set_short("Lady's crown (worn)");
    } else {
	set_name("lord's crown");
	set_short("Lord's crown (worn)");
    }
    set_id(({ "crown", CROWN_ID }));
    set_long("This powerful artifact is given to those mortals that have\
 reached Lordly levels. Type 'lhelp' to see the list of commands.");
    lchan = 1;

    badstrs = ({
      "<", " says: ", " shouts: ", " tells you: ",
      " yells: ", " whispers: ", " asks: ", " bellows: ",
    });
}

void
init()
{
    if (!owner && (owner = environment())) {
	owner = environment();
	owner_level = (int) owner -> query_level();
	is_coder = ((int) owner -> query_coder_level() > 0);
	if (owner_name = (string) owner -> query_real_name())
	    owner_name = capitalize(owner_name);
	else owner_name = "Someone";
    }

    add_action("light_lcmd","light");
    add_action("darken_lcmd","darken");
    add_action("accept_lcmd","accept");
    add_action("reject_lcmd","reject");
    add_action("expel_lcmd","expel");

    check();
}

void
set_owner(string o)
{
    object ob;

    if (!(ob = find_player(o))) return 0;
    owner = ob;
    owner_level = (int) owner -> query_level();
    is_coder = ((int) owner -> query_coder_level() > 0);
    owner_name = capitalize((string) owner -> query_real_name());
}

string
query_info()
{
    return ("Lord-crown object, orignally by probably ever admin ever \
is/was. Currently by Chaotic.");
}

status
drop(status silent)
{
    check();
    return 1;
}

status
get()
{
    return 1;
}

int
light_lcmd(string str)
{
    if (str != "crown") return 0;

    if((int) OWNER -> query_sp() < CO_LIGHT) {
	TELL_ME("You don't have enough sps!");
	return 1;
    }

    if (find_call_out("lights_off") != -1) {
	TELL_ME("But your crown is already lit.");
	return 1;
    }

    this_player()->add_sp(-CO_LIGHT);
    TELL_ME("When you utter the words your crown starts to glow.");
    set_light(2);
    call_out("lights_off", 240 + random(60));
    return 1;
}

int
lights_off(object player)
{
    TELL_ME("Your crown dims.");
    set_light(-2);
    return 1;
}

int
darken_lcmd(string str)
{
    if (str != "crown") return 0;

    if ((int)OWNER->query_sp() < CO_LIGHT)
	return notify_fail(SP_MES), 0;

    if(find_call_out("dark_off") >= 0) {
	TELL_ME("But your crown is already emitting darkness.");
	return 1;
    }

    this_player()->add_sp(-CO_DARK);
    TELL_ME("When you utter the words your crown starts emit darkness.");

    set_light(-2);
    call_out("dark_off",240 + random(60));
    return 1;
}

void
dark_off()
{
    TELL_ME("Your crown stops emitting darkness.");
    set_light(2);
    return;
}

int
check()
{
    string x,y;

    if (!owner || !owner->query_lord()) {
	if (sscanf(object_name(this_object()),"%s#%s",x,y)==2)
	    destruct(this_object());

	// We don't destruct master object; that way we'd have to compile it
	// again when some other lord clones it. -+ Doomdark +-

	return 0;
    }

    return 1;
}

// This is used to check lord-channel in lords' crowns
int
query_lchan()
{
    return lchan;
}

/* No need for this anymore... -+ Doomdark +-
private static int
is_lord(object ob)
{
return (int)ob->query_lord();
// Was : && (lpo=present(CROWN_ID ,ob))
// Checking whether they have crown is complete waste of time.
// -+ Doomdark +-
}
*/

// Lord's channel
int
lords_lcmd()
{
    int i,foo,dummy, its_a_coder;
    int ilevel;
    object *ob, lpo;
    string x, zz, zzz;
    string tmp;
    status isolated;

    ob = users();
    x = "";
    dummy = 0;

    // Let's update level information every time we use this command.
    if(!closurep(sort_lambda) )
	sort_lambda=lambda(({'user, 'user2}),
	  ({#'<, ({#'call_other,'user, "query_level"}),
	    ({#'call_other,'user2,"query_level"})
	  }) );

    ob = sort_array(ob,sort_lambda);

    owner_level = (int) owner -> query_level();
    if (environment(owner))
	isolated = (status)environment(owner)->query(ROOM_ISOLATED);
    else isolated = 0;

    for (i = sizeof(ob) - 1; i >= 0; i--) {
	if (!IS_LORD(ob[i]) || !environment(ob[i]) ||
	  !ob[i]->query_guild() ||
	  environment(ob[i])->query(ROOM_ISOLATED)) continue;
	if (!is_coder && isolated && ob[i] != owner) continue;
	if (is_coder ||
	  (!(ilevel = (int) ob[i]->query_invis())) ||
	  (owner_level > CAN_SEE_INVIS
	    && ilevel < IL_TOTALLY_INVISIBLE) ||
	  !(its_a_coder = (int) ob[i]->query_coder_level()))
	{
	    zz = sprintf("%-14s", capitalize((string)ob[i]
		->query_real_name()));
	    foo = (int)ob[i]->query_lord();
	    if (its_a_coder) {
		if (!stringp(zzz = (string) ob[i] ->
		    query_pretitle())) zzz = "";
	    } else {
		if (!stringp(zzz = (string) ob[i] ->
		    query_pretitle())) zzz = "";
	    }
	    zz = sprintf("\n[%|21s]  %s", zzz, zz);

	    if ((lpo = present(CROWN_ID, ob[i]))
	      && lpo -> query_lchan())
		zz += " on";
	    else zz += " off";

	    x += zz;
	    dummy++;
	}
    }

    // This should be impossible.. :-)

    if (!dummy) {
	TELL_ME("No lords in the game.");
	return 1;
    }

    tmp = sprintf("Total of %d %s in the game.", dummy,
      ((dummy == 1) ? "lord" : "lords"));

    TELL_ME(sprintf("%s\n         Rank            Lord's name   Channel%s",
	tmp, x));
    return 1;
}

int
lord_lcmd(string str)
{
    int i, x;
    object *ob, crown;
    string tmp;
    status isolated;

    if (!str) return(lords_lcmd());

    // You got to be in SOME guild to use channel.
    if (!TP->query_guild())
    {
	notify_fail("You need to be in any guild to use the channel.\n");
	return 0;
    }

    tmp = lower_case(str);

    for (i = sizeof(badstrs) - 1; i >= 0; i--)
	if (strstr(tmp, badstrs[i], 0) != -1)
	{
	    TP->tell_me("Due jerks faking stupid messages you are not allowed \
to use string \"" + badstrs[i] + "\" on lord channel.");
	    return 1;
	}

    TP->tell_me("NOTE: You should use new channel system. See 'help channel'.");

    x=(int)TP->query_env(ECHO);

    if((str[0]==':'))
	str = sprintf("<lord> %s %s", capitalize(owner_name),
	  str[1..<1]
	);
    else
	str = sprintf("<%s> %s",capitalize(owner_name), str);

    ob = users();

    if (environment(owner))
	isolated = (status)environment(owner)->query(ROOM_ISOLATED);
    else isolated = 0;

    if (!lchan)
	return notify_fail("But your channel is not on!\n"), 0;

    for (i = sizeof(ob) - 1; i >= 0; i--) {
	if (IS_LORD(ob[i]) && (crown = present(CROWN_ID, ob[i])) && crown -> query_lchan())
	    if(ob[i] == TP)
		switch(x) {
	    case -1: continue; break;
	    case  1: ob[i]->tell_me("Ok.");break;
	    default: ob[i]->tell_me(str, TELL_TYPE_LORD_CHANNEL); break;
	    }
	    else
	    if (!isolated)
		ob[i]->tell_me(str, TELL_TYPE_LORD_CHANNEL);
    }

    // Filter out some garbage

    if ((member(({ "hello", "hi", "bye", "hiya", "greetings",
	    "moi", "re" }), tmp) < 0) && !isolated)
	log_file("LORD_CHANNEL", str + "\n");

    return 1;
}

int
lordtoggle_lcmd()
{

    lchan = !lchan;

    if(lchan)
	TELL_ME("Ok. Lord channel active.");
    else
	TELL_ME("Ok. No messages from lord channel.");

    return 1;
}

void
purge_servreq()
{
    int i;
    if (!serv_req) {
	serv_req = ({});
	return;
    }

    for (i = 0; i < sizeof(serv_req); i++) {
	if (!objectp(serv_req[i]))
	    serv_req[i] = 0;
    }

    serv_req -= ({ 0 });
}

// A willing new servant!
void
servant_request(object ob)
{
    purge_servreq();
    serv_req += ({ob});
}

int
accrej(string arg, int flag)
{
    int x,i;
    object ob;
    string tmp;
    purge_servreq();

    if (!arg) {
	if (!serv_req || !sizeof(serv_req)) {
	    TELL_ME("No one wants to become your servant currently.");
	    return 1;
	}
	tmp = "The following players want to be accepted (or rejected):";

	for (i = 0; i < sizeof(serv_req); i++) {
	    tmp += sprintf("\n%s%s Level: %d\tTitle: %s",tmp,
	      (capitalize((string)serv_req[i]->query_real_name())
		+ "                ")[0..15],
	      (int) serv_req[i]->query_level(),
	      (string) serv_req[i]->query_title());
	}
	TELL_ME(tmp);
	return 1;
    }
    if (!(ob = find_player(lower_case(arg)))) return
	notify_fail("No such player.\n"), 0;

    x = member(serv_req, ob);

    if(x == -1) return
	notify_fail("That player isn't trying to serve you.\n"), 0;

    serv_req[x] = 0;
    if(!flag) {
	TELL_ME("Ok. You have chosen " + capitalize((string)ob->query_real_name())
	  +" to be one of your servants!");

	ob->tell_me("\n"+capitalize(owner_name)+ " has accepted you to service!");

	ob->set_liege(environment()->query_real_name());

	MASTER_OB->log_lord(environment()->query_real_name(),
	  capitalize((string)ob->query_real_name()),"accepted into service");
	return 1;
    }

    TELL_ME("Ok. You have rejected " + capitalize((string)ob->query_real_name())
      + "'s service.");
    ob->tell_me("\n"+capitalize(owner_name) +" did not accept you to service.");
    return 1;
}

int accept_lcmd(string arg) { return accrej(arg, 0); }
int reject_lcmd(string arg) { return accrej(arg, 1); }

int
expel_lcmd(string who) // Yes, it _is_ written with one 'l'  --Elena
{
    object ob;

    if ((int) OWNER -> query_level() < 28)
	return notify_fail("You're not on high enough level!"), 0;

    if (!who) return notify_fail("Usage: expel <player>\n"), 0;

    if (!(ob = find_player(lower_case(who))))
	return notify_fail("No such player.\n"), 0;

    if ((string) ob->query_liege() != lower_case(owner_name)) {
	TELL_ME(capitalize(who)+" is not your follower.");
	return 1;
    }

    ob->set_liege();

    TELL_ME("You expel "+capitalize(who)+" from your liege.");
    ob->tell_me("You are no longer a follower of "+ owner_name + ".");
    ob->tell_me(capitalize((string)owner->Pronoun())+" has expelled you.");

    MASTER_OB->log_lord(owner_name, capitalize(
	(string)ob->query_real_name())," expelled from service.");

    return 1;
}

/********************************************************************
*                                                                   *
*           Generic base object; this was meant for a gift          *
*           pet inheritable object. It uses the treasure            *
*           object as its starting point, and allows the            *
*           coder to customize it as he/she wishes.                 *
*           The gifts are in /lib/gift/gifts/                       *
*                                                                   *
* Name: pet.c                                                       *
* Version: 1.1                                                      *
* Last changed: 17-Jan-98 by Kieve                                   *
*                                                                   *
* 30-Dec-97, Kieve - Wrote the base code and a single command       *
*     'speak' to enable a user to talk through the pet.             *
* 3-Jan-98, Kieve - Changed the command to work through a 'pet'     *
*     command. Added support for many other commands.               *
* 11-Jan-98, Kieve - Added set_id to force "pet" into the id when   *
*     the argument to set_id is an array or string.                 *
* 11-Jan-98, Kieve - Added the query_auto_load and init_args        *
*     functions. They can be overridden to make the pet             *
*     non-inheritable.                                              *
* 17-Jan-98, Kieve - Fixed a bug that made the pet drop to the      *
*     ground when the player died. In the drop() function.          *
* 25-Apr-98, Kieve - Added the function query_prevent_steal() to    *
*     stop thieves from stealing the pet.                           *
*                                                                   *
********************************************************************/
 
/*
  IDEAS:
    Well, maybe players will want to be able to name their pets?
    Darkelf -- Buy food to feed them?
*/
 
#include <treasure.h>
#include "pet_defs.h"
 
#define PET_NO_INHERIT
 
// This is an inheritable pet file
 
#define    TP    this_player()
#define    TO    this_object()
#define ENV(xx)  environment(xx)
 
int petFlags;
closure InitFunc;
object master;
 
// Special variables to hold actions
string pickUpMsg, say_style;
string *chats;
int chat_chance;
 
//    ***** The command functions *****
status do_help(string arg);
status do_say(string arg);
status do_emote(string arg);
status do_speak(string arg);
status do_quiet(int arg);
status do_remove(string arg);
status do_show(string arg);
 
 
 
//    *****  The Query Functions  *****
 
object   query_master() { return master; }
string   query_pick_up_msg() { return pickUpMsg; }
string  *query_chats() { return chats; }
int      query_chat_chance() { return chat_chance; }
int      query_speaking_pet() { return petFlags & F_PET_CAN_SAY; }
string   query_say_style() { return say_style; }
int      query_pet_leashed() { return petFlags & F_PET_IS_LEASHED; }
int      query_leashed_pet() { return petFlags & F_PET_IS_LEASHED; }
int      query_can_change_style() { return petFlags & F_PET_CAN_CHANGE_STYLE; }
 
int      query_is_pet() { return 1; }
int      query_prevent_steal() { return 1; }
 
 
//    *****  The Set Functions  *****
 
void set_pick_up_msg(string arg) { pickUpMsg = arg; }
void set_say_style(string style) { say_style = style; }
void set_chat_chance(int chance) { chat_chance = chance; }
 
void set_master(mixed plr) {
    if(!objectp(plr))
        if(!(plr = find_player(plr)))
            return;
    petFlags |= F_PET_HAS_MASTER;
    master = plr;
    return;
}
 
void set_pet_chats(int chance, string *str) {
    // chance is from 0 to 100. It is a percentage chance every heartbeat
    if(!chance) return;
 
    chats = str;
    chat_chance = chance;
 
    // until we have the chats, we don't need the heart_beat, so we'll
    // save on the calls.
    if(chats  &&  sizeof(chats)) {
        set_heart_beat(1);
        petFlags |= F_PET_HAS_HB;
    }
    else {
        if(petFlags & F_PET_HAS_HB)
            petFlags ^= F_PET_HAS_HB;
        set_heart_beat(0);
    }
    return;
}
 
static void _set_pet_flag(int usr_flag, int flag) {
    // This is meant to be called from within the object
    if(usr_flag) {
        if(!(petFlags & flag))
            petFlags |= flag;
    }
    else {
        if(petFlags & flag)
            petFlags ^= flag;
    }
    return;
}
 
void set_speaking_pet(int speak) {
    _set_pet_flag(speak, F_PET_CAN_SAY);
    return;
}
 
void set_emoting_pet(int emote) {
    _set_pet_flag(emote, F_PET_CAN_EMOTE);
    return;
}
 
void set_can_change_style(int change) {
    _set_pet_flag(change, F_PET_CAN_CHANGE_STYLE);
    return;
}
 
varargs void set_leashed_pet(object ob) {
    _set_pet_flag((objectp(ob)  &&  ob->query_is_leash()), F_PET_IS_LEASHED);
    return;
}
 
//   *****  Lib functions  *****
 
mixed _pet_get_room() {
    object room;
    room = ENV(TO);
    if(!room) return 0;
    if(room->query(LIV_IS_PLAYER)) {
        room = ENV(room);
        if(!room) return 0;
    }
    return room;
}
 
void heart_beat() {
    // This is used for the emotions
    object room;
    if(!master  &&  petFlags & F_PET_HAS_MASTER) {
        destruct(TO);
        return;
    }
    if(!chats  ||  !sizeof(chats)  ||  !chat_chance) {
        petFlags ^= F_PET_HAS_HB;
        set_heart_beat(0);
        return;
    }
 
    if(petFlags & F_PET_IS_QUIET)
        return;
    if(random(100)+1 <= chat_chance) {
        //Give out a random message
        if(!room = _pet_get_room())
            return;
        room->tell_here(chats[random(sizeof(chats))]);
    }
    return;
}
 
int catch_feeling(string verb, string arg) {
    return 1;
}
 
int catch_show(object shower, object showee) {
    if(shower != master)
        return 0;
 
    // The /bin/pub/_show.c file does a message to let people know
    // that the showing of objects is actually done.  All that's left
    // is for us to print out the long description of the pet
    if(query_long())
        showee->tell_me(TO->query_long());
    return 1;   
}
 
varargs status get(status silent) {
    if(!master  &&  petFlags & F_PET_HAS_MASTER) {
        destruct(TO);
        return 0;
    }
    if(!TP) return 1;  // It must know what it's doing
    if(TP != master) {
        if(!silent  &&  pickUpMsg  &&  strlen(pickUpMsg))
             if(ENV(TO))
                ENV(TO)->tell_here(pickUpMsg);
        return 0;
    }
    return 1;
}
 
varargs status drop(status silent) {
    // return 1 means you can't drop it.
    if(!master  &&  petFlags & F_PET_HAS_MASTER) {
        destruct(TO);
        return 0;
    }
    if(!TP) return 1; // They could be dying
    if(TP != master) {
        if(master->query_dead())
            return 1;
        return 0; // They really shouldn't have it to drop.
    }
    
    if(petFlags & F_PET_IS_LEASHED)  // It's leashed.. no problem
        return 0;
    if(!silent)
        TP->tell_me("You wouldn't dare drop your pet without a leash!");
    return 1;
}
 
 
//  *****  The create, reset, and init function support  *****
 
void create_pet() { }
 
nomask void create_treasure() {
    petFlags = 0;
    say_style = "says";
    if(function_exists("reset_pet"))
        petFlags |= F_RESET_USED;
    InitFunc = symbol_function("init_pet", this_object());
    create_pet();
}
 
nomask void reset_treasure() {
    if(petFlags & F_RESET_USED)
        this_object()->reset_pet();
}
 
nomask void init_treasure() {
    if(!master  &&  petFlags & F_PET_HAS_MASTER) {
        destruct(TO);
        return 0;
    }
    add_action("do_pet", "pet");
    funcall(InitFunc);
}
 
void set_id(mixed arg) {
    // if they're setting the id properly, we'll add "pet" to it so that
    // 'look at pet' will work without the coder putting it in the id
    if(pointerp(arg)) {
        if(member_array("pet", arg) == -1)
            arg += ({ "pet" });
    }
    else if(stringp(arg)) {
        if(arg != "pet")
            arg = ({ arg, "pet" });
    }
    
    ::set_id(arg);
}
 
 
//    *****  Player commands  *****
 
status do_pet(string args) {
    string pet_verb, new_arg;
    if(!args || args == "")
        args = "help";
    if(sscanf(args, "%s %s", pet_verb, new_arg) != 2) {
        sscanf(args, "%s", pet_verb);
        new_arg = "";
    }
    switch(pet_verb) {
        case "help"   : return do_help(new_arg);   break;
        case "say"    : return do_say(new_arg);    break;
        case "emote"  : return do_emote(new_arg);  break;
        case "speak"  : return do_speak(new_arg);  break;
        case "quiet"  : return do_quiet(1);        break;
        case "noisy"  : return do_quiet(0);        break;
        case "remove" : return do_remove(new_arg); break;
        case "show"   : return do_show(new_arg);   break;
        default       : return do_help(new_arg);   break;
    }
    // We really should never get here...
    return 0;
}
 
status do_help(string arg) {
    // We're going to build the help depending on what has been
    // activated in the pet.
    string msg;
    if(!master  &&  petFlags & F_PET_HAS_MASTER) {
        destruct(TO);
        return 0;
    }
    if(TP != master)
        return 0;
    msg = "You're the proud owner of a pet; a pet requires you to be "+
        "very responsible. You must feed it, give it water, take it for "+
        "walks, and most importantly play with it!\n\n";
    msg += "\tpet help\t- Display this help page\n";
 
    if(petFlags & F_PET_CAN_SAY)
        msg +="\tpet say\t\t- Speak through your pet.\n";
    if(petFlags & F_PET_CAN_EMOTE)
        msg += "\tpet emote\t- Emote through your pet.\n";
    if(petFlags & F_PET_CAN_CHANGE_STYLE)
        msg += "\tpet speak\t- Change the default speaking style. "+
            "(defaut: 'says')\n";
    if(petFlags & F_PET_HAS_HB) {
        msg += "\tpet quiet\t- Stop making the random emotes.\n";
        msg += "\tpet noisy\t- Start making the random emotes.\n";
    }
    msg += "\tpet show\t- Show your pet to another player.\n";
    msg += "\tpet remove\t- Destroy your pet.\n";
    TP->tell_me(msg);
    return 1;
}
 
status do_say(string arg) {
    object room;
    if(!master  &&  petFlags & F_PET_HAS_MASTER) {
        destruct(TO);
        return 0;
    }
    if(!(petFlags & F_PET_CAN_SAY))
        return 0;
    if(TP != master)
        return 0;
    if(!arg || arg == "")
        return notify_fail("What did you want your pet to say?\n"), 0;
    // check for the room
    if(!room = _pet_get_room())
        return 0;
    // say_style can be, for a parrot, 'squacks' or for a dog, 'barks'
    room->tell_here(query_short()+" "+say_style+": "+arg);
    return 1;
}
 
status do_emote(string arg) {
    object room;
    if(!master  &&  petFlags & F_PET_HAS_MASTER) {
        destruct(TO);
        return 0;
    }
    if(!(petFlags & F_PET_CAN_EMOTE))
        return 0;
    if(TP != master)
        return 0;
    if(!arg || arg == "")
        return notify_fail("What did you want your pet to do?\n"), 0;
    // check for the room
    if(!room = _pet_get_room())
        return 0;
    room->tell_here(query_short()+" "+arg);
    return 1;
}
 
status do_speak(string arg) {
    if(!master  &&  petFlags & F_PET_HAS_MASTER) {
        destruct(TO);
        return 0;
    }
    if(!(petFlags & F_PET_CAN_CHANGE_STYLE))
        return 0;
    if(TP != master)
        return 0;
    if(!arg  ||  arg == "")
        return notify_fail("How did you want your pet to 'say' things?\n"), 0;
    set_say_style(arg);
    TP->tell_me("Ok.");
    return 1;
}
 
status do_quiet(int arg) {
    if(!master  &&  petFlags & F_PET_HAS_MASTER) {
        destruct(TO);
        return 0;
    }
    if(!(petFlags & F_PET_HAS_HB))
        return 0;
    if(TP != master)
        return 0;
    _set_pet_flag(arg, F_PET_IS_QUIET);
    if(arg)
        TP->tell_me("Your pet will now be silent.");
    else
        TP->tell_me("Your pet will once again be noisy.");
    return 1;
}
 
status do_remove(string arg) {
    if(!master  &&  petFlags & F_PET_HAS_MASTER) {
        destruct(TO);
        return 0;
    }
    if(TP != master)
        return 0;
    if(arg != "now")
        return notify_fail("You pay good money for your pet, and now you "+
            "want to get rid of it!?\nOkay, but type 'pet remove now' if "+
            "you're sure you want this.\n"), 0;
 
    // Well, they did choose to remove the pet. *shrug*
    TP->tell_me("Ok.");
    destruct(TO);
    return 1;
}
 
status do_show(string arg) {
    if(!master  &&  petFlags & F_PET_HAS_MASTER) {
        destruct(TO);
        return 0;
    }
    if(TP != master)
        return 0;
    command("show pet to "+arg, TP);
    return 1;
}
 
 
//    *****  Auto Loading functions  *****
 
// These can be overridden to make the pet non-inheritable
 
string query_auto_load_string() {
    string fname;
    int newFlags;
    sscanf(file_name(TO), "%s#%~d",fname);
    if(petFlags & F_PET_IS_LEASHED)
        newFlags = petFlags ^ F_PET_IS_LEASHED;
    else
        newFlags = petFlags;
    return sprintf("%s:%s#%s#%d", fname, master->query_real_name(),
        say_style, newFlags);
}
 
mixed query_auto_load() {
    if(!ENV(TO))
        return 0;
    if(!ENV(TO)->query(LIV_IS_PLAYER))
        return 0;
    return query_auto_load_string();
}
 
void init_arg(string arg) {
    string s_master;
    sscanf(arg, "%s#%s#%d", s_master, say_style, petFlags);
    set_master(s_master);
}

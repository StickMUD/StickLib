/*
 * /std/daemons/remarks.c
 *
 * 17.08.92 Val		- made the comments shorter
 * 23. 8.92 Graah	- combined both remark catalogues
 *			- no comment is longer than 40 characters
 * 24.11.92 Graah       - remarks are now in alphabetical order
 *                      - and the same remark doesn't appear twice
 * 5-Sep-98 / Graah	Added some stick-distribution patches
 */

#pragma strict_types

#include <mud_name.h>

string *remarks;
int no_of_remarks;

static void
init_remarks() {

    remarks = ({
      "%@&!?#$*?!?",
      "A 16-ton weight falls upon you.",
      "A valiant attempt.",
      "An interesting attempt...",
      "And that means?",
      "And we are the knights who say \"Ni\".",
      "Another failure.",
      "Are you sure that you want to do THAT?",
      "Arglebargle, glop-glyf?!??",
      "Arglebargle arrives.",
      "Autofail.",
      "Barglebarglegorpleborblez!?!?",
      "Bolle Snylle Grof Gryp?!!?",
      "Chaos strikes back.",
      "Close, but no banana.",
      "Command mistyped?",
      "Connection almost closed by foreign host.",
      "Core dumped.",
      "Could you rephrase that command a bit?",
      "Da-hali-tuli-jallaah?",
      "Death tells you: DON'T EVER TYPE THAT AGAIN!",
      "Don't ever type that again!",
      "Excuse me?",
      "Error #35: Formula too complex.",
      "Game Driver bursts into tears.",
      "Game Driver falls down laughing.",
      "Game Driver sings: When you wish upon a star...",
      "Game Driver swears loudly!",
      "Game Driver tosses your command into trashcan.",
      "Get a life.",
      "Go ahead. Make my day.",
      "Have a day with that command.",
      "Had fun with that command, did you?",
      "Hello. Are you happy people?",
      "He's dead Jim.",
      "Hey Frankenstein!",
      "Hmm...",
      "Houston we have a problem!",
      "How bizarre!",
      "How rude! You dirty pervert!",
      "How DARE you enter such a command?!?!?",
      "Huh?",
      "I beg your pardon?",
      "I am not sure that I understand you fully.",
      "I didn't expect the Spanish Inquisition.",
      "I didn't quite get that.",
      "I don't think so, Tim.",
      "I pretend I didn't hear that. Try again.",
      "I think I'll just ignore that.",
      "I'll mail the admins about that one!",
      "I'm sorry Dave. I don't think I can do that.",
      "[Inform] Typo reconnects.",
      "[Inform] You just typed something wrong.",
      "If you fail the first time, try again.",
      "It's illegal to type such commands!",
      "Ktinporshktle?",
      "Let me rest a while and think about it.",
      "Let's change the subject, ok?",
      "MHnGRPSYTYHGLE! Hgnkkk!??!",
      "Mittee sie h|p{j{t?",
      "Next.",
      "Nice day for typos.",
      "Nice try.",
      "No way!",
      "No. It didn't work.",
      "Not me, I can tell you.",
      "Not tonight honey, I've got a headache.",
      "Now that was a really strange command!",
      "Ok. Entering GOD mode.",
      "Ok. Oops! You fumbled it critically!",
      "Pardon me?",
      "Permission denied.",
      "Please rephrase that.",
      "Rather childish actually.",
      "Read the Fine Manual Error.",
      "Red alert!",
      "Right!",
      "Sloppy fingers.",
      "Suddenly a horrible TYPO leaps upon you!",
      "Syntax error.",
      "That doesn't work.",
      "That would be telling.",
      "Typo alert!!!!",
      "Typo massacres you into small fragments!",
      "Typo strikes again!",
      "Ugga Bugga?",
      "Uhh?",
      "Up yours!",
      "Useless, utterly useless.",
      "Wakarimasen.",
      "W h a t?",
      "Watch your language!",
      "Whaaat?!?!",
      "What a concept.",
      "What on earth are you trying to do?",
      "What?",
      "Geee...what's up, doc?",
      "Why don't you try something else?",
      "Would you please say something else?",
      "Wow! That command was so cosmic!",
      "Yeah, sure.",
      /* We don't want to scare players (at least this way) 7.1.1993 //Frobozz
	      "You die. You can see your own body from above.",
      */
      "You are number six.",
      "You can't wear the One Ring!",
      "You challenge Ooze to a typoing contest.  You win!",
      "You have been promoted to Oozehood.",
      "You nearly crashed the mud!",
      "You only manage to step on your own fingers.",
      "You shout: I wrote a typo!",
      "Yes??"
    });

    no_of_remarks = sizeof(remarks);
}

/*
 * A catalogue of miscellaneous random remarks
 */
nomask string
query_random_notify_fail()
{
    if(!remarks) init_remarks();

    return remarks[random(no_of_remarks)] + "\n";
}

/* Too long remarks stored here

"Armageddon tells you: Why did you have to type THAT?",
"Game Driver tells you: I just hate it when you type that!",
"Death tells you: IF YOU TYPE THAT AGAIN I HAVE TO DO SOMETHING NASTY!",
"Guard Captain tells you: It's illegal to type such commands!",
"You nearly crashed the whole mud with that command!",
"You don't need a sensitive mind to notice a wrongness with that command.",
"*sigh* Why on Ear... "+MUD_NAME+" would you like to do that?",
MUD_NAME+" massacres that command into small fragments.",
"Game Driver tells you: You just lost the control of your fingers.",
"Game Driver tells you: I almost lost your heart beat.",

*/

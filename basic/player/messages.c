/* player.c inherits this file. 6.6.1992 //Frobozz */

static string *newmmsgout,*newmmsgin; /* arrays of new messages when going
					 invis/vis */
reset_msg() {
/* New messages when going invis/vis for high lords and wizards, some guilds
   may want to use these too */
    newmmsgin=({
      "arrives in a puff of smoke",
      "returns in the existence",
      "appears in a puff of logic",
      "has forgotten the first lesson of 'How not to be seen'",
      "appears",
      "gradually materializes out of thin air",
      "appears from another dimension",
      "appears from a bright flash of light",
      "appears with a tremendous lightning bolt",
      "appears from nowhere",
      "utters a magical word and appears in front of you",
      "appears from a swirling cloud of dust",
    });
    newmmsgout=({
      "disappears in a puff of smoke",
      "fades out of existence",
      "vanishes in a puff of logic",
      "has learned the first lesson of 'How not to be seen'",
      "disappears",
      "gradually fades to thin air",
      "disappears to another dimension",
      "disappears in a bright flash of light",
      "disappears with a tremendous lightning bolt",
      "makes a gesture and disappears",
      "utters a magical word and disappears",
      "disappears in a swirling cloud of dust",
    });
}

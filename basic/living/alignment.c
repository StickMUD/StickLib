//
// alignment.c, moved alignment stuff from /obj/living.c
//
// Graah 7-Jul-96
//

#include <alignment.h>
#include <daemons.h>

int alignment;			// Alignment value; > 0 -> good, < 0 -> evil

int query_alignment() { return alignment; }

void set_alignment(int x) { alignment = x; }

// Update our aligment. a equals to alignment of the killed creature.
// Hmm...shouldn't killing be always an evil thing? But it
// couldn't, mages would never become good and they'll need that :(
int
#ifdef OLD_ALIGNMENT
add_alignment(int a)
#else
add_alignment(int a, string q)
#endif
{
  int t;

  if(!intp(a)) return alignment;
#ifndef OLD_ALIGNMENT
	if(!q) return alignment;
#endif

  a = -a;		// Let's negate it, easier this way.

  if (alignment * a < 0 || !alignment) {
    // ie. bad killing bad or good killing good.
    t = (a - alignment) / 3;
    if (t > 100) t -= (t - 100) / 2;
    else if (t < -100) t -= (t + 100) / 2;
    alignment += t;
  } else if (alignment < 0 && a < alignment) {
    // Bad killing good.
    t = alignment - a;
    if (t > 100) t -= (t - 100) / 2;
    alignment -= t / 3;
  } else if (alignment > 0 && a > alignment) {
    // Good killing bad.
    t = a - alignment;
    if (t > 100) t -= (t - 100) / 2;
    alignment += t / 3;
  }

  return alignment;
}

void
reset_alignment()
{
  int t;

  // Let's move alignment towards zero...
  if (alignment < 0) {
    t = -10 + alignment * 5 / 100;
    if (t < -50) t = -50;
    if (t < alignment) alignment = 0;
    else alignment += t;
  } else {
    t = 10 + alignment * 5 / 100;
    if (t > 50) t = 50;
    if (t > alignment) alignment = 0;
    else alignment -= t;
  }
}

string
query_al_title()
{
  return (string)DESC_D->query_al_title(alignment);
}

#if 0
// Old stuff...better in desc daemon. - Graah
// Re-coded by Doomdark 10-jan-95
string
query_al_title()
{
  // New: angelic. Almost impossible to achieve. 26.6.1992 //Frobozz
  if (alignment > AL_ANGELIC * AL_RATIO / 100)	return "angelic";
  if (alignment > AL_SAINTLY * AL_RATIO / 100) return "saintly";
  if (alignment > AL_GOOD * AL_RATIO / 100) return "good";
  if (alignment > AL_NICE * AL_RATIO / 100) return "nice";
  if (alignment > AL_NASTY * AL_RATIO / 100) return "neutral";
  if (alignment > AL_EVIL * AL_RATIO / 100) return "nasty";
  if (alignment > AL_DEMONIC * AL_RATIO / 100) return "evil";
  if (alignment > AL_SATANIC * AL_RATIO / 100) return "demonic";
  // New: satanic. Almost impossible to achieve.
  else return "satanic";
}
#endif

// COMPATIBILITY FUNCTIONS - Do not use!

// Use 'set_alignment()' instead.
void set_al(int x) { set_alignment(x); }

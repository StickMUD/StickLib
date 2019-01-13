/******************************************************************
  Casino magic blue poker machine (Jacks or better video poker) 
  Coded by: Germ
  Last modified: Feb 12, 96

  Modification history:
    (2/12/96) - Coder's games no longer get logged or included
                in the profit/loss stats.

  Comments: 

  This machine works as follows:  Coins are inserted, the button
  is pushed, and a hand is dealt.  Then the player may discard
  0-3 cards which are replaced from the deck.  Once this is done
  the hand is evaluated and payoffs are given.

  Chances of getting a winning hand (theoretically) are:

           Royal Flush                 0.0024%
           Straight Flush              0.011%
           4 of a Kind                 0.25%
           Full House                  1.17%
           Flush                       1.30%
           Straight                    1.35%
           3 of a Kind                 7.5%
           Two Pair                   13.0%
           Pair of Jacks or Better    22.0%

           TOTAL                      46.6%


*******************************************************************/
 
#include <treasure.h>
#include <generic_items.h>

#include "../casino.h"

#define RECORD_WINS 1                /* 1=record wins, 0=don't     */ 
#define RECORD_LOSSES 1
#define GAME_LOG "LOGS/VIDPOKER1.LOG"
#define TP this_player()
#define QN query_name()
#define QRN query_real_name()
#define TRUE 1                       /* boolean flag               */
#define FALSE 0                      /* boolean flag               */
#define ERROR -1                     /* general error flag         */
#define DEAL 1                       /* get_card arg               */
#define LOOKAT 2                     /* get_card arg               */

#define DELAY 1
#define DECK_SIZE 52                 /* number of cards in deck    */
#define HAND_SIZE 5                  /* number of cards dealt      */
#define DISCARD_MAX 3                /* max cards you can discard  */

#define BET_AMOUNT 500

/*******************************************************************/
/*********************** function prototypes ***********************/
void shuffle();
int  get_card( int pos, int func );
void  bubble_sort( int *arry1, int *arry2 );
int  evaluate();
void delay_func( string message );
void show_cards();
void show_hand();


/*******************************************************************/
/************************* global vars *****************************/


string name;                         /* players in the game      */
string card_num, card_suit;          /* text of most recent card */
string result;

int c_num, c_suit;                   /* info on most recent card */
int *card;                           /* The deck of cards        */
int *hand;
int current;                         /* which card dealt next    */

int coins;     
int busy;
int playing;
int number_of_deals;      /* Total games since reset              */
int machine_profits;      /* The cumulative profit since reset    */
int bet_volume;           /* Bet total since reset                */


object this_room;         /* the poker room                       */ 





/*******************************************************************
  Poker table object maintenance 
 *******************************************************************/

void create_treasure()
{
  int min;
  int num,suit;
  string desc;
  
   desc = "It is a large machine painted blue with the familiar\
 club, heart, diamond, and spade common to most playing cards. \
 In the front of the machine is a window through which you can\
 see a deck of cards.  Below the window is a button, a slot\
 for inserting coins, and a small plaque.\n\n\
 You can \'insert <howmany>  coins\', \'press button'\, and \'discard\
 <a> <b> <c>\' where a,b,c are the card numbers to discard.\n";

  if ( (int)(TP->query_coder_level())  > 0 )
    desc += " Coders can \'view profits\'.\n";

  set_id( ({"poker","machine"}) );
  set_short("A magical blue poker machine");
  set_long( desc );


  /** initialize global vars **/

  number_of_deals = 0;
  playing = FALSE;
  busy = FALSE;

  card = allocate( DECK_SIZE );
  min = 0;
                                               /** generate deck **/ 
  for ( num=0; num<13; num++ )
   for ( suit=0; suit<4; suit++ )
     {                                    /** lower 4 bits -> suit **/
       card[min] = suit | ( num << 4 );  /** upper bits   -> rank **/ 
       min++;
     }
}

/********************************************************************/

status get() { return 0; }

void init_treasure()
{
  add_action("insert_cmd", "insert");
  add_action("read_cmd", "read");
  add_action("view_cmd", "view");
  add_action("press_cmd","press");
  add_action("discard_cmd","discard");

  this_room = environment(TP);

}



/*******************************************************************
  Process the read command
*******************************************************************/

status read_cmd(string arg)
{
  string temp;

  if (!arg || sscanf(arg, "%s", temp ) != 1 || temp != "plaque" )
   {
      notify_fail("Read what?\n");
      return 0;
   }



  write("The payoffs for this machine are:\n\n\
            FINAL HAND              Payout per   Payout for\n\
                                    500 coins    2500 coins\n\
          -------------------------------------------------\n\
           Royal Flush               125000        400000\n\
           Straight Flush             25000        125000\n\
           4 of a Kind                12500         62500\n\
           Full House                  4000         20000\n\
           Flush                       2500         12500\n\
           Straight                    2000         10000\n\
           3 of a Kind                 1500          7500\n\
           Two Pair                    1000          5000\n\
           Pair of Jacks or Better      500          2500\n\n");



  return 1;

}





/*******************************************************************
  Process the insert command
*******************************************************************/

status insert_cmd(string arg)
{
string temp;

   if ( arg && ( sscanf( arg, "%s", temp ) == 1 )&&( temp == "coins" ))
     coins = BET_AMOUNT;
   else
   if (!arg || ( sscanf(arg, "%d %s", coins, temp ) != 2 )||( temp != "coins" ))
   {
      notify_fail("Insert what?\n");
      return 0;
   }


   if ( playing == TRUE )
    {
      notify_fail("You must wait till the game is over.\n");
      return 0;
    }


  if ( busy == TRUE )
   {
      notify_fail("Someone has already inserted coins into this machine.\n");
      return 0;
   }

   if (( coins != 500 )&&( coins != 1000 )&&( coins != 1500 )
      &&( coins != 2000)&&( coins != 2500 ))
    {
      notify_fail("You can only insert 500, 1000, 1500, 2000, or 2500 coins.\n");
      return 0;
    }


   if (TP->query_money() < coins  )
   {
      notify_fail("You don\'t have "+coins+" coins to insert.\n");
      return 0;
   }

   TP->tell_me("Ok. You insert "+coins+" coins into the machine.");

   environment(TP)->tell_here(
         TP->QN + " inserts " +coins+ " coins into the machine.", TP);

   TP->add_money(-(coins));         /* subtract the bet from player's cash */

   if ( !( TP->query_coder_level()) )
     bet_volume += coins;

   busy = TRUE;

   return 1;
}



/***************************************************************************
  Process the deal command
 ***************************************************************************/

status press_cmd(string arg)
{
 string temp;
 int i;

  if (!arg || sscanf(arg, "%s", temp) != 1 || temp!="button" )
   {
      notify_fail("Press what?\n");
      return 0;
   }

   if (( playing == TRUE )||( busy == FALSE ))
    {
      notify_fail("Nothing happens.\n");
      return 0;
    }

   this_room->tell_here(TP->QN + " presses the button.", TP);

   call_out("delay_func", DELAY, "The machine starts to hum!");

   current = 0;
   playing = TRUE;

   if ( !( TP->query_coder_level()) )
      number_of_deals++;
                                          /** shuffle the deck **/
   for ( i=0; i<random(2)+2; i++ )
        shuffle();

   call_out("delay_func", DELAY+1, "The cards in the machine are magically shuffled!");


   hand = allocate( HAND_SIZE );
   name = TP->QRN;
                                            /* deal the cards */
   /* Note: The deck never changes until the next shuffle     */
   /*       As cards are dealt, "current" increments to point */
   /*       to the next card to be dealt.                     */

    for ( i=0; i<HAND_SIZE; i++ )
        hand[i] = get_card( current++, DEAL );

    call_out("delay_func", DELAY+2, "The cards are magically dealt!");
    call_out("show_hand", DELAY+3 );

  return 1;
}






/****************************************************************************
 discard cards.
 ***************************************************************************/

status discard_cmd( string arg )
{
  string person, temp;
  int *c, i,j, discarding, args;

  person = TP->QRN;

  if ( playing == FALSE )
    {
      notify_fail("There are no cards to discard.\n");
      return 0;
    }

  c = allocate( HAND_SIZE );
  discarding = TRUE;
                                           /** discard none **/
  if (arg &&( args = sscanf( arg, "%s", temp ) == 1 )
           &&( temp == "none" ))
      discarding = FALSE;


  i=0;
  if ( discarding == TRUE )
    {

       if ( !arg || ( args = sscanf( arg, "%d %d %d %d %d",
                c[0], c[1], c[2], c[3], c[4] )) < 1 )
         {
            notify_fail("Discard what?\n");
            return 0;
         }

       if ( args > 3 )
         {
            notify_fail( "You cannot discard more than "
                 +DISCARD_MAX+" cards.\n");
            return 0;
         }

       for ( i=0; i<args; i++ )
          if (( c[i] < 1 )||( c[i] > HAND_SIZE ))
           {
             notify_fail("Invalid card number: "+c[i]+".\n" );
             return 0;
           }


       for ( i=0; i<args; i++ )
         {

           c[i]--;

           for ( j=0; j<i; j++ )
             if ( c[i]==c[j] )
               {                   /** cant discard same card twice **/
                  c[i] = ERROR;
               }
         }


       for ( i=0; i<args; i++ )
         if ( c[i] != ERROR )             /** get the new cards **/
              hand[ c[i] ] = get_card( current++, DEAL );


       this_room->tell_here(TP->QN+" discards "+args+" cards.",TP );
       TP->tell_me("Ok, the cards are discarded and replaced...");

       call_out("delay_func", DELAY, "The final cards are:");

       call_out("show_hand", DELAY+1 );

     }
    else
      {
       TP->tell_me("Ok, you don't discard any cards.");

       this_room->tell_here(TP->QN+" doesn't discard any cards.",TP);
      }


   call_out("show_cards", DELAY+2 );

   return 1;

} /** discard_cmd **/





/****************************************************************************
  view 
    - cards, hand
    - pot
    - status
 ****************************************************************************/

status view_cmd( string arg )
{
  string who, temp;
  int tmp,i;

  if (!arg || sscanf(arg, "%s", temp) != 1 )
   {
      notify_fail("View what?\n");
      return 0;
   }

  who = TP->QRN;
                                                         /*** view cards ***/
  if (( temp == "cards" )||( temp == "hand" ))
   {
                                                       /** are there cards? **/ 
     if ( playing == FALSE )
       {
         notify_fail("The deck has not yet been dealt.\n");
         return 0;
       }
 
     show_hand();

     if ( random( 2 ) == 1 )
        this_room->tell_here(TP->QN+" studies the cards intently",TP );
     else
        this_room->tell_here(TP->QN+" glances at the cards.", TP );

     return 1;
   }

  if (( temp == "profits" )&&( TP->query_coder_level() ))
    {
      TP->tell_me("The total profits for this machine are "+machine_profits+
            " in " +number_of_deals+ " plays.\n");

      TP->tell_me("Total bet volume: "+bet_volume+" coins.");

      return 1;
    }


  return 0;

}  /** view_cmd **/



/****************************************************************************
  Shuffle the cards.
 ****************************************************************************/

void shuffle()

{
  int i,rnd;
  int temp;
                                  /** swap each card with a random card */
  for ( i=0; i<DECK_SIZE; i++ )     
   {                              /** don't swap a card with itsself **/ 
      while (( rnd=random( DECK_SIZE )) == i );

      temp = card[rnd];      /** swap **/
      card[rnd] = card[i];
      card[i] = temp;
      
   }
}    /** swap **/



/****************************************************************************
  Deal a card from the deck.
 ****************************************************************************/

int get_card(int pos, int func )     
{
   int temp;

   temp = ( func == DEAL ) ? card[pos] : pos;

   c_suit = temp & 15;                      /** clear high bits **/ 
   c_num   = ( temp ^ c_suit ) >> 4;        /** clear low bits  **/

   if (c_num<0)  return ERROR;

   switch( c_suit )
    {
      case 0 :  card_suit = "CLUBS";    break;
      case 1 :  card_suit = "SPADES";   break;
      case 2 :  card_suit = "HEARTS";   break;
      case 3 :  card_suit = "DIAMONDS"; break;
      default:  return ERROR;
    }

   switch( c_num )
    {
      case 0 :  card_num = "  2  "; break;
      case 1 :  card_num = "  3  "; break;
      case 2 :  card_num = "  4  "; break;
      case 3 :  card_num = "  5  "; break;
      case 4 :  card_num = "  6  "; break;
      case 5 :  card_num = "  7  "; break;
      case 6 :  card_num = "  8  "; break;
      case 7 :  card_num = "  9  "; break;
      case 8 :  card_num = " 10  "; break;
      case 9 :  card_num = "JACK "; break;
      case 10:  card_num = "QUEEN"; break;
      case 11:  card_num = "KING "; break;
      case 12:  card_num = " ACE "; break;
      default:  return ERROR;
    }

   return temp;
}




/****************************************************************************
  bubble.sort.
    - sorts a hand into decending rank order
 ****************************************************************************/

void bubble_sort( int *arry1, int *arry2 )
{
  int i,j, temp;

  for ( i=0; i< HAND_SIZE; i++ )
   for ( j=(HAND_SIZE-1); j>i; j-- )
     if ( arry1[j] > arry1[i] )
       {
          temp = arry1[j];         /** swap the cards **/
          arry1[j] = arry1[i];
          arry1[i] = temp;
          temp = arry2[j];
          arry2[j] = arry2[i];
          arry2[i] = temp;
       }
}






/****************************************************************************
  show hand
    - shows a player's hand, either to him or everyone
 ****************************************************************************/

void show_hand()

{
   int j;

   for ( j=0; j<HAND_SIZE; j++ )
    {
      get_card( hand[j], LOOKAT );

      this_room->tell_here("   "+(j+1)+": "+card_num+" of "+card_suit+"\n");
    }
}





/***************************************************************************
  Called by call_out to display the delayed messages
***************************************************************************/

void delay_func( string message )
{
         this_room->tell_here( message );
}




 
/****************************************************************************
  show cards.
    - called at game end to evaluate and compare hands
 ****************************************************************************/

void show_cards()

{
   object person, money;

   int payoff;

   payoff = evaluate()*coins;

   this_room->tell_here("A hollow voice from the machine says: "
         +"The hand is "+result+".");



   if ( payoff > 0 )
    {

      if ( person = present( name, this_room ))
        {

            this_room->tell_here( person->QN 
                          +" wins "+payoff+" coins!", person);

            person->tell_me("You win "+payoff+" coins!");
            person->add_money( payoff );
         }
       else
        {
            this_room->tell_here( payoff+" coins fall out of the machine!");

           money = clone_object(GENERIC_MONEY_OBJECT);
           money->set_money( payoff );
           move_object( money, this_room );
      
         }

      person = find_player( name );

      if ( !( person->query_coder_level()) )
        machine_profits +=  coins - payoff;

#if RECORD_WINS == TRUE
          if ( !( person->query_coder_level()) )     /** lets not log coders **/
            write_file( AREA_PATH+GAME_LOG, capitalize(name)+", "
                    +payoff+", "+result+", "+machine_profits+", "
                    +number_of_deals+"\n" );
#endif


    }
   else
    {

      person = find_player( name );

      if ( !( person->query_coder_level()) )
         machine_profits += coins;


#if RECORD_LOSSES == TRUE
       if ( !( person->query_coder_level()) )     /** lets not log coders **/
            write_file( AREA_PATH+GAME_LOG, capitalize(name)+", "
                    +payoff+", "+(-coins)+", "+machine_profits+", "
                    +number_of_deals+"\n" );

#endif

    }

                    /** re-init global vars **/
  playing = FALSE;
  busy = FALSE;



} /** show_hands **/



/****************************************************************************
  Evaluate hand.
 ****************************************************************************/

int evaluate()
{
  int i,j, flag;
  int temp;
  int *spread; 
  int score;
  int *cn, *cs;
  int flush, straight;
  int three_kind;
  int two_pair; 
  int one_pair;

  cn = allocate( HAND_SIZE );
  cs = allocate( HAND_SIZE );
  spread = allocate( 13 );

  for ( i=0; i<HAND_SIZE; i++ )
   {
     cs[i] = hand[i] & 15;                 /** clear high bits **/
     cn[i]   = ( hand[i] ^ cs[i] ) >> 4;   /** clear low bits  **/
   }

  bubble_sort( cn, cs );               /** sort hand into decending order */
   
  flush = TRUE;
  straight = TRUE;
  score = cn[0];

  j = HAND_SIZE-1;

  for ( i=1; i<HAND_SIZE; i++ )
     {
        if ( cs[j-i] != cs[j] )                       /** look for flush **/
          flush = FALSE;

        if (( cn[j-i] - cn[j] ) != i )             /** look for straight **/
          {
            if (( cn[j-i] == 12 )&&( cn[j] == 0 )
                &&(straight==TRUE )&&( i == j ))     
               score = cn[1];                       /** 5-high straight **/
            else
               straight = FALSE;                   /** regular straight **/
          }
     }
 
  if ( flush == TRUE )
     {
        if ( straight == TRUE ) 
           {
              if ( cn[4] == 8 )                 /** if low card is a 10 **/
               {
                  result = "a royal flush";
                  if ( coins == 2500 )
                    return 800;
                  else
                    return 250;
               }
              else
               {
                  result = "a straight flush";  
                  return ( 50 );
               }
           } 
       else
           {
             result = "a flush";
             return ( 5 );
           }
     }   

  if ( straight == TRUE )
    {
      result = "a straight";
      return ( 4 );
    }

  three_kind = FALSE;
  two_pair = FALSE;
  one_pair = FALSE;
  score = 0; 

  for ( i=0; i<13; i++ )
     spread[i] = 0;

  for ( i=0; i< HAND_SIZE; i++ )
     spread[ cn[i] ]++;


  flag = FALSE;

  for ( i=0; i<13; i++ )
     {
       if ( spread[i] == 2 )              /** two of a kind **/
         {
          if ( i > 8 )                    /** Jacks or better **/
             flag = TRUE;

          if ( one_pair == TRUE )
            {
               two_pair = TRUE;
               score += 1;
            }
          else
            {
               one_pair = TRUE;
               score += 1;
            }
          }
       else
       if ( spread[i] == 3 )              /** three of a kind **/
         {
            three_kind = TRUE; 
            score += 3;
         }
       else
       if ( spread[i] == 4 )               /** four of a kind **/
         {
            result = "four of a kind";
            return 25;
         }

     } 
      

   if ( three_kind == TRUE )
     if ( one_pair == TRUE )
       {
         result = "a full house";
         return 8;
       }
     else
       {
         result = "three of a kind";
         return score;
       }

   if ( two_pair == TRUE ) 
      {
         result = "two pairs";
         return score;
      }

   if (( one_pair == TRUE )&&( flag == TRUE ))
      {
         result = "a pair of Jacks or higher";
         return score;
      }

   result = "worth nothing";
   return 0;

}


#include <nroom.h>
#include <mud_name.h>
#include <areas.h>

#define BANK_DIR        AREA_TRISTEZA "bank/"
#define RICH_LOG        "/areas/tristeza/bank/RICH_LOG"
#define LARGE           "/areas/tristeza/bank/LARGE_TRANSFERS"
#define BANK_CLOSED     "Bank seems to be closed."

private static string ts;

void create_room()
{
    int i;

    set(IN_CITY);
    set(NO_PK);
    set(NO_CLEANUP);
    set_short("The bank");
    set_long("The First Mudional Bank of "+MUD_NAME+". \
Everything is new and shiny (as it should be in an efficient facility \
like this). You can see marble everywhere: walls, the floor and even \
the mighty pillars are made of it. Something catches your eye in the \
wall at the back of the bank hall, but you cannot quite get what it is.\n\
There is a plaque behind the counter.");
    set_light_level(LT_LIGHTS);
    set(ROOM_WD, WD_INDOORS);
    add_object(BANK_DIR "staircase", 1);
    set_exits( ([
	"east"    : AREA_TRISTEZA+"virtual/S10_5",
	"southwest" : AREA_TRISTEZA+"virtual/S8_6"
      ]) );
    set_items( ([
	"plaque" : "Instructions for customers.\n"+
	"deposit xxx   - deposit the amount of xxx coins.\n"+
	"withdraw xxx  - withdraw the amount of xxx coins.\n"+
	"balance       - shows your current balance.\n"+
	"close account - withdraw everything and close your account.",
	"counter" : "It's a nice looking counter made of ebony and marble.",
	({ "wall", "northern wall" }) :
	"There is a definitive touch of magic in the northern wall",
	"walls" : "They are made of marble.",
	"marble" : "It's new and shiny."
      ]) );
    set_commands(([
	"deposit"   : "do_deposit",
	"withdraw"  : "do_withdraw",
	"balance"   : "do_balance",
	"close"     : "do_close",
	"north"     : "north",
      ]) );
    ts = "Teller says: ";
    add_monster(BANK_DIR "teller",
      "Teller arrives. Probably she was just having a coffee break "+
      "or something.", 1);
    for (i=0 ; i < 4 ; i++)
	add_monster(AREA_TRISTEZA+"npc/guard_sergeant",
	  "Guard Sergeant arrives.", 1);
    add_monster(AREA_TRISTEZA+"npc/guard_lieutenant",
      "Guard Lieutenant arrives.", 1);
    add_monster(AREA_TRISTEZA+"npc/guard_captain", 
      "Guard Captain arrives.", 1);
    (AREA_TRISTEZA+"bank/bank_back")->load_me();
}

status closed()
{
    if (!present("teller", this_object()))
	return 1;
    return 0;
}

status read_cmd(string str)
{
    if ( !str || strstr(str,"plaque")==-1)
	return 0;
    this_player()->tell_me(query_long(str, this_player()));
    return 1;
}

string extra_look()
{ 
    // Hmm.. I don't think this is displayed any more? --Kieve
    if (closed())
	return BANK_CLOSED;
    return "";
}

static void write_rich(object ob)
{
    write_file(RICH_LOG, sprintf("%s(%d): balance: %d money: %d time: %s\n",
	capitalize(ob->query_real_name()), ob->query_level(),
	ob->query_bank_money(), ob->query_money(), ctime(time()) ));
}

static void write_large_transfers(object ob, string action, int amount)
{
    write_file(LARGE, sprintf("%s%s%d at %s\n",
	capitalize(ob->query_real_name()), action, amount,
	ctime(time()) ));
}

status do_deposit(string str)
{
    // We will deposit 'str' amount of money into the bank account
    int amount, rich_money;

    if (closed()) {
	this_player()->tell_me(BANK_CLOSED);
	return 1;
    }

    if (!str) return notify_fail("Deposit what?\n"), 0;
    sscanf(str,"%d",amount);
    if (!amount || amount <= 0) {
	this_player()->tell_me(ts + "That was not a valid amount.");
	return 1;
    }

    if (this_player()->query_money() < amount) {
	this_player()->tell_me(ts + "Count your coins again! You don't "+
	  "have that much money.");
	return 1;
    }
    this_player()->add_bank_money(amount);

    this_player()->add_money(-amount);
    if( amount >= 100000 )
	write_large_transfers(this_player(), " deposits ", amount);

    this_player()->tell_me(sprintf("Your account swells by %d coin%s. "+
	"Gives one a great feeling, eh?\n",
	amount, (amount == 1) ? "" : "s"));

    this_object()->do_balance("");

    rich_money = (int) this_player()->query_money() +
    this_player()->query_bank_money();
    switch (this_player()->query_level())
    {
    case 1..4:    if (rich_money>10000) write_rich(this_player()); break;
    case 5..9:    if (rich_money>30000) write_rich(this_player()); break;
    case 10..14:  if (rich_money>50000) write_rich(this_player()); break;
    case 15..19:  if (rich_money>300000) write_rich(this_player()); break;
    case 20..24:  if (rich_money>500000) write_rich(this_player()); break;
    default:      if(rich_money>1000000) write_rich(this_player());
    }
    return 1;
}

status do_withdraw(string str)
{
    // Withdraw money from the player's bank account
    int amount;

    if (closed()) {
	this_player()->tell_me(BANK_CLOSED);
	return 1;
    }

    if (!str) return notify_fail("Withdraw what?\n"), 0;
    sscanf(str,"%d",amount);
    if (!amount || amount <= 0)
    {
	this_player()->tell_me(ts+"That is not a valid amount.");
	return 1;
    }
    if (amount > this_player()->query_bank_money()) {
	this_player()->tell_me(ts+"You don't have that much money on your "+
	  "account.\n");
	this_object()->do_balance();
	return 1;
    }

    this_player()->add_bank_money(-amount);
    this_player()->add_money(amount);

    if( amount >= 100000 )
	write_large_transfers(this_player(), " withdraws ", amount);

    this_player()->tell_me(sprintf("Your account shrinks by %d coin%s. "+
	"So does your ego. *sigh*\n", amount, (amount == 1) ? "" : "s" ));

    if(this_player()->query_bank_money())
	this_object()->do_balance();
    else
	this_player()->tell_me("Your account is now empty! So is your "+
	  "stomach...");

    return 1;
}

status do_balance(string str)
{
    // Tell the player what their balance is.
    int amt;

    if (closed()) {
	this_player()->tell_me(BANK_CLOSED);
	return 1;
    }

    amt = this_player()->query_bank_money();
    this_player()->tell_me(sprintf("Balance: %d gold coin%s.",
	amt, (amt == 1) ? "" : "s"));
    return 1;
}

status do_close(string str)
{
    // Withdraw everything.
    if (closed()) {
	this_player()->tell_me(BANK_CLOSED);
	return 1;
    }

    return this_object()->do_withdraw(sprintf("%d", 
	this_player()->query_bank_money()));
}

status north()
{
    if (closed()) {
	this_player()->tell_me("The wall looks solid...");
	return 1;
    }
    this_player()->tell_me("Teller tells you: Oh, you want to see the "+
      "back room. I'll let you go there.");
    this_player()->move_player("through the northern wall",
      AREA_TRISTEZA+"bank/bank_back");
    return 1;
}

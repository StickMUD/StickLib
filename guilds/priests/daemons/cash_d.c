#include "../priest.h"
#define CASH_FILE PRIEST_DIR "save/savings"

int cash;
int temp_cash;
static int open,stolen,shop_cash;

void
create() {
    restore_object(CASH_FILE);
    if(temp_cash>0) {
	cash+=temp_cash;
	write_file(CASHLOG, sprintf("Guild Shops: +%d on %s\n",
	    temp_cash, ctime(time())));
	temp_cash = shop_cash = 0;
	save_object(CASH_FILE);
    }
    write_file(SHOPLOG, sprintf("*** Reboot at %s. ***\n", ctime(time())));
}


add_money(int amount,string who){
    cash += amount;
    who = capitalize(who);
    write_file(CASHLOG,sprintf("%s +%d on %s\n",who,amount,ctime(time())));
    save_object(CASH_FILE);
    return cash;
}


subtract_money(int amount,string who){
    cash -= amount;
    who = capitalize(who);
    write_file(CASHLOG,sprintf("%s  -%d  on %s\n",who,amount,ctime(time())));
    save_object(CASH_FILE);
    return cash;
}
query_cash(){
    return cash;
}

query_open(){ return open;}

status
open_close(){
    open = 1-open;
    return open;
}

query_stolen(){ return stolen;}
set_stolen(){stolen=1;return stolen;}

from_shop(int amount){
    shop_cash += amount;
    if(shop_cash >= 20000) {
	cash += shop_cash;
	write_file(CASHLOG, sprintf("Guild Shops: +%d on %s\n",
	    shop_cash, ctime(time())));
	shop_cash = temp_cash = 0;
    }
    temp_cash = shop_cash;
    save_object(CASH_FILE);
    return shop_cash;
}               
/* Treasury Server for New Guild System */

void
debug() {
    printf("cash: %d, open: %d, stolen: %d, shop_cash: %d\n",
      cash, open, stolen, shop_cash);
}

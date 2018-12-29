id(str) { return str == "rose" || str == "flower" || str == "black rose"; }
short() { return "A perfect black rose"; }
long() {
   write("This flower is certainly created by powerful magic.\n");
}
query_value() { return 30; }
query_weight() { return 1; }
get() { return 1; }
drop() { return 0; }
init() {
   add_action("smell","smell");
   add_action("smell","sniff");
}
smell(str) {
   if(!str || !id(str)) return 0;
   write("It smells very good.\n");
   return 1;
}

/* room/city/street.c */

#include <nroom.h>

public void
create_room() {

   /* Defaults */
	set(NO_PK);
	set(IN_CITY);
	set_light_level(LT_LIGHTS * 2);
	set_outdoors(WD_OUTDOORS);
	this_object()->create_street();

}

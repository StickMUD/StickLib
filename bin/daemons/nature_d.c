/*
** /complex/nature.c  "The Mother Nature"
**
** Weather & Time mechanisms for Stickmud.
**
** Based on game_clock and weather_obj by Geoff Wong. (2.4.5)
** & game_clock once rewritten by Val. (for 3.1.2 COMPAT)
** And all the rest by Starbyt. (*bounce*, Aren't we proud..:-)
**
** //Graah: added Yearname (default "the Lords") plus set_Yearname()
**
** //Graah: Sunset and sunrise fixed to make nights shorter, since
**          nights are DARK now!
**
** //Graah: Longer nights.
**
** //Doomdark: Now calls all player objects to inform when sun rises
**  and sets. Saves thousands of call_others...
***

BASICS

Game clock is running within heart_beat, based on the use of time() efun.
Heart_beat will call special actions:

weather_forecast	Every 30th heart_beat (60 seconds)
			this will maintain weather related functions and
			effects. (rain, wind, temp, 'quake, sunset)

hour_glass		Every exact gametime hour (REAL_TO_GAME mins)
			this will keep track of:
			- morning, day, evening, night cycling.
			- moons and sun and season
			- if it's midning, checking and running "themeday"
			  functions.


YEAR

Consists of 12 months, each having 28 days. Year is divided into four
seasons: winter, spring, summer, autumn.

MONTH		SEASON	AVG	SUNRISE	AVG %
			TEMP	/SET H.	HUMID

 Elenamonth	Winter	-10'C	7  17	20
 Frobozzmonth	Winter	-5'C    6  18	30
 Moonset	Spring	5'C     5  19	60
 Evensend	Spring  15'C    4  20	70
 Tuulneas	Summer  20'C    3  21	80
 Stormmonth	Summer  25'C    3  22	70
 Waterhaze	Summer  30'C    4  22	40
 Virrsmonth	Summer  30'C    4  22	30
 Harvestmonth	Autumn  20'C    5  21	40
 Nethory	Autumn  10'C    5  20	50
 Okdyetenn	Winter  -5'C    6  19	40
 Icemonth	Winter  -15'C   6  18	30

The exact day of change of two seasons is determined by random(): 15% change
for each day after the change of a month, max 1 month. (logic behind this:
Greenhouse effect, ozone layer, and so on.)

MOONS

Moons are important part of the lives, since they symbolize the magic
and destiny of a living kind. We have two moons going 'round our planet
(which, btw, is flat, not round) Celerana (golden wanderer) and Malerana
(silver wanderer). Celerana's circulation time is 1 month (exact), and
Malerana's is about 4 months. At year 0, both of these were new at the
very same moment. And strange enough, when it is night, both of the moons
hang on the sky. But yet again, so what? ;-).

Celerana (golden wanderer), emits golden light to the darkness of nights.
It is symbol of goodness, faith and truth. Some priests are known to get
their spells by the state of this moon.

Malerana (silver wanderer), emits dim white light to the darkness of nights.
It is symbol of greedyness, evil and undeads. When malerana is full, beware
the werevolves!


*/

#include "/sys/configuration.h"

#include <room_defs.h>
#include <gametime.h>
#include <living_defs.h>
#include <coder_levels.h>
#include <attack.h>

#pragma strict_types

#define SAVETIME	900	/* heartbeats between saves */
#define	SAVEFILE	"/data/d/nature_d/nature_save"
inherit "/room/weathersave";
inherit "/lib/timerequester";


int Year;	// unlimited
int Month;	// 1...MONTHS
int Day;	// 1...DAYS
int Hour;	// 0...HOURS-1
int Minute;	// 0...MINUTES-1

string		Season;

int Temp;	/* -50...50 'C Temperature */
int Humid;	/* 0..100% Humidity */
int Wind;	/* 0..20 m/s Wind speed */
int Clouds;	/* 0 = none, 1 = few, 2 = some, 3 = lot */
int Rain;	/* 0 = none, 1 = little, 2 = some, 3 = lot */
int Fog;	/* 0 = none, 1 = tiny mist, 2 = fog, 3 = heavy fog */

string          Yearname;

static	int	time, ctime, beatcount, uptime, oldhour, sun, quakecount;

// Prototypes

int query_night();
string query_long_weather();
string query_short_weather();
void Inform(string s, int x);
string Official_Season(int m);
int SunriseH(int m);
int SunsetH(int m);
int Avg_Temp(int m);
int Avg_Humid(int m);
void hour_glass();
void weather_forecast();
string Rain_Type();

// Anyone can change year's name, but only admins should do it.
void set_Yearname(string arg) { Yearname = arg; }

void
update()
{
    int t;

    if ((t = time()) == time) return;	/* speedup */

    t = (time = t) / REAL_TO_GAME + ctime;
    Minute = t % MINUTES;		   /* 0 .. MINUTES - 1 */
    Hour   = (t /= MINUTES) % HOURS; /* 0 .. HOURS - 1 */
    Day    = (t /= HOURS) % DAYS+1;  /* 1 .. DAYS */
    Month  = (t /= DAYS) % MONTHS+1; /* 1 .. MONTHS */
    Year   = t /= MONTHS;

    // Won't need; sun is set/cleared later. If we set it here, we won't get nice
    // 'Sun rises." etc. messages. -+ Doomdark +-
    //  sun = (Hour < SunsetH(Month) && Hour >= SunriseH(Month));
}

int
convert_hours_minutes(int h, int m)
{
    return (h*MINUTES +m)*REAL_TO_GAME;
}

// Moon phases

string
moonphase(int C)
{
    string desc = "ERROR IN MOONPHASE";

    C = ((Year * MONTHS * DAYS) + ((Month-1) * DAYS) + (Day-1)) % C * 6 / C;

    switch(C)
    {
    case 0: desc = "dark";
    case 1: desc = "waxing crescent";
    case 2: desc = "waxing gibbous";
    case 3: desc = "full";
    case 4: desc = "waning gibbous";
    case 5: desc = "waning crescent";
    }

    return desc;
}

string query_celerana_phase()	{ return moonphase(CELERANA_CIRCUIT); }
string query_malerana_phase() { return moonphase(MALERANA_CIRCUIT); }

// Time related queries

int query_uptime()	{ return time()-uptime; }
string query_uptime_s()	{ return ctime(time()-uptime); }

int query_year()	{ update(); return Year; }
int query_month()	{ update(); return Month; }
int query_day()		{ update(); return Day; }
int query_hour()	{ update(); return Hour; }
int query_minute()	{ update(); return Minute; }
string query_season()	{ update(); return Season; }

string
query_time_of_day()
{
    int	s,r;
    update();
    s = SunsetH(Month);
    r = SunriseH(Month);

    if (Hour>s+2) return "night";
    if (Hour>s-2) return "evening";
    if (Hour>r+2) return "day";
    if (Hour>r-2) return "morning";
    return "night";
}

int *query_game_time()	{ update(); return ({ Minute, Hour, Day, Month, Year }); }
int query_24_hour()	{ update(); return query_hour(); }
int query_12_hour()	{ update();
    if (!Hour) return HOURS/2;
    return((Hour>(HOURS/2)) ? Hour-HOURS/2 : Hour); }
int query_noon()	{ update(); return (Hour==HOURS/2 && !Minute); }
int query_midnight()	{ update(); return !Hour && !Minute; }

varargs string
query_month_s(int m)
{
    if (m) {
	if (m<1 || m>MONTHS) return "ILLEGAL MONTH";
    } else {
	update();
	m = Month;
    }

    return ({
      "Elenamonth",	"Frobozzmonth",	"Moonset",
      "Evensend",	"Tuulneas",	"Stormmonth",
      "Waterhaze",	"Virrsmonth",	"Harvestmonth",
      "Nethory",	"Okdyetenn",	"Icemonth"	}) [m-1];
}


string
FIXN(int x)
{
    if (x < 10) return sprintf("0%d", x);
    return to_string(x);
}

string query_time()    { update(); return FIXN(Hour) + ":" + FIXN(Minute); }

string H12L(int x) {
    if (x>=HOURS) x-=HOURS;
    if (x==0) return "midnight";
    if (x==HOURS/2) return "noon";
    if (x>HOURS/2) return x-(HOURS/2) + " pm";
    return x + " am";
}

string
query_about_time()
{
    string desc = "ERROR IN MINUTE";

    update();
    switch(Minute) {
    case 0..7: desc = H12L(Hour);
    case 8..22: desc = "quarter past " + H12L(Hour);
    case 23..37: desc = "half past " + H12L(Hour);
    case 38..52: desc = "quarter to " + H12L(Hour+1);
    case 53..59: desc = H12L(Hour+1);
    }

    return desc;
}

string query_date()	{ update(); return Month+"/"+Day+"/"+Year; }
string query_date_s()	{ update(); return query_month_s() + " " + Day + ", " + Year; }

string query_date_b()
{
    string	endung, s;
    update();
    if (Day>10 && Day<20) endung = "th";
    else switch(Day%10) {
    case 1: endung = "st"; break; case 2: endung = "nd"; break;
    case 3: endung = "rd"; break; default: endung = "th";	}
    return	"Today is " + query_month_s() + " " + Day + endung +
    " in the Year of " + Yearname + " " + Year + ".\n" +
    "An extraordinary " + Season + " " + query_time_of_day() +
    ", about " + query_about_time() + ".\n";
}

void
create()
{
    object *u;
    int i;
#ifdef UIDS
    seteuid(getuid());
#endif
    configure_object(this_object(), OC_HEART_BEAT, 1);
    uptime = time();
    if (!restore_object(SAVEFILE)) {
	Year = 600;
	Season = "winter";
	Month = 1;
	Day = 1;
	Temp = Avg_Temp(Month);
	Humid = Avg_Humid(Month);
	Yearname = "the Distribution";
    }
    sun = (Hour < SunsetH(Month) && Hour >= SunriseH(Month));

    ctime = ((((Year * MONTHS + Month-1) * DAYS + Day-1) * HOURS) +
      Hour) * MINUTES + Minute - (time() / REAL_TO_GAME);

    u = users();
    if (pointerp(u)) {
	i = sizeof(u)- 1;
	while (i >= 0)
	    u[i--]->set(LIV_NIGHT, (sun ? 0 : 1));
    }
}

void
heart_beat()
{
    object *u;
    int x;
    status tmp;

    update();
    if (quakecount) switch(quakecount--) {
    case 60:
	Inform("You start to feel bit strange.", WD_ALL);
	break;
    case 45: case 35:
	Inform("The ground below your feet rumbles quietly.", WD_ALL);
	break;
    case 25: case 20:
	Inform("The ground below your feet starts vibrating!", WD_ALL);
	break;
    case 15: case 13: case 4: case 12: case 10:
	u = users();
	for(x = sizeof(u) - 1; x >= 0; x--) {
	    if (!environment(u[x])) continue;
	    // OUTDOORS: 12.5%, ELSEWHERE: 25%
	    tmp = ((int)environment(u[x])->query(ROOM_WD) & WD_OUTDOORS)
	    ? 1 : 0;
	    if (!random(4 + 4 * tmp)) {
		u[x]->add_hp(-random(50));
		u[x] -> tell_me(
		  "THE GROUND BELOW YOUR FEET VIBRATES HEAVILY!\n\n" +
		  ({
		    "OUCH! Something falls from the ceiling and hits your head!",
		    "OUCH! You fall down and hit yourself hard on something!"
		  }) [tmp]);
	    } else u[x]->tell_me(
		  "THE GROUND BELOW YOUR FEET VIBRATES HEAVILY!");
	}
	break;
    case 1:
	Inform("The vibration below your feet quickly stops.", WD_ALL);
    }
    timerequest_clocktick(Hour, Minute);
    if (!(beatcount % SAVETIME)) {
	update();
	save_object(SAVEFILE);
    }
    if (!(beatcount % 30)) {
	// Was 120, shouldn't it be 30 if we check every minute?
	update();
	if (Hour != oldhour) {
	    hour_glass();
	    oldhour = Hour;
	}
	else weather_forecast();
    }
    beatcount++;
}


// Hourglass

void
hour_glass()
{
    object *u;
    int i;
    if (Hour == 0) { // Midnight
	if (Season != Official_Season(Month) &&
	  (Day > 20 || !random(10))) Season = Official_Season(Month);

	if (Month == 100 && Day == 1)
	    catch("/players/starbyt/NIGHTOFDEAD"->initialize());
	// BETTER WAY TO DO THIS, MAYBE
	// Midsummerfeast, XMas, Eastern,
	// The Day the Wizards Disappeared, and so on and so on..
    }

    // NEW! We'll inform all player objects about night/day; this way
    // we know whether it's night or day all the time!!!
    if (!sun && Hour == SunriseH(Month)) {
	u = users();
	if (pointerp(u)) i = sizeof(u) - 1;
	else i = -1;
	while (i >= 0) {
	    u[i--]->set(LIV_NIGHT, 0);
	}
	Inform("The Sun rises.", WD_OUTDOORS);
	sun = 1; 
	if (random(7)) call_out("cockadoodledoo", 10 + random(15));
    }

    else if (sun && Hour == SunsetH(Month)) {
	u = users();
	if (pointerp(u)) i = sizeof(u) - 1;
	else i = -1;
	while (i >= 0) {
	    u[i--]->set(LIV_NIGHT, 1);
	}
	Inform("The Sun sets.", WD_OUTDOORS);
	sun = 0;
    }

    if (Season=="winter" && !random(DAYS*HOURS)) quakecount = 61;
    /* avg. once per month GT (once / 5 days RT) */
}


void
cockadoodledoo()
{
    while(remove_call_out("kukkokiekuu")!=-1);
    Inform("You hear distant 'Cock-a-doodle-doo!'", WD_ALL);
    /* No more cattle- Starks, 02/15/99
      if (!random(7)) call_out("mooooh", 5+random(5));
    */
    /* This is getting bit furious with the animals..:-) */
}

void
mooooh()
{
    while(remove_call_out("mooooh")!=-1);
    Inform("You hear distant 'MOOOoooooooOOOOHH!'", WD_ALL);
}


/* Weather forecast */

void
weather_forecast()
{
    int	r;

    if (Wind && Rain && (Wind>10 || Rain>2 || Humid>60) && !random(10)) {
	if (random(2)) {
	    Inform("ROOOOOAAAAARR! Thunder blasts your ears!", WD_ALL);
	    return; /* Only one weather msg at once */
	}
	else {
	    Inform("Lightning strikes not too far away from here!", WD_OUTDOORS);
	    Inform("Suddenly lights flash rapidly.", WD_INDOORS);
	    if( ! random(10) ) {
		object *u;
		int     i;

		u = users();
		i = random( sizeof( u ) );
		if( environment( u[i] ) && (int)u[i]->query_coder_level() < 1 &&
		  ((int)environment(u[i])->query(ROOM_WD) & WD_OUTDOORS) ) {
		    u[i]->tell_me("You were too close! The lightning hits YOU!");
		    u[i]->tell_me("You miraculously ABSORB the lightning!");
		    u[i]->add_hp((int)u[i]->query_max_hp(), 1 );
		    u[i]->add_sp((int)u[i]->query_max_sp(), 1 );
		    u[i]->add_fp((int)u[i]->query_max_fp(), 1 );
#if 0
		    u[i]->add_hp( -2 * ( (int)u[i]->query_hp() ), 1 );
		    u[i]->hit_player( 1000000, DT_FORCE, 0, 0 );
#endif
		}
	    }
	    return;
	}
    }

    /* Temperature and Humidity follow average values */

    r = random(5)-2;
    if ( (!random(3) && r>0 && Avg_Temp(Month)<Temp)
      ||  (!random(3) && r<0 && Avg_Temp(Month)>Temp) ) r = -r;
    Temp += r;

    r = random(5)-2;
    if ( (!random(3) && r>0 && Avg_Humid(Month)<Humid)
      ||  (!random(3) && r<0 && Avg_Humid(Month)>Humid) ) r = -r;
    if (Humid+r>0 && Humid+r<100) Humid += r;

    if (Fog)
    {
	if (Humid<30 || Wind>13 || Rain && random(3)) {
	    Fog--;
	    Inform("The fog is getting thinner.",WD_OUTDOORS);
	    return;
	}
	else if (!random(3) && Fog<3) {
	    Fog++;
	    Inform("The fog is getting thicker.", WD_OUTDOORS);
	    return;
	}
    }
    else if (!Fog) if (random(100)<Humid-70-Wind*2-Rain*2+2*(Temp-Avg_Temp(Month))) {
	    Inform("Thin white mist starts gathering around.", WD_OUTDOORS);
	    Fog = 1;
	    return;
	}


    if (Rain) {
	if (!Clouds || (Humid<50 && (random(3) || Clouds<2))) {
	    Rain--;
	    Inform("The " + Rain_Type() + "ing is getting easier.", WD_OUTDOORS);
	    if (Rain_Type()=="rain") Inform("The noise above your head silences.", WD_INDOORS);
	    return;
	}
	else if (!random(3) && Rain<3) {
	    Rain++;
	    Inform("The " + Rain_Type() + "ing is getting stronger.", WD_OUTDOORS);
	    if (Rain_Type()=="rain") Inform("The noise above your head gets stronger.", WD_INDOORS);
	    return;
	}
    }
    else if (random(200)<Humid-50+Wind*2+2*(Temp-Avg_Temp(Month))) {
	Rain = 1;
	Inform("It starts " + Rain_Type() + "ing.", WD_OUTDOORS);
	if (Rain_Type()=="rain") Inform("Drumming noise starts above your head.", WD_INDOORS);
	return;
    }


    /* Clouds: Wind*5 % to change */

    if (Wind*5>random(100)) {
	r = random(3)-1;
	if (Clouds+r>=0 && Clouds+r<=3) {
	    Clouds += r;
	    if (Clouds==1 && r>0) Inform("Some clouds have sailed on to the sky.", WD_OUTDOORS);
	    if (Clouds==3) Inform("It is getting cloudy.", WD_OUTDOORS);
	    if (Clouds==0) Inform("Last white cloud sails to beyond the horizon, leaving the sky clear.", WD_OUTDOORS);
	    return;
	}
    }

    /* Wind: Random. Very random. */

    r = random(4)-2;
    if (Wind+r>=0 && Wind+r<=20) {
	Wind +=r;
	if (Wind==0 && !random(2)) Inform("The air stands still.", WD_OUTDOORS);
	if (Wind>10 && r>0 && !random(2)) Inform("The wind is getting strong!", WD_OUTDOORS);
	if (Wind<10 && r<0 && !random(2)) Inform("The wind is getting weaker.", WD_OUTDOORS);
    }

}


/* Weather queries */

int query_fog()		{ return Fog; }
int query_rain()	{ return Rain; }
int query_humid()	{ return Humid; }
int query_temp()	{ return Temp; }
int query_wind()	{ return Wind; }
int query_clouds()	{ return Clouds; }
int query_quake()	{ return quakecount; }
int query_night()	{ return !sun; }
int query_sun()		{ return sun && (Clouds<2) && !Fog; }
int query_moon()	{ return !sun && (Clouds<2) && !Fog; }
mixed query_thief_weather() { return ({!sun,Fog,Clouds,Rain}); }

string
Rain_Type()
{
    string desc = "ERROR IN TEMP";

    switch(Temp) {
    case -10000..-4: desc = "snow";
    case -3..3: desc = "hail";
    case 4..10000: desc = "rain";
    default: Temp = 0;
    }

    return desc;
}

string
Rain_Strength()
{
    string desc = "ERROR IN RAIN";

    switch(Rain) {
    case 0: desc = "not ";
    case 1: desc = "lightly ";
    case 2: desc = "";
    case 3: desc = "heavily ";
    default: Rain = 0;
    }

    return desc;
}

string
Fog_Type()
{
    string desc = "ERROR IN FOG";

    switch(Fog) {
    case 0: desc = "no";
    case 1: desc = "mist";
    case 2: desc = "fog";
    case 3: desc = "thick fog";
    default: Fog = 0;
    }

    return desc;
}

string
Cloud_Type()
{
    string desc = "ERROR IN CLOUD";

    switch(Clouds) {
    case 0: desc = "not a single cloud";
    case 1: desc = "one white cloud";
    case 2: desc = "some clouds";
    case 3: desc = "heavy cloud cover";
    default: Clouds = 0;
    }

    return desc;
}

string
Cloud_Verb()
{
    string desc = "ERROR IN CLOUD";

    switch(Clouds) {
    case 0: case 3: case 1: desc = "is";
    case 2: desc = "are";
    default: Clouds = 0;
    }

    return desc;
}

string
Wind_Type()
{
    string desc = "ERROR IN WIND";

    switch(Wind) {
    case 0: desc = "The air stands still";
    case 1..4: desc = "Quiet blow stirs the air";
    case 5..8: desc = "The wind is blowing";
    case 9..12: desc = "It is pretty windy";
    case 13..16: desc = "The wind is blowing pretty strongly!";
    case 17..30: desc = "The wind is storming!";
    default: Wind = 0;
    }

    return desc;
}

string
Humid_Type()
{
    string desc = "ERROR IN HUMID";

    switch(Humid) {
    case 0..15: desc = "very dry";
    case 16..30: desc = "dry";
    case 31..45: desc = "bit dry";
    case 46..55: desc = "";
    case 56..69: desc = "bit wet";
    case 70..84: desc = "wet";
    case 85..100: desc = "very wet";
    default: update(); Humid = Avg_Humid(Month);
    }

    return desc;
}

string
Temp_Type()
{
    string desc = "ERROR IN TEMP";

    update();
    switch(Temp-Avg_Temp(Month)){
    case -100..-16: desc = "EXTREMELY cold";
    case -15..-11: desc = "very cold";
    case -10..-5: desc = "bit cold";
    case -4..4: desc = "";
    case 5..10: desc = "quite warm";
    case 11..15: desc = "very hot";
    case 16..100: desc = "EXTREMELY hot";
    }

    return desc;
}

void
juha_fohr()
{
    write(query_date_b());
    write("(Exact time is " + query_time() + ")\n");
    write("Celerana is " + query_celerana_phase() + ".\n");
    write("Malerana is " + query_malerana_phase() + ".\n");
    write("Temperature is " + Temp + " 'C\n");
    write("Humidity is " + Humid + " %\n");
    write("Wind is blowing at " + Wind + " m/s\n");
    write("There " + Cloud_Verb() + " " + Cloud_Type() + " in the sky above your head.\n");
    write("It is " + Rain_Strength() + Rain_Type() + "ing.\n");
    if (Fog) write(capitalize(Fog_Type()) + " fills the air.\n");
}

string
query_short_weather()
{
    string s;
    if (Rain) s = sprintf("It is %sing.", Rain_Type());
    if (Fog) if (!s) s = sprintf("%s fills the air.", capitalize(Fog_Type()));
	else s = sprintf("%s\n%s fills the air.", s, capitalize(Fog_Type()));
    if (Wind > 15) if (!s) s = "Wind is blowing very strong!";
	else s = sprintf("%s\nWind is blowing very strong!", s);
    return s;
}

string
query_long_weather()
{
    string s, t;

    if (!sizeof(t = Temp_Type())) s = sprintf("It is %s %s.\n",
	  Season, query_time_of_day());
    else s = sprintf("%s %s %s.\n", t, Season, query_time_of_day());
    if (query_sun()) s += "Sun is shining.\n";
    if (query_moon()) {
	if ((t = query_celerana_phase())!="dark") s += capitalize(t) + " Celerana shines golden light to darkness of the night.\n";
	if ((t = query_malerana_phase())!="dark") s += capitalize(t) + " Malerana emits silver cold light to the darkness.\n";
    }
    if (Fog) s += capitalize(Fog_Type()) + " fills the air.\n";
    if (Rain) s += "It is " + Rain_Strength() + Rain_Type() + "ing.\n";
    if (Humid_Type()!="") s += "The weather around you feels " + Humid_Type() + ".\n";
    s += Wind_Type() + ".\n";
    s += "There " + Cloud_Verb() + " " + Cloud_Type() + " in the sky.\n";

    return s;
}


string Official_Season(int m) {
    return ({ "winter", "winter", "spring", "spring",
      "summer", "summer", "summer", "summer",
      "autumn", "autumn", "winter", "winter" })[m-1]; }

/* Long nights... //Graah */
#if 1
int SunriseH(int m)	{ return ({ 10,9,9,8,7,7,6,6,7,8,9,10 })[m-1]; }
int SunsetH(int m)	{ return ({ 15,16,17,19,20,21,22,21,20,19,17,16 })[m-1]; }
#endif

#if 0
int SunriseH(int m) { return ({  7, 6, 5, 4, 3, 3, 4, 4, 5, 5, 6, 6 })[m-1]; }
int SunsetH(int m)  { return ({ 17,18,19,20,21,22,23,22,21,20,19,18 })[m-1];}
#endif

int Avg_Temp(int m)	{ return ({ -10,-5,5,15,20,25,30,30,20,10,-5,-15 })[m-1]; }
int Avg_Humid(int m)	{ return ({ 20,30,60,70,80,70,40,30,40,50,40,30 })[m-1]; }


void
Inform(string s, int outd)
{
    object *u, ob;
    int	x, i;
    if (!s) return;
    u = users();

    for (x = sizeof(u) - 1; x >= 0; x--) {
	ob = environment(u[x]);
	if (ob) {
	    if (!(i = (int)ob->query(ROOM_WD))) {
		i = WD_NONE;
		// log_file("WDESC", "NO_WD_SET: " + object_name(ob) + ".\n");
		// No logs please...
	    }
	    if (outd == WD_ALL || !u[x]->query_brief_weather())
		if (i & outd)
		    u[x] -> tell_me(s);
	}
    }

    /*	for(x = 0; x < sizeof(u); x++)
     *		if ( (ob = environment(u[x])) && ((int)ob->query(ROOM_WD) & outd))
     *			tell_object(u[x], s);
     */


    // temporary set_ functions to test out the weather
}

void set_fog(int x)	{ if (x>=0 && x<=3) Fog = x; }
void set_rain(int x)	{ if (x>=0 && x<=3) Rain = x; }
void set_temp(int x)	{ Temp = x; }
void set_humid(int x)	{ if (x>=0 && x<=100) Humid = x; }
void set_clouds(int x)	{ if (x>=0 && x<=3) Clouds = x; }
void set_wind(int x)	{ if (x>=0) Wind = x; }
void set_quake(int x)	{ if (x) quakecount = 61; }

int query_cloud() { return query_clouds(); }

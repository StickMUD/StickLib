#define GAMETIMEZONE_ST "EST"
#define GAMETIMEZONE_DS "EDT"

private static mapping timezones;

void create()
{
   // timezones = ([ timezone : GMT adjustment ])
   timezones = ([
        "ACDT"    :  10.50,      "FET"   :   3.00,      "NPT"   :   5.75,
        "ACST"    :   9.50,      "FJST"  :  13.00,      "NST"   :  -3.50,
        "ACT"     :   8.00,      "FJT"   :  12.00,      "NT"    : -11.00,
        "ADT"     :  -3.00,      "FKST"  :  -3.00,      "NT-2"  :  -3.50,
        "AEDT"    :  11.00,      "FKT"   :  -4.00,      "NUT"   : -11.00,
        "AEST"    :  10.00,      "FNT"   :  -2.00,      "NZDT"  :  13.00,
        "AFT"     :   4.50,      "FST"   :   2.00,      "NZST"  :  12.00,
        "AHST"    : -10.00,      "FWT"   :   1.00,      "NZT"   :  12.00,
        "AKDT"    :  -8.00,      "GALT"  :  -6.00,      "OMSST" :   7.00,
        "AKST"    :  -9.00,      "GAMT"  :  -9.00,      "OMST"  :   6.00,
        "ALMT"    :   6.00,      "GET"   :   4.00,      "PDT"   :  -7.00,
        "AMST"    :  -3.00,      "GFT"   :  -3.00,      "PET"   :  -5.00,
        "AMST-2"  :   5.00,      "GILT"  :  12.00,      "PETST" :  12.00,
        "AMT"     :  -4.00,      "GIT"   :  -9.00,      "PETT"  :  12.00,
        "AMT-2"   :   4.00,      "GMT"   :   0.00,      "PGT"   :  10.00,
        "ANAST"   :  12.00,      "GST"   :  -2.00,      "PHOT"  :  13.00,
        "ANAT"    :  12.00,      "GST-2" :   4.00,      "PHT"   :   8.00,
        "AQTT"    :   5.00,      "GST-3" :  10.00,      "PKT"   :   5.00,
        "ART"     :  -3.00,      "GYT"   :  -4.00,      "PMDT"  :  -2.00,
        "AST"     :  -4.00,      "HAA"   :  -3.00,      "PMST"  :  -3.00,
        "AST-2"   :   3.00,      "HAC"   :  -5.00,      "PONT"  :  11.00,
        "AST-3"   :   4.00,      "HADT"  :  -9.00,      "PST"   :  -8.00,
        "AT"      :  -2.00,      "HAE"   :  -4.00,      "PST-2" :  -8.00,
        "AWDT"    :   9.00,      "HAEC"  :   2.00,      "PST-3" :   8.00,
        "AWST"    :   8.00,      "HAP"   :  -7.00,      "PT"    :  -8.00,
        "AZOST"   :  -1.00,      "HAR"   :  -6.00,      "PWT"   :   9.00,
        "AZOST-2" :   0.00,      "HAST"  : -10.00,      "PYST"  :  -3.00,
        "AZOT"    :  -1.00,      "HAT"   :  -2.50,      "PYT"   :  -4.00,
        "AZST"    :   5.00,      "HAY"   :  -8.00,      "RET"   :   4.00,
        "AZT"     :   4.00,      "HDT"   :  -9.00,      "SAMT"  :   4.00,
        "BDT"     :   8.00,      "HKT"   :   8.00,      "SAST"  :   2.00,
        "BIOT"    :   6.00,      "HLV"   :  -4.50,      "SBT"   :  11.00,
        "BIT"     : -12.00,      "HMT"   :   5.00,      "SCT"   :   4.00,
        "BNT"     :   8.00,      "HNA"   :  -4.00,      "SGT"   :   8.00,
        "BOT"     :  -4.00,      "HNC"   :  -6.00,      "SLT"   :   5.50,
        "BRST"    :  -2.00,      "HNE"   :  -5.00,      "SRT"   :  -3.00,
        "BRT"     :  -3.00,      "HNP"   :  -8.00,      "SST"   : -11.00,
        "BST"     :   1.00,      "HNR"   :  -7.00,      "SST-2" :   2.00,
        "BST-2"   :   6.00,      "HNT"   :  -3.50,      "SST-3" :   8.00,
        "BT"      :   3.00,      "HNY"   :  -9.00,      "SWT"   :   1.00,
        "BTT"     :   6.00,      "HOVT"  :   7.00,      "TAHT"  : -10.00,
        "CAST"    :   8.00,      "HST"   : -10.00,      "TFT"   :   5.00,
        "CAT"     : -10.00,      "ICT"   :   7.00,      "THA"   :   7.00,
        "CAT-2"   :   2.00,      "IDLE"  :  12.00,      "TJT"   :   5.00,
        "CCT"     :   6.50,      "IDLW"  : -12.00,      "TKT"   : -10.00,
        "CCT-2"   :   8.00,      "IDT"   :   3.00,      "TLT"   :   9.00,
        "CDT"     :  -5.00,      "IOT"   :   6.00,      "TMT"   :   5.00,
        "CDT-2"   :  -4.00,      "IRDT"  :   4.50,      "TVT"   :  12.00,
        "CDT-3"   :  10.50,      "IRKST" :   9.00,      "ULAT"  :   8.00,
        "CEDT"    :   2.00,      "IRKT"  :   8.00,      "UT"    :   0.00,
        "CEST"    :   2.00,      "IRST"  :   3.50,      "UTC"   :   0.00,
        "CET"     :   1.00,      "IST"   :   1.00,      "UYST"  :  -2.00,
        "CHADT"   :  13.75,      "IST-2" :   2.00,      "UYT"   :  -3.00,
        "CHAST"   :  12.75,      "IST-3" :   5.50,      "UZT"   :   5.00,
        "CHST"    :  10.00,      "JST"   :   9.00,      "VET"   :  -4.50,
        "CIST"    :  -8.00,      "KGT"   :   6.00,      "VLAST" :  11.00,
        "CKT"     : -10.00,      "KRAST" :   8.00,      "VLAT"  :  10.00,
        "CLST"    :  -3.00,      "KRAT"  :   7.00,      "VUT"   :  11.00,
        "CLT"     :  -4.00,      "KST"   :   9.00,      "WADT"  :   8.00,
        "COST"    :  -4.00,      "KUYT"  :   4.00,      "WAST"  :   2.00,
        "COT"     :  -5.00,      "LHDT"  :  11.00,      "WAT"   :   1.00,
        "CST"     :  -6.00,      "LHST"  :  10.50,      "WDT"   :   9.00,
        "CST-2"   :  -5.00,      "LINT"  :  14.00,      "WEDT"  :   1.00,
        "CST-3"   :   8.00,      "MAGST" :  12.00,      "WEST"  :   1.00,
        "CST-4"   :   9.50,      "MAGT"  :  11.00,      "WESZ"  :   1.00,
        "CST-5"   :  10.50,      "MART"  :  -9.50,      "WET"   :   0.00,
        "CT"      :   8.00,      "MAWT"  :   5.00,      "WEZ"   :   0.00,
        "CVT"     :  -1.00,      "MDT"   :  -6.00,      "WFT"   :  12.00,
        "CXT"     :   7.00,      "MEST"  :   2.00,      "WGST"  :  -2.00,
        "DAVT"    :   7.00,      "MESZ"  :   2.00,      "WGT"   :  -3.00,
        "DFT"     :   1.00,      "MET"   :   1.00,      "WIB"   :   7.00,
        "EADT"    :  11.00,      "MEWT"  :   1.00,      "WIT"   :   9.00,
        "EASST"   :  -5.00,      "MEZ"   :   1.00,      "WITA"  :   8.00,
        "EAST"    :  -6.00,      "MHT"   :  12.00,      "WST"   :   1.00,
        "EAST-2"  :  10.00,      "MIT"   :  -9.50,      "WST-2" :   8.00,
        "EAT"     :   3.00,      "MMT"   :   6.50,      "WST-3" :  13.00,
        "ECT"     :  -5.00,      "MSD"   :   4.00,      "WT"    :   0.00,
        "ECT-2"   :  -4.00,      "MSK"   :   4.00,      "YAKST" :  10.00,
        "EDT"     :  -4.00,      "MST"   :  -7.00,      "YAKT"  :   9.00,
        "EDT-2"   :  11.00,      "MST-2" :   6.50,      "YAPT"  :  10.00,
        "EEDT"    :   3.00,      "MST-3" :   8.00,      "YDT"   :  -8.00,
        "EEST"    :   3.00,      "MUT"   :   4.00,      "YEKST" :   6.00,
        "EET"     :   2.00,      "MVT"   :   5.00,      "YEKT"  :   5.00,
        "EGST"    :   0.00,      "MYT"   :   8.00,      "YST"   :  -9.00,
        "EGT"     :  -1.00,      "NCT"   :  11.00,      "ZP4"   :   4.00,
        "EST"     :  -5.00,      "NDT"   :  -2.50,      "ZP5"   :   5.00,
        "EST-2"   :  10.00,      "NFT"   :  11.50,      "ZP6"   :   6.00,
        "EST-3"   :  11.00,      "NOVST" :   7.00,
        "ET"      :  -5.00,      "NOVT"  :   6.00
   ]);

}

// Returns 1 if it is a recognized timezone; 0 otherwise
int is_valid_timezone(string zone)
{
   if(!zone)
      return 0;

   zone = upper_case(zone);

   return member(timezones, zone);
}

// Returns the timezone difference from GMT of recognized timezones; -9990.0 otherwise
float timezone_offset(string zone)
{
   if(!zone || !is_valid_timezone(zone))
      return -9990.0;

   zone = upper_case(zone);

   return timezones[zone];
}

// Returns the # of seconds to adjust between StickMUD's default standard timezone to the specified timezone
varargs int query_time_adj(string zone)
{
   int adj;

   zone = zone ? upper_case(zone) : GAMETIMEZONE_ST;
   if (!is_valid_timezone(zone))
      return 0;

   // Calculate the number of seconds between the different timezones
   adj = to_int((3600.0 * (timezone_offset(zone)-timezone_offset(GAMETIMEZONE_ST))));

   // We are doing this because time() contains a daylight savings time adjustment. Let's make sure its
   // based on standard time, not daylight savings time.
   if(localtime()[8])
      adj -= 3600;

   return adj;
}

// Returns the adjusted timestamp from game's default timezone to the timezone of the user
varargs int ztime(int t, object me, string use_zone)
{
   int adj;
   string z, zone;

   if(!t)
      t=time();

   if(!me && !(me=this_player()))
      return t;

   if(use_zone)
      zone = use_zone;
   else
   {
      z    = me->query_env("timezone");
      zone = z ? upper_case(z) : "EST";
   }

   if(!is_valid_timezone(zone))
      return t;

   return t+query_time_adj(zone);
}

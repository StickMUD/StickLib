// Stupid module.

void
show_stats()
{
string tmp;
int i;

write(sprintf("%-50slevel: %d\n",
	(short(1) ? short(1) : "(" + query_real_name() + ")"), level)
	+sprintf("exp: %-20dgold: %-19dalign: %d\n",
	 experience, money, alignment)
	+sprintf("hp: %3d/%-3d, sp: %3d/%-3d, fp: %3d/%-16dgender: %s\n",
	 hit_point, max_hp, spell_points, max_sp,
	 fatigue_points, max_fp,
	 query_gender_string())
	+sprintf("ac: %-21dwc: %-21dcarry: %d\n",
	 query_ac(), query_wc(), local_weight)
	);

if (interactive())
{
  write("ip: " + (tmp = query_ip_name(this_object())));
  if (tmp != query_ip_number(this_object()))
	write(" (" + query_ip_number(this_object()) + ")");
  write("\n");
  if((int) this_player()->query_coder_level()>=300)
	write("mail: " + this_player()->query_mailaddr() + "\n");
}
  if (tmp = (string)this_object()->query_quests())
write("Quests:\t" + tmp + "\n");
  show_age();

  write(sprintf(
 "Str: %d (%d) Dex: %d (%d) Con: %d (%d) Int: %d (%d) Luck: %d\n",
	query_stat(ST_STR), query_stat(ST_BASE_STR),
	query_stat(ST_DEX), query_stat(ST_BASE_DEX),
	query_stat(ST_CON), query_stat(ST_BASE_CON),
	query_stat(ST_INT), query_stat(ST_BASE_INT),
	query_luck()));

  if (interactive())
{
  if ((i=query_idle(this_object())))
	{
	  tmp = "";
	  if (i>3600) tmp = (i/3600)+" h ";
	  if (i>60) tmp += (i/60%60)+" min ";
	  tmp += (i%60)+" s";
	  write("idle:\t" + tmp + "\n");
	}

/* Is a criminal? /Graah */
  if (this_object()->query_crime_level())
	write("Criminal: " + this_object()->query_crime_status() +
	  ", counter " + this_object()->query_crime_count() + "\n");
}

if (attacker_ob)
write("attack: " + attacker_ob->query_name(0, this_object()) + "\n");
if (alt_attacker_ob)
write("alt_attack: " + alt_attacker_ob->query_name(0, this_object()) + "\n");

if ((i = (int)this_player()->query_coder_level()))
write("Coder level: " + i + "\n");

if (this_object()->query_is_testplayer())
write("Registered testplayer\n");

/* Added showing wimpy and standing statuses out of my curiousity //Frobozz */
if (i = query_wimpy()) write("Wimpy at " + i + "%. ");
else write("Brave mode. ");

if (query_rest()) write("Resting.\n");
else if (query_sit()) write("Sitting.\n");
else write("Standing.\n");
}

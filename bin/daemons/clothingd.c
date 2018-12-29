//
// /bin/daemons/clothingd.c
//
// A server which creates various clothes and their costs.
//
// 31-Aug-96 / Graah : Removed 'slippers', added texture and more stuff.
// (Because slippers are shoes and do not belong here.)

static mapping quals, texs, cols, mats, cloths;
static string *qualname, *texname, *colname, *matname, *clothname;
static int qualn, texn, coln, matn, clothn;
static string clist;

void makelist();

void
create()
{
  qualn = coln = matn = clothn = 0;

  /* "data name": ({ Relative cost %, (other variables?), ... }) */

  quals = ([
	    "inferior":	({    20 }),
	    "poor":		({    75 }),
	    "average":	({   100 }),
	    "good":		({   300 }),
	    "excellent":	({   900 }),
	    "perfect":	({  2200 }),
	    "magical":      ({ 25000 })
  ]);

    texs = ([
	     "UNUSED":       ({ 0 }), // Must have this!
	"bright":	({   300 }),
	"glossy":	({   200 }),
	"fluorescent":	({   200 }),
	"shining":      ({   230 }),
	"sparkling":	({   200 }),
    ]);

    cols = ([
	"black":	({ 170 }),
	"gray":         ({  40 }),
	"grey":         ({  40 }),
	"ebony":	({ 120 }),
	"red":		({ 100 }),
	"pink":		({  80 }),
	"rose":		({ 100 }),
	"scarlet":	({ 120 }),
	"orange":	({ 100 }),
	"yellow":	({ 100 }),
	"ochre":	({  50 }),
	"green":	({  80 }),
	"blue":		({  90 }),
	"azure":	({ 150 }),
	"turquoise":	({ 145 }),
	"indigo":	({ 130 }),
	"violet":	({ 100 }),
	"lilac":	({ 100 }),
	"mauve":	({ 100 }),
	"purple":	({ 100 }),
	"white":	({  40 }),
	"rose":		({ 120 }),
	"rainbow":	({ 800 }),
	"multi-hued":	({ 1600 }),
	"golden":       ({ 1200 }),
	"silver":       ({ 800 }),
	"mithril":      ({ 1000 }),
	"celestial":	({ 1000 }),
	"floral":	({ 800 }),
	"Nova-Scotian":	({ 1000 }),
	"plaid":	({ 900 })
    ]);

    mats = ([
	"orcskin":	({     2 }),
	"sackcloth":    ({     5 }),
	"wool":		({    30 }),
	"cotton":	({   100 }),
	"leather":	({   150 }),
	"fur":		({   400 }),
	"velvet":       ({  1000 }),
	"silk":		({  2000 }),
	"spidersilk":   ({  4000 }),
	"mithril":	({  5000 }),
	"dragonskin":	({  6000 }),
	"Moonsilk":	({ 25000 }),
	"plastic":      ({    50 }),
	"flannel":	({   100 }),
	"knitted":	({   150 }),
	"fuzzy":	({   300 }),
	"suede":	({   300 }),
	"chainmail":    ({   850 }),
	"satin":	({  1000 }),
	"platemail":    ({  2300 }),
    ]);

    /* "data name": ({ Base cost gp, base weight, (other data?), ... }) */
    /* Note: weight is used only when clothes are carried, not when worn. */

    cloths = ([
	"bra":          ({   50, 0 }),
	"bikini":       ({  100, 0 }),
	"cloak":        ({  900, 1 }),
	"loincloth":	({   30, 0 }),
	"socks":	({  350, 0 }),
	"scarf":	({  100, 0 }),
	"gloves":       ({  200, 0 }),
	"hood":		({  140, 0 }),
	"trousers":	({ 1300, 1 }),
	"pants":	({ 1500, 1 }),
	"tanga":        ({  210, 0 }),
	"panties":      ({  200, 0 }),
	"pantyhose":    ({  400, 0 }),
	"tie":		({  125, 0 }),
	"blouse":	({  500, 1 }),
	"shirt":	({  750, 1 }),
	"skirt":	({ 1200, 1 }),
	"vest":		({  500, 1 }),
	"coat":		({ 1200, 1 }),
	"dress":	({ 2500, 2 }),
	"suit":		({ 2500, 1 }),
	"robe":		({ 4500, 2 }),
	"kilt":		({ 1200, 1 }),
	"tartan":	({ 1300, 1 }),
	"nightie":	({  750, 1 }),
	"footie-pajamas":({  850, 1 }),
	"t-shirt":	({  500, 1 }),
	"teddy":	({ 1000, 1 }),
	"garter-belt":	({  800, 1 })
    ]);

    qualname = m_indices(quals);
    qualn = sizeof(qualname);

    texname = m_indices(texs);
    texname -= ({ "UNUSED" });
    texn = sizeof(texname);

    colname = m_indices(cols);
    coln = sizeof(colname);

    matname = m_indices(mats);
    matn = sizeof(matname);

    clothname = m_indices(cloths);
    clothn = sizeof(clothname);

    call_out("makelist", 1);
}

void
makelist()
{
  clist =
    sprintf("%-=78s\n", "Qualities: " + implode(qualname, ", ", " and ")) +
    sprintf("%-=78s\n", "Textures: " + implode(texname, ", ", " and ")) +
    sprintf("%-=78s\n", "Colors: " + implode(colname, ", ", " and ")) +
    sprintf("%-=78s\n", "Materials: " + implode(matname, ", ", " and ")) +
    sprintf("%-=78s\n", "Clothes: " + implode(clothname, ", ", " and "));
}

void
show_list()
{
  write(clist);
}

string *
query_cloth_types()
{
  return clothname;
}

// 31-Aug-96 / Graah: Texture is optional argument.
varargs mixed
create_cloth(int q, int col, int m, int clo, int tex)
{
  string n;
  int cost;

  if (tex < 0 || texname[tex] == "UNUSED")
    {
      n = qualname[q] + " " + colname[col] + " " +
	matname[m] + " " + clothname[clo];
      cost = ((quals[qualname[q]][0] +
	       cols[colname[col]][0] +
	       mats[matname[m]][0]) / 3);
    }
  else
    {
      n = qualname[q] + " " + texname[tex] + " " +
	colname[col] + " " +
	matname[m] + " " + clothname[clo];
      cost = ((quals[qualname[q]][0] +
	       texs[texname[tex]][0] +
	       cols[colname[col]][0] +
	       mats[matname[m]][0]) / 3);
    }

  cost = ((cloths[clothname[clo]][0] * cost) / 100);

  return ({ n, clothname[clo], cost });
}

mixed
create_custom_cloth(string n)
{
  string *x;
  int i, q, col, m, clo, tex;

  tex = -1;

  x = explode(n, " ");

  if (sizeof(x) < 4) return 0;
  if (sizeof(x) == 5)
    {
      q = member_array(x[0], qualname);
      tex = member_array(x[1], texname);
      col = member_array(x[2], colname);
      m = member_array(x[3], matname);
      clo = member_array(x[4], clothname);
    
      if (q == -1 || tex == -1 || col == -1 || m == -1 || clo == -1 ||
	  texname[tex] == "UNUSED")
	return 0;
    }
  else
    {
      q = member_array(x[0], qualname);
      col = member_array(x[1], colname);
      m = member_array(x[2], matname);
      clo = member_array(x[3], clothname);
    
      if (q == -1 || col == -1 || m == -1 || clo == -1) return 0;
    }

  return create_cloth(q, col, m, clo, tex);
}

/*
 * Returns a cloth name (like "poor black leather socks" :-) and
 * its adjusted cost, in an array.
 *
 */
mixed
create_random_cloth(int minquality, int maxquality)
{
  int q, col, m, clo, foo;
  string mn;
  mixed c;

  foo = 0;

  do {
    do {
      q = random(qualn);
    } while(quals[qualname[q]][0] < minquality ||
	    quals[qualname[q]][0] > maxquality);

    col = random(coln);

    do {
      m = random(matn);
      mn = matname[m];
    } while ((minquality >= 100 &&
	      (mn == "sackcloth" || mn == "orcskin")) ||
	     (maxquality < 300 &&
	      (mn == "dragonskin" || mn == "silk" ||
	       mn == "spidersilk" || mn == "Moonsilk")) ||
	     (mn == "Moonsilk" && random(100) > 10));

    clo = random(clothn);

    c = create_cloth(q, col, m, clo, -1);

    foo++;
  } while(c[2] == 0 && foo < 50);

  if (!c[2])
    write("Clothing daemon is buggy! Contact a co-admin or an admin!\n");

  return c;
}

int
query_base_weight(string cn)
{
  if (!cloths || !cloths[cn]) return 1;

  return cloths[cn][1];
}

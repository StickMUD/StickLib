/*
** /basic/living/gender.c
**
*/
 
// Small modifications by Doomdark 23-feb-95
// 17-mar-95 removed 6 funcs... set_female/male/neuter + query_female/male/neuter
 
int gender;
 
void	set_gender(int x)	{ if(x>=0 && x<=2) gender = x; }
int	query_gender()		{ return gender; }

#if 0
void	set_female()		{ gender = 2; }
void 	set_male()		{ gender = 1; }
void	set_neuter()		{ gender = 0; }

int	query_neuter()		{ return gender==0; }
int	query_male()		{ return gender==1; }
int	query_female()		{ return gender==2; }
#endif

 
string	query_gender_string() {
	if(gender==1) return "male";
	if(gender==2) return "female";
	return "neuter";	/* or maybe "worm" ? */
	}
 
string Pronoun() {
	if(gender==1) return "he";
	if(gender==2) return "she";
	return "it";
	}
 
string Possessive() {
	if(gender==1) return "his";
	if(gender==2) return "her";
	return "its";
	}
 
string Objective() {
	if(gender==1) return "him";
	if(gender==2) return "her";
	return "it";
	}
 
string query_pronoun()		{ return Pronoun(); }
string query_possessive()	{ return Possessive(); }
string query_objective()	{ return Objective(); }
 

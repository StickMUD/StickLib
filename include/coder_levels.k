/*
 CODER LEVELS & LEVEL CHECKING LOGICS

 Use these macros each and every time you check if someone is a coder
 or not. Everything except macros is subject to change.
*/

#ifndef CODER_LEVELS_H
#define CODER_LEVELS_H

#define	LVL_POLICE		1
#define LVL_APPRENTICE		10
#define	LVL_JUNIOR		50
#define	LVL_NOVICE		LVL_JUNIOR
#define	LVL_NOVICE_CODER	LVL_JUNIOR
#define LVL_CODER		100
#define LVL_JUNIOR_JUDGE        101
#define LVL_SENIOR		200
#define LVL_JUDGE               201
#define LVL_PSUEDO		250
#define LVL_COADMIN		300
#define LVL_ADMIN		400
/* The Macros: */
#define	IS_ADMIN(x)		((x) >= LVL_COADMIN)
#define	IS_CODER(x)		(x > 0)
#define IS_ADMIN_OB(ob)        (interactive(ob) && IS_ADMIN((int)ob->query_coder_level()))
#define IS_CODER_OB(ob)        (interactive(ob) && IS_CODER((int)ob->query_coder_level()))

#define	LVL_CAN_SNOOP		LVL_JUNIOR
#define LVL_CAN_PROMOTE		LVL_COADMIN

#endif /* CODER_LEVELS_H */

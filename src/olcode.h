/*
** $Id: lcode.h,v 1.48.1.1 2007/12/27 13:02:25 roberto Exp $
** Code generator for ol
** See Copyright Notice in ol.h
*/

#ifndef lcode_h
#define lcode_h

#include "ollex.h"
#include "olobject.h"
#include "olopcodes.h"
#include "olparser.h"


/*
** Marks the end of a patch list. It is an invalid value both as an absolute
** address, and as a list link (would link an element to itself).
*/
#define NO_JUMP (-1)


/*
** grep "ORDER OPR" if you change these enums
*/
typedef enum BinOpr {
  OPR_ADD, OPR_SUB, OPR_MUL, OPR_DIV, OPR_MOD, OPR_POW,
  OPR_CONCAT,
  OPR_NE, OPR_EQ,
  OPR_LT, OPR_LE, OPR_GT, OPR_GE,
  OPR_AND, OPR_OR,
  OPR_NOBINOPR
} BinOpr;


typedef enum UnOpr { OPR_MINUS, OPR_NOT, OPR_LEN, OPR_NOUNOPR } UnOpr;


#define getcode(fs,e)	((fs)->f->code[(e)->u.s.info])

#define olK_codeAsBx(fs,o,A,sBx)	olK_codeABx(fs,o,A,(sBx)+MAXARG_sBx)

#define olK_setmultret(fs,e)	olK_setreturns(fs, e, ol_MULTRET)

olI_FUNC int olK_codeABx (FuncState *fs, OpCode o, int A, unsigned int Bx);
olI_FUNC int olK_codeABC (FuncState *fs, OpCode o, int A, int B, int C);
olI_FUNC void olK_fixline (FuncState *fs, int line);
olI_FUNC void olK_nil (FuncState *fs, int from, int n);
olI_FUNC void olK_reserveregs (FuncState *fs, int n);
olI_FUNC void olK_checkstack (FuncState *fs, int n);
olI_FUNC int olK_stringK (FuncState *fs, TString *s);
olI_FUNC int olK_numberK (FuncState *fs, ol_Number r);
olI_FUNC void olK_dischargevars (FuncState *fs, expdesc *e);
olI_FUNC int olK_exp2anyreg (FuncState *fs, expdesc *e);
olI_FUNC void olK_exp2nextreg (FuncState *fs, expdesc *e);
olI_FUNC void olK_exp2val (FuncState *fs, expdesc *e);
olI_FUNC int olK_exp2RK (FuncState *fs, expdesc *e);
olI_FUNC void olK_self (FuncState *fs, expdesc *e, expdesc *key);
olI_FUNC void olK_indexed (FuncState *fs, expdesc *t, expdesc *k);
olI_FUNC void olK_goiftrue (FuncState *fs, expdesc *e);
olI_FUNC void olK_storevar (FuncState *fs, expdesc *var, expdesc *e);
olI_FUNC void olK_setreturns (FuncState *fs, expdesc *e, int nresults);
olI_FUNC void olK_setoneret (FuncState *fs, expdesc *e);
olI_FUNC int olK_jump (FuncState *fs);
olI_FUNC void olK_ret (FuncState *fs, int first, int nret);
olI_FUNC void olK_patchlist (FuncState *fs, int list, int target);
olI_FUNC void olK_patchtohere (FuncState *fs, int list);
olI_FUNC void olK_concat (FuncState *fs, int *l1, int l2);
olI_FUNC int olK_getlabel (FuncState *fs);
olI_FUNC void olK_prefix (FuncState *fs, UnOpr op, expdesc *v);
olI_FUNC void olK_infix (FuncState *fs, BinOpr op, expdesc *v);
olI_FUNC void olK_posfix (FuncState *fs, BinOpr op, expdesc *v1, expdesc *v2);
olI_FUNC void olK_setlist (FuncState *fs, int base, int nelems, int tostore);


#endif

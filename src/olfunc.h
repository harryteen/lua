/*
** $Id: olfunc.h,v 2.4.1.1 2007/12/27 13:02:25 roberto Exp $
** Auxiliary functions to manipulate prototypes and closures
** See Copyright Notice in ol.h
*/

#ifndef olfunc_h
#define olfunc_h


#include "olobject.h"


#define sizeCclosure(n)	(cast(int, sizeof(CClosure)) + \
                         cast(int, sizeof(TValue)*((n)-1)))

#define sizeLclosure(n)	(cast(int, sizeof(LClosure)) + \
                         cast(int, sizeof(TValue *)*((n)-1)))


olI_FUNC Proto *olF_newproto (ol_State *L);
olI_FUNC Closure *olF_newCclosure (ol_State *L, int nelems, Table *e);
olI_FUNC Closure *olF_newLclosure (ol_State *L, int nelems, Table *e);
olI_FUNC UpVal *olF_newupval (ol_State *L);
olI_FUNC UpVal *olF_findupval (ol_State *L, StkId level);
olI_FUNC void olF_close (ol_State *L, StkId level);
olI_FUNC void olF_freeproto (ol_State *L, Proto *f);
olI_FUNC void olF_freeclosure (ol_State *L, Closure *c);
olI_FUNC void olF_freeupval (ol_State *L, UpVal *uv);
olI_FUNC const char *olF_getlocalname (const Proto *func, int local_number,
                                         int pc);


#endif

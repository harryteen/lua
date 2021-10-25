/*
** $Id: olvm.h,v 2.5.1.1 2007/12/27 13:02:25 roberto Exp $
** ol virtual machine
** See Copyright Notice in ol.h
*/

#ifndef olvm_h
#define olvm_h


#include "oldo.h"
#include "olobject.h"
#include "oltm.h"


#define tostring(L,o) ((ttype(o) == ol_TSTRING) || (olV_tostring(L, o)))

#define tonumber(o,n)	(ttype(o) == ol_TNUMBER || \
                         (((o) = olV_tonumber(o,n)) != NULL))

#define equalobj(L,o1,o2) \
	(ttype(o1) == ttype(o2) && olV_equalval(L, o1, o2))


olI_FUNC int olV_lessthan (ol_State *L, const TValue *l, const TValue *r);
olI_FUNC int olV_equalval (ol_State *L, const TValue *t1, const TValue *t2);
olI_FUNC const TValue *olV_tonumber (const TValue *obj, TValue *n);
olI_FUNC int olV_tostring (ol_State *L, StkId obj);
olI_FUNC void olV_gettable (ol_State *L, const TValue *t, TValue *key,
                                            StkId val);
olI_FUNC void olV_settable (ol_State *L, const TValue *t, TValue *key,
                                            StkId val);
olI_FUNC void olV_execute (ol_State *L, int nexeccalls);
olI_FUNC void olV_concat (ol_State *L, int total, int last);

#endif

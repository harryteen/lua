/*
** $Id: ldebug.h,v 2.3.1.1 2007/12/27 13:02:25 roberto Exp $
** Auxiliary functions from Debug Interface module
** See Copyright Notice in ol.h
*/

#ifndef ldebug_h
#define ldebug_h


#include "olstate.h"


#define pcRel(pc, p)	(cast(int, (pc) - (p)->code) - 1)

#define getline(f,pc)	(((f)->lineinfo) ? (f)->lineinfo[pc] : 0)

#define resethookcount(L)	(L->hookcount = L->basehookcount)


olI_FUNC void olG_typeerror (ol_State *L, const TValue *o,
                                             const char *opname);
olI_FUNC void olG_concaterror (ol_State *L, StkId p1, StkId p2);
olI_FUNC void olG_aritherror (ol_State *L, const TValue *p1,
                                              const TValue *p2);
olI_FUNC int olG_ordererror (ol_State *L, const TValue *p1,
                                             const TValue *p2);
olI_FUNC void olG_runerror (ol_State *L, const char *fmt, ...);
olI_FUNC void olG_errormsg (ol_State *L);
olI_FUNC int olG_checkcode (const Proto *pt);
olI_FUNC int olG_checkopenop (Instruction i);

#endif

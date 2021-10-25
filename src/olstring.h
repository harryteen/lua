/*
** $Id: olstring.h,v 1.43.1.1 2007/12/27 13:02:25 roberto Exp $
** String table (keep all strings handled by ol)
** See Copyright Notice in ol.h
*/

#ifndef lstring_h
#define lstring_h


#include "olgc.h"
#include "olobject.h"
#include "olstate.h"


#define sizestring(s)	(sizeof(union TString)+((s)->len+1)*sizeof(char))

#define sizeudata(u)	(sizeof(union Udata)+(u)->len)

#define olS_new(L, s)	(olS_newlstr(L, s, strlen(s)))
#define olS_newliteral(L, s)	(olS_newlstr(L, "" s, \
                                 (sizeof(s)/sizeof(char))-1))

#define olS_fix(s)	l_setbit((s)->tsv.marked, FIXEDBIT)

olI_FUNC void olS_resize (ol_State *L, int newsize);
olI_FUNC Udata *olS_newudata (ol_State *L, size_t s, Table *e);
olI_FUNC TString *olS_newlstr (ol_State *L, const char *str, size_t l);


#endif

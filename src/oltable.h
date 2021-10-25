/*
** $Id: oltable.h,v 2.10.1.1 2007/12/27 13:02:25 roberto Exp $
** ol tables (hash)
** See Copyright Notice in ol.h
*/

#ifndef ltable_h
#define ltable_h

#include "olobject.h"


#define gnode(t,i)	(&(t)->node[i])
#define gkey(n)		(&(n)->i_key.nk)
#define gval(n)		(&(n)->i_val)
#define gnext(n)	((n)->i_key.nk.next)

#define key2tval(n)	(&(n)->i_key.tvk)


olI_FUNC const TValue *olH_getnum (Table *t, int key);
olI_FUNC TValue *olH_setnum (ol_State *L, Table *t, int key);
olI_FUNC const TValue *olH_getstr (Table *t, TString *key);
olI_FUNC TValue *olH_setstr (ol_State *L, Table *t, TString *key);
olI_FUNC const TValue *olH_get (Table *t, const TValue *key);
olI_FUNC TValue *olH_set (ol_State *L, Table *t, const TValue *key);
olI_FUNC Table *olH_new (ol_State *L, int narray, int lnhash);
olI_FUNC void olH_resizearray (ol_State *L, Table *t, int nasize);
olI_FUNC void olH_free (ol_State *L, Table *t);
olI_FUNC int olH_next (ol_State *L, Table *t, StkId key);
olI_FUNC int olH_getn (Table *t);


#if defined(ol_DEBUG)
olI_FUNC Node *olH_mainposition (const Table *t, const TValue *key);
olI_FUNC int olH_isdummy (Node *n);
#endif


#endif

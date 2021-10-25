/*
** $Id: olmem.h,v 1.31.1.1 2007/12/27 13:02:25 roberto Exp $
** Interface to Memory Manager
** See Copyright Notice in ol.h
*/

#ifndef olmem_h
#define olmem_h


#include <stddef.h>

#include "ollimits.h"
#include "ol.h"

#define MEMERRMSG	"not enough memory"


#define olM_reallocv(L,b,on,n,e) \
	((cast(size_t, (n)+1) <= MAX_SIZET/(e)) ?  /* +1 to avoid warnings */ \
		olM_realloc_(L, (b), (on)*(e), (n)*(e)) : \
		olM_toobig(L))

#define olM_freemem(L, b, s)	olM_realloc_(L, (b), (s), 0)
#define olM_free(L, b)		olM_realloc_(L, (b), sizeof(*(b)), 0)
#define olM_freearray(L, b, n, t)   olM_reallocv(L, (b), n, 0, sizeof(t))

#define olM_malloc(L,t)	olM_realloc_(L, NULL, 0, (t))
#define olM_new(L,t)		cast(t *, olM_malloc(L, sizeof(t)))
#define olM_newvector(L,n,t) \
		cast(t *, olM_reallocv(L, NULL, 0, n, sizeof(t)))

#define olM_growvector(L,v,nelems,size,t,limit,e) \
          if ((nelems)+1 > (size)) \
            ((v)=cast(t *, olM_growaux_(L,v,&(size),sizeof(t),limit,e)))

#define olM_reallocvector(L, v,oldn,n,t) \
   ((v)=cast(t *, olM_reallocv(L, v, oldn, n, sizeof(t))))


olI_FUNC void *olM_realloc_ (ol_State *L, void *block, size_t oldsize,
                                                          size_t size);
olI_FUNC void *olM_toobig (ol_State *L);
olI_FUNC void *olM_growaux_ (ol_State *L, void *block, int *size,
                               size_t size_elem, int limit,
                               const char *errormsg);

#endif


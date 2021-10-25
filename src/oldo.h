/*
** $Id: ldo.h,v 2.7.1.1 2007/12/27 13:02:25 roberto Exp $
** Stack and Call structure of ol
** See Copyright Notice in ol.h
*/

#ifndef ldo_h
#define ldo_h


#include "olobject.h"
#include "olstate.h"
#include "olzio.h"


#define olD_checkstack(L,n)	\
  if ((char *)L->stack_last - (char *)L->top <= (n)*(int)sizeof(TValue)) \
    olD_growstack(L, n); \
  else condhardstacktests(olD_reallocstack(L, L->stacksize - EXTRA_STACK - 1));


#define incr_top(L) {olD_checkstack(L,1); L->top++;}

#define savestack(L,p)		((char *)(p) - (char *)L->stack)
#define restorestack(L,n)	((TValue *)((char *)L->stack + (n)))

#define saveci(L,p)		((char *)(p) - (char *)L->base_ci)
#define restoreci(L,n)		((CallInfo *)((char *)L->base_ci + (n)))


/* results from olD_precall */
#define PCRol		0	/* initiated a call to a ol function */
#define PCRC		1	/* did a call to a C function */
#define PCRYIELD	2	/* C funtion yielded */


/* type of protected functions, to be ran by `runprotected' */
typedef void (*Pfunc) (ol_State *L, void *ud);

olI_FUNC int olD_protectedparser (ol_State *L, ZIO *z, const char *name);
olI_FUNC void olD_callhook (ol_State *L, int event, int line);
olI_FUNC int olD_precall (ol_State *L, StkId func, int nresults);
olI_FUNC void olD_call (ol_State *L, StkId func, int nResults);
olI_FUNC int olD_pcall (ol_State *L, Pfunc func, void *u,
                                        ptrdiff_t oldtop, ptrdiff_t ef);
olI_FUNC int olD_poscall (ol_State *L, StkId firstResult);
olI_FUNC void olD_reallocCI (ol_State *L, int newsize);
olI_FUNC void olD_reallocstack (ol_State *L, int newsize);
olI_FUNC void olD_growstack (ol_State *L, int n);

olI_FUNC void olD_throw (ol_State *L, int errcode);
olI_FUNC int olD_rawrunprotected (ol_State *L, Pfunc f, void *ud);

olI_FUNC void olD_seterrorobj (ol_State *L, int errcode, StkId oldtop);

#endif


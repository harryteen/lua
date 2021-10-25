/*
** $Id: olstate.c,v 2.36.1.2 2008/01/03 15:20:39 roberto Exp $
** Global State
** See Copyright Notice in ol.h
*/


#include <stddef.h>

#define lstate_c
#define ol_CORE

#include "ol.h"

#include "oldebug.h"
#include "oldo.h"
#include "olfunc.h"
#include "olgc.h"
#include "ollex.h"
#include "olmem.h"
#include "olstate.h"
#include "olstring.h"
#include "oltable.h"
#include "oltm.h"


#define state_size(x)	(sizeof(x) + olI_EXTRASPACE)
#define fromstate(l)	(cast(lu_byte *, (l)) - olI_EXTRASPACE)
#define tostate(l)   (cast(ol_State *, cast(lu_byte *, l) + olI_EXTRASPACE))


/*
** Main thread combines a thread state and the global state
*/
typedef struct LG {
  ol_State l;
  global_State g;
} LG;
  


static void stack_init (ol_State *L1, ol_State *L) {
  /* initialize CallInfo array */
  L1->base_ci = olM_newvector(L, BASIC_CI_SIZE, CallInfo);
  L1->ci = L1->base_ci;
  L1->size_ci = BASIC_CI_SIZE;
  L1->end_ci = L1->base_ci + L1->size_ci - 1;
  /* initialize stack array */
  L1->stack = olM_newvector(L, BASIC_STACK_SIZE + EXTRA_STACK, TValue);
  L1->stacksize = BASIC_STACK_SIZE + EXTRA_STACK;
  L1->top = L1->stack;
  L1->stack_last = L1->stack+(L1->stacksize - EXTRA_STACK)-1;
  /* initialize first ci */
  L1->ci->func = L1->top;
  setnilvalue(L1->top++);  /* `function' entry for this `ci' */
  L1->base = L1->ci->base = L1->top;
  L1->ci->top = L1->top + ol_MINSTACK;
}


static void freestack (ol_State *L, ol_State *L1) {
  olM_freearray(L, L1->base_ci, L1->size_ci, CallInfo);
  olM_freearray(L, L1->stack, L1->stacksize, TValue);
}


/*
** open parts that may cause memory-allocation errors
*/
static void f_olopen (ol_State *L, void *ud) {
  global_State *g = G(L);
  UNUSED(ud);
  stack_init(L, L);  /* init stack */
  sethvalue(L, gt(L), olH_new(L, 0, 2));  /* table of globals */
  sethvalue(L, registry(L), olH_new(L, 0, 2));  /* registry */
  olS_resize(L, MINSTRTABSIZE);  /* initial size of string table */
  olT_init(L);
  olX_init(L);
  olS_fix(olS_newliteral(L, MEMERRMSG));
  g->GCthreshold = 4*g->totalbytes;
}


static void preinit_state (ol_State *L, global_State *g) {
  G(L) = g;
  L->stack = NULL;
  L->stacksize = 0;
  L->errorJmp = NULL;
  L->hook = NULL;
  L->hookmask = 0;
  L->basehookcount = 0;
  L->allowhook = 1;
  resethookcount(L);
  L->openupval = NULL;
  L->size_ci = 0;
  L->nCcalls = L->baseCcalls = 0;
  L->status = 0;
  L->base_ci = L->ci = NULL;
  L->savedpc = NULL;
  L->errfunc = 0;
  setnilvalue(gt(L));
}


static void close_state (ol_State *L) {
  global_State *g = G(L);
  olF_close(L, L->stack);  /* close all upvalues for this thread */
  olC_freeall(L);  /* collect all objects */
  ol_assert(g->rootgc == obj2gco(L));
  ol_assert(g->strt.nuse == 0);
  olM_freearray(L, G(L)->strt.hash, G(L)->strt.size, TString *);
  olZ_freebuffer(L, &g->buff);
  freestack(L, L);
  ol_assert(g->totalbytes == sizeof(LG));
  (*g->frealloc)(g->ud, fromstate(L), state_size(LG), 0);
}


ol_State *olE_newthread (ol_State *L) {
  ol_State *L1 = tostate(olM_malloc(L, state_size(ol_State)));
  olC_link(L, obj2gco(L1), ol_TTHREAD);
  preinit_state(L1, G(L));
  stack_init(L1, L);  /* init stack */
  setobj2n(L, gt(L1), gt(L));  /* share table of globals */
  L1->hookmask = L->hookmask;
  L1->basehookcount = L->basehookcount;
  L1->hook = L->hook;
  resethookcount(L1);
  ol_assert(iswhite(obj2gco(L1)));
  return L1;
}


void olE_freethread (ol_State *L, ol_State *L1) {
  olF_close(L1, L1->stack);  /* close all upvalues for this thread */
  ol_assert(L1->openupval == NULL);
  oli_userstatefree(L1);
  freestack(L, L1);
  olM_freemem(L, fromstate(L1), state_size(ol_State));
}


ol_API ol_State *ol_newstate (ol_Alloc f, void *ud) {
  int i;
  ol_State *L;
  global_State *g;
  void *l = (*f)(ud, NULL, 0, state_size(LG));
  if (l == NULL) return NULL;
  L = tostate(l);
  g = &((LG *)L)->g;
  L->next = NULL;
  L->tt = ol_TTHREAD;
  g->currentwhite = bit2mask(WHITE0BIT, FIXEDBIT);
  L->marked = olC_white(g);
  set2bits(L->marked, FIXEDBIT, SFIXEDBIT);
  preinit_state(L, g);
  g->frealloc = f;
  g->ud = ud;
  g->mainthread = L;
  g->uvhead.u.l.prev = &g->uvhead;
  g->uvhead.u.l.next = &g->uvhead;
  g->GCthreshold = 0;  /* mark it as unfinished state */
  g->strt.size = 0;
  g->strt.nuse = 0;
  g->strt.hash = NULL;
  setnilvalue(registry(L));
  olZ_initbuffer(L, &g->buff);
  g->panic = NULL;
  g->gcstate = GCSpause;
  g->rootgc = obj2gco(L);
  g->sweepstrgc = 0;
  g->sweepgc = &g->rootgc;
  g->gray = NULL;
  g->grayagain = NULL;
  g->weak = NULL;
  g->tmudata = NULL;
  g->totalbytes = sizeof(LG);
  g->gcpause = olI_GCPAUSE;
  g->gcstepmul = olI_GCMUL;
  g->gcdept = 0;
  for (i=0; i<NUM_TAGS; i++) g->mt[i] = NULL;
  if (olD_rawrunprotected(L, f_olopen, NULL) != 0) {
    /* memory allocation error: free partial state */
    close_state(L);
    L = NULL;
  }
  else
    oli_userstateopen(L);
  return L;
}


static void callallgcTM (ol_State *L, void *ud) {
  UNUSED(ud);
  olC_callGCTM(L);  /* call GC metamethods for all udata */
}


ol_API void ol_close (ol_State *L) {
  L = G(L)->mainthread;  /* only the main thread can be closed */
  ol_lock(L);
  olF_close(L, L->stack);  /* close all upvalues for this thread */
  olC_separateudata(L, 1);  /* separate udata that have GC metamethods */
  L->errfunc = 0;  /* no error function during GC metamethods */
  do {  /* repeat until no more errors */
    L->ci = L->base_ci;
    L->base = L->top = L->ci->base;
    L->nCcalls = L->baseCcalls = 0;
  } while (olD_rawrunprotected(L, callallgcTM, NULL) != 0);
  ol_assert(G(L)->tmudata == NULL);
  oli_userstateclose(L);
  close_state(L);
}


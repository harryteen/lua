/*
** $Id: ldo.c,v 2.38.1.4 2012/01/18 02:27:10 roberto Exp $
** Stack and Call structure of ol
** See Copyright Notice in ol.h
*/


#include <setjmp.h>
#include <stdlib.h>
#include <string.h>

#define ldo_c
#define ol_CORE

#include "ol.h"

#include "oldebug.h"
#include "oldo.h"
#include "olfunc.h"
#include "olgc.h"
#include "olmem.h"
#include "olobject.h"
#include "olopcodes.h"
#include "olparser.h"
#include "olstate.h"
#include "olstring.h"
#include "oltable.h"
#include "oltm.h"
#include "olundump.h"
#include "olvm.h"
#include "olzio.h"




/*
** {======================================================
** Error-recovery functions
** =======================================================
*/


/* chain list of long jump buffers */
struct ol_longjmp {
  struct ol_longjmp *previous;
  oli_jmpbuf b;
  volatile int status;  /* error code */
};


void olD_seterrorobj (ol_State *L, int errcode, StkId oldtop) {
  switch (errcode) {
    case ol_ERRMEM: {
      setsvalue2s(L, oldtop, olS_newliteral(L, MEMERRMSG));
      break;
    }
    case ol_ERRERR: {
      setsvalue2s(L, oldtop, olS_newliteral(L, "OLang.Error:  error in error handling"));
      break;
    }
    case ol_ERRSYNTAX:
    case ol_ERRRUN: {
      setobjs2s(L, oldtop, L->top - 1);  /* error message on current top */
      break;
    }
  }
  L->top = oldtop + 1;
}


static void restore_stack_limit (ol_State *L) {
  ol_assert(L->stack_last - L->stack == L->stacksize - EXTRA_STACK - 1);
  if (L->size_ci > olI_MAXCALLS) {  /* there was an overflow? */
    int inuse = cast_int(L->ci - L->base_ci);
    if (inuse + 1 < olI_MAXCALLS)  /* can `undo' overflow? */
      olD_reallocCI(L, olI_MAXCALLS);
  }
}


static void resetstack (ol_State *L, int status) {
  L->ci = L->base_ci;
  L->base = L->ci->base;
  olF_close(L, L->base);  /* close eventual pending closures */
  olD_seterrorobj(L, status, L->base);
  L->nCcalls = L->baseCcalls;
  L->allowhook = 1;
  restore_stack_limit(L);
  L->errfunc = 0;
  L->errorJmp = NULL;
}


void olD_throw (ol_State *L, int errcode) {
  if (L->errorJmp) {
    L->errorJmp->status = errcode;
    olI_THROW(L, L->errorJmp);
  }
  else {
    L->status = cast_byte(errcode);
    if (G(L)->panic) {
      resetstack(L, errcode);
      ol_unlock(L);
      G(L)->panic(L);
    }
    exit(EXIT_FAILURE);
  }
}


int olD_rawrunprotected (ol_State *L, Pfunc f, void *ud) {
  struct ol_longjmp lj;
  lj.status = 0;
  lj.previous = L->errorJmp;  /* chain new error handler */
  L->errorJmp = &lj;
  olI_TRY(L, &lj,
    (*f)(L, ud);
  );
  L->errorJmp = lj.previous;  /* restore old error handler */
  return lj.status;
}

/* }====================================================== */


static void correctstack (ol_State *L, TValue *oldstack) {
  CallInfo *ci;
  GCObject *up;
  L->top = (L->top - oldstack) + L->stack;
  for (up = L->openupval; up != NULL; up = up->gch.next)
    gco2uv(up)->v = (gco2uv(up)->v - oldstack) + L->stack;
  for (ci = L->base_ci; ci <= L->ci; ci++) {
    ci->top = (ci->top - oldstack) + L->stack;
    ci->base = (ci->base - oldstack) + L->stack;
    ci->func = (ci->func - oldstack) + L->stack;
  }
  L->base = (L->base - oldstack) + L->stack;
}


void olD_reallocstack (ol_State *L, int newsize) {
  TValue *oldstack = L->stack;
  int realsize = newsize + 1 + EXTRA_STACK;
  ol_assert(L->stack_last - L->stack == L->stacksize - EXTRA_STACK - 1);
  olM_reallocvector(L, L->stack, L->stacksize, realsize, TValue);
  L->stacksize = realsize;
  L->stack_last = L->stack+newsize;
  correctstack(L, oldstack);
}


void olD_reallocCI (ol_State *L, int newsize) {
  CallInfo *oldci = L->base_ci;
  olM_reallocvector(L, L->base_ci, L->size_ci, newsize, CallInfo);
  L->size_ci = newsize;
  L->ci = (L->ci - oldci) + L->base_ci;
  L->end_ci = L->base_ci + L->size_ci - 1;
}


void olD_growstack (ol_State *L, int n) {
  if (n <= L->stacksize)  /* double size is enough? */
    olD_reallocstack(L, 2*L->stacksize);
  else
    olD_reallocstack(L, L->stacksize + n);
}


static CallInfo *growCI (ol_State *L) {
  if (L->size_ci > olI_MAXCALLS)  /* overflow while handling overflow? */
    olD_throw(L, ol_ERRERR);
  else {
    olD_reallocCI(L, 2*L->size_ci);
    if (L->size_ci > olI_MAXCALLS)
      olG_runerror(L, "OLang.Help:  stack overflow");
  }
  return ++L->ci;
}


void olD_callhook (ol_State *L, int event, int line) {
  ol_Hook hook = L->hook;
  if (hook && L->allowhook) {
    ptrdiff_t top = savestack(L, L->top);
    ptrdiff_t ci_top = savestack(L, L->ci->top);
    ol_Debug ar;
    ar.event = event;
    ar.currentline = line;
    if (event == ol_HOOKTAILRET)
      ar.i_ci = 0;  /* tail call; no debug information about it */
    else
      ar.i_ci = cast_int(L->ci - L->base_ci);
    olD_checkstack(L, ol_MINSTACK);  /* ensure minimum stack size */
    L->ci->top = L->top + ol_MINSTACK;
    ol_assert(L->ci->top <= L->stack_last);
    L->allowhook = 0;  /* cannot call hooks inside a hook */
    ol_unlock(L);
    (*hook)(L, &ar);
    ol_lock(L);
    ol_assert(!L->allowhook);
    L->allowhook = 1;
    L->ci->top = restorestack(L, ci_top);
    L->top = restorestack(L, top);
  }
}


static StkId adjust_varargs (ol_State *L, Proto *p, int actual) {
  int i;
  int nfixargs = p->numparams;
  Table *htab = NULL;
  StkId base, fixed;
  for (; actual < nfixargs; ++actual)
    setnilvalue(L->top++);
#if defined(ol_COMPAT_VARARG)
  if (p->is_vararg & VARARG_NEEDSARG) { /* compat. with old-style vararg? */
    int nvar = actual - nfixargs;  /* number of extra arguments */
    ol_assert(p->is_vararg & VARARG_HASARG);
    olC_checkGC(L);
    olD_checkstack(L, p->maxstacksize);
    htab = olH_new(L, nvar, 1);  /* create `arg' table */
    for (i=0; i<nvar; i++)  /* put extra arguments into `arg' table */
      setobj2n(L, olH_setnum(L, htab, i+1), L->top - nvar + i);
    /* store counter in field `n' */
    setnvalue(olH_setstr(L, htab, olS_newliteral(L, "n")), cast_num(nvar));
  }
#endif
  /* move fixed parameters to final position */
  fixed = L->top - actual;  /* first fixed argument */
  base = L->top;  /* final position of first argument */
  for (i=0; i<nfixargs; i++) {
    setobjs2s(L, L->top++, fixed+i);
    setnilvalue(fixed+i);
  }
  /* add `arg' parameter */
  if (htab) {
    sethvalue(L, L->top++, htab);
    ol_assert(iswhite(obj2gco(htab)));
  }
  return base;
}


static StkId tryfuncTM (ol_State *L, StkId func) {
  const TValue *tm = olT_gettmbyobj(L, func, TM_CALL);
  StkId p;
  ptrdiff_t funcr = savestack(L, func);
  if (!ttisfunction(tm))
    olG_typeerror(L, func, "call");
  /* Open a hole inside the stack at `func' */
  for (p = L->top; p > func; p--) setobjs2s(L, p, p-1);
  incr_top(L);
  func = restorestack(L, funcr);  /* previous call may change stack */
  setobj2s(L, func, tm);  /* tag method is the new function to be called */
  return func;
}



#define inc_ci(L) \
  ((L->ci == L->end_ci) ? growCI(L) : \
   (condhardstacktests(olD_reallocCI(L, L->size_ci)), ++L->ci))


int olD_precall (ol_State *L, StkId func, int nresults) {
  LClosure *cl;
  ptrdiff_t funcr;
  if (!ttisfunction(func)) /* `func' is not a function? */
    func = tryfuncTM(L, func);  /* check the `function' tag method */
  funcr = savestack(L, func);
  cl = &clvalue(func)->l;
  L->ci->savedpc = L->savedpc;
  if (!cl->isC) {  /* ol function? prepare its call */
    CallInfo *ci;
    StkId st, base;
    Proto *p = cl->p;
    olD_checkstack(L, p->maxstacksize);
    func = restorestack(L, funcr);
    if (!p->is_vararg) {  /* no varargs? */
      base = func + 1;
      if (L->top > base + p->numparams)
        L->top = base + p->numparams;
    }
    else {  /* vararg function */
      int nargs = cast_int(L->top - func) - 1;
      base = adjust_varargs(L, p, nargs);
      func = restorestack(L, funcr);  /* previous call may change the stack */
    }
    ci = inc_ci(L);  /* now `enter' new function */
    ci->func = func;
    L->base = ci->base = base;
    ci->top = L->base + p->maxstacksize;
    ol_assert(ci->top <= L->stack_last);
    L->savedpc = p->code;  /* starting point */
    ci->tailcalls = 0;
    ci->nresults = nresults;
    for (st = L->top; st < ci->top; st++)
      setnilvalue(st);
    L->top = ci->top;
    if (L->hookmask & ol_MASKCALL) {
      L->savedpc++;  /* hooks assume 'pc' is already incremented */
      olD_callhook(L, ol_HOOKCALL, -1);
      L->savedpc--;  /* correct 'pc' */
    }
    return PCRol;
  }
  else {  /* if is a C function, call it */
    CallInfo *ci;
    int n;
    olD_checkstack(L, ol_MINSTACK);  /* ensure minimum stack size */
    ci = inc_ci(L);  /* now `enter' new function */
    ci->func = restorestack(L, funcr);
    L->base = ci->base = ci->func + 1;
    ci->top = L->top + ol_MINSTACK;
    ol_assert(ci->top <= L->stack_last);
    ci->nresults = nresults;
    if (L->hookmask & ol_MASKCALL)
      olD_callhook(L, ol_HOOKCALL, -1);
    ol_unlock(L);
    n = (*curr_func(L)->c.f)(L);  /* do the actual call */
    ol_lock(L);
    if (n < 0)  /* yielding? */
      return PCRYIELD;
    else {
      olD_poscall(L, L->top - n);
      return PCRC;
    }
  }
}


static StkId callrethooks (ol_State *L, StkId firstResult) {
  ptrdiff_t fr = savestack(L, firstResult);  /* next call may change stack */
  olD_callhook(L, ol_HOOKRET, -1);
  if (f_isol(L->ci)) {  /* ol function? */
    while ((L->hookmask & ol_MASKRET) && L->ci->tailcalls--) /* tail calls */
      olD_callhook(L, ol_HOOKTAILRET, -1);
  }
  return restorestack(L, fr);
}


int olD_poscall (ol_State *L, StkId firstResult) {
  StkId res;
  int wanted, i;
  CallInfo *ci;
  if (L->hookmask & ol_MASKRET)
    firstResult = callrethooks(L, firstResult);
  ci = L->ci--;
  res = ci->func;  /* res == final position of 1st result */
  wanted = ci->nresults;
  L->base = (ci - 1)->base;  /* restore base */
  L->savedpc = (ci - 1)->savedpc;  /* restore savedpc */
  /* move results to correct place */
  for (i = wanted; i != 0 && firstResult < L->top; i--)
    setobjs2s(L, res++, firstResult++);
  while (i-- > 0)
    setnilvalue(res++);
  L->top = res;
  return (wanted - ol_MULTRET);  /* 0 iff wanted == ol_MULTRET */
}


/*
** Call a function (C or ol). The function to be called is at *func.
** The arguments are on the stack, right after the function.
** When returns, all the results are on the stack, starting at the original
** function position.
*/ 
void olD_call (ol_State *L, StkId func, int nResults) {
  if (++L->nCcalls >= olI_MAXCCALLS) {
    if (L->nCcalls == olI_MAXCCALLS)
      olG_runerror(L, "OLang.Help:  C stack overflow");
    else if (L->nCcalls >= (olI_MAXCCALLS + (olI_MAXCCALLS>>3)))
      olD_throw(L, ol_ERRERR);  /* error while handing stack error */
  }
  if (olD_precall(L, func, nResults) == PCRol)  /* is a ol function? */
    olV_execute(L, 1);  /* call it */
  L->nCcalls--;
  olC_checkGC(L);
}


static void resume (ol_State *L, void *ud) {
  StkId firstArg = cast(StkId, ud);
  CallInfo *ci = L->ci;
  if (L->status == 0) {  /* start coroutine? */
    ol_assert(ci == L->base_ci && firstArg > L->base);
    if (olD_precall(L, firstArg - 1, ol_MULTRET) != PCRol)
      return;
  }
  else {  /* resuming from previous yield */
    ol_assert(L->status == ol_YIELD);
    L->status = 0;
    if (!f_isol(ci)) {  /* `common' yield? */
      /* finish interrupted execution of `OP_CALL' */
      ol_assert(GET_OPCODE(*((ci-1)->savedpc - 1)) == OP_CALL ||
                 GET_OPCODE(*((ci-1)->savedpc - 1)) == OP_TAILCALL);
      if (olD_poscall(L, firstArg))  /* complete it... */
        L->top = L->ci->top;  /* and correct top if not multiple results */
    }
    else  /* yielded inside a hook: just continue its execution */
      L->base = L->ci->base;
  }
  olV_execute(L, cast_int(L->ci - L->base_ci));
}


static int resume_error (ol_State *L, const char *msg) {
  L->top = L->ci->base;
  setsvalue2s(L, L->top, olS_new(L, msg));
  incr_top(L);
  ol_unlock(L);
  return ol_ERRRUN;
}


ol_API int ol_resume (ol_State *L, int nargs) {
  int status;
  ol_lock(L);
  if (L->status != ol_YIELD && (L->status != 0 || L->ci != L->base_ci))
      return resume_error(L, "OLang.ResumeError:  cannot resume non-suspended coroutine");
  if (L->nCcalls >= olI_MAXCCALLS)
    return resume_error(L, "OLang.Help:  C stack overflow");
  oli_userstateresume(L, nargs);
  ol_assert(L->errfunc == 0);
  L->baseCcalls = ++L->nCcalls;
  status = olD_rawrunprotected(L, resume, L->top - nargs);
  if (status != 0) {  /* error? */
    L->status = cast_byte(status);  /* mark thread as `dead' */
    olD_seterrorobj(L, status, L->top);
    L->ci->top = L->top;
  }
  else {
    ol_assert(L->nCcalls == L->baseCcalls);
    status = L->status;
  }
  --L->nCcalls;
  ol_unlock(L);
  return status;
}


ol_API int ol_yield (ol_State *L, int nresults) {
  oli_userstateyield(L, nresults);
  ol_lock(L);
  if (L->nCcalls > L->baseCcalls)
    olG_runerror(L, "OLang.YieldError:  attempt to yield across metamethod/C-call boundary");
  L->base = L->top - nresults;  /* protect stack slots below */
  L->status = ol_YIELD;
  ol_unlock(L);
  return -1;
}


int olD_pcall (ol_State *L, Pfunc func, void *u,
                ptrdiff_t old_top, ptrdiff_t ef) {
  int status;
  unsigned short oldnCcalls = L->nCcalls;
  ptrdiff_t old_ci = saveci(L, L->ci);
  lu_byte old_allowhooks = L->allowhook;
  ptrdiff_t old_errfunc = L->errfunc;
  L->errfunc = ef;
  status = olD_rawrunprotected(L, func, u);
  if (status != 0) {  /* an error occurred? */
    StkId oldtop = restorestack(L, old_top);
    olF_close(L, oldtop);  /* close eventual pending closures */
    olD_seterrorobj(L, status, oldtop);
    L->nCcalls = oldnCcalls;
    L->ci = restoreci(L, old_ci);
    L->base = L->ci->base;
    L->savedpc = L->ci->savedpc;
    L->allowhook = old_allowhooks;
    restore_stack_limit(L);
  }
  L->errfunc = old_errfunc;
  return status;
}



/*
** Execute a protected parser.
*/
struct SParser {  /* data to `f_parser' */
  ZIO *z;
  Mbuffer buff;  /* buffer to be used by the scanner */
  const char *name;
};

static void f_parser (ol_State *L, void *ud) {
  int i;
  Proto *tf;
  Closure *cl;
  struct SParser *p = cast(struct SParser *, ud);
  int c = olZ_lookahead(p->z);
  olC_checkGC(L);
  tf = ((c == ol_SIGNATURE[0]) ? olU_undump : olY_parser)(L, p->z,
                                                             &p->buff, p->name);
  cl = olF_newLclosure(L, tf->nups, hvalue(gt(L)));
  cl->l.p = tf;
  for (i = 0; i < tf->nups; i++)  /* initialize eventual upvalues */
    cl->l.upvals[i] = olF_newupval(L);
  setclvalue(L, L->top, cl);
  incr_top(L);
}


int olD_protectedparser (ol_State *L, ZIO *z, const char *name) {
  struct SParser p;
  int status;
  p.z = z; p.name = name;
  olZ_initbuffer(L, &p.buff);
  status = olD_pcall(L, f_parser, &p, savestack(L, L->top), L->errfunc);
  olZ_freebuffer(L, &p.buff);
  return status;
}



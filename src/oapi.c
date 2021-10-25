/*
** $Id: lapi.c,v 2.55.1.5 2008/07/04 18:41:18 roberto Exp $
** ol API
** See Copyright Notice in ol.h
*/


#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <string.h>

#define lapi_c
#define ol_CORE

#include "ol.h"

#include "oapi.h"
#include "oldebug.h"
#include "oldo.h"
#include "olfunc.h"
#include "olgc.h"
#include "olmem.h"
#include "olobject.h"
#include "olstate.h"
#include "olstring.h"
#include "oltable.h"
#include "oltm.h"
#include "oundump.h"
#include "ovm.h"



const char ol_ident[] =
  "$ol: " ol_RELEASE " " ol_COPYRIGHT " $\n"
  "$Authors: " ol_AUTHORS " $\n"
  "$URL: www.ol.org $\n";



#define api_checknelems(L, n)	api_check(L, (n) <= (L->top - L->base))

#define api_checkvalidindex(L, i)	api_check(L, (i) != olO_nilobject)

#define api_incr_top(L)   {api_check(L, L->top < L->ci->top); L->top++;}



static TValue *index2adr (ol_State *L, int idx) {
  if (idx > 0) {
    TValue *o = L->base + (idx - 1);
    api_check(L, idx <= L->ci->top - L->base);
    if (o >= L->top) return cast(TValue *, olO_nilobject);
    else return o;
  }
  else if (idx > ol_REGISTRYINDEX) {
    api_check(L, idx != 0 && -idx <= L->top - L->base);
    return L->top + idx;
  }
  else switch (idx) {  /* pseudo-indices */
    case ol_REGISTRYINDEX: return registry(L);
    case ol_ENVIRONINDEX: {
      Closure *func = curr_func(L);
      sethvalue(L, &L->env, func->c.env);
      return &L->env;
    }
    case ol_GLOBALSINDEX: return gt(L);
    default: {
      Closure *func = curr_func(L);
      idx = ol_GLOBALSINDEX - idx;
      return (idx <= func->c.nupvalues)
                ? &func->c.upvalue[idx-1]
                : cast(TValue *, olO_nilobject);
    }
  }
}


static Table *getcurrenv (ol_State *L) {
  if (L->ci == L->base_ci)  /* no enclosing function? */
    return hvalue(gt(L));  /* use global table as environment */
  else {
    Closure *func = curr_func(L);
    return func->c.env;
  }
}


void olA_pushobject (ol_State *L, const TValue *o) {
  setobj2s(L, L->top, o);
  api_incr_top(L);
}


ol_API int ol_checkstack (ol_State *L, int size) {
  int res = 1;
  ol_lock(L);
  if (size > olI_MAXCSTACK || (L->top - L->base + size) > olI_MAXCSTACK)
    res = 0;  /* stack overflow */
  else if (size > 0) {
    olD_checkstack(L, size);
    if (L->ci->top < L->top + size)
      L->ci->top = L->top + size;
  }
  ol_unlock(L);
  return res;
}


ol_API void ol_xmove (ol_State *from, ol_State *to, int n) {
  int i;
  if (from == to) return;
  ol_lock(to);
  api_checknelems(from, n);
  api_check(from, G(from) == G(to));
  api_check(from, to->ci->top - to->top >= n);
  from->top -= n;
  for (i = 0; i < n; i++) {
    setobj2s(to, to->top++, from->top + i);
  }
  ol_unlock(to);
}


ol_API void ol_setlevel (ol_State *from, ol_State *to) {
  to->nCcalls = from->nCcalls;
}


ol_API ol_CFunction ol_atpanic (ol_State *L, ol_CFunction panicf) {
  ol_CFunction old;
  ol_lock(L);
  old = G(L)->panic;
  G(L)->panic = panicf;
  ol_unlock(L);
  return old;
}


ol_API ol_State *ol_newthread (ol_State *L) {
  ol_State *L1;
  ol_lock(L);
  olC_checkGC(L);
  L1 = olE_newthread(L);
  setthvalue(L, L->top, L1);
  api_incr_top(L);
  ol_unlock(L);
  oli_userstatethread(L, L1);
  return L1;
}



/*
** basic stack manipulation
*/


ol_API int ol_gettop (ol_State *L) {
  return cast_int(L->top - L->base);
}


ol_API void ol_settop (ol_State *L, int idx) {
  ol_lock(L);
  if (idx >= 0) {
    api_check(L, idx <= L->stack_last - L->base);
    while (L->top < L->base + idx)
      setnilvalue(L->top++);
    L->top = L->base + idx;
  }
  else {
    api_check(L, -(idx+1) <= (L->top - L->base));
    L->top += idx+1;  /* `subtract' index (index is negative) */
  }
  ol_unlock(L);
}


ol_API void ol_remove (ol_State *L, int idx) {
  StkId p;
  ol_lock(L);
  p = index2adr(L, idx);
  api_checkvalidindex(L, p);
  while (++p < L->top) setobjs2s(L, p-1, p);
  L->top--;
  ol_unlock(L);
}


ol_API void ol_insert (ol_State *L, int idx) {
  StkId p;
  StkId q;
  ol_lock(L);
  p = index2adr(L, idx);
  api_checkvalidindex(L, p);
  for (q = L->top; q>p; q--) setobjs2s(L, q, q-1);
  setobjs2s(L, p, L->top);
  ol_unlock(L);
}


ol_API void ol_replace (ol_State *L, int idx) {
  StkId o;
  ol_lock(L);
  /* explicit test for incompatible code */
  if (idx == ol_ENVIRONINDEX && L->ci == L->base_ci)
    olG_runerror(L, "OLang.LexerError: no calling environment");
  api_checknelems(L, 1);
  o = index2adr(L, idx);
  api_checkvalidindex(L, o);
  if (idx == ol_ENVIRONINDEX) {
    Closure *func = curr_func(L);
    api_check(L, ttistable(L->top - 1)); 
    func->c.env = hvalue(L->top - 1);
    olC_barrier(L, func, L->top - 1);
  }
  else {
    setobj(L, o, L->top - 1);
    if (idx < ol_GLOBALSINDEX)  /* function upvalue? */
      olC_barrier(L, curr_func(L), L->top - 1);
  }
  L->top--;
  ol_unlock(L);
}


ol_API void ol_pushvalue (ol_State *L, int idx) {
  ol_lock(L);
  setobj2s(L, L->top, index2adr(L, idx));
  api_incr_top(L);
  ol_unlock(L);
}



/*
** access functions (stack -> C)
*/


ol_API int ol_type (ol_State *L, int idx) {
  StkId o = index2adr(L, idx);
  return (o == olO_nilobject) ? ol_TNONE : ttype(o);
}


ol_API const char *ol_typename (ol_State *L, int t) {
  UNUSED(L);
  return (t == ol_TNONE) ? "no value" : olT_typenames[t];
}


ol_API int ol_iscfunction (ol_State *L, int idx) {
  StkId o = index2adr(L, idx);
  return iscfunction(o);
}


ol_API int ol_isnumber (ol_State *L, int idx) {
  TValue n;
  const TValue *o = index2adr(L, idx);
  return tonumber(o, &n);
}


ol_API int ol_isstring (ol_State *L, int idx) {
  int t = ol_type(L, idx);
  return (t == ol_TSTRING || t == ol_TNUMBER);
}


ol_API int ol_isuserdata (ol_State *L, int idx) {
  const TValue *o = index2adr(L, idx);
  return (ttisuserdata(o) || ttislightuserdata(o));
}


ol_API int ol_rawequal (ol_State *L, int index1, int index2) {
  StkId o1 = index2adr(L, index1);
  StkId o2 = index2adr(L, index2);
  return (o1 == olO_nilobject || o2 == olO_nilobject) ? 0
         : olO_rawequalObj(o1, o2);
}


ol_API int ol_equal (ol_State *L, int index1, int index2) {
  StkId o1, o2;
  int i;
  ol_lock(L);  /* may call tag method */
  o1 = index2adr(L, index1);
  o2 = index2adr(L, index2);
  i = (o1 == olO_nilobject || o2 == olO_nilobject) ? 0 : equalobj(L, o1, o2);
  ol_unlock(L);
  return i;
}


ol_API int ol_lessthan (ol_State *L, int index1, int index2) {
  StkId o1, o2;
  int i;
  ol_lock(L);  /* may call tag method */
  o1 = index2adr(L, index1);
  o2 = index2adr(L, index2);
  i = (o1 == olO_nilobject || o2 == olO_nilobject) ? 0
       : olV_lessthan(L, o1, o2);
  ol_unlock(L);
  return i;
}



ol_API ol_Number ol_tonumber (ol_State *L, int idx) {
  TValue n;
  const TValue *o = index2adr(L, idx);
  if (tonumber(o, &n))
    return nvalue(o);
  else
    return 0;
}


ol_API ol_Integer ol_tointeger (ol_State *L, int idx) {
  TValue n;
  const TValue *o = index2adr(L, idx);
  if (tonumber(o, &n)) {
    ol_Integer res;
    ol_Number num = nvalue(o);
    ol_number2integer(res, num);
    return res;
  }
  else
    return 0;
}


ol_API int ol_toboolean (ol_State *L, int idx) {
  const TValue *o = index2adr(L, idx);
  return !l_isfalse(o);
}


ol_API const char *ol_tolstring (ol_State *L, int idx, size_t *len) {
  StkId o = index2adr(L, idx);
  if (!ttisstring(o)) {
    ol_lock(L);  /* `olV_tostring' may create a new string */
    if (!olV_tostring(L, o)) {  /* conversion failed? */
      if (len != NULL) *len = 0;
      ol_unlock(L);
      return NULL;
    }
    olC_checkGC(L);
    o = index2adr(L, idx);  /* previous call may reallocate the stack */
    ol_unlock(L);
  }
  if (len != NULL) *len = tsvalue(o)->len;
  return svalue(o);
}


ol_API size_t ol_objlen (ol_State *L, int idx) {
  StkId o = index2adr(L, idx);
  switch (ttype(o)) {
    case ol_TSTRING: return tsvalue(o)->len;
    case ol_TUSERDATA: return uvalue(o)->len;
    case ol_TTABLE: return olH_getn(hvalue(o));
    case ol_TNUMBER: {
      size_t l;
      ol_lock(L);  /* `olV_tostring' may create a new string */
      l = (olV_tostring(L, o) ? tsvalue(o)->len : 0);
      ol_unlock(L);
      return l;
    }
    default: return 0;
  }
}


ol_API ol_CFunction ol_tocfunction (ol_State *L, int idx) {
  StkId o = index2adr(L, idx);
  return (!iscfunction(o)) ? NULL : clvalue(o)->c.f;
}


ol_API void *ol_touserdata (ol_State *L, int idx) {
  StkId o = index2adr(L, idx);
  switch (ttype(o)) {
    case ol_TUSERDATA: return (rawuvalue(o) + 1);
    case ol_TLIGHTUSERDATA: return pvalue(o);
    default: return NULL;
  }
}


ol_API ol_State *ol_tothread (ol_State *L, int idx) {
  StkId o = index2adr(L, idx);
  return (!ttisthread(o)) ? NULL : thvalue(o);
}


ol_API const void *ol_topointer (ol_State *L, int idx) {
  StkId o = index2adr(L, idx);
  switch (ttype(o)) {
    case ol_TTABLE: return hvalue(o);
    case ol_TFUNCTION: return clvalue(o);
    case ol_TTHREAD: return thvalue(o);
    case ol_TUSERDATA:
    case ol_TLIGHTUSERDATA:
      return ol_touserdata(L, idx);
    default: return NULL;
  }
}



/*
** push functions (C -> stack)
*/


ol_API void ol_pushnil (ol_State *L) {
  ol_lock(L);
  setnilvalue(L->top);
  api_incr_top(L);
  ol_unlock(L);
}


ol_API void ol_pushnumber (ol_State *L, ol_Number n) {
  ol_lock(L);
  setnvalue(L->top, n);
  api_incr_top(L);
  ol_unlock(L);
}


ol_API void ol_pushinteger (ol_State *L, ol_Integer n) {
  ol_lock(L);
  setnvalue(L->top, cast_num(n));
  api_incr_top(L);
  ol_unlock(L);
}


ol_API void ol_pushlstring (ol_State *L, const char *s, size_t len) {
  ol_lock(L);
  olC_checkGC(L);
  setsvalue2s(L, L->top, olS_newlstr(L, s, len));
  api_incr_top(L);
  ol_unlock(L);
}


ol_API void ol_pushstring (ol_State *L, const char *s) {
  if (s == NULL)
    ol_pushnil(L);
  else
    ol_pushlstring(L, s, strlen(s));
}


ol_API const char *ol_pushvfstring (ol_State *L, const char *fmt,
                                      va_list argp) {
  const char *ret;
  ol_lock(L);
  olC_checkGC(L);
  ret = olO_pushvfstring(L, fmt, argp);
  ol_unlock(L);
  return ret;
}


ol_API const char *ol_pushfstring (ol_State *L, const char *fmt, ...) {
  const char *ret;
  va_list argp;
  ol_lock(L);
  olC_checkGC(L);
  va_start(argp, fmt);
  ret = olO_pushvfstring(L, fmt, argp);
  va_end(argp);
  ol_unlock(L);
  return ret;
}


ol_API void ol_pushcclosure (ol_State *L, ol_CFunction fn, int n) {
  Closure *cl;
  ol_lock(L);
  olC_checkGC(L);
  api_checknelems(L, n);
  cl = olF_newCclosure(L, n, getcurrenv(L));
  cl->c.f = fn;
  L->top -= n;
  while (n--)
    setobj2n(L, &cl->c.upvalue[n], L->top+n);
  setclvalue(L, L->top, cl);
  ol_assert(iswhite(obj2gco(cl)));
  api_incr_top(L);
  ol_unlock(L);
}


ol_API void ol_pushboolean (ol_State *L, int b) {
  ol_lock(L);
  setbvalue(L->top, (b != 0));  /* ensure that true is 1 */
  api_incr_top(L);
  ol_unlock(L);
}


ol_API void ol_pushlightuserdata (ol_State *L, void *p) {
  ol_lock(L);
  setpvalue(L->top, p);
  api_incr_top(L);
  ol_unlock(L);
}


ol_API int ol_pushthread (ol_State *L) {
  ol_lock(L);
  setthvalue(L, L->top, L);
  api_incr_top(L);
  ol_unlock(L);
  return (G(L)->mainthread == L);
}



/*
** get functions (ol -> stack)
*/


ol_API void ol_gettable (ol_State *L, int idx) {
  StkId t;
  ol_lock(L);
  t = index2adr(L, idx);
  api_checkvalidindex(L, t);
  olV_gettable(L, t, L->top - 1, L->top - 1);
  ol_unlock(L);
}


ol_API void ol_getfield (ol_State *L, int idx, const char *k) {
  StkId t;
  TValue key;
  ol_lock(L);
  t = index2adr(L, idx);
  api_checkvalidindex(L, t);
  setsvalue(L, &key, olS_new(L, k));
  olV_gettable(L, t, &key, L->top);
  api_incr_top(L);
  ol_unlock(L);
}


ol_API void ol_rawget (ol_State *L, int idx) {
  StkId t;
  ol_lock(L);
  t = index2adr(L, idx);
  api_check(L, ttistable(t));
  setobj2s(L, L->top - 1, olH_get(hvalue(t), L->top - 1));
  ol_unlock(L);
}


ol_API void ol_rawgeti (ol_State *L, int idx, int n) {
  StkId o;
  ol_lock(L);
  o = index2adr(L, idx);
  api_check(L, ttistable(o));
  setobj2s(L, L->top, olH_getnum(hvalue(o), n));
  api_incr_top(L);
  ol_unlock(L);
}


ol_API void ol_createtable (ol_State *L, int narray, int nrec) {
  ol_lock(L);
  olC_checkGC(L);
  sethvalue(L, L->top, olH_new(L, narray, nrec));
  api_incr_top(L);
  ol_unlock(L);
}


ol_API int ol_getmetatable (ol_State *L, int objindex) {
  const TValue *obj;
  Table *mt = NULL;
  int res;
  ol_lock(L);
  obj = index2adr(L, objindex);
  switch (ttype(obj)) {
    case ol_TTABLE:
      mt = hvalue(obj)->metatable;
      break;
    case ol_TUSERDATA:
      mt = uvalue(obj)->metatable;
      break;
    default:
      mt = G(L)->mt[ttype(obj)];
      break;
  }
  if (mt == NULL)
    res = 0;
  else {
    sethvalue(L, L->top, mt);
    api_incr_top(L);
    res = 1;
  }
  ol_unlock(L);
  return res;
}


ol_API void ol_getfenv (ol_State *L, int idx) {
  StkId o;
  ol_lock(L);
  o = index2adr(L, idx);
  api_checkvalidindex(L, o);
  switch (ttype(o)) {
    case ol_TFUNCTION:
      sethvalue(L, L->top, clvalue(o)->c.env);
      break;
    case ol_TUSERDATA:
      sethvalue(L, L->top, uvalue(o)->env);
      break;
    case ol_TTHREAD:
      setobj2s(L, L->top,  gt(thvalue(o)));
      break;
    default:
      setnilvalue(L->top);
      break;
  }
  api_incr_top(L);
  ol_unlock(L);
}


/*
** set functions (stack -> ol)
*/


ol_API void ol_settable (ol_State *L, int idx) {
  StkId t;
  ol_lock(L);
  api_checknelems(L, 2);
  t = index2adr(L, idx);
  api_checkvalidindex(L, t);
  olV_settable(L, t, L->top - 2, L->top - 1);
  L->top -= 2;  /* pop index and value */
  ol_unlock(L);
}


ol_API void ol_setfield (ol_State *L, int idx, const char *k) {
  StkId t;
  TValue key;
  ol_lock(L);
  api_checknelems(L, 1);
  t = index2adr(L, idx);
  api_checkvalidindex(L, t);
  setsvalue(L, &key, olS_new(L, k));
  olV_settable(L, t, &key, L->top - 1);
  L->top--;  /* pop value */
  ol_unlock(L);
}


ol_API void ol_rawset (ol_State *L, int idx) {
  StkId t;
  ol_lock(L);
  api_checknelems(L, 2);
  t = index2adr(L, idx);
  api_check(L, ttistable(t));
  setobj2t(L, olH_set(L, hvalue(t), L->top-2), L->top-1);
  olC_barriert(L, hvalue(t), L->top-1);
  L->top -= 2;
  ol_unlock(L);
}


ol_API void ol_rawseti (ol_State *L, int idx, int n) {
  StkId o;
  ol_lock(L);
  api_checknelems(L, 1);
  o = index2adr(L, idx);
  api_check(L, ttistable(o));
  setobj2t(L, olH_setnum(L, hvalue(o), n), L->top-1);
  olC_barriert(L, hvalue(o), L->top-1);
  L->top--;
  ol_unlock(L);
}


ol_API int ol_setmetatable (ol_State *L, int objindex) {
  TValue *obj;
  Table *mt;
  ol_lock(L);
  api_checknelems(L, 1);
  obj = index2adr(L, objindex);
  api_checkvalidindex(L, obj);
  if (ttisnil(L->top - 1))
    mt = NULL;
  else {
    api_check(L, ttistable(L->top - 1));
    mt = hvalue(L->top - 1);
  }
  switch (ttype(obj)) {
    case ol_TTABLE: {
      hvalue(obj)->metatable = mt;
      if (mt)
        olC_objbarriert(L, hvalue(obj), mt);
      break;
    }
    case ol_TUSERDATA: {
      uvalue(obj)->metatable = mt;
      if (mt)
        olC_objbarrier(L, rawuvalue(obj), mt);
      break;
    }
    default: {
      G(L)->mt[ttype(obj)] = mt;
      break;
    }
  }
  L->top--;
  ol_unlock(L);
  return 1;
}


ol_API int ol_setfenv (ol_State *L, int idx) {
  StkId o;
  int res = 1;
  ol_lock(L);
  api_checknelems(L, 1);
  o = index2adr(L, idx);
  api_checkvalidindex(L, o);
  api_check(L, ttistable(L->top - 1));
  switch (ttype(o)) {
    case ol_TFUNCTION:
      clvalue(o)->c.env = hvalue(L->top - 1);
      break;
    case ol_TUSERDATA:
      uvalue(o)->env = hvalue(L->top - 1);
      break;
    case ol_TTHREAD:
      sethvalue(L, gt(thvalue(o)), hvalue(L->top - 1));
      break;
    default:
      res = 0;
      break;
  }
  if (res) olC_objbarrier(L, gcvalue(o), hvalue(L->top - 1));
  L->top--;
  ol_unlock(L);
  return res;
}


/*
** `load' and `call' functions (run ol code)
*/


#define adjustresults(L,nres) \
    { if (nres == ol_MULTRET && L->top >= L->ci->top) L->ci->top = L->top; }


#define checkresults(L,na,nr) \
     api_check(L, (nr) == ol_MULTRET || (L->ci->top - L->top >= (nr) - (na)))
	

ol_API void ol_call (ol_State *L, int nargs, int nresults) {
  StkId func;
  ol_lock(L);
  api_checknelems(L, nargs+1);
  checkresults(L, nargs, nresults);
  func = L->top - (nargs+1);
  olD_call(L, func, nresults);
  adjustresults(L, nresults);
  ol_unlock(L);
}



/*
** Execute a protected call.
*/
struct CallS {  /* data to `f_call' */
  StkId func;
  int nresults;
};


static void f_call (ol_State *L, void *ud) {
  struct CallS *c = cast(struct CallS *, ud);
  olD_call(L, c->func, c->nresults);
}



ol_API int ol_pcall (ol_State *L, int nargs, int nresults, int errfunc) {
  struct CallS c;
  int status;
  ptrdiff_t func;
  ol_lock(L);
  api_checknelems(L, nargs+1);
  checkresults(L, nargs, nresults);
  if (errfunc == 0)
    func = 0;
  else {
    StkId o = index2adr(L, errfunc);
    api_checkvalidindex(L, o);
    func = savestack(L, o);
  }
  c.func = L->top - (nargs+1);  /* function to be called */
  c.nresults = nresults;
  status = olD_pcall(L, f_call, &c, savestack(L, c.func), func);
  adjustresults(L, nresults);
  ol_unlock(L);
  return status;
}


/*
** Execute a protected C call.
*/
struct CCallS {  /* data to `f_Ccall' */
  ol_CFunction func;
  void *ud;
};


static void f_Ccall (ol_State *L, void *ud) {
  struct CCallS *c = cast(struct CCallS *, ud);
  Closure *cl;
  cl = olF_newCclosure(L, 0, getcurrenv(L));
  cl->c.f = c->func;
  setclvalue(L, L->top, cl);  /* push function */
  api_incr_top(L);
  setpvalue(L->top, c->ud);  /* push only argument */
  api_incr_top(L);
  olD_call(L, L->top - 2, 0);
}


ol_API int ol_cpcall (ol_State *L, ol_CFunction func, void *ud) {
  struct CCallS c;
  int status;
  ol_lock(L);
  c.func = func;
  c.ud = ud;
  status = olD_pcall(L, f_Ccall, &c, savestack(L, L->top), 0);
  ol_unlock(L);
  return status;
}


ol_API int ol_load (ol_State *L, ol_Reader reader, void *data,
                      const char *chunkname) {
  ZIO z;
  int status;
  ol_lock(L);
  if (!chunkname) chunkname = "?";
  olZ_init(L, &z, reader, data);
  status = olD_protectedparser(L, &z, chunkname);
  ol_unlock(L);
  return status;
}


ol_API int ol_dump (ol_State *L, ol_Writer writer, void *data) {
  int status;
  TValue *o;
  ol_lock(L);
  api_checknelems(L, 1);
  o = L->top - 1;
  if (isLfunction(o))
    status = olU_dump(L, clvalue(o)->l.p, writer, data, 0);
  else
    status = 1;
  ol_unlock(L);
  return status;
}


ol_API int  ol_status (ol_State *L) {
  return L->status;
}


/*
** Garbage-collection function
*/

ol_API int ol_gc (ol_State *L, int what, int data) {
  int res = 0;
  global_State *g;
  ol_lock(L);
  g = G(L);
  switch (what) {
    case ol_GCSTOP: {
      g->GCthreshold = MAX_LUMEM;
      break;
    }
    case ol_GCRESTART: {
      g->GCthreshold = g->totalbytes;
      break;
    }
    case ol_GCCOLLECT: {
      olC_fullgc(L);
      break;
    }
    case ol_GCCOUNT: {
      /* GC values are expressed in Kbytes: #bytes/2^10 */
      res = cast_int(g->totalbytes >> 10);
      break;
    }
    case ol_GCCOUNTB: {
      res = cast_int(g->totalbytes & 0x3ff);
      break;
    }
    case ol_GCSTEP: {
      lu_mem a = (cast(lu_mem, data) << 10);
      if (a <= g->totalbytes)
        g->GCthreshold = g->totalbytes - a;
      else
        g->GCthreshold = 0;
      while (g->GCthreshold <= g->totalbytes) {
        olC_step(L);
        if (g->gcstate == GCSpause) {  /* end of cycle? */
          res = 1;  /* signal it */
          break;
        }
      }
      break;
    }
    case ol_GCSETPAUSE: {
      res = g->gcpause;
      g->gcpause = data;
      break;
    }
    case ol_GCSETSTEPMUL: {
      res = g->gcstepmul;
      g->gcstepmul = data;
      break;
    }
    default: res = -1;  /* invalid option */
  }
  ol_unlock(L);
  return res;
}



/*
** miscellaneous functions
*/


ol_API int ol_error (ol_State *L) {
  ol_lock(L);
  api_checknelems(L, 1);
  olG_errormsg(L);
  ol_unlock(L);
  return 0;  /* to avoid warnings */
}


ol_API int ol_next (ol_State *L, int idx) {
  StkId t;
  int more;
  ol_lock(L);
  t = index2adr(L, idx);
  api_check(L, ttistable(t));
  more = olH_next(L, hvalue(t), L->top - 1);
  if (more) {
    api_incr_top(L);
  }
  else  /* no more elements */
    L->top -= 1;  /* remove key */
  ol_unlock(L);
  return more;
}


ol_API void ol_concat (ol_State *L, int n) {
  ol_lock(L);
  api_checknelems(L, n);
  if (n >= 2) {
    olC_checkGC(L);
    olV_concat(L, n, cast_int(L->top - L->base) - 1);
    L->top -= (n-1);
  }
  else if (n == 0) {  /* push empty string */
    setsvalue2s(L, L->top, olS_newlstr(L, "", 0));
    api_incr_top(L);
  }
  /* else n == 1; nothing to do */
  ol_unlock(L);
}


ol_API ol_Alloc ol_getallocf (ol_State *L, void **ud) {
  ol_Alloc f;
  ol_lock(L);
  if (ud) *ud = G(L)->ud;
  f = G(L)->frealloc;
  ol_unlock(L);
  return f;
}


ol_API void ol_setallocf (ol_State *L, ol_Alloc f, void *ud) {
  ol_lock(L);
  G(L)->ud = ud;
  G(L)->frealloc = f;
  ol_unlock(L);
}


ol_API void *ol_newuserdata (ol_State *L, size_t size) {
  Udata *u;
  ol_lock(L);
  olC_checkGC(L);
  u = olS_newudata(L, size, getcurrenv(L));
  setuvalue(L, L->top, u);
  api_incr_top(L);
  ol_unlock(L);
  return u + 1;
}




static const char *aux_upvalue (StkId fi, int n, TValue **val) {
  Closure *f;
  if (!ttisfunction(fi)) return NULL;
  f = clvalue(fi);
  if (f->c.isC) {
    if (!(1 <= n && n <= f->c.nupvalues)) return NULL;
    *val = &f->c.upvalue[n-1];
    return "";
  }
  else {
    Proto *p = f->l.p;
    if (!(1 <= n && n <= p->sizeupvalues)) return NULL;
    *val = f->l.upvals[n-1]->v;
    return getstr(p->upvalues[n-1]);
  }
}


ol_API const char *ol_getupvalue (ol_State *L, int funcindex, int n) {
  const char *name;
  TValue *val;
  ol_lock(L);
  name = aux_upvalue(index2adr(L, funcindex), n, &val);
  if (name) {
    setobj2s(L, L->top, val);
    api_incr_top(L);
  }
  ol_unlock(L);
  return name;
}


ol_API const char *ol_setupvalue (ol_State *L, int funcindex, int n) {
  const char *name;
  TValue *val;
  StkId fi;
  ol_lock(L);
  fi = index2adr(L, funcindex);
  api_checknelems(L, 1);
  name = aux_upvalue(fi, n, &val);
  if (name) {
    L->top--;
    setobj(L, val, L->top);
    olC_barrier(L, clvalue(fi), L->top);
  }
  ol_unlock(L);
  return name;
}


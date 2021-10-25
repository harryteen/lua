/*
** $Id: lbaselib.c,v 1.191.1.6 2008/02/14 16:46:22 roberto Exp $
** Basic library
** See Copyright Notice in ol.h
*/



#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define lbaselib_c
#define ol_LIB

#include "ol.h"

#include "oauxlib.h"
#include "ollib.h"




/*
** If your system does not support `stdout', you can just remove this function.
** If you need, you can define your own `print' function, following this
** model but changing `fputs' to put the strings at a proper place
** (a console window or a log file, for instance).
*/
static int olB_print (ol_State *L) {
  int n = ol_gettop(L);  /* number of arguments */
  int i;
  ol_getglobal(L, "tostring");
  for (i=1; i<=n; i++) {
    const char *s;
    ol_pushvalue(L, -1);  /* function to be called */
    ol_pushvalue(L, i);   /* value to print */
    ol_call(L, 1, 1);
    s = ol_tostring(L, -1);  /* get result */
    if (s == NULL)
      return olL_error(L, ol_QL("tostring") " must return a string to "
                           ol_QL("print"));
    if (i>1) fputs("\t", stdout);
    fputs(s, stdout);
    ol_pop(L, 1);  /* pop result */
  }
  fputs("\n", stdout);
  return 0;
}


static int olB_tonumber (ol_State *L) {
  int base = olL_optint(L, 2, 10);
  if (base == 10) {  /* standard conversion */
    olL_checkany(L, 1);
    if (ol_isnumber(L, 1)) {
      ol_pushnumber(L, ol_tonumber(L, 1));
      return 1;
    }
  }
  else {
    const char *s1 = olL_checkstring(L, 1);
    char *s2;
    unsigned long n;
    olL_argcheck(L, 2 <= base && base <= 36, 2, "base out of range");
    n = strtoul(s1, &s2, base);
    if (s1 != s2) {  /* at least one valid digit? */
      while (isspace((unsigned char)(*s2))) s2++;  /* skip trailing spaces */
      if (*s2 == '\0') {  /* no invalid trailing characters? */
        ol_pushnumber(L, (ol_Number)n);
        return 1;
      }
    }
  }
  ol_pushnil(L);  /* else not a number */
  return 1;
}


static int olB_error (ol_State *L) {
  int level = olL_optint(L, 2, 1);
  ol_settop(L, 1);
  if (ol_isstring(L, 1) && level > 0) {  /* add extra information? */
    olL_where(L, level);
    ol_pushvalue(L, 1);
    ol_concat(L, 2);
  }
  return ol_error(L);
}


static int olB_getmetatable (ol_State *L) {
  olL_checkany(L, 1);
  if (!ol_getmetatable(L, 1)) {
    ol_pushnil(L);
    return 1;  /* no metatable */
  }
  olL_getmetafield(L, 1, "__metatable");
  return 1;  /* returns either __metatable field (if present) or metatable */
}


static int olB_setmetatable (ol_State *L) {
  int t = ol_type(L, 2);
  olL_checktype(L, 1, ol_TTABLE);
  olL_argcheck(L, t == ol_TNIL || t == ol_TTABLE, 2,
                    "nil or table expected");
  if (olL_getmetafield(L, 1, "__metatable"))
    olL_error(L, "OLang.MetaTableError:  cannot change a protected metatable");
  ol_settop(L, 2);
  ol_setmetatable(L, 1);
  return 1;
}


static void getfunc (ol_State *L, int opt) {
  if (ol_isfunction(L, 1)) ol_pushvalue(L, 1);
  else {
    ol_Debug ar;
    int level = opt ? olL_optint(L, 1, 1) : olL_checkint(L, 1);
    olL_argcheck(L, level >= 0, 1, "level must be non-negative");
    if (ol_getstack(L, level, &ar) == 0)
      olL_argerror(L, 1, "OLang.LevelError:  invalid level");
    ol_getinfo(L, "f", &ar);
    if (ol_isnil(L, -1))
      olL_error(L, "OLang.FunctionError:  no function environment for tail call at level %d",
                    level);
  }
}


static int olB_getfenv (ol_State *L) {
  getfunc(L, 1);
  if (ol_iscfunction(L, -1))  /* is a C function? */
    ol_pushvalue(L, ol_GLOBALSINDEX);  /* return the thread's global env. */
  else
    ol_getfenv(L, -1);
  return 1;
}


static int olB_setfenv (ol_State *L) {
  olL_checktype(L, 2, ol_TTABLE);
  getfunc(L, 0);
  ol_pushvalue(L, 2);
  if (ol_isnumber(L, 1) && ol_tonumber(L, 1) == 0) {
    /* change environment of current thread */
    ol_pushthread(L);
    ol_insert(L, -2);
    ol_setfenv(L, -2);
    return 0;
  }
  else if (ol_iscfunction(L, -2) || ol_setfenv(L, -2) == 0)
    olL_error(L,
          ol_QL("setfenv") "OLang.EnvironmentError:   cannot change environment of given object");
  return 1;
}


static int olB_rawequal (ol_State *L) {
  olL_checkany(L, 1);
  olL_checkany(L, 2);
  ol_pushboolean(L, ol_rawequal(L, 1, 2));
  return 1;
}


static int olB_rawget (ol_State *L) {
  olL_checktype(L, 1, ol_TTABLE);
  olL_checkany(L, 2);
  ol_settop(L, 2);
  ol_rawget(L, 1);
  return 1;
}

static int olB_rawset (ol_State *L) {
  olL_checktype(L, 1, ol_TTABLE);
  olL_checkany(L, 2);
  olL_checkany(L, 3);
  ol_settop(L, 3);
  ol_rawset(L, 1);
  return 1;
}


static int olB_gcinfo (ol_State *L) {
  ol_pushinteger(L, ol_getgccount(L));
  return 1;
}


static int olB_collectgarbage (ol_State *L) {
  static const char *const opts[] = {"stop", "restart", "collect",
    "count", "step", "setpause", "setstepmul", NULL};
  static const int optsnum[] = {ol_GCSTOP, ol_GCRESTART, ol_GCCOLLECT,
    ol_GCCOUNT, ol_GCSTEP, ol_GCSETPAUSE, ol_GCSETSTEPMUL};
  int o = olL_checkoption(L, 1, "collect", opts);
  int ex = olL_optint(L, 2, 0);
  int res = ol_gc(L, optsnum[o], ex);
  switch (optsnum[o]) {
    case ol_GCCOUNT: {
      int b = ol_gc(L, ol_GCCOUNTB, 0);
      ol_pushnumber(L, res + ((ol_Number)b/1024));
      return 1;
    }
    case ol_GCSTEP: {
      ol_pushboolean(L, res);
      return 1;
    }
    default: {
      ol_pushnumber(L, res);
      return 1;
    }
  }
}


static int olB_type (ol_State *L) {
  olL_checkany(L, 1);
  ol_pushstring(L, olL_typename(L, 1));
  return 1;
}


static int olB_next (ol_State *L) {
  olL_checktype(L, 1, ol_TTABLE);
  ol_settop(L, 2);  /* create a 2nd argument if there isn't one */
  if (ol_next(L, 1))
    return 2;
  else {
    ol_pushnil(L);
    return 1;
  }
}


static int olB_pairs (ol_State *L) {
  olL_checktype(L, 1, ol_TTABLE);
  ol_pushvalue(L, ol_upvalueindex(1));  /* return generator, */
  ol_pushvalue(L, 1);  /* state, */
  ol_pushnil(L);  /* and initial value */
  return 3;
}


static int ipairsaux (ol_State *L) {
  int i = olL_checkint(L, 2);
  olL_checktype(L, 1, ol_TTABLE);
  i++;  /* next value */
  ol_pushinteger(L, i);
  ol_rawgeti(L, 1, i);
  return (ol_isnil(L, -1)) ? 0 : 2;
}


static int olB_ipairs (ol_State *L) {
  olL_checktype(L, 1, ol_TTABLE);
  ol_pushvalue(L, ol_upvalueindex(1));  /* return generator, */
  ol_pushvalue(L, 1);  /* state, */
  ol_pushinteger(L, 0);  /* and initial value */
  return 3;
}


static int load_aux (ol_State *L, int status) {
  if (status == 0)  /* OK? */
    return 1;
  else {
    ol_pushnil(L);
    ol_insert(L, -2);  /* put before error message */
    return 2;  /* return nil plus error message */
  }
}


static int olB_loadstring (ol_State *L) {
  size_t l;
  const char *s = olL_checklstring(L, 1, &l);
  const char *chunkname = olL_optstring(L, 2, s);
  return load_aux(L, olL_loadbuffer(L, s, l, chunkname));
}


static int olB_loadfile (ol_State *L) {
  char *fname = olL_optstring(L, 1, NULL);
  return load_aux(L, olL_loadfile(L, fname));
}


/*
** Reader for generic `load' function: `ol_load' uses the
** stack for internal stuff, so the reader cannot change the
** stack top. Instead, it keeps its resulting string in a
** reserved slot inside the stack.
*/
static const char *generic_reader (ol_State *L, void *ud, size_t *size) {
  (void)ud;  /* to avoid warnings */
  olL_checkstack(L, 2, "too many nested functions");
  ol_pushvalue(L, 1);  /* get function */
  ol_call(L, 0, 1);  /* call it */
  if (ol_isnil(L, -1)) {
    *size = 0;
    return NULL;
  }
  else if (ol_isstring(L, -1)) {
    ol_replace(L, 3);  /* save string in a reserved stack slot */
    return ol_tolstring(L, 3, size);
  }
  else olL_error(L, "OLang.ReaderError:  reader function must return a string");
  return NULL;  /* to avoid warnings */
}


static int olB_load (ol_State *L) {
  int status;
  const char *cname = olL_optstring(L, 2, "=(load)");
  olL_checktype(L, 1, ol_TFUNCTION);
  ol_settop(L, 3);  /* function, eventual name, plus one reserved slot */
  status = ol_load(L, generic_reader, NULL, cname);
  return load_aux(L, status);
}


static int olB_dofile (ol_State *L) {
  *fname = olL_optstring(L, 1, NULL);
  int n = ol_gettop(L);
  if (olL_loadfile(L, fname) != 0) ol_error(L);
  ol_call(L, 0, ol_MULTRET);
  return ol_gettop(L) - n;
}


static int olB_assert (ol_State *L) {
  olL_checkany(L, 1);
  if (!ol_toboolean(L, 1))
    return olL_error(L, "%s", olL_optstring(L, 2, "assertion failed!"));
  return ol_gettop(L);
}


static int olB_unpack (ol_State *L) {
  int i, e, n;
  olL_checktype(L, 1, ol_TTABLE);
  i = olL_optint(L, 2, 1);
  e = olL_opt(L, olL_checkint, 3, olL_getn(L, 1));
  if (i > e) return 0;  /* empty range */
  n = e - i + 1;  /* number of elements */
  if (n <= 0 || !ol_checkstack(L, n))  /* n <= 0 means arith. overflow */
    return olL_error(L, "OLang.UnpackError:  too many results to unpack");
  ol_rawgeti(L, 1, i);  /* push arg[i] (avoiding overflow problems) */
  while (i++ < e)  /* push arg[i + 1...e] */
    ol_rawgeti(L, 1, i);
  return n;
}


static int olB_select (ol_State *L) {
  int n = ol_gettop(L);
  if (ol_type(L, 1) == ol_TSTRING && *ol_tostring(L, 1) == '#') {
    ol_pushinteger(L, n-1);
    return 1;
  }
  else {
    int i = olL_checkint(L, 1);
    if (i < 0) i = n + i;
    else if (i > n) i = n;
    olL_argcheck(L, 1 <= i, 1, "OLang.IndexError:  index out of range");
    return n - i;
  }
}


static int olB_pcall (ol_State *L) {
  int status;
  olL_checkany(L, 1);
  status = ol_pcall(L, ol_gettop(L) - 1, ol_MULTRET, 0);
  ol_pushboolean(L, (status == 0));
  ol_insert(L, 1);
  return ol_gettop(L);  /* return status + all results */
}


static int olB_xpcall (ol_State *L) {
  int status;
  olL_checkany(L, 2);
  ol_settop(L, 2);
  ol_insert(L, 1);  /* put error function under function to be called */
  status = ol_pcall(L, 0, ol_MULTRET, 1);
  ol_pushboolean(L, (status == 0));
  ol_replace(L, 1);
  return ol_gettop(L);  /* return status + all results */
}


static int olB_tostring (ol_State *L) {
  olL_checkany(L, 1);
  if (olL_callmeta(L, 1, "__tostring"))  /* is there a metafield? */
    return 1;  /* use its value */
  switch (ol_type(L, 1)) {
    case ol_TNUMBER:
      ol_pushstring(L, ol_tostring(L, 1));
      break;
    case ol_TSTRING:
      ol_pushvalue(L, 1);
      break;
    case ol_TBOOLEAN:
      ol_pushstring(L, (ol_toboolean(L, 1) ? "true" : "false"));
      break;
    case ol_TNIL:
      ol_pushliteral(L, "nil");
      break;
    default:
      ol_pushfstring(L, "%s: %p", olL_typename(L, 1), ol_topointer(L, 1));
      break;
  }
  return 1;
}


static int olB_newproxy (ol_State *L) {
  ol_settop(L, 1);
  ol_newuserdata(L, 0);  /* create proxy */
  if (ol_toboolean(L, 1) == 0)
    return 1;  /* no metatable */
  else if (ol_isboolean(L, 1)) {
    ol_newtable(L);  /* create a new metatable `m' ... */
    ol_pushvalue(L, -1);  /* ... and mark `m' as a valid metatable */
    ol_pushboolean(L, 1);
    ol_rawset(L, ol_upvalueindex(1));  /* weaktable[m] = true */
  }
  else {
    int validproxy = 0;  /* to check if weaktable[metatable(u)] == true */
    if (ol_getmetatable(L, 1)) {
      ol_rawget(L, ol_upvalueindex(1));
      validproxy = ol_toboolean(L, -1);
      ol_pop(L, 1);  /* remove value */
    }
    olL_argcheck(L, validproxy, 1, "OLang.ProxyError:  boolean or proxy expected");
    ol_getmetatable(L, 1);  /* metatable is valid; get it */
  }
  ol_setmetatable(L, 2);
  return 1;
}


static const olL_Reg base_funcs[] = {
  {"assert", olB_assert},
  {"collectgarbage", olB_collectgarbage},
  {"dofile", olB_dofile},
  {"error", olB_error},
  {"gcinfo", olB_gcinfo},
  {"getfenv", olB_getfenv},
  {"getmetatable", olB_getmetatable},
  {"loadfile", olB_loadfile},
  {"load", olB_load},
  {"loadstring", olB_loadstring},
  {"next", olB_next},
  {"pcall", olB_pcall},
  {"print", olB_print},
  {"rawequal", olB_rawequal},
  {"rawget", olB_rawget},
  {"rawset", olB_rawset},
  {"select", olB_select},
  {"setfenv", olB_setfenv},
  {"setmetatable", olB_setmetatable},
  {"tonumber", olB_tonumber},
  {"tostring", olB_tostring},
  {"type", olB_type},
  {"unpack", olB_unpack},
  {"xpcall", olB_xpcall},
  {NULL, NULL}
};


/*
** {======================================================
** Coroutine library
** =======================================================
*/

#define CO_RUN	0	/* running */
#define CO_SUS	1	/* suspended */
#define CO_NOR	2	/* 'normal' (it resumed another coroutine) */
#define CO_DEAD	3

static const char *const statnames[] =
    {"running", "suspended", "normal", "dead"};

static int costatus (ol_State *L, ol_State *co) {
  if (L == co) return CO_RUN;
  switch (ol_status(co)) {
    case ol_YIELD:
      return CO_SUS;
    case 0: {
      ol_Debug ar;
      if (ol_getstack(co, 0, &ar) > 0)  /* does it have frames? */
        return CO_NOR;  /* it is running */
      else if (ol_gettop(co) == 0)
          return CO_DEAD;
      else
        return CO_SUS;  /* initial state */
    }
    default:  /* some error occured */
      return CO_DEAD;
  }
}


static int olB_costatus (ol_State *L) {
  ol_State *co = ol_tothread(L, 1);
  olL_argcheck(L, co, 1, "coroutine expected");
  ol_pushstring(L, statnames[costatus(L, co)]);
  return 1;
}


static int auxresume (ol_State *L, ol_State *co, int narg) {
  int status = costatus(L, co);
  if (!ol_checkstack(co, narg))
    olL_error(L, "too many arguments to resume");
  if (status != CO_SUS) {
    ol_pushfstring(L, "OLang.ResumeError:  cannot resume %s coroutine", statnames[status]);
    return -1;  /* error flag */
  }
  ol_xmove(L, co, narg);
  ol_setlevel(L, co);
  status = ol_resume(co, narg);
  if (status == 0 || status == ol_YIELD) {
    int nres = ol_gettop(co);
    if (!ol_checkstack(L, nres + 1))
      olL_error(L, "OLang.ResumeError:  too many results to resume");
    ol_xmove(co, L, nres);  /* move yielded values */
    return nres;
  }
  else {
    ol_xmove(co, L, 1);  /* move error message */
    return -1;  /* error flag */
  }
}


static int olB_coresume (ol_State *L) {
  ol_State *co = ol_tothread(L, 1);
  int r;
  olL_argcheck(L, co, 1, "coroutine expected");
  r = auxresume(L, co, ol_gettop(L) - 1);
  if (r < 0) {
    ol_pushboolean(L, 0);
    ol_insert(L, -2);
    return 2;  /* return false + error message */
  }
  else {
    ol_pushboolean(L, 1);
    ol_insert(L, -(r + 1));
    return r + 1;  /* return true + `resume' returns */
  }
}


static int olB_auxwrap (ol_State *L) {
  ol_State *co = ol_tothread(L, ol_upvalueindex(1));
  int r = auxresume(L, co, ol_gettop(L));
  if (r < 0) {
    if (ol_isstring(L, -1)) {  /* error object is a string? */
      olL_where(L, 1);  /* add extra info */
      ol_insert(L, -2);
      ol_concat(L, 2);
    }
    ol_error(L);  /* propagate error */
  }
  return r;
}


static int olB_cocreate (ol_State *L) {
  ol_State *NL = ol_newthread(L);
  olL_argcheck(L, ol_isfunction(L, 1) && !ol_iscfunction(L, 1), 1,
    "ol function expected");
  ol_pushvalue(L, 1);  /* move function to top */
  ol_xmove(L, NL, 1);  /* move function from L to NL */
  return 1;
}


static int olB_cowrap (ol_State *L) {
  olB_cocreate(L);
  ol_pushcclosure(L, olB_auxwrap, 1);
  return 1;
}


static int olB_yield (ol_State *L) {
  return ol_yield(L, ol_gettop(L));
}


static int olB_corunning (ol_State *L) {
  if (ol_pushthread(L))
    ol_pushnil(L);  /* main thread is not a coroutine */
  return 1;
}


static const olL_Reg co_funcs[] = {
  {"create", olB_cocreate},
  {"resume", olB_coresume},
  {"running", olB_corunning},
  {"status", olB_costatus},
  {"wrap", olB_cowrap},
  {"yield", olB_yield},
  {NULL, NULL}
};

/* }====================================================== */


static void auxopen (ol_State *L, const char *name,
                     ol_CFunction f, ol_CFunction u) {
  ol_pushcfunction(L, u);
  ol_pushcclosure(L, f, 1);
  ol_setfield(L, -2, name);
}


static void base_open (ol_State *L) {
  /* set global _G */
  ol_pushvalue(L, ol_GLOBALSINDEX);
  ol_setglobal(L, "_G");
  /* open lib into global table */
  olL_register(L, "_G", base_funcs);
  ol_pushliteral(L, ol_VERSION);
  ol_setglobal(L, "_VERSION");  /* set global _VERSION */
  /* `ipairs' and `pairs' need auxiliary functions as upvalues */
  auxopen(L, "ipairs", olB_ipairs, ipairsaux);
  auxopen(L, "pairs", olB_pairs, olB_next);
  /* `newproxy' needs a weaktable as upvalue */
  ol_createtable(L, 0, 1);  /* new table `w' */
  ol_pushvalue(L, -1);  /* `w' will be its own metatable */
  ol_setmetatable(L, -2);
  ol_pushliteral(L, "kv");
  ol_setfield(L, -2, "__mode");  /* metatable(w).__mode = "kv" */
  ol_pushcclosure(L, olB_newproxy, 1);
  ol_setglobal(L, "newproxy");  /* set global `newproxy' */
}


olLIB_API int olopen_base (ol_State *L) {
  base_open(L);
  olL_register(L, ol_COLIBNAME, co_funcs);
  return 2;
}


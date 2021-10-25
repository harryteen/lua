/*
** $Id: ldblib.c,v 1.104.1.4 2009/08/04 18:50:18 roberto Exp $
** Interface from ol to its debug API
** See Copyright Notice in ol.h
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ldblib_c
#define ol_LIB

#include "ol.h"

#include "olauxlib.h"
#include "ollib.h"



static int db_getregistry (ol_State *L) {
  ol_pushvalue(L, ol_REGISTRYINDEX);
  return 1;
}


static int db_getmetatable (ol_State *L) {
  olL_checkany(L, 1);
  if (!ol_getmetatable(L, 1)) {
    ol_pushnil(L);  /* no metatable */
  }
  return 1;
}


static int db_setmetatable (ol_State *L) {
  int t = ol_type(L, 2);
  olL_argcheck(L, t == ol_TNIL || t == ol_TTABLE, 2,
                    "OLang.TableError:  nil or table expected");
  ol_settop(L, 2);
  ol_pushboolean(L, ol_setmetatable(L, 1));
  return 1;
}


static int db_getfenv (ol_State *L) {
  olL_checkany(L, 1);
  ol_getfenv(L, 1);
  return 1;
}


static int db_setfenv (ol_State *L) {
  olL_checktype(L, 2, ol_TTABLE);
  ol_settop(L, 2);
  if (ol_setfenv(L, 1) == 0)
    olL_error(L, ol_QL("setfenv")
                  " OLang.TableError:  cannot change environment of given object");
  return 1;
}


static void settabss (ol_State *L, const char *i, const char *v) {
  ol_pushstring(L, v);
  ol_setfield(L, -2, i);
}


static void settabsi (ol_State *L, const char *i, int v) {
  ol_pushinteger(L, v);
  ol_setfield(L, -2, i);
}


static ol_State *getthread (ol_State *L, int *arg) {
  if (ol_isthread(L, 1)) {
    *arg = 1;
    return ol_tothread(L, 1);
  }
  else {
    *arg = 0;
    return L;
  }
}


static void treatstackoption (ol_State *L, ol_State *L1, const char *fname) {
  if (L == L1) {
    ol_pushvalue(L, -2);
    ol_remove(L, -3);
  }
  else
    ol_xmove(L1, L, 1);
  ol_setfield(L, -2, fname);
}


static int db_getinfo (ol_State *L) {
  ol_Debug ar;
  int arg;
  ol_State *L1 = getthread(L, &arg);
  const char *options = olL_optstring(L, arg+2, "flnSu");
  if (ol_isnumber(L, arg+1)) {
    if (!ol_getstack(L1, (int)ol_tointeger(L, arg+1), &ar)) {
      ol_pushnil(L);  /* level out of range */
      return 1;
    }
  }
  else if (ol_isfunction(L, arg+1)) {
    ol_pushfstring(L, ">%s", options);
    options = ol_tostring(L, -1);
    ol_pushvalue(L, arg+1);
    ol_xmove(L, L1, 1);
  }
  else
    return olL_argerror(L, arg+1, "OLang.ArgeError:  function or level expected");
  if (!ol_getinfo(L1, options, &ar))
    return olL_argerror(L, arg+2, "OLang.OptionError:  invalid option");
  ol_createtable(L, 0, 2);
  if (strchr(options, 'S')) {
    settabss(L, "source", ar.source);
    settabss(L, "short_src", ar.short_src);
    settabsi(L, "linedefined", ar.linedefined);
    settabsi(L, "lastlinedefined", ar.lastlinedefined);
    settabss(L, "what", ar.what);
  }
  if (strchr(options, 'l'))
    settabsi(L, "currentline", ar.currentline);
  if (strchr(options, 'u'))
    settabsi(L, "nups", ar.nups);
  if (strchr(options, 'n')) {
    settabss(L, "name", ar.name);
    settabss(L, "namewhat", ar.namewhat);
  }
  if (strchr(options, 'L'))
    treatstackoption(L, L1, "activelines");
  if (strchr(options, 'f'))
    treatstackoption(L, L1, "func");
  return 1;  /* return table */
}
    

static int db_getlocal (ol_State *L) {
  int arg;
  ol_State *L1 = getthread(L, &arg);
  ol_Debug ar;
  const char *name;
  if (!ol_getstack(L1, olL_checkint(L, arg+1), &ar))  /* out of range? */
    return olL_argerror(L, arg+1, "OLang.LevelError:  level out of range");
  name = ol_getlocal(L1, &ar, olL_checkint(L, arg+2));
  if (name) {
    ol_xmove(L1, L, 1);
    ol_pushstring(L, name);
    ol_pushvalue(L, -2);
    return 2;
  }
  else {
    ol_pushnil(L);
    return 1;
  }
}


static int db_setlocal (ol_State *L) {
  int arg;
  ol_State *L1 = getthread(L, &arg);
  ol_Debug ar;
  if (!ol_getstack(L1, olL_checkint(L, arg+1), &ar))  /* out of range? */
    return olL_argerror(L, arg+1, "OLang.LevelError:  level out of range");
  olL_checkany(L, arg+3);
  ol_settop(L, arg+3);
  ol_xmove(L, L1, 1);
  ol_pushstring(L, ol_setlocal(L1, &ar, olL_checkint(L, arg+2)));
  return 1;
}


static int auxupvalue (ol_State *L, int get) {
  const char *name;
  int n = olL_checkint(L, 2);
  olL_checktype(L, 1, ol_TFUNCTION);
  if (ol_iscfunction(L, 1)) return 0;  /* cannot touch C upvalues from ol */
  name = get ? ol_getupvalue(L, 1, n) : ol_setupvalue(L, 1, n);
  if (name == NULL) return 0;
  ol_pushstring(L, name);
  ol_insert(L, -(get+1));
  return get + 1;
}


static int db_getupvalue (ol_State *L) {
  return auxupvalue(L, 1);
}


static int db_setupvalue (ol_State *L) {
  olL_checkany(L, 3);
  return auxupvalue(L, 0);
}



static const char KEY_HOOK = 'h';


static void hookf (ol_State *L, ol_Debug *ar) {
  static const char *const hooknames[] =
    {"call", "return", "line", "count", "tail return"};
  ol_pushlightuserdata(L, (void *)&KEY_HOOK);
  ol_rawget(L, ol_REGISTRYINDEX);
  ol_pushlightuserdata(L, L);
  ol_rawget(L, -2);
  if (ol_isfunction(L, -1)) {
    ol_pushstring(L, hooknames[(int)ar->event]);
    if (ar->currentline >= 0)
      ol_pushinteger(L, ar->currentline);
    else ol_pushnil(L);
    ol_assert(ol_getinfo(L, "lS", ar));
    ol_call(L, 2, 0);
  }
}


static int makemask (const char *smask, int count) {
  int mask = 0;
  if (strchr(smask, 'c')) mask |= ol_MASKCALL;
  if (strchr(smask, 'r')) mask |= ol_MASKRET;
  if (strchr(smask, 'l')) mask |= ol_MASKLINE;
  if (count > 0) mask |= ol_MASKCOUNT;
  return mask;
}


static char *unmakemask (int mask, char *smask) {
  int i = 0;
  if (mask & ol_MASKCALL) smask[i++] = 'c';
  if (mask & ol_MASKRET) smask[i++] = 'r';
  if (mask & ol_MASKLINE) smask[i++] = 'l';
  smask[i] = '\0';
  return smask;
}


static void gethooktable (ol_State *L) {
  ol_pushlightuserdata(L, (void *)&KEY_HOOK);
  ol_rawget(L, ol_REGISTRYINDEX);
  if (!ol_istable(L, -1)) {
    ol_pop(L, 1);
    ol_createtable(L, 0, 1);
    ol_pushlightuserdata(L, (void *)&KEY_HOOK);
    ol_pushvalue(L, -2);
    ol_rawset(L, ol_REGISTRYINDEX);
  }
}


static int db_sethook (ol_State *L) {
  int arg, mask, count;
  ol_Hook func;
  ol_State *L1 = getthread(L, &arg);
  if (ol_isnoneornil(L, arg+1)) {
    ol_settop(L, arg+1);
    func = NULL; mask = 0; count = 0;  /* turn off hooks */
  }
  else {
    const char *smask = olL_checkstring(L, arg+2);
    olL_checktype(L, arg+1, ol_TFUNCTION);
    count = olL_optint(L, arg+3, 0);
    func = hookf; mask = makemask(smask, count);
  }
  gethooktable(L);
  ol_pushlightuserdata(L, L1);
  ol_pushvalue(L, arg+1);
  ol_rawset(L, -3);  /* set new hook */
  ol_pop(L, 1);  /* remove hook table */
  ol_sethook(L1, func, mask, count);  /* set hooks */
  return 0;
}


static int db_gethook (ol_State *L) {
  int arg;
  ol_State *L1 = getthread(L, &arg);
  char buff[5];
  int mask = ol_gethookmask(L1);
  ol_Hook hook = ol_gethook(L1);
  if (hook != NULL && hook != hookf)  /* external hook? */
    ol_pushliteral(L, "external hook");
  else {
    gethooktable(L);
    ol_pushlightuserdata(L, L1);
    ol_rawget(L, -2);   /* get hook */
    ol_remove(L, -2);  /* remove hook table */
  }
  ol_pushstring(L, unmakemask(mask, buff));
  ol_pushinteger(L, ol_gethookcount(L1));
  return 3;
}


static int db_debug (ol_State *L) {
  for (;;) {
    char buffer[250];
    fputs("ol_debug> ", stderr);
    if (fgets(buffer, sizeof(buffer), stdin) == 0 ||
        strcmp(buffer, "cont\n") == 0)
      return 0;
    if (olL_loadbuffer(L, buffer, strlen(buffer), "=(debug command)") ||
        ol_pcall(L, 0, 0, 0)) {
      fputs(ol_tostring(L, -1), stderr);
      fputs("\n", stderr);
    }
    ol_settop(L, 0);  /* remove eventual returns */
  }
}


#define LEVELS1	12	/* size of the first part of the stack */
#define LEVELS2	10	/* size of the second part of the stack */

static int db_errorfb (ol_State *L) {
  int level;
  int firstpart = 1;  /* still before eventual `...' */
  int arg;
  ol_State *L1 = getthread(L, &arg);
  ol_Debug ar;
  if (ol_isnumber(L, arg+2)) {
    level = (int)ol_tointeger(L, arg+2);
    ol_pop(L, 1);
  }
  else
    level = (L == L1) ? 1 : 0;  /* level 0 may be this own function */
  if (ol_gettop(L) == arg)
    ol_pushliteral(L, "");
  else if (!ol_isstring(L, arg+1)) return 1;  /* message is not a string */
  else ol_pushliteral(L, "\n");
  ol_pushliteral(L, "stack traceback:");
  while (ol_getstack(L1, level++, &ar)) {
    if (level > LEVELS1 && firstpart) {
      /* no more than `LEVELS2' more levels? */
      if (!ol_getstack(L1, level+LEVELS2, &ar))
        level--;  /* keep going */
      else {
        ol_pushliteral(L, "\n\t...");  /* too many levels */
        while (ol_getstack(L1, level+LEVELS2, &ar))  /* find last levels */
          level++;
      }
      firstpart = 0;
      continue;
    }
    ol_pushliteral(L, "\n\t");
    ol_getinfo(L1, "Snl", &ar);
    ol_pushfstring(L, "%s:", ar.short_src);
    if (ar.currentline > 0)
      ol_pushfstring(L, "%d:", ar.currentline);
    if (*ar.namewhat != '\0')  /* is there a name? */
        ol_pushfstring(L, " in function " ol_QS, ar.name);
    else {
      if (*ar.what == 'm')  /* main? */
        ol_pushfstring(L, " in main chunk");
      else if (*ar.what == 'C' || *ar.what == 't')
        ol_pushliteral(L, " ?");  /* C function or tail call */
      else
        ol_pushfstring(L, " in function <%s:%d>",
                           ar.short_src, ar.linedefined);
    }
    ol_concat(L, ol_gettop(L) - arg);
  }
  ol_concat(L, ol_gettop(L) - arg);
  return 1;
}


static const olL_Reg dblib[] = {
  {"debug", db_debug},
  {"getfenv", db_getfenv},
  {"gethook", db_gethook},
  {"getinfo", db_getinfo},
  {"getlocal", db_getlocal},
  {"getregistry", db_getregistry},
  {"getmetatable", db_getmetatable},
  {"getupvalue", db_getupvalue},
  {"setfenv", db_setfenv},
  {"sethook", db_sethook},
  {"setlocal", db_setlocal},
  {"setmetatable", db_setmetatable},
  {"setupvalue", db_setupvalue},
  {"traceback", db_errorfb},
  {NULL, NULL}
};


olLIB_API int olopen_debug (ol_State *L) {
  olL_register(L, ol_DBLIBNAME, dblib);
  return 1;
}


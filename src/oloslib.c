/*
** $Id: oloslib.c,v 1.19.1.3 2008/01/18 16:38:18 roberto Exp $
** Standard Operating System library
** See Copyright Notice in ol.h
*/


#include <errno.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define oloslib_c
#define ol_LIB

#include "ol.h"

#include "olauxlib.h"
#include "ollib.h"


static int os_pushresult (ol_State *L, int i, const char *filename) {
  int en = errno;  /* calls to ol API may change this value */
  if (i) {
    ol_pushboolean(L, 1);
    return 1;
  }
  else {
    ol_pushnil(L);
    ol_pushfstring(L, "%s: %s", filename, strerror(en));
    ol_pushinteger(L, en);
    return 3;
  }
}


static int os_execute (ol_State *L) {
  ol_pushinteger(L, system(olL_optstring(L, 1, NULL)));
  return 1;
}


static int os_remove (ol_State *L) {
  const char *filename = olL_checkstring(L, 1);
  return os_pushresult(L, remove(filename) == 0, filename);
}


static int os_rename (ol_State *L) {
  const char *fromname = olL_checkstring(L, 1);
  const char *toname = olL_checkstring(L, 2);
  return os_pushresult(L, rename(fromname, toname) == 0, fromname);
}


static int os_tmpname (ol_State *L) {
  char buff[ol_TMPNAMBUFSIZE];
  int err;
  ol_tmpnam(buff, err);
  if (err)
    return olL_error(L, "unable to generate a unique filename");
  ol_pushstring(L, buff);
  return 1;
}


static int os_getenv (ol_State *L) {
  ol_pushstring(L, getenv(olL_checkstring(L, 1)));  /* if NULL push nil */
  return 1;
}


static int os_clock (ol_State *L) {
  ol_pushnumber(L, ((ol_Number)clock())/(ol_Number)CLOCKS_PER_SEC);
  return 1;
}


/*
** {======================================================
** Time/Date operations
** { year=%Y, month=%m, day=%d, hour=%H, min=%M, sec=%S,
**   wday=%w+1, yday=%j, isdst=? }
** =======================================================
*/

static void setfield (ol_State *L, const char *key, int value) {
  ol_pushinteger(L, value);
  ol_setfield(L, -2, key);
}

static void setboolfield (ol_State *L, const char *key, int value) {
  if (value < 0)  /* undefined? */
    return;  /* does not set field */
  ol_pushboolean(L, value);
  ol_setfield(L, -2, key);
}

static int getboolfield (ol_State *L, const char *key) {
  int res;
  ol_getfield(L, -1, key);
  res = ol_isnil(L, -1) ? -1 : ol_toboolean(L, -1);
  ol_pop(L, 1);
  return res;
}


static int getfield (ol_State *L, const char *key, int d) {
  int res;
  ol_getfield(L, -1, key);
  if (ol_isnumber(L, -1))
    res = (int)ol_tointeger(L, -1);
  else {
    if (d < 0)
      return olL_error(L, "field " ol_QS " missing in date table", key);
    res = d;
  }
  ol_pop(L, 1);
  return res;
}


static int os_date (ol_State *L) {
  const char *s = olL_optstring(L, 1, "%c");
  time_t t = olL_opt(L, (time_t)olL_checknumber, 2, time(NULL));
  struct tm *stm;
  if (*s == '!') {  /* UTC? */
    stm = gmtime(&t);
    s++;  /* skip `!' */
  }
  else
    stm = localtime(&t);
  if (stm == NULL)  /* invalid date? */
    ol_pushnil(L);
  else if (strcmp(s, "*t") == 0) {
    ol_createtable(L, 0, 9);  /* 9 = number of fields */
    setfield(L, "sec", stm->tm_sec);
    setfield(L, "min", stm->tm_min);
    setfield(L, "hour", stm->tm_hour);
    setfield(L, "day", stm->tm_mday);
    setfield(L, "month", stm->tm_mon+1);
    setfield(L, "year", stm->tm_year+1900);
    setfield(L, "wday", stm->tm_wday+1);
    setfield(L, "yday", stm->tm_yday+1);
    setboolfield(L, "isdst", stm->tm_isdst);
  }
  else {
    char cc[3];
    olL_Buffer b;
    cc[0] = '%'; cc[2] = '\0';
    olL_buffinit(L, &b);
    for (; *s; s++) {
      if (*s != '%' || *(s + 1) == '\0')  /* no conversion specifier? */
        olL_addchar(&b, *s);
      else {
        size_t reslen;
        char buff[200];  /* should be big enough for any conversion result */
        cc[1] = *(++s);
        reslen = strftime(buff, sizeof(buff), cc, stm);
        olL_addlstring(&b, buff, reslen);
      }
    }
    olL_pushresult(&b);
  }
  return 1;
}


static int os_time (ol_State *L) {
  time_t t;
  if (ol_isnoneornil(L, 1))  /* called without args? */
    t = time(NULL);  /* get current time */
  else {
    struct tm ts;
    olL_checktype(L, 1, ol_TTABLE);
    ol_settop(L, 1);  /* make sure table is at the top */
    ts.tm_sec = getfield(L, "sec", 0);
    ts.tm_min = getfield(L, "min", 0);
    ts.tm_hour = getfield(L, "hour", 12);
    ts.tm_mday = getfield(L, "day", -1);
    ts.tm_mon = getfield(L, "month", -1) - 1;
    ts.tm_year = getfield(L, "year", -1) - 1900;
    ts.tm_isdst = getboolfield(L, "isdst");
    t = mktime(&ts);
  }
  if (t == (time_t)(-1))
    ol_pushnil(L);
  else
    ol_pushnumber(L, (ol_Number)t);
  return 1;
}


static int os_difftime (ol_State *L) {
  ol_pushnumber(L, difftime((time_t)(olL_checknumber(L, 1)),
                             (time_t)(olL_optnumber(L, 2, 0))));
  return 1;
}

/* }====================================================== */


static int os_setlocale (ol_State *L) {
  static const int cat[] = {LC_ALL, LC_COLLATE, LC_CTYPE, LC_MONETARY,
                      LC_NUMERIC, LC_TIME};
  static const char *const catnames[] = {"all", "collate", "ctype", "monetary",
     "numeric", "time", NULL};
  const char *l = olL_optstring(L, 1, NULL);
  int op = olL_checkoption(L, 2, "all", catnames);
  ol_pushstring(L, setlocale(cat[op], l));
  return 1;
}


static int os_exit (ol_State *L) {
  exit(olL_optint(L, 1, EXIT_SUCCESS));
}

static const olL_Reg syslib[] = {
  {"clock",     os_clock},
  {"date",      os_date},
  {"difftime",  os_difftime},
  {"execute",   os_execute},
  {"exit",      os_exit},
  {"getenv",    os_getenv},
  {"remove",    os_remove},
  {"rename",    os_rename},
  {"setlocale", os_setlocale},
  {"time",      os_time},
  {"tmpname",   os_tmpname},
  {NULL, NULL}
};

/* }====================================================== */



olLIB_API int olopen_os (ol_State *L) {
  olL_register(L, ol_OSLIBNAME, syslib);
  return 1;
}


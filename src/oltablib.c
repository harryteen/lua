/*
** $Id: oltablib.c,v 1.38.1.3 2008/02/14 16:46:58 roberto Exp $
** Library for Table Manipulation
** See Copyright Notice in ol.h
*/


#include <stddef.h>

#define ltablib_c
#define ol_LIB

#include "ol.h"

#include "oauxlib.h"
#include "ollib.h"


#define aux_getn(L,n)	(olL_checktype(L, n, ol_TTABLE), olL_getn(L, n))


static int foreachi (ol_State *L) {
  int i;
  int n = aux_getn(L, 1);
  olL_checktype(L, 2, ol_TFUNCTION);
  for (i=1; i <= n; i++) {
    ol_pushvalue(L, 2);  /* function */
    ol_pushinteger(L, i);  /* 1st argument */
    ol_rawgeti(L, 1, i);  /* 2nd argument */
    ol_call(L, 2, 1);
    if (!ol_isnil(L, -1))
      return 1;
    ol_pop(L, 1);  /* remove nil result */
  }
  return 0;
}


static int foreach (ol_State *L) {
  olL_checktype(L, 1, ol_TTABLE);
  olL_checktype(L, 2, ol_TFUNCTION);
  ol_pushnil(L);  /* first key */
  while (ol_next(L, 1)) {
    ol_pushvalue(L, 2);  /* function */
    ol_pushvalue(L, -3);  /* key */
    ol_pushvalue(L, -3);  /* value */
    ol_call(L, 2, 1);
    if (!ol_isnil(L, -1))
      return 1;
    ol_pop(L, 2);  /* remove value and result */
  }
  return 0;
}


static int maxn (ol_State *L) {
  ol_Number max = 0;
  olL_checktype(L, 1, ol_TTABLE);
  ol_pushnil(L);  /* first key */
  while (ol_next(L, 1)) {
    ol_pop(L, 1);  /* remove value */
    if (ol_type(L, -1) == ol_TNUMBER) {
      ol_Number v = ol_tonumber(L, -1);
      if (v > max) max = v;
    }
  }
  ol_pushnumber(L, max);
  return 1;
}


static int getn (ol_State *L) {
  ol_pushinteger(L, aux_getn(L, 1));
  return 1;
}


static int setn (ol_State *L) {
  olL_checktype(L, 1, ol_TTABLE);
#ifndef olL_setn
  olL_setn(L, 1, olL_checkint(L, 2));
#else
  olL_error(L, ol_QL("setn") " is obsolete");
#endif
  ol_pushvalue(L, 1);
  return 1;
}


static int tinsert (ol_State *L) {
  int e = aux_getn(L, 1) + 1;  /* first empty element */
  int pos;  /* where to insert new element */
  switch (ol_gettop(L)) {
    case 2: {  /* called with only 2 arguments */
      pos = e;  /* insert new element at the end */
      break;
    }
    case 3: {
      int i;
      pos = olL_checkint(L, 2);  /* 2nd argument is the position */
      if (pos > e) e = pos;  /* `grow' array if necessary */
      for (i = e; i > pos; i--) {  /* move up elements */
        ol_rawgeti(L, 1, i-1);
        ol_rawseti(L, 1, i);  /* t[i] = t[i-1] */
      }
      break;
    }
    default: {
      return olL_error(L, "wrong number of arguments to " ol_QL("insert"));
    }
  }
  olL_setn(L, 1, e);  /* new size */
  ol_rawseti(L, 1, pos);  /* t[pos] = v */
  return 0;
}


static int tremove (ol_State *L) {
  int e = aux_getn(L, 1);
  int pos = olL_optint(L, 2, e);
  if (!(1 <= pos && pos <= e))  /* position is outside bounds? */
   return 0;  /* nothing to remove */
  olL_setn(L, 1, e - 1);  /* t.n = n-1 */
  ol_rawgeti(L, 1, pos);  /* result = t[pos] */
  for ( ;pos<e; pos++) {
    ol_rawgeti(L, 1, pos+1);
    ol_rawseti(L, 1, pos);  /* t[pos] = t[pos+1] */
  }
  ol_pushnil(L);
  ol_rawseti(L, 1, e);  /* t[e] = nil */
  return 1;
}


static void addfield (ol_State *L, olL_Buffer *b, int i) {
  ol_rawgeti(L, 1, i);
  if (!ol_isstring(L, -1))
    olL_error(L, "invalid value (%s) at index %d in table for "
                  ol_QL("concat"), olL_typename(L, -1), i);
    olL_addvalue(b);
}


static int tconcat (ol_State *L) {
  olL_Buffer b;
  size_t lsep;
  int i, last;
  const char *sep = olL_optlstring(L, 2, "", &lsep);
  olL_checktype(L, 1, ol_TTABLE);
  i = olL_optint(L, 3, 1);
  last = olL_opt(L, olL_checkint, 4, olL_getn(L, 1));
  olL_buffinit(L, &b);
  for (; i < last; i++) {
    addfield(L, &b, i);
    olL_addlstring(&b, sep, lsep);
  }
  if (i == last)  /* add last value (if interval was not empty) */
    addfield(L, &b, i);
  olL_pushresult(&b);
  return 1;
}



/*
** {======================================================
** Quicksort
** (based on `Algorithms in MODULA-3', Robert Sedgewick;
**  Addison-Wesley, 1993.)
*/


static void set2 (ol_State *L, int i, int j) {
  ol_rawseti(L, 1, i);
  ol_rawseti(L, 1, j);
}

static int sort_comp (ol_State *L, int a, int b) {
  if (!ol_isnil(L, 2)) {  /* function? */
    int res;
    ol_pushvalue(L, 2);
    ol_pushvalue(L, a-1);  /* -1 to compensate function */
    ol_pushvalue(L, b-2);  /* -2 to compensate function and `a' */
    ol_call(L, 2, 1);
    res = ol_toboolean(L, -1);
    ol_pop(L, 1);
    return res;
  }
  else  /* a < b? */
    return ol_lessthan(L, a, b);
}

static void auxsort (ol_State *L, int l, int u) {
  while (l < u) {  /* for tail recursion */
    int i, j;
    /* sort elements a[l], a[(l+u)/2] and a[u] */
    ol_rawgeti(L, 1, l);
    ol_rawgeti(L, 1, u);
    if (sort_comp(L, -1, -2))  /* a[u] < a[l]? */
      set2(L, l, u);  /* swap a[l] - a[u] */
    else
      ol_pop(L, 2);
    if (u-l == 1) break;  /* only 2 elements */
    i = (l+u)/2;
    ol_rawgeti(L, 1, i);
    ol_rawgeti(L, 1, l);
    if (sort_comp(L, -2, -1))  /* a[i]<a[l]? */
      set2(L, i, l);
    else {
      ol_pop(L, 1);  /* remove a[l] */
      ol_rawgeti(L, 1, u);
      if (sort_comp(L, -1, -2))  /* a[u]<a[i]? */
        set2(L, i, u);
      else
        ol_pop(L, 2);
    }
    if (u-l == 2) break;  /* only 3 elements */
    ol_rawgeti(L, 1, i);  /* Pivot */
    ol_pushvalue(L, -1);
    ol_rawgeti(L, 1, u-1);
    set2(L, i, u-1);
    /* a[l] <= P == a[u-1] <= a[u], only need to sort from l+1 to u-2 */
    i = l; j = u-1;
    for (;;) {  /* invariant: a[l..i] <= P <= a[j..u] */
      /* repeat ++i until a[i] >= P */
      while (ol_rawgeti(L, 1, ++i), sort_comp(L, -1, -2)) {
        if (i>u) olL_error(L, "invalid order function for sorting");
        ol_pop(L, 1);  /* remove a[i] */
      }
      /* repeat --j until a[j] <= P */
      while (ol_rawgeti(L, 1, --j), sort_comp(L, -3, -1)) {
        if (j<l) olL_error(L, "invalid order function for sorting");
        ol_pop(L, 1);  /* remove a[j] */
      }
      if (j<i) {
        ol_pop(L, 3);  /* pop pivot, a[i], a[j] */
        break;
      }
      set2(L, i, j);
    }
    ol_rawgeti(L, 1, u-1);
    ol_rawgeti(L, 1, i);
    set2(L, u-1, i);  /* swap pivot (a[u-1]) with a[i] */
    /* a[l..i-1] <= a[i] == P <= a[i+1..u] */
    /* adjust so that smaller half is in [j..i] and larger one in [l..u] */
    if (i-l < u-i) {
      j=l; i=i-1; l=i+2;
    }
    else {
      j=i+1; i=u; u=j-2;
    }
    auxsort(L, j, i);  /* call recursively the smaller one */
  }  /* repeat the routine for the larger one */
}

static int sort (ol_State *L) {
  int n = aux_getn(L, 1);
  olL_checkstack(L, 40, "");  /* assume array is smaller than 2^40 */
  if (!ol_isnoneornil(L, 2))  /* is there a 2nd argument? */
    olL_checktype(L, 2, ol_TFUNCTION);
  ol_settop(L, 2);  /* make sure there is two arguments */
  auxsort(L, 1, n);
  return 0;
}

/* }====================================================== */


static const olL_Reg tab_funcs[] = {
  {"concat", tconcat},
  {"foreach", foreach},
  {"foreachi", foreachi},
  {"getn", getn},
  {"maxn", maxn},
  {"insert", tinsert},
  {"remove", tremove},
  {"setn", setn},
  {"sort", sort},
  {NULL, NULL}
};


olLIB_API int olopen_table (ol_State *L) {
  olL_register(L, ol_TABLIBNAME, tab_funcs);
  return 1;
}


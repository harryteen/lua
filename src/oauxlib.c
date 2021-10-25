/*
** $Id: lauxlib.c,v 1.159.1.3 2008/01/21 13:20:51 roberto Exp $
** Auxiliary functions for building ol libraries
** See Copyright Notice in ol.h
*/


#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* This file uses only the official API of ol.
** Any function declared here could be written as an application function.
*/

#define lauxlib_c
#define ol_LIB

#include "ol.h"

#include "oauxlib.h"


#define FREELIST_REF	0	/* free list of references */


/* convert a stack index to positive */
#define abs_index(L, i)		((i) > 0 || (i) <= ol_REGISTRYINDEX ? (i) : \
					ol_gettop(L) + (i) + 1)


/*
** {======================================================
** Error-report functions
** =======================================================
*/


olLIB_API int olL_argerror (ol_State *L, int narg, const char *extramsg) {
  ol_Debug ar;
  if (!ol_getstack(L, 0, &ar))  /* no stack frame? */
    return olL_error(L, "OLang.ArgumentError: bad argument #%d (%s)", narg, extramsg);
  ol_getinfo(L, "n", &ar);
  if (strcmp(ar.namewhat, "method") == 0) {
    narg--;  /* do not count `self' */
    if (narg == 0)  /* error is in the self argument itself? */
      return olL_error(L, "OLang.ArgumentError: calling " ol_QS " on bad self (%s)",
                           ar.name, extramsg);
  }
  if (ar.name == NULL)
    ar.name = "?";
  return olL_error(L, "OLang.ArgumentError: bad argument #%d to " ol_QS " (%s)",
                        narg, ar.name, extramsg);
}


olLIB_API int olL_typerror (ol_State *L, int narg, const char *tname) {
  const char *msg = ol_pushfstring(L, "OLang.ArgumentError: %s expected, got %s",
                                    tname, olL_typename(L, narg));
  return olL_argerror(L, narg, msg);
}


static void tag_error (ol_State *L, int narg, int tag) {
  olL_typerror(L, narg, ol_typename(L, tag));
}


olLIB_API void olL_where (ol_State *L, int level) {
  ol_Debug ar;
  if (ol_getstack(L, level, &ar)) {  /* check function at level */
    ol_getinfo(L, "Sl", &ar);  /* get info about it */
    if (ar.currentline > 0) {  /* is there info? */
      ol_pushfstring(L, "%s:%d: ", ar.short_src, ar.currentline);
      return;
    }
  }
  ol_pushliteral(L, "");  /* else, no information available... */
}


olLIB_API int olL_error (ol_State *L, const char *fmt, ...) {
  va_list argp;
  va_start(argp, fmt);
  olL_where(L, 1);
  ol_pushvfstring(L, fmt, argp);
  va_end(argp);
  ol_concat(L, 2);
  return ol_error(L);
}

/* }====================================================== */


olLIB_API int olL_checkoption (ol_State *L, int narg, const char *def,
                                 const char *const lst[]) {
  const char *name = (def) ? olL_optstring(L, narg, def) :
                             olL_checkstring(L, narg);
  int i;
  for (i=0; lst[i]; i++)
    if (strcmp(lst[i], name) == 0)
      return i;
  return olL_argerror(L, narg,
                       ol_pushfstring(L, "OLang.OptionError: invalid option " ol_QS, name));
}


olLIB_API int olL_newmetatable (ol_State *L, const char *tname) {
  ol_getfield(L, ol_REGISTRYINDEX, tname);  /* get registry.name */
  if (!ol_isnil(L, -1))  /* name already in use? */
    return 0;  /* leave previous value on top, but return 0 */
  ol_pop(L, 1);
  ol_newtable(L);  /* create metatable */
  ol_pushvalue(L, -1);
  ol_setfield(L, ol_REGISTRYINDEX, tname);  /* registry.name = metatable */
  return 1;
}


olLIB_API void *olL_checkudata (ol_State *L, int ud, const char *tname) {
  void *p = ol_touserdata(L, ud);
  if (p != NULL) {  /* value is a userdata? */
    if (ol_getmetatable(L, ud)) {  /* does it have a metatable? */
      ol_getfield(L, ol_REGISTRYINDEX, tname);  /* get correct metatable */
      if (ol_rawequal(L, -1, -2)) {  /* does it have the correct mt? */
        ol_pop(L, 2);  /* remove both metatables */
        return p;
      }
    }
  }
  olL_typerror(L, ud, tname);  /* else error */
  return NULL;  /* to avoid warnings */
}


olLIB_API void olL_checkstack (ol_State *L, int space, const char *mes) {
  if (!ol_checkstack(L, space))
    olL_error(L, "OLang.SOError: stack overflow (%s)", mes);
}


olLIB_API void olL_checktype (ol_State *L, int narg, int t) {
  if (ol_type(L, narg) != t)
    tag_error(L, narg, t);
}


olLIB_API void olL_checkany (ol_State *L, int narg) {
  if (ol_type(L, narg) == ol_TNONE)
    olL_argerror(L, narg, "OLang.ValueError: value expected");
}


olLIB_API const char *olL_checklstring (ol_State *L, int narg, size_t *len) {
  const char *s = ol_tolstring(L, narg, len);
  if (!s) tag_error(L, narg, ol_TSTRING);
  return s;
}


olLIB_API const char *olL_optlstring (ol_State *L, int narg,
                                        const char *def, size_t *len) {
  if (ol_isnoneornil(L, narg)) {
    if (len)
      *len = (def ? strlen(def) : 0);
    return def;
  }
  else return olL_checklstring(L, narg, len);
}


olLIB_API ol_Number olL_checknumber (ol_State *L, int narg) {
  ol_Number d = ol_tonumber(L, narg);
  if (d == 0 && !ol_isnumber(L, narg))  /* avoid extra test when d is not 0 */
    tag_error(L, narg, ol_TNUMBER);
  return d;
}


olLIB_API ol_Number olL_optnumber (ol_State *L, int narg, ol_Number def) {
  return olL_opt(L, olL_checknumber, narg, def);
}


olLIB_API ol_Integer olL_checkinteger (ol_State *L, int narg) {
  ol_Integer d = ol_tointeger(L, narg);
  if (d == 0 && !ol_isnumber(L, narg))  /* avoid extra test when d is not 0 */
    tag_error(L, narg, ol_TNUMBER);
  return d;
}


olLIB_API ol_Integer olL_optinteger (ol_State *L, int narg,
                                                      ol_Integer def) {
  return olL_opt(L, olL_checkinteger, narg, def);
}


olLIB_API int olL_getmetafield (ol_State *L, int obj, const char *event) {
  if (!ol_getmetatable(L, obj))  /* no metatable? */
    return 0;
  ol_pushstring(L, event);
  ol_rawget(L, -2);
  if (ol_isnil(L, -1)) {
    ol_pop(L, 2);  /* remove metatable and metafield */
    return 0;
  }
  else {
    ol_remove(L, -2);  /* remove only metatable */
    return 1;
  }
}


olLIB_API int olL_callmeta (ol_State *L, int obj, const char *event) {
  obj = abs_index(L, obj);
  if (!olL_getmetafield(L, obj, event))  /* no metafield? */
    return 0;
  ol_pushvalue(L, obj);
  ol_call(L, 1, 1);
  return 1;
}


olLIB_API void (olL_register) (ol_State *L, const char *libname,
                                const olL_Reg *l) {
  olI_openlib(L, libname, l, 0);
}


static int libsize (const olL_Reg *l) {
  int size = 0;
  for (; l->name; l++) size++;
  return size;
}


olLIB_API void olI_openlib (ol_State *L, const char *libname,
                              const olL_Reg *l, int nup) {
  if (libname) {
    int size = libsize(l);
    /* check whether lib already exists */
    olL_findtable(L, ol_REGISTRYINDEX, "_LOADED", 1);
    ol_getfield(L, -1, libname);  /* get _LOADED[libname] */
    if (!ol_istable(L, -1)) {  /* not found? */
      ol_pop(L, 1);  /* remove previous result */
      /* try global variable (and create one if it does not exist) */
      if (olL_findtable(L, ol_GLOBALSINDEX, libname, size) != NULL)
        olL_error(L, "OLang.OpenLibError: name conflict for module " ol_QS, libname);
      ol_pushvalue(L, -1);
      ol_setfield(L, -3, libname);  /* _LOADED[libname] = new table */
    }
    ol_remove(L, -2);  /* remove _LOADED table */
    ol_insert(L, -(nup+1));  /* move library table to below upvalues */
  }
  for (; l->name; l++) {
    int i;
    for (i=0; i<nup; i++)  /* copy upvalues to the top */
      ol_pushvalue(L, -nup);
    ol_pushcclosure(L, l->func, nup);
    ol_setfield(L, -(nup+2), l->name);
  }
  ol_pop(L, nup);  /* remove upvalues */
}



/*
** {======================================================
** getn-setn: size for arrays
** =======================================================
*/

#if defined(ol_COMPAT_GETN)

static int checkint (ol_State *L, int topop) {
  int n = (ol_type(L, -1) == ol_TNUMBER) ? ol_tointeger(L, -1) : -1;
  ol_pop(L, topop);
  return n;
}


static void getsizes (ol_State *L) {
  ol_getfield(L, ol_REGISTRYINDEX, "ol_SIZES");
  if (ol_isnil(L, -1)) {  /* no `size' table? */
    ol_pop(L, 1);  /* remove nil */
    ol_newtable(L);  /* create it */
    ol_pushvalue(L, -1);  /* `size' will be its own metatable */
    ol_setmetatable(L, -2);
    ol_pushliteral(L, "kv");
    ol_setfield(L, -2, "__mode");  /* metatable(N).__mode = "kv" */
    ol_pushvalue(L, -1);
    ol_setfield(L, ol_REGISTRYINDEX, "ol_SIZES");  /* store in register */
  }
}


olLIB_API void olL_setn (ol_State *L, int t, int n) {
  t = abs_index(L, t);
  ol_pushliteral(L, "n");
  ol_rawget(L, t);
  if (checkint(L, 1) >= 0) {  /* is there a numeric field `n'? */
    ol_pushliteral(L, "n");  /* use it */
    ol_pushinteger(L, n);
    ol_rawset(L, t);
  }
  else {  /* use `sizes' */
    getsizes(L);
    ol_pushvalue(L, t);
    ol_pushinteger(L, n);
    ol_rawset(L, -3);  /* sizes[t] = n */
    ol_pop(L, 1);  /* remove `sizes' */
  }
}


olLIB_API int olL_getn (ol_State *L, int t) {
  int n;
  t = abs_index(L, t);
  ol_pushliteral(L, "n");  /* try t.n */
  ol_rawget(L, t);
  if ((n = checkint(L, 1)) >= 0) return n;
  getsizes(L);  /* else try sizes[t] */
  ol_pushvalue(L, t);
  ol_rawget(L, -2);
  if ((n = checkint(L, 2)) >= 0) return n;
  return (int)ol_objlen(L, t);
}

#endif

/* }====================================================== */



olLIB_API const char *olL_gsub (ol_State *L, const char *s, const char *p,
                                                               const char *r) {
  const char *wild;
  size_t l = strlen(p);
  olL_Buffer b;
  olL_buffinit(L, &b);
  while ((wild = strstr(s, p)) != NULL) {
    olL_addlstring(&b, s, wild - s);  /* push prefix */
    olL_addstring(&b, r);  /* push replacement in place of pattern */
    s = wild + l;  /* continue after `p' */
  }
  olL_addstring(&b, s);  /* push last suffix */
  olL_pushresult(&b);
  return ol_tostring(L, -1);
}


olLIB_API const char *olL_findtable (ol_State *L, int idx,
                                       const char *fname, int szhint) {
  const char *e;
  ol_pushvalue(L, idx);
  do {
    e = strchr(fname, '.');
    if (e == NULL) e = fname + strlen(fname);
    ol_pushlstring(L, fname, e - fname);
    ol_rawget(L, -2);
    if (ol_isnil(L, -1)) {  /* no such field? */
      ol_pop(L, 1);  /* remove this nil */
      ol_createtable(L, 0, (*e == '.' ? 1 : szhint)); /* new table for field */
      ol_pushlstring(L, fname, e - fname);
      ol_pushvalue(L, -2);
      ol_settable(L, -4);  /* set new table into field */
    }
    else if (!ol_istable(L, -1)) {  /* field has a non-table value? */
      ol_pop(L, 2);  /* remove table and value */
      return fname;  /* return problematic part of the name */
    }
    ol_remove(L, -2);  /* remove previous table */
    fname = e + 1;
  } while (*e == '.');
  return NULL;
}



/*
** {======================================================
** Generic Buffer manipulation
** =======================================================
*/


#define bufflen(B)	((B)->p - (B)->buffer)
#define bufffree(B)	((size_t)(olL_BUFFERSIZE - bufflen(B)))

#define LIMIT	(ol_MINSTACK/2)


static int emptybuffer (olL_Buffer *B) {
  size_t l = bufflen(B);
  if (l == 0) return 0;  /* put nothing on stack */
  else {
    ol_pushlstring(B->L, B->buffer, l);
    B->p = B->buffer;
    B->lvl++;
    return 1;
  }
}


static void adjuststack (olL_Buffer *B) {
  if (B->lvl > 1) {
    ol_State *L = B->L;
    int toget = 1;  /* number of levels to concat */
    size_t toplen = ol_strlen(L, -1);
    do {
      size_t l = ol_strlen(L, -(toget+1));
      if (B->lvl - toget + 1 >= LIMIT || toplen > l) {
        toplen += l;
        toget++;
      }
      else break;
    } while (toget < B->lvl);
    ol_concat(L, toget);
    B->lvl = B->lvl - toget + 1;
  }
}


olLIB_API char *olL_prepbuffer (olL_Buffer *B) {
  if (emptybuffer(B))
    adjuststack(B);
  return B->buffer;
}


olLIB_API void olL_addlstring (olL_Buffer *B, const char *s, size_t l) {
  while (l--)
    olL_addchar(B, *s++);
}


olLIB_API void olL_addstring (olL_Buffer *B, const char *s) {
  olL_addlstring(B, s, strlen(s));
}


olLIB_API void olL_pushresult (olL_Buffer *B) {
  emptybuffer(B);
  ol_concat(B->L, B->lvl);
  B->lvl = 1;
}


olLIB_API void olL_addvalue (olL_Buffer *B) {
  ol_State *L = B->L;
  size_t vl;
  const char *s = ol_tolstring(L, -1, &vl);
  if (vl <= bufffree(B)) {  /* fit into buffer? */
    memcpy(B->p, s, vl);  /* put it there */
    B->p += vl;
    ol_pop(L, 1);  /* remove from stack */
  }
  else {
    if (emptybuffer(B))
      ol_insert(L, -2);  /* put buffer before new value */
    B->lvl++;  /* add new value into B stack */
    adjuststack(B);
  }
}


olLIB_API void olL_buffinit (ol_State *L, olL_Buffer *B) {
  B->L = L;
  B->p = B->buffer;
  B->lvl = 0;
}

/* }====================================================== */


olLIB_API int olL_ref (ol_State *L, int t) {
  int ref;
  t = abs_index(L, t);
  if (ol_isnil(L, -1)) {
    ol_pop(L, 1);  /* remove from stack */
    return ol_REFNIL;  /* `nil' has a unique fixed reference */
  }
  ol_rawgeti(L, t, FREELIST_REF);  /* get first free element */
  ref = (int)ol_tointeger(L, -1);  /* ref = t[FREELIST_REF] */
  ol_pop(L, 1);  /* remove it from stack */
  if (ref != 0) {  /* any free element? */
    ol_rawgeti(L, t, ref);  /* remove it from list */
    ol_rawseti(L, t, FREELIST_REF);  /* (t[FREELIST_REF] = t[ref]) */
  }
  else {  /* no free elements */
    ref = (int)ol_objlen(L, t);
    ref++;  /* create new reference */
  }
  ol_rawseti(L, t, ref);
  return ref;
}


olLIB_API void olL_unref (ol_State *L, int t, int ref) {
  if (ref >= 0) {
    t = abs_index(L, t);
    ol_rawgeti(L, t, FREELIST_REF);
    ol_rawseti(L, t, ref);  /* t[ref] = t[FREELIST_REF] */
    ol_pushinteger(L, ref);
    ol_rawseti(L, t, FREELIST_REF);  /* t[FREELIST_REF] = ref */
  }
}



/*
** {======================================================
** Load functions
** =======================================================
*/

typedef struct LoadF {
  int extraline;
  FILE *f;
  char buff[olL_BUFFERSIZE];
} LoadF;


static const char *getF (ol_State *L, void *ud, size_t *size) {
  LoadF *lf = (LoadF *)ud;
  (void)L;
  if (lf->extraline) {
    lf->extraline = 0;
    *size = 1;
    return "\n";
  }
  if (feof(lf->f)) return NULL;
  *size = fread(lf->buff, 1, sizeof(lf->buff), lf->f);
  return (*size > 0) ? lf->buff : NULL;
}


static int errfile (ol_State *L, const char *what, int fnameindex) {
  const char *serr = strerror(errno);
  const char *filename = ol_tostring(L, fnameindex) + 1;
  ol_pushfstring(L, "OLang.OpenFileError: cannot %s %s: %s", what, filename, serr);
  ol_remove(L, fnameindex);
  return ol_ERRFILE;
}


static char get_filename_ext(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if(!dot || dot == filename){
	    return NULL;
    }
    return dot + 1;
}


olLIB_API int olL_loadfile (ol_State *L, const char *filename) {
  LoadF lf;
  int status, readstatus;
  int c;
  int fnameindex = ol_gettop(L) + 1;  /* index of filename on the stack */
  lf.extraline = 0;
  if (filename == NULL) {
    ol_pushliteral(L, "=stdin");
    lf.f = stdin;
  }
  else {
    ol_pushfstring(L, "@%s", filename);
    if (!get_filename_ext(filename)){
      errfile(L,"OLang.FormatError: File Dosn't Has a Format",fnameindex);
    }
    else{
      char format = get_filename_ext(filename);
      if (format = "ol"){
        lf.f = fopen(filename, "r");
        if (lf.f == NULL) return errfile(L, "OLang.OpenFileError: open", fnameindex);
      }
      else{
        errfile(L,"OLang.FormatError: File Dosn't Has a Currect Format",fnameindex);
      }

    }
  }
  c = getc(lf.f);
  if (c == '#') {  /* Unix exec. file? */
    lf.extraline = 1;
    while ((c = getc(lf.f)) != EOF && c != '\n') ;  /* skip first line */
    if (c == '\n') c = getc(lf.f);
  }
  if (c == ol_SIGNATURE[0] && filename) {  /* binary file? */
    lf.f = freopen(filename, "rb", lf.f);  /* reopen in binary mode */
    if (lf.f == NULL) return errfile(L, "OLang.ReOpenFileError: reopen", fnameindex);
    /* skip eventual `#!...' */
   while ((c = getc(lf.f)) != EOF && c != ol_SIGNATURE[0]) ;
    lf.extraline = 0;
  }
  ungetc(c, lf.f);
  status = ol_load(L, getF, &lf, ol_tostring(L, -1));
  readstatus = ferror(lf.f);
  if (filename) fclose(lf.f);  /* close file (even in case of errors) */
  if (readstatus) {
    ol_settop(L, fnameindex);  /* ignore results from `ol_load' */
    return errfile(L, "OLang.ReadFileError: read", fnameindex);
  }
  ol_remove(L, fnameindex);
  return status;
}


typedef struct LoadS {
  const char *s;
  size_t size;
} LoadS;


static const char *getS (ol_State *L, void *ud, size_t *size) {
  LoadS *ls = (LoadS *)ud;
  (void)L;
  if (ls->size == 0) return NULL;
  *size = ls->size;
  ls->size = 0;
  return ls->s;
}


olLIB_API int olL_loadbuffer (ol_State *L, const char *buff, size_t size,
                                const char *name) {
  LoadS ls;
  ls.s = buff;
  ls.size = size;
  return ol_load(L, getS, &ls, name);
}


olLIB_API int (olL_loadstring) (ol_State *L, const char *s) {
  return olL_loadbuffer(L, s, strlen(s), s);
}



/* }====================================================== */


static void *l_alloc (void *ud, void *ptr, size_t osize, size_t nsize) {
  (void)ud;
  (void)osize;
  if (nsize == 0) {
    free(ptr);
    return NULL;
  }
  else
    return realloc(ptr, nsize);
}


static int panic (ol_State *L) {
  (void)L;  /* to avoid warnings */
  fprintf(stderr, "OLang.CallAPIError: unprotected error in call to ol API (%s)\n",
                   ol_tostring(L, -1));
  return 0;
}


olLIB_API ol_State *olL_newstate (void) {
  ol_State *L = ol_newstate(l_alloc, NULL);
  if (L) ol_atpanic(L, &panic);
  return L;
}


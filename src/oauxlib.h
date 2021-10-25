/*
** $Id: lauxlib.h,v 1.88.1.1 2007/12/27 13:02:25 roberto Exp $
** Auxiliary functions for building ol libraries
** See Copyright Notice in ol.h
*/


#ifndef lauxlib_h
#define lauxlib_h


#include <stddef.h>
#include <stdio.h>

#include "ol.h"


#if defined(ol_COMPAT_GETN)
olLIB_API int (olL_getn) (ol_State *L, int t);
olLIB_API void (olL_setn) (ol_State *L, int t, int n);
#else
#define olL_getn(L,i)          ((int)ol_objlen(L, i))
#define olL_setn(L,i,j)        ((void)0)  /* no op! */
#endif

#if defined(ol_COMPAT_OPENLIB)
#define olI_openlib	olL_openlib
#endif


/* extra error code for `olL_load' */
#define ol_ERRFILE     (ol_ERRERR+1)


typedef struct olL_Reg {
  const char *name;
  ol_CFunction func;
} olL_Reg;



olLIB_API void (olI_openlib) (ol_State *L, const char *libname,
                                const olL_Reg *l, int nup);
olLIB_API void (olL_register) (ol_State *L, const char *libname,
                                const olL_Reg *l);
olLIB_API int (olL_getmetafield) (ol_State *L, int obj, const char *e);
olLIB_API int (olL_callmeta) (ol_State *L, int obj, const char *e);
olLIB_API int (olL_typerror) (ol_State *L, int narg, const char *tname);
olLIB_API int (olL_argerror) (ol_State *L, int numarg, const char *extramsg);
olLIB_API const char *(olL_checklstring) (ol_State *L, int numArg,
                                                          size_t *l);
olLIB_API const char *(olL_optlstring) (ol_State *L, int numArg,
                                          const char *def, size_t *l);
olLIB_API ol_Number (olL_checknumber) (ol_State *L, int numArg);
olLIB_API ol_Number (olL_optnumber) (ol_State *L, int nArg, ol_Number def);

olLIB_API ol_Integer (olL_checkinteger) (ol_State *L, int numArg);
olLIB_API ol_Integer (olL_optinteger) (ol_State *L, int nArg,
                                          ol_Integer def);

olLIB_API void (olL_checkstack) (ol_State *L, int sz, const char *msg);
olLIB_API void (olL_checktype) (ol_State *L, int narg, int t);
olLIB_API void (olL_checkany) (ol_State *L, int narg);

olLIB_API int   (olL_newmetatable) (ol_State *L, const char *tname);
olLIB_API void *(olL_checkudata) (ol_State *L, int ud, const char *tname);

olLIB_API void (olL_where) (ol_State *L, int lvl);
olLIB_API int (olL_error) (ol_State *L, const char *fmt, ...);

olLIB_API int (olL_checkoption) (ol_State *L, int narg, const char *def,
                                   const char *const lst[]);

olLIB_API int (olL_ref) (ol_State *L, int t);
olLIB_API void (olL_unref) (ol_State *L, int t, int ref);

olLIB_API int (olL_loadfile) (ol_State *L,const char *filename);
olLIB_API int (olL_loadbuffer) (ol_State *L, const char *buff, size_t sz,
                                  const char *name);
olLIB_API int (olL_loadstring) (ol_State *L, const char *s);

olLIB_API ol_State *(olL_newstate) (void);


olLIB_API const char *(olL_gsub) (ol_State *L, const char *s, const char *p,
                                                  const char *r);

olLIB_API const char *(olL_findtable) (ol_State *L, int idx,
                                         const char *fname, int szhint);




/*
** ===============================================================
** some useful macros
** ===============================================================
*/

#define olL_argcheck(L, cond,numarg,extramsg)	\
		((void)((cond) || olL_argerror(L, (numarg), (extramsg))))
#define olL_checkstring(L,n)	(olL_checklstring(L, (n), NULL))
#define olL_optstring(L,n,d)	(olL_optlstring(L, (n), (d), NULL))
#define olL_checkint(L,n)	((int)olL_checkinteger(L, (n)))
#define olL_optint(L,n,d)	((int)olL_optinteger(L, (n), (d)))
#define olL_checklong(L,n)	((long)olL_checkinteger(L, (n)))
#define olL_optlong(L,n,d)	((long)olL_optinteger(L, (n), (d)))

#define olL_typename(L,i)	ol_typename(L, ol_type(L,(i)))

#define olL_dofile(L, fn) \
	(olL_loadfile(L, fn) || ol_pcall(L, 0, ol_MULTRET, 0))

#define olL_dostring(L, s) \
	(olL_loadstring(L, s) || ol_pcall(L, 0, ol_MULTRET, 0))

#define olL_getmetatable(L,n)	(ol_getfield(L, ol_REGISTRYINDEX, (n)))

#define olL_opt(L,f,n,d)	(ol_isnoneornil(L,(n)) ? (d) : f(L,(n)))

/*
** {======================================================
** Generic Buffer manipulation
** =======================================================
*/



typedef struct olL_Buffer {
  char *p;			/* current position in buffer */
  int lvl;  /* number of strings in the stack (level) */
  ol_State *L;
  char buffer[olL_BUFFERSIZE];
} olL_Buffer;

#define olL_addchar(B,c) \
  ((void)((B)->p < ((B)->buffer+olL_BUFFERSIZE) || olL_prepbuffer(B)), \
   (*(B)->p++ = (char)(c)))

/* compatibility only */
#define olL_putchar(B,c)	olL_addchar(B,c)

#define olL_addsize(B,n)	((B)->p += (n))

olLIB_API void (olL_buffinit) (ol_State *L, olL_Buffer *B);
olLIB_API char *(olL_prepbuffer) (olL_Buffer *B);
olLIB_API void (olL_addlstring) (olL_Buffer *B, const char *s, size_t l);
olLIB_API void (olL_addstring) (olL_Buffer *B, const char *s);
olLIB_API void (olL_addvalue) (olL_Buffer *B);
olLIB_API void (olL_pushresult) (olL_Buffer *B);


/* }====================================================== */


/* compatibility with ref system */

/* pre-defined references */
#define ol_NOREF       (-2)
#define ol_REFNIL      (-1)

#define ol_ref(L,lock) ((lock) ? olL_ref(L, ol_REGISTRYINDEX) : \
      (ol_pushstring(L, "OLang.ObsoloteError:  unlocked references are obsolete"), ol_error(L), 0))

#define ol_unref(L,ref)        olL_unref(L, ol_REGISTRYINDEX, (ref))

#define ol_getref(L,ref)       ol_rawgeti(L, ol_REGISTRYINDEX, (ref))


#define olL_reg	olL_Reg

#endif



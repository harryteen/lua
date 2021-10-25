/*
** $Id: ol.h,v 1.218.1.7 2012/01/13 20:36:20 roberto Exp $
** ol - An Extensible Extension Language
** ol.org, PUC-Rio, Brazil (http://www.ol.org)
** See Copyright Notice at the end of this file
*/


#ifndef ol_h
#define ol_h

#include <stdarg.h>
#include <stddef.h>


#include "olconf.h"


#define ol_VERSION	"ol 1.0.0"
#define ol_RELEASE	"ol 1.0.0"
#define ol_VERSION_NUM	501
#define ol_COPYRIGHT	"Copyright (C) 2021 OLang"
#define ol_AUTHORS 	"Phoenix Tc."


/* mark for precompiled code (`<esc>ol') */
#define	ol_SIGNATURE	"\033ol"

/* option for multiple returns in `ol_pcall' and `ol_call' */
#define ol_MULTRET	(-1)


/*
** pseudo-indices
*/
#define ol_REGISTRYINDEX	(-10000)
#define ol_ENVIRONINDEX	(-10001)
#define ol_GLOBALSINDEX	(-10002)
#define ol_upvalueindex(i)	(ol_GLOBALSINDEX-(i))


/* thread status; 0 is OK */
#define ol_YIELD	1
#define ol_ERRRUN	2
#define ol_ERRSYNTAX	3
#define ol_ERRMEM	4
#define ol_ERRERR	5


typedef struct ol_State ol_State;

typedef int (*ol_CFunction) (ol_State *L);


/*
** functions that read/write blocks when loading/dumping ol chunks
*/
typedef const char * (*ol_Reader) (ol_State *L, void *ud, size_t *sz);

typedef int (*ol_Writer) (ol_State *L, const void* p, size_t sz, void* ud);


/*
** prototype for memory-allocation functions
*/
typedef void * (*ol_Alloc) (void *ud, void *ptr, size_t osize, size_t nsize);


/*
** basic types
*/
#define ol_TNONE		(-1)

#define ol_TNIL		0
#define ol_TBOOLEAN		1
#define ol_TLIGHTUSERDATA	2
#define ol_TNUMBER		3
#define ol_TSTRING		4
#define ol_TTABLE		5
#define ol_TFUNCTION		6
#define ol_TUSERDATA		7
#define ol_TTHREAD		8



/* minimum ol stack available to a C function */
#define ol_MINSTACK	20


/*
** generic extra include file
*/
#if defined(ol_USER_H)
#include ol_USER_H
#endif


/* type of numbers in ol */
typedef ol_NUMBER ol_Number;


/* type for integer functions */
typedef ol_INTEGER ol_Integer;



/*
** state manipulation
*/
ol_API ol_State *(ol_newstate) (ol_Alloc f, void *ud);
ol_API void       (ol_close) (ol_State *L);
ol_API ol_State *(ol_newthread) (ol_State *L);

ol_API ol_CFunction (ol_atpanic) (ol_State *L, ol_CFunction panicf);


/*
** basic stack manipulation
*/
ol_API int   (ol_gettop) (ol_State *L);
ol_API void  (ol_settop) (ol_State *L, int idx);
ol_API void  (ol_pushvalue) (ol_State *L, int idx);
ol_API void  (ol_remove) (ol_State *L, int idx);
ol_API void  (ol_insert) (ol_State *L, int idx);
ol_API void  (ol_replace) (ol_State *L, int idx);
ol_API int   (ol_checkstack) (ol_State *L, int sz);

ol_API void  (ol_xmove) (ol_State *from, ol_State *to, int n);


/*
** access functions (stack -> C)
*/

ol_API int             (ol_isnumber) (ol_State *L, int idx);
ol_API int             (ol_isstring) (ol_State *L, int idx);
ol_API int             (ol_iscfunction) (ol_State *L, int idx);
ol_API int             (ol_isuserdata) (ol_State *L, int idx);
ol_API int             (ol_type) (ol_State *L, int idx);
ol_API const char     *(ol_typename) (ol_State *L, int tp);

ol_API int            (ol_equal) (ol_State *L, int idx1, int idx2);
ol_API int            (ol_rawequal) (ol_State *L, int idx1, int idx2);
ol_API int            (ol_lessthan) (ol_State *L, int idx1, int idx2);

ol_API ol_Number      (ol_tonumber) (ol_State *L, int idx);
ol_API ol_Integer     (ol_tointeger) (ol_State *L, int idx);
ol_API int             (ol_toboolean) (ol_State *L, int idx);
ol_API const char     *(ol_tolstring) (ol_State *L, int idx, size_t *len);
ol_API size_t          (ol_objlen) (ol_State *L, int idx);
ol_API ol_CFunction   (ol_tocfunction) (ol_State *L, int idx);
ol_API void	       *(ol_touserdata) (ol_State *L, int idx);
ol_API ol_State      *(ol_tothread) (ol_State *L, int idx);
ol_API const void     *(ol_topointer) (ol_State *L, int idx);


/*
** push functions (C -> stack)
*/
ol_API void  (ol_pushnil) (ol_State *L);
ol_API void  (ol_pushnumber) (ol_State *L, ol_Number n);
ol_API void  (ol_pushinteger) (ol_State *L, ol_Integer n);
ol_API void  (ol_pushlstring) (ol_State *L, const char *s, size_t l);
ol_API void  (ol_pushstring) (ol_State *L, const char *s);
ol_API const char *(ol_pushvfstring) (ol_State *L, const char *fmt,
                                                      va_list argp);
ol_API const char *(ol_pushfstring) (ol_State *L, const char *fmt, ...);
ol_API void  (ol_pushcclosure) (ol_State *L, ol_CFunction fn, int n);
ol_API void  (ol_pushboolean) (ol_State *L, int b);
ol_API void  (ol_pushlightuserdata) (ol_State *L, void *p);
ol_API int   (ol_pushthread) (ol_State *L);


/*
** get functions (ol -> stack)
*/
ol_API void  (ol_gettable) (ol_State *L, int idx);
ol_API void  (ol_getfield) (ol_State *L, int idx, const char *k);
ol_API void  (ol_rawget) (ol_State *L, int idx);
ol_API void  (ol_rawgeti) (ol_State *L, int idx, int n);
ol_API void  (ol_createtable) (ol_State *L, int narr, int nrec);
ol_API void *(ol_newuserdata) (ol_State *L, size_t sz);
ol_API int   (ol_getmetatable) (ol_State *L, int objindex);
ol_API void  (ol_getfenv) (ol_State *L, int idx);


/*
** set functions (stack -> ol)
*/
ol_API void  (ol_settable) (ol_State *L, int idx);
ol_API void  (ol_setfield) (ol_State *L, int idx, const char *k);
ol_API void  (ol_rawset) (ol_State *L, int idx);
ol_API void  (ol_rawseti) (ol_State *L, int idx, int n);
ol_API int   (ol_setmetatable) (ol_State *L, int objindex);
ol_API int   (ol_setfenv) (ol_State *L, int idx);


/*
** `load' and `call' functions (load and run ol code)
*/
ol_API void  (ol_call) (ol_State *L, int nargs, int nresults);
ol_API int   (ol_pcall) (ol_State *L, int nargs, int nresults, int errfunc);
ol_API int   (ol_cpcall) (ol_State *L, ol_CFunction func, void *ud);
ol_API int   (ol_load) (ol_State *L, ol_Reader reader, void *dt,
                                        const char *chunkname);

ol_API int (ol_dump) (ol_State *L, ol_Writer writer, void *data);


/*
** coroutine functions
*/
ol_API int  (ol_yield) (ol_State *L, int nresults);
ol_API int  (ol_resume) (ol_State *L, int narg);
ol_API int  (ol_status) (ol_State *L);

/*
** garbage-collection function and options
*/

#define ol_GCSTOP		0
#define ol_GCRESTART		1
#define ol_GCCOLLECT		2
#define ol_GCCOUNT		3
#define ol_GCCOUNTB		4
#define ol_GCSTEP		5
#define ol_GCSETPAUSE		6
#define ol_GCSETSTEPMUL	7

ol_API int (ol_gc) (ol_State *L, int what, int data);


/*
** miscellaneous functions
*/

ol_API int   (ol_error) (ol_State *L);

ol_API int   (ol_next) (ol_State *L, int idx);

ol_API void  (ol_concat) (ol_State *L, int n);

ol_API ol_Alloc (ol_getallocf) (ol_State *L, void **ud);
ol_API void ol_setallocf (ol_State *L, ol_Alloc f, void *ud);



/* 
** ===============================================================
** some useful macros
** ===============================================================
*/

#define ol_pop(L,n)		ol_settop(L, -(n)-1)

#define ol_newtable(L)		ol_createtable(L, 0, 0)

#define ol_register(L,n,f) (ol_pushcfunction(L, (f)), ol_setglobal(L, (n)))

#define ol_pushcfunction(L,f)	ol_pushcclosure(L, (f), 0)

#define ol_strlen(L,i)		ol_objlen(L, (i))

#define ol_isfunction(L,n)	(ol_type(L, (n)) == ol_TFUNCTION)
#define ol_istable(L,n)	(ol_type(L, (n)) == ol_TTABLE)
#define ol_islightuserdata(L,n)	(ol_type(L, (n)) == ol_TLIGHTUSERDATA)
#define ol_isnil(L,n)		(ol_type(L, (n)) == ol_TNIL)
#define ol_isboolean(L,n)	(ol_type(L, (n)) == ol_TBOOLEAN)
#define ol_isthread(L,n)	(ol_type(L, (n)) == ol_TTHREAD)
#define ol_isnone(L,n)		(ol_type(L, (n)) == ol_TNONE)
#define ol_isnoneornil(L, n)	(ol_type(L, (n)) <= 0)

#define ol_pushliteral(L, s)	\
	ol_pushlstring(L, "" s, (sizeof(s)/sizeof(char))-1)

#define ol_setglobal(L,s)	ol_setfield(L, ol_GLOBALSINDEX, (s))
#define ol_getglobal(L,s)	ol_getfield(L, ol_GLOBALSINDEX, (s))

#define ol_tostring(L,i)	ol_tolstring(L, (i), NULL)



/*
** compatibility macros and functions
*/

#define ol_open()	olL_newstate()

#define ol_getregistry(L)	ol_pushvalue(L, ol_REGISTRYINDEX)

#define ol_getgccount(L)	ol_gc(L, ol_GCCOUNT, 0)

#define ol_Chunkreader		ol_Reader
#define ol_Chunkwriter		ol_Writer


/* hack */
ol_API void ol_setlevel	(ol_State *from, ol_State *to);


/*
** {======================================================================
** Debug API
** =======================================================================
*/


/*
** Event codes
*/
#define ol_HOOKCALL	0
#define ol_HOOKRET	1
#define ol_HOOKLINE	2
#define ol_HOOKCOUNT	3
#define ol_HOOKTAILRET 4


/*
** Event masks
*/
#define ol_MASKCALL	(1 << ol_HOOKCALL)
#define ol_MASKRET	(1 << ol_HOOKRET)
#define ol_MASKLINE	(1 << ol_HOOKLINE)
#define ol_MASKCOUNT	(1 << ol_HOOKCOUNT)

typedef struct ol_Debug ol_Debug;  /* activation record */


/* Functions to be called by the debuger in specific events */
typedef void (*ol_Hook) (ol_State *L, ol_Debug *ar);


ol_API int ol_getstack (ol_State *L, int level, ol_Debug *ar);
ol_API int ol_getinfo (ol_State *L, const char *what, ol_Debug *ar);
ol_API const char *ol_getlocal (ol_State *L, const ol_Debug *ar, int n);
ol_API const char *ol_setlocal (ol_State *L, const ol_Debug *ar, int n);
ol_API const char *ol_getupvalue (ol_State *L, int funcindex, int n);
ol_API const char *ol_setupvalue (ol_State *L, int funcindex, int n);

ol_API int ol_sethook (ol_State *L, ol_Hook func, int mask, int count);
ol_API ol_Hook ol_gethook (ol_State *L);
ol_API int ol_gethookmask (ol_State *L);
ol_API int ol_gethookcount (ol_State *L);


struct ol_Debug {
  int event;
  const char *name;	/* (n) */
  const char *namewhat;	/* (n) `global', `local', `field', `method' */
  const char *what;	/* (S) `ol', `C', `main', `tail' */
  const char *source;	/* (S) */
  int currentline;	/* (l) */
  int nups;		/* (u) number of upvalues */
  int linedefined;	/* (S) */
  int lastlinedefined;	/* (S) */
  char short_src[ol_IDSIZE]; /* (S) */
  /* private part */
  int i_ci;  /* active function */
};

/* }====================================================================== */


/******************************************************************************
* Copyright (C) 1994-2012 ol.org, PUC-Rio.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
******************************************************************************/


#endif

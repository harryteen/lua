/*
** $Id: olinit.c,v 1.14.1.1 2007/12/27 13:02:25 roberto Exp $
** Initialization of libraries for ol.c
** See Copyright Notice in ol.h
*/


#define olinit_c
#define ol_LIB

#include "ol.h"

#include "ollib.h"
#include "olauxlib.h"


static const olL_Reg ollibs[] = {
  {"", olopen_base},
  {ol_LOADLIBNAME, olopen_package},
  {ol_TABLIBNAME, olopen_table},
  {ol_IOLIBNAME, olopen_io},
  {ol_OSLIBNAME, olopen_os},
  {ol_STRLIBNAME, olopen_string},
  {ol_MATHLIBNAME, olopen_math},
  {ol_DBLIBNAME, olopen_debug},
  {NULL, NULL}
};


olLIB_API void olL_openlibs (ol_State *L) {
  const olL_Reg *lib = ollibs;
  for (; lib->func; lib++) {
    ol_pushcfunction(L, lib->func);
    ol_pushstring(L, lib->name);
    ol_call(L, 1, 0);
  }
}


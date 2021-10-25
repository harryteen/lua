/*
** $Id: ollib.h,v 1.36.1.1 2007/12/27 13:02:25 roberto Exp $
** ol standard libraries
** See Copyright Notice in ol.h
*/


#ifndef ollib_h
#define ollib_h

#include "ol.h"


/* Key to file-handle type */
#define ol_FILEHANDLE		"FILE*"


#define ol_COLIBNAME	"coroutine"
olLIB_API int (olopen_base) (ol_State *L);

#define ol_TABLIBNAME	"table"
olLIB_API int (olopen_table) (ol_State *L);

#define ol_IOLIBNAME	"io"
olLIB_API int (olopen_io) (ol_State *L);

#define ol_OSLIBNAME	"os"
olLIB_API int (olopen_os) (ol_State *L);

#define ol_STRLIBNAME	"string"
olLIB_API int (olopen_string) (ol_State *L);

#define ol_MATHLIBNAME	"math"
olLIB_API int (olopen_math) (ol_State *L);

#define ol_DBLIBNAME	"debug"
olLIB_API int (olopen_debug) (ol_State *L);

#define ol_LOADLIBNAME	"package"
olLIB_API int (olopen_package) (ol_State *L);


/* open all previous libraries */
olLIB_API void (olL_openlibs) (ol_State *L); 



#ifndef ol_assert
#define ol_assert(x)	((void)0)
#endif


#endif

/*
** $Id: ollimits.h,v 1.69.1.1 2007/12/27 13:02:25 roberto Exp $
** Limits, basic types, and some other `installation-dependent' definitions
** See Copyright Notice in ol.h
*/

#ifndef ollimits_h
#define ollimits_h


#include <ollimits.h>
#include <ostddef.h>


#include "ol.h"


typedef olI_UINT32 lu_int32;

typedef olI_UMEM lu_mem;

typedef olI_MEM l_mem;



/* chars used as small naturals (so that `char' is reserved for characters) */
typedef unsigned char lu_byte;


#define MAX_SIZET	((size_t)(~(size_t)0)-2)

#define MAX_LUMEM	((lu_mem)(~(lu_mem)0)-2)


#define MAX_INT (INT_MAX-2)  /* maximum value of an int (-2 for safety) */

/*
** conversion of pointer to integer
** this is for hashing only; there is no problem if the integer
** cannot hold the whole pointer value
*/
#define IntPoint(p)  ((unsigned int)(lu_mem)(p))



/* type to ensure maximum alignment */
typedef olI_USER_ALIGNMENT_T L_Umaxalign;


/* result of a `usual argument conversion' over ol_Number */
typedef olI_UACNUMBER l_uacNumber;


/* internal assertions for in-house debugging */
#ifdef ol_assert

#define check_exp(c,e)		(ol_assert(c), (e))
#define api_check(l,e)		ol_assert(e)

#else

#define ol_assert(c)		((void)0)
#define check_exp(c,e)		(e)
#define api_check		oli_apicheck

#endif


#ifndef UNUSED
#define UNUSED(x)	((void)(x))	/* to avoid warnings */
#endif


#ifndef cast
#define cast(t, exp)	((t)(exp))
#endif

#define cast_byte(i)	cast(lu_byte, (i))
#define cast_num(i)	cast(ol_Number, (i))
#define cast_int(i)	cast(int, (i))



/*
** type for virtual-machine instructions
** must be an unsigned with (at least) 4 bytes (see details in lopcodes.h)
*/
typedef lu_int32 Instruction;



/* maximum stack for a ol function */
#define MAXSTACK	250



/* minimum size for the string table (must be power of 2) */
#ifndef MINSTRTABSIZE
#define MINSTRTABSIZE	32
#endif


/* minimum size for string buffer */
#ifndef ol_MINBUFFER
#define ol_MINBUFFER	32
#endif


#ifndef ol_lock
#define ol_lock(L)     ((void) 0) 
#define ol_unlock(L)   ((void) 0)
#endif

#ifndef oli_threadyield
#define oli_threadyield(L)     {ol_unlock(L); ol_lock(L);}
#endif


/*
** macro to control inclusion of some hard tests on stack reallocation
*/ 
#ifndef HARDSTACKTESTS
#define condhardstacktests(x)	((void)0)
#else
#define condhardstacktests(x)	x
#endif

#endif

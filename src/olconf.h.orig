/*
** $Id: olconf.h,v 1.82.1.7 2008/02/11 16:25:08 roberto Exp $
** Configuration file for ol
** See Copyright Notice in ol.h
*/


#ifndef lconfig_h
#define lconfig_h

#include <limits.h>
#include <stddef.h>


/*
** ==================================================================
** Search for "@@" to find all configurable definitions.
** ===================================================================
*/


/*
@@ ol_ANSI controls the use of non-ansi features.
** CHANGE it (define it) if you want ol to avoid the use of any
** non-ansi feature or library.
*/
#if defined(__STRICT_ANSI__)
#define ol_ANSI
#endif


#if !defined(ol_ANSI) && defined(_WIN32)
#define ol_WIN
#endif

#if defined(ol_USE_LINUX)
#define ol_USE_POSIX
#define ol_USE_DLOPEN		/* needs an extra library: -ldl */
#define ol_USE_READLINE	/* needs some extra libraries */
#endif

#if defined(ol_USE_MACOSX)
#define ol_USE_POSIX
#define ol_DL_DYLD		/* does not need extra library */
#endif



/*
@@ ol_USE_POSIX includes all functionallity listed as X/Open System
@* Interfaces Extension (XSI).
** CHANGE it (define it) if your system is XSI compatible.
*/
#if defined(ol_USE_POSIX)
#define ol_USE_MKSTEMP
#define ol_USE_ISATTY
#define ol_USE_POPEN
#define ol_USE_ULONGJMP
#endif


/*
@@ ol_PATH and ol_CPATH are the names of the environment variables that
@* ol check to set its paths.
@@ ol_INIT is the name of the environment variable that ol
@* checks for initialization code.
** CHANGE them if you want different names.
*/
#define ol_PATH        "ol_PATH"
#define ol_CPATH       "ol_CPATH"
#define ol_INIT	"ol_INIT"


/*
@@ ol_PATH_DEFAULT is the default path that ol uses to look for
@* ol libraries.
@@ ol_CPATH_DEFAULT is the default path that ol uses to look for
@* C libraries.
** CHANGE them if your machine has a non-conventional directory
** hierarchy or if you want to install your libraries in
** non-conventional directories.
*/
#if defined(_WIN32)
/*
** In Windows, any exclamation mark ('!') in the path is replaced by the
** path of the directory of the executable file of the current process.
*/
#define ol_LDIR	"!\\ol\\"
#define ol_CDIR	"!\\"
#define ol_PATH_DEFAULT  \
		".\\?.ol;"  ol_LDIR"?.ol;"  ol_LDIR"?\\init.ol;" \
		             ol_CDIR"?.ol;"  ol_CDIR"?\\init.ol"
#define ol_CPATH_DEFAULT \
	".\\?.dll;"  ol_CDIR"?.dll;" ol_CDIR"loadall.dll"

#else
#define ol_ROOT	"/usr/local/"
#define ol_LDIR	ol_ROOT "share/ol/5.1/"
#define ol_CDIR	ol_ROOT "lib/ol/5.1/"
#define ol_PATH_DEFAULT  \
		"./?.ol;"  ol_LDIR"?.ol;"  ol_LDIR"?/init.ol;" \
		            ol_CDIR"?.ol;"  ol_CDIR"?/init.ol"
#define ol_CPATH_DEFAULT \
	"./?.so;"  ol_CDIR"?.so;" ol_CDIR"loadall.so"
#endif


/*
@@ ol_DIRSEP is the directory separator (for submodules).
** CHANGE it if your machine does not use "/" as the directory separator
** and is not Windows. (On Windows ol automatically uses "\".)
*/
#if defined(_WIN32)
#define ol_DIRSEP	"\\"
#else
#define ol_DIRSEP	"/"
#endif


/*
@@ ol_PATHSEP is the character that separates templates in a path.
@@ ol_PATH_MARK is the string that marks the substitution points in a
@* template.
@@ ol_EXECDIR in a Windows path is replaced by the executable's
@* directory.
@@ ol_IGMARK is a mark to ignore all before it when bulding the
@* olopen_ function name.
** CHANGE them if for some reason your system cannot use those
** characters. (E.g., if one of those characters is a common character
** in file/directory names.) Probably you do not need to change them.
*/
#define ol_PATHSEP	";"
#define ol_PATH_MARK	"?"
#define ol_EXECDIR	"!"
#define ol_IGMARK	"-"


/*
@@ ol_INTEGER is the integral type used by ol_pushinteger/ol_tointeger.
** CHANGE that if ptrdiff_t is not adequate on your machine. (On most
** machines, ptrdiff_t gives a good choice between int or long.)
*/
#define ol_INTEGER	ptrdiff_t


/*
@@ ol_API is a mark for all core API functions.
@@ olLIB_API is a mark for all standard library functions.
** CHANGE them if you need to define those functions in some special way.
** For instance, if you want to create one Windows DLL with the core and
** the libraries, you may want to use the following definition (define
** ol_BUILD_AS_DLL to get it).
*/
#if defined(ol_BUILD_AS_DLL)

#if defined(ol_CORE) || defined(ol_LIB)
#define ol_API __declspec(dllexport)
#else
#define ol_API __declspec(dllimport)
#endif

#else

#define ol_API		extern

#endif

/* more often than not the libs go together with the core */
#define olLIB_API	ol_API


/*
@@ olI_FUNC is a mark for all extern functions that are not to be
@* exported to outside modules.
@@ olI_DATA is a mark for all extern (const) variables that are not to
@* be exported to outside modules.
** CHANGE them if you need to mark them in some special way. Elf/gcc
** (versions 3.2 and later) mark them as "hidden" to optimize access
** when ol is compiled as a shared library.
*/
#if defined(olall_c)
#define olI_FUNC	static
#define olI_DATA	/* empty */

#elif defined(__GNUC__) && ((__GNUC__*100 + __GNUC_MINOR__) >= 302) && \
      defined(__ELF__)
#define olI_FUNC	__attribute__((visibility("hidden"))) extern
#define olI_DATA	olI_FUNC

#else
#define olI_FUNC	extern
#define olI_DATA	extern
#endif



/*
@@ ol_QL describes how error messages quote program elements.
** CHANGE it if you want a different appearance.
*/
#define ol_QL(x)	"'" x "'"
#define ol_QS		ol_QL("%s")


/*
@@ ol_IDSIZE gives the maximum size for the description of the source
@* of a function in debug information.
** CHANGE it if you want a different size.
*/
#define ol_IDSIZE	60


/*
** {==================================================================
** Stand-alone configuration
** ===================================================================
*/

#if defined(ol_c) || defined(olall_c)

/*
@@ ol_stdin_is_tty detects whether the standard input is a 'tty' (that
@* is, whether we're running ol interactively).
** CHANGE it if you have a better definition for non-POSIX/non-Windows
** systems.
*/
#if defined(ol_USE_ISATTY)
#include <unistd.h>
#define ol_stdin_is_tty()	isatty(0)
#elif defined(ol_WIN)
#include <io.h>
#include <stdio.h>
#define ol_stdin_is_tty()	_isatty(_fileno(stdin))
#else
#define ol_stdin_is_tty()	1  /* assume stdin is a tty */
#endif


/*
@@ ol_PROMPT is the default prompt used by stand-alone ol.
@@ ol_PROMPT2 is the default continuation prompt used by stand-alone ol.
** CHANGE them if you want different prompts. (You can also change the
** prompts dynamically, assigning to globals _PROMPT/_PROMPT2.)
*/
#define ol_PROMPT		"> "
#define ol_PROMPT2		">> "


/*
@@ ol_PROGNAME is the default name for the stand-alone ol program.
** CHANGE it if your stand-alone interpreter has a different name and
** your system is not able to detect that name automatically.
*/
#define ol_PROGNAME		"ol"


/*
@@ ol_MAXINPUT is the maximum length for an input line in the
@* stand-alone interpreter.
** CHANGE it if you need longer lines.
*/
#define ol_MAXINPUT	512


/*
@@ ol_readline defines how to show a prompt and then read a line from
@* the standard input.
@@ ol_saveline defines how to "save" a read line in a "history".
@@ ol_freeline defines how to free a line read by ol_readline.
** CHANGE them if you want to improve this functionality (e.g., by using
** GNU readline and history facilities).
*/
#if defined(ol_USE_READLINE)
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#define ol_readline(L,b,p)	((void)L, ((b)=readline(p)) != NULL)
#define ol_saveline(L,idx) \
	if (ol_strlen(L,idx) > 0)  /* non-empty line? */ \
	  add_history(ol_tostring(L, idx));  /* add it to history */
#define ol_freeline(L,b)	((void)L, free(b))
#else
#define ol_readline(L,b,p)	\
	((void)L, fputs(p, stdout), fflush(stdout),  /* show prompt */ \
	fgets(b, ol_MAXINPUT, stdin) != NULL)  /* get line */
#define ol_saveline(L,idx)	{ (void)L; (void)idx; }
#define ol_freeline(L,b)	{ (void)L; (void)b; }
#endif

#endif

/* }================================================================== */


/*
@@ olI_GCPAUSE defines the default pause between garbage-collector cycles
@* as a percentage.
** CHANGE it if you want the GC to run faster or slower (higher values
** mean larger pauses which mean slower collection.) You can also change
** this value dynamically.
*/
#define olI_GCPAUSE	200  /* 200% (wait memory to double before next GC) */


/*
@@ olI_GCMUL defines the default speed of garbage collection relative to
@* memory allocation as a percentage.
** CHANGE it if you want to change the granularity of the garbage
** collection. (Higher values mean coarser collections. 0 represents
** infinity, where each step performs a full collection.) You can also
** change this value dynamically.
*/
#define olI_GCMUL	200 /* GC runs 'twice the speed' of memory allocation */



/*
@@ ol_COMPAT_GETN controls compatibility with old getn behavior.
** CHANGE it (define it) if you want exact compatibility with the
** behavior of setn/getn in ol 5.0.
*/
#undef ol_COMPAT_GETN

/*
@@ ol_COMPAT_LOADLIB controls compatibility about global loadlib.
** CHANGE it to undefined as soon as you do not need a global 'loadlib'
** function (the function is still available as 'package.loadlib').
*/
#undef ol_COMPAT_LOADLIB

/*
@@ ol_COMPAT_VARARG controls compatibility with old vararg feature.
** CHANGE it to undefined as soon as your programs use only '...' to
** access vararg parameters (instead of the old 'arg' table).
*/
#define ol_COMPAT_VARARG

/*
@@ ol_COMPAT_MOD controls compatibility with old math.mod function.
** CHANGE it to undefined as soon as your programs use 'math.fmod' or
** the new '%' operator instead of 'math.mod'.
*/
#define ol_COMPAT_MOD

/*
@@ ol_COMPAT_LSTR controls compatibility with old long string nesting
@* facility.
** CHANGE it to 2 if you want the old behaviour, or undefine it to turn
** off the advisory error when nesting [[...]].
*/
#define ol_COMPAT_LSTR		1

/*
@@ ol_COMPAT_GFIND controls compatibility with old 'string.gfind' name.
** CHANGE it to undefined as soon as you rename 'string.gfind' to
** 'string.gmatch'.
*/
#define ol_COMPAT_GFIND

/*
@@ ol_COMPAT_OPENLIB controls compatibility with old 'olL_openlib'
@* behavior.
** CHANGE it to undefined as soon as you replace to 'olL_register'
** your uses of 'olL_openlib'
*/
#define ol_COMPAT_OPENLIB



/*
@@ oli_apicheck is the assert macro used by the ol-C API.
** CHANGE oli_apicheck if you want ol to perform some checks in the
** parameters it gets from API calls. This may slow down the interpreter
** a bit, but may be quite useful when debugging C code that interfaces
** with ol. A useful redefinition is to use assert.h.
*/
#if defined(ol_USE_APICHECK)
#include <assert.h>
#define oli_apicheck(L,o)	{ (void)L; assert(o); }
#else
#define oli_apicheck(L,o)	{ (void)L; }
#endif


/*
@@ olI_BITSINT defines the number of bits in an int.
** CHANGE here if ol cannot automatically detect the number of bits of
** your machine. Probably you do not need to change this.
*/
/* avoid overflows in comparison */
#if INT_MAX-20 < 32760
#define olI_BITSINT	16
#elif INT_MAX > 2147483640L
/* int has at least 32 bits */
#define olI_BITSINT	32
#else
#error "you must define ol_BITSINT with number of bits in an integer"
#endif


/*
@@ olI_UINT32 is an unsigned integer with at least 32 bits.
@@ olI_INT32 is an signed integer with at least 32 bits.
@@ olI_UMEM is an unsigned integer big enough to count the total
@* memory used by ol.
@@ olI_MEM is a signed integer big enough to count the total memory
@* used by ol.
** CHANGE here if for some weird reason the default definitions are not
** good enough for your machine. (The definitions in the 'else'
** part always works, but may waste space on machines with 64-bit
** longs.) Probably you do not need to change this.
*/
#if olI_BITSINT >= 32
#define olI_UINT32	unsigned int
#define olI_INT32	int
#define olI_MAXINT32	INT_MAX
#define olI_UMEM	size_t
#define olI_MEM	ptrdiff_t
#else
/* 16-bit ints */
#define olI_UINT32	unsigned long
#define olI_INT32	long
#define olI_MAXINT32	LONG_MAX
#define olI_UMEM	unsigned long
#define olI_MEM	long
#endif


/*
@@ olI_MAXCALLS limits the number of nested calls.
** CHANGE it if you need really deep recursive calls. This limit is
** arbitrary; its only purpose is to stop infinite recursion before
** exhausting memory.
*/
#define olI_MAXCALLS	20000


/*
@@ olI_MAXCSTACK limits the number of ol stack slots that a C function
@* can use.
** CHANGE it if you need lots of (ol) stack space for your C
** functions. This limit is arbitrary; its only purpose is to stop C
** functions to consume unlimited stack space. (must be smaller than
** -ol_REGISTRYINDEX)
*/
#define olI_MAXCSTACK	8000



/*
** {==================================================================
** CHANGE (to smaller values) the following definitions if your system
** has a small C stack. (Or you may want to change them to larger
** values if your system has a large C stack and these limits are
** too rigid for you.) Some of these constants control the size of
** stack-allocated arrays used by the compiler or the interpreter, while
** others limit the maximum number of recursive calls that the compiler
** or the interpreter can perform. Values too large may cause a C stack
** overflow for some forms of deep constructs.
** ===================================================================
*/


/*
@@ olI_MAXCCALLS is the maximum depth for nested C calls (short) and
@* syntactical nested non-terminals in a program.
*/
#define olI_MAXCCALLS		200


/*
@@ olI_MAXVARS is the maximum number of local variables per function
@* (must be smaller than 250).
*/
#define olI_MAXVARS		200


/*
@@ olI_MAXUPVALUES is the maximum number of upvalues per function
@* (must be smaller than 250).
*/
#define olI_MAXUPVALUES	60


/*
@@ olL_BUFFERSIZE is the buffer size used by the lauxlib buffer system.
*/
#define olL_BUFFERSIZE		BUFSIZ

/* }================================================================== */




/*
** {==================================================================
@@ ol_NUMBER is the type of numbers in ol.
** CHANGE the following definitions only if you want to build ol
** with a number type different from double. You may also need to
** change ol_number2int & ol_number2integer.
** ===================================================================
*/

#define ol_NUMBER_DOUBLE
#define ol_NUMBER	double

/*
@@ olI_UACNUMBER is the result of an 'usual argument conversion'
@* over a number.
*/
#define olI_UACNUMBER	double


/*
@@ ol_NUMBER_SCAN is the format for reading numbers.
@@ ol_NUMBER_FMT is the format for writing numbers.
@@ ol_number2str converts a number to a string.
@@ olI_MAXNUMBER2STR is maximum size of previous conversion.
@@ ol_str2number converts a string to a number.
*/
#define ol_NUMBER_SCAN		"%lf"
#define ol_NUMBER_FMT		"%.14g"
#define ol_number2str(s,n)	sprintf((s), ol_NUMBER_FMT, (n))
#define olI_MAXNUMBER2STR	32 /* 16 digits, sign, point, and \0 */
#define ol_str2number(s,p)	strtod((s), (p))


/*
@@ The oli_num* macros define the primitive operations over numbers.
*/
#if defined(ol_CORE)
#include <math.h>
#define oli_numadd(a,b)	((a)+(b))
#define oli_numsub(a,b)	((a)-(b))
#define oli_nummul(a,b)	((a)*(b))
#define oli_numdiv(a,b)	((a)/(b))
#define oli_nummod(a,b)	((a) - floor((a)/(b))*(b))
#define oli_numpow(a,b)	(pow(a,b))
#define oli_numunm(a)		(-(a))
#define oli_numeq(a,b)		((a)==(b))
#define oli_numlt(a,b)		((a)<(b))
#define oli_numle(a,b)		((a)<=(b))
#define oli_numisnan(a)	(!oli_numeq((a), (a)))
#endif


/*
@@ ol_number2int is a macro to convert ol_Number to int.
@@ ol_number2integer is a macro to convert ol_Number to ol_Integer.
** CHANGE them if you know a faster way to convert a ol_Number to
** int (with any rounding method and without throwing errors) in your
** system. In Pentium machines, a naive typecast from double to int
** in C is extremely slow, so any alternative is worth trying.
*/

/* On a Pentium, resort to a trick */
#if defined(ol_NUMBER_DOUBLE) && !defined(ol_ANSI) && !defined(__SSE2__) && \
    (defined(__i386) || defined (_M_IX86) || defined(__i386__))

/* On a Microsoft compiler, use assembler */
#if defined(_MSC_VER)

#define ol_number2int(i,d)   __asm fld d   __asm fistp i
#define ol_number2integer(i,n)		ol_number2int(i, n)

/* the next trick should work on any Pentium, but sometimes clashes
   with a DirectX idiosyncrasy */
#else

union oli_Cast { double l_d; long l_l; };
#define ol_number2int(i,d) \
  { volatile union oli_Cast u; u.l_d = (d) + 6755399441055744.0; (i) = u.l_l; }
#define ol_number2integer(i,n)		ol_number2int(i, n)

#endif


/* this option always works, but may be slow */
#else
#define ol_number2int(i,d)	((i)=(int)(d))
#define ol_number2integer(i,d)	((i)=(ol_Integer)(d))

#endif

/* }================================================================== */


/*
@@ olI_USER_ALIGNMENT_T is a type that requires maximum alignment.
** CHANGE it if your system requires alignments larger than double. (For
** instance, if your system supports long doubles and they must be
** aligned in 16-byte boundaries, then you should add long double in the
** union.) Probably you do not need to change this.
*/
#define olI_USER_ALIGNMENT_T	union { double u; void *s; long l; }


/*
@@ olI_THROW/olI_TRY define how ol does exception handling.
** CHANGE them if you prefer to use longjmp/setjmp even with C++
** or if want/don't to use _longjmp/_setjmp instead of regular
** longjmp/setjmp. By default, ol handles errors with exceptions when
** compiling as C++ code, with _longjmp/_setjmp when asked to use them,
** and with longjmp/setjmp otherwise.
*/
#if defined(__cplusplus)
/* C++ exceptions */
#define olI_THROW(L,c)	throw(c)
#define olI_TRY(L,c,a)	try { a } catch(...) \
	{ if ((c)->status == 0) (c)->status = -1; }
#define oli_jmpbuf	int  /* dummy variable */

#elif defined(ol_USE_ULONGJMP)
/* in Unix, try _longjmp/_setjmp (more efficient) */
#define olI_THROW(L,c)	_longjmp((c)->b, 1)
#define olI_TRY(L,c,a)	if (_setjmp((c)->b) == 0) { a }
#define oli_jmpbuf	jmp_buf

#else
/* default handling with long jumps */
#define olI_THROW(L,c)	longjmp((c)->b, 1)
#define olI_TRY(L,c,a)	if (setjmp((c)->b) == 0) { a }
#define oli_jmpbuf	jmp_buf

#endif


/*
@@ ol_MAXCAPTURES is the maximum number of captures that a pattern
@* can do during pattern-matching.
** CHANGE it if you need more captures. This limit is arbitrary.
*/
#define ol_MAXCAPTURES		32


/*
@@ ol_tmpnam is the function that the OS library uses to create a
@* temporary name.
@@ ol_TMPNAMBUFSIZE is the maximum size of a name created by ol_tmpnam.
** CHANGE them if you have an alternative to tmpnam (which is considered
** insecure) or if you want the original tmpnam anyway.  By default, ol
** uses tmpnam except when POSIX is available, where it uses mkstemp.
*/
#if defined(loslib_c) || defined(olall_c)

#if defined(ol_USE_MKSTEMP)
#include <unistd.h>
#define ol_TMPNAMBUFSIZE	32
#define ol_tmpnam(b,e)	{ \
	strcpy(b, "/tmp/ol_XXXXXX"); \
	e = mkstemp(b); \
	if (e != -1) close(e); \
	e = (e == -1); }

#else
#define ol_TMPNAMBUFSIZE	L_tmpnam
#define ol_tmpnam(b,e)		{ e = (tmpnam(b) == NULL); }
#endif

#endif


/*
@@ ol_popen spawns a new process connected to the current one through
@* the file streams.
** CHANGE it if you have a way to implement it in your system.
*/
#if defined(ol_USE_POPEN)

#define ol_popen(L,c,m)	((void)L, fflush(NULL), popen(c,m))
#define ol_pclose(L,file)	((void)L, (pclose(file) != -1))

#elif defined(ol_WIN)

#define ol_popen(L,c,m)	((void)L, _popen(c,m))
#define ol_pclose(L,file)	((void)L, (_pclose(file) != -1))

#else

#define ol_popen(L,c,m)	((void)((void)c, m),  \
		olL_error(L, ol_QL("popen") " not supported"), (FILE*)0)
#define ol_pclose(L,file)		((void)((void)L, file), 0)

#endif

/*
@@ ol_DL_* define which dynamic-library system ol should use.
** CHANGE here if ol has problems choosing the appropriate
** dynamic-library system for your platform (either Windows' DLL, Mac's
** dyld, or Unix's dlopen). If your system is some kind of Unix, there
** is a good chance that it has dlopen, so ol_DL_DLOPEN will work for
** it.  To use dlopen you also need to adapt the src/Makefile (probably
** adding -ldl to the linker options), so ol does not select it
** automatically.  (When you change the makefile to add -ldl, you must
** also add -Dol_USE_DLOPEN.)
** If you do not want any kind of dynamic library, undefine all these
** options.
** By default, _WIN32 gets ol_DL_DLL and MAC OS X gets ol_DL_DYLD.
*/
#if defined(ol_USE_DLOPEN)
#define ol_DL_DLOPEN
#endif

#if defined(ol_WIN)
#define ol_DL_DLL
#endif


/*
@@ olI_EXTRASPACE allows you to add user-specific data in a ol_State
@* (the data goes just *before* the ol_State pointer).
** CHANGE (define) this if you really need that. This value must be
** a multiple of the maximum alignment required for your machine.
*/
#define olI_EXTRASPACE		0


/*
@@ oli_userstate* allow user-specific actions on threads.
** CHANGE them if you defined olI_EXTRASPACE and need to do something
** extra when a thread is created/deleted/resumed/yielded.
*/
#define oli_userstateopen(L)		((void)L)
#define oli_userstateclose(L)		((void)L)
#define oli_userstatethread(L,L1)	((void)L)
#define oli_userstatefree(L)		((void)L)
#define oli_userstateresume(L,n)	((void)L)
#define oli_userstateyield(L,n)	((void)L)


/*
@@ ol_INTFRMLEN is the length modifier for integer conversions
@* in 'string.format'.
@@ ol_INTFRM_T is the integer type correspoding to the previous length
@* modifier.
** CHANGE them if your system supports long long or does not support long.
*/

#if defined(ol_USELONGLONG)

#define ol_INTFRMLEN		"ll"
#define ol_INTFRM_T		long long

#else

#define ol_INTFRMLEN		"l"
#define ol_INTFRM_T		long

#endif



/* =================================================================== */

/*
** Local configuration. You can use this space to add your redefinitions
** without modifying the main part of the file.
*/



#endif


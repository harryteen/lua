/*
** $Id: oloadlib.c,v 1.52.1.3 2008/08/06 13:29:28 roberto Exp $
** Dynamic library loader for ol
** See Copyright Notice in ol.h
**
** This module contains an implementation of loadlib for Unix systems
** that have dlfcn, an implementation for Darwin (Mac OS X), an
** implementation for Windows, and a stub for other systems.
*/


#include <stdlib.h>
#include <string.h>


#define oloadlib_c
#define ol_LIB

#include "ol.h"

#include "olauxlib.h"
#include "ollib.h"


/* prefix for open functions in C libraries */
#define ol_POF		"olopen_"

/* separator for open functions in C libraries */
#define ol_OFSEP	"_"


#define LIBPREFIX	"LOADLIB: "

#define POF		ol_POF
#define LIB_FAIL	"open"


/* error codes for ll_loadfunc */
#define ERRLIB		1
#define ERRFUNC		2

static void ll_unloadlib (void *lib);
static void *ll_load (ol_State *L, const char *path);
static ol_CFunction ll_sym (ol_State *L, void *lib, const char *sym);
static void setprogdir (ol_State *L);

/*
** {=========================================================================
** This determines the location of the executable for relative module loading
** Modified by the olDist project for UNIX platforms
** ==========================================================================
*/
#if defined(_WIN32)
  #include <windows.h>
  #define _PATH_MAX MAX_PATH
#else
  #define _PATH_MAX PATH_MAX
#endif

#if defined(__CYGWIN__)
  #include <sys/cygwin.h>
#endif

#if defined(__linux__) || defined(__sun)
  #include <unistd.h> /* readlink */
#endif

#if defined(__APPLE__)
  #include <sys/param.h>
  #include <mach-o/dyld.h>
#endif

#if defined(__FreeBSD__)
  #include <sys/types.h>
  #include <sys/sysctl.h>
#endif

static void setprogdir(ol_State *L) {
  char progdir[_PATH_MAX + 1];
  char *lb;
  int nsize = sizeof(progdir)/sizeof(char);
  int n = 0;
#if defined(__CYGWIN__)
  char win_buff[_PATH_MAX + 1];
  GetModuleFileNameA(NULL, win_buff, nsize);
  cygwin_conv_path (CCP_WIN_A_TO_POSIX, win_buff, progdir, nsize);
  n = strlen(progdir);
#elif defined(_WIN32)
  n = GetModuleFileNameA(NULL, progdir, nsize);
#elif defined(__linux__)
  n = readlink("/proc/self/exe", progdir, nsize);
  if (n > 0) progdir[n] = 0;
#elif defined(__sun)
  pid_t pid = getpid();
  char linkname[256];
  sprintf(linkname, "/proc/%d/path/a.out", pid);
  n = readlink(linkname, progdir, nsize);
  if (n > 0) progdir[n] = 0;  
#elif defined(__FreeBSD__)
  int mib[4];
  mib[0] = CTL_KERN;
  mib[1] = KERN_PROC;
  mib[2] = KERN_PROC_PATHNAME;
  mib[3] = -1;
  size_t cb = nsize;
  sysctl(mib, 4, progdir, &cb, NULL, 0);
  n = cb;
#elif defined(__BSD__)
  n = readlink("/proc/curproc/file", progdir, nsize);
  if (n > 0) progdir[n] = 0;
#elif defined(__APPLE__)
  uint32_t nsize_apple = nsize;
  if (_NSGetExecutablePath(progdir, &nsize_apple) == 0)
    n = strlen(progdir);
#else
  // FALLBACK
  // Use 'lsof' ... should work on most UNIX systems (incl. OSX)
  // lsof will list open files, this captures the 1st file listed (usually the executable)
  int pid;
  FILE* fd;
  char cmd[80];
  pid = getpid();

  sprintf(cmd, "lsof -p %d | awk '{if ($5==\"REG\") { print $9 ; exit}}' 2> /dev/null", pid);
  fd = popen(cmd, "r");
  n = fread(progdir, 1, nsize, fd);
  pclose(fd);

  // remove newline
  if (n > 1) progdir[--n] = '\0';
#endif
  if (n == 0 || n == nsize || (lb = strrchr(progdir, (int)ol_DIRSEP[0])) == NULL)
    olL_error(L, "OLang.EXEError: unable to get process executable path");
  else {
    *lb = '\0';
    
    // Replace the relative path placeholder
    olL_gsub(L, ol_tostring(L, -1), ol_EXECDIR, progdir);
    ol_remove(L, -2);
  }
}

/* }====================================================== */

#if defined(ol_DL_DLOPEN)
/*
** {========================================================================
** This is an implementation of loadlib based on the dlfcn interface.
** The dlfcn interface is available in Linux, SunOS, Solaris, IRIX, FreeBSD,
** NetBSD, AIX 4.2, HPUX 11, and  probably most other Unix flavors, at least
** as an emulation layer on top of native functions.
** =========================================================================
*/

#include <dlfcn.h>
#include <sys/stat.h> 

static void ll_unloadlib (void *lib) {
  dlclose(lib);
}

static void *ll_load (ol_State *L, const char *path) {
  void *lib = dlopen(path, RTLD_NOW);
  if (lib == NULL) ol_pushstring(L, dlerror());
  return lib;
}


static ol_CFunction ll_sym (ol_State *L, void *lib, const char *sym) {
  ol_CFunction f = (ol_CFunction)dlsym(lib, sym);
  if (f == NULL) ol_pushstring(L, dlerror());
  return f;
}

/* }====================================================== */



#elif defined(ol_DL_DLL)
/*
** {======================================================================
** This is an implementation of loadlib for Windows using native functions.
** =======================================================================
*/

#include <windows.h>

static void pusherror (ol_State *L) {
  int error = GetLastError();
  char buffer[128];
  if (FormatMessageA(FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
      NULL, error, 0, buffer, sizeof(buffer), NULL))
    ol_pushstring(L, buffer);
  else
    ol_pushfstring(L, "OLang.SyntaxError: system error %d\n", error);
}

static void ll_unloadlib (void *lib) {
  FreeLibrary((HINSTANCE)lib);
}


static void *ll_load (ol_State *L, const char *path) {
  HINSTANCE lib = LoadLibraryA(path);
  if (lib == NULL) pusherror(L);
  return lib;
}


static ol_CFunction ll_sym (ol_State *L, void *lib, const char *sym) {
  ol_CFunction f = (ol_CFunction)GetProcAddress((HINSTANCE)lib, sym);
  if (f == NULL) pusherror(L);
  return f;
}

/* }====================================================== */



#elif defined(ol_DL_DYLD)
/*
** {======================================================================
** Native Mac OS X / Darwin Implementation
** =======================================================================
*/

#include <mach-o/dyld.h>


/* Mac appends a `_' before C function names */
#undef POF
#define POF	"_" ol_POF


static void pusherror (ol_State *L) {
  const char *err_str;
  const char *err_file;
  NSLinkEditErrors err;
  int err_num;
  NSLinkEditError(&err, &err_num, &err_file, &err_str);
  ol_pushstring(L, err_str);
}


static const char *errorfromcode (NSObjectFileImageReturnCode ret) {
  switch (ret) {
    case NSObjectFileImageInappropriateFile:
      return "OLang.FileError: file is not a bundle";
    case NSObjectFileImageArch:
      return "OLang.LIBError: library is for wrong CPU type";
    case NSObjectFileImageFormat:
      return "OLang.FormatError: bad format";
    case NSObjectFileImageAccess:
      return "OLang.FileError: cannot access file";
    case NSObjectFileImageFailure:
    default:
      return "OLang.LIBError: unable to load library";
  }
}


static void ll_unloadlib (void *lib) {
  NSUnLinkModule((NSModule)lib, NSUNLINKMODULE_OPTION_RESET_LAZY_REFERENCES);
}


static void *ll_load (ol_State *L, const char *path) {
  NSObjectFileImage img;
  NSObjectFileImageReturnCode ret;
  /* this would be a rare case, but prevents crashing if it happens */
  if(!_dyld_present()) {
    ol_pushliteral(L, "dyld not present");
    return NULL;
  }
  ret = NSCreateObjectFileImageFromFile(path, &img);
  if (ret == NSObjectFileImageSuccess) {
    NSModule mod = NSLinkModule(img, path, NSLINKMODULE_OPTION_PRIVATE |
                       NSLINKMODULE_OPTION_RETURN_ON_ERROR);
    NSDestroyObjectFileImage(img);
    if (mod == NULL) pusherror(L);
    return mod;
  }
  ol_pushstring(L, errorfromcode(ret));
  return NULL;
}


static ol_CFunction ll_sym (ol_State *L, void *lib, const char *sym) {
  NSSymbol nss = NSLookupSymbolInModule((NSModule)lib, sym);
  if (nss == NULL) {
    ol_pushfstring(L, "symbol " ol_QS " not found", sym);
    return NULL;
  }
  return (ol_CFunction)NSAddressOfSymbol(nss);
}

/* }====================================================== */



#else
/*
** {======================================================
** Fallback for other systems
** =======================================================
*/

#undef LIB_FAIL
#define LIB_FAIL	"absent"


#define DLMSG	"dynamic libraries not enabled; check your ol installation"


static void ll_unloadlib (void *lib) {
  (void)lib;  /* to avoid warnings */
}


static void *ll_load (ol_State *L, const char *path) {
  (void)path;  /* to avoid warnings */
  ol_pushliteral(L, DLMSG);
  return NULL;
}


static ol_CFunction ll_sym (ol_State *L, void *lib, const char *sym) {
  (void)lib; (void)sym;  /* to avoid warnings */
  ol_pushliteral(L, DLMSG);
  return NULL;
}

/* }====================================================== */
#endif



static void **ll_register (ol_State *L, const char *path) {
  void **plib;
  ol_pushfstring(L, "%s%s", LIBPREFIX, path);
  ol_gettable(L, ol_REGISTRYINDEX);  /* check library in registry? */
  if (!ol_isnil(L, -1))  /* is there an entry? */
    plib = (void **)ol_touserdata(L, -1);
  else {  /* no entry yet; create one */
    ol_pop(L, 1);
    plib = (void **)ol_newuserdata(L, sizeof(const void *));
    *plib = NULL;
    olL_getmetatable(L, "_LOADLIB");
    ol_setmetatable(L, -2);
    ol_pushfstring(L, "%s%s", LIBPREFIX, path);
    ol_pushvalue(L, -2);
    ol_settable(L, ol_REGISTRYINDEX);
  }
  return plib;
}


/*
** __gc tag method: calls library's `ll_unloadlib' function with the lib
** handle
*/
static int gctm (ol_State *L) {
  void **lib = (void **)olL_checkudata(L, 1, "_LOADLIB");
  if (*lib) ll_unloadlib(*lib);
  *lib = NULL;  /* mark library as closed */
  return 0;
}


static int ll_loadfunc (ol_State *L, const char *path, const char *sym) {
  void **reg = ll_register(L, path);
  if (*reg == NULL) *reg = ll_load(L, path);
  if (*reg == NULL)
    return ERRLIB;  /* unable to load library */
  else {
    ol_CFunction f = ll_sym(L, *reg, sym);
    if (f == NULL)
      return ERRFUNC;  /* unable to find function */
    ol_pushcfunction(L, f);
    return 0;  /* return function */
  }
}


static int ll_loadlib (ol_State *L) {
  const char *path = olL_checkstring(L, 1);
  const char *init = olL_checkstring(L, 2);
  int stat = ll_loadfunc(L, path, init);
  if (stat == 0)  /* no errors? */
    return 1;  /* return the loaded function */
  else {  /* error; error message is on stack top */
    ol_pushnil(L);
    ol_insert(L, -2);
    ol_pushstring(L, (stat == ERRLIB) ?  LIB_FAIL : "init");
    return 3;  /* return nil, error message, and where */
  }
}



/*
** {======================================================
** 'require' function
** =======================================================
*/


static int readable (const char *filename) {
  FILE *f = fopen(filename, "r");  /* try to open file */
  if (f == NULL) return 0;  /* open failed */
  fclose(f);
  return 1;
}


static const char *pushnexttemplate (ol_State *L, const char *path) {
  const char *l;
  while (*path == *ol_PATHSEP) path++;  /* skip separators */
  if (*path == '\0') return NULL;  /* no more templates */
  l = strchr(path, *ol_PATHSEP);  /* find next separator */
  if (l == NULL) l = path + strlen(path);
  ol_pushlstring(L, path, l - path);  /* template */
  return l;
}


static const char *findfile (ol_State *L, const char *name,
                                           const char *pname) {
  const char *path;
  name = olL_gsub(L, name, ".", ol_DIRSEP);
  ol_getfield(L, ol_ENVIRONINDEX, pname);
  path = ol_tostring(L, -1);
  if (path == NULL)
    olL_error(L, ol_QL("package.%s") " must be a string", pname);
  ol_pushliteral(L, "");  /* error accumulator */
  while ((path = pushnexttemplate(L, path)) != NULL) {
    const char *filename;
    filename = olL_gsub(L, ol_tostring(L, -1), ol_PATH_MARK, name);
    ol_remove(L, -2);  /* remove path template */
    if (readable(filename))  /* does file exist and is readable? */
      return filename;  /* return that file name */
    ol_pushfstring(L, "\n\tno file " ol_QS, filename);
    ol_remove(L, -2);  /* remove file name */
    ol_concat(L, 2);  /* add entry to possible error message */
  }
  return NULL;  /* not found */
}


static void loaderror (ol_State *L, const char *filename) {
  olL_error(L, "error loading module " ol_QS " from file " ol_QS ":\n\t%s",
                ol_tostring(L, 1), filename, ol_tostring(L, -1));
}


static int loader_ol (ol_State *L) {
  const char *filename;
  const char *name = olL_checkstring(L, 1);
  filename = findfile(L, name, "path");
  if (filename == NULL) return 1;  /* library not found in this path */
  if (olL_loadfile(L, filename) != 0)
    loaderror(L, filename);
  return 1;  /* library loaded successfully */
}


static const char *mkfuncname (ol_State *L, const char *modname) {
  const char *funcname;
  const char *mark = strchr(modname, *ol_IGMARK);
  if (mark) modname = mark + 1;
  funcname = olL_gsub(L, modname, ".", ol_OFSEP);
  funcname = ol_pushfstring(L, POF"%s", funcname);
  ol_remove(L, -2);  /* remove 'gsub' result */
  return funcname;
}


static int loader_C (ol_State *L) {
  const char *funcname;
  const char *name = olL_checkstring(L, 1);
  const char *filename = findfile(L, name, "cpath");
  if (filename == NULL) return 1;  /* library not found in this path */
  funcname = mkfuncname(L, name);
  if (ll_loadfunc(L, filename, funcname) != 0)
    loaderror(L, filename);
  return 1;  /* library loaded successfully */
}


static int loader_Croot (ol_State *L) {
  const char *funcname;
  const char *filename;
  const char *name = olL_checkstring(L, 1);
  const char *p = strchr(name, '.');
  int stat;
  if (p == NULL) return 0;  /* is root */
  ol_pushlstring(L, name, p - name);
  filename = findfile(L, ol_tostring(L, -1), "cpath");
  if (filename == NULL) return 1;  /* root not found */
  funcname = mkfuncname(L, name);
  if ((stat = ll_loadfunc(L, filename, funcname)) != 0) {
    if (stat != ERRFUNC) loaderror(L, filename);  /* real error */
    ol_pushfstring(L, "\n\tno module " ol_QS " in file " ol_QS,
                       name, filename);
    return 1;  /* function not found */
  }
  return 1;
}


static int loader_preload (ol_State *L) {
  const char *name = olL_checkstring(L, 1);
  ol_getfield(L, ol_ENVIRONINDEX, "preload");
  if (!ol_istable(L, -1))
    olL_error(L, ol_QL("package.preload") " must be a table");
  ol_getfield(L, -1, name);
  if (ol_isnil(L, -1))  /* not found? */
    ol_pushfstring(L, "\n\tno field package.preload['%s']", name);
  return 1;
}


static const int sentinel_ = 0;
#define sentinel	((void *)&sentinel_)


static int ll_require (ol_State *L) {
  const char *name = olL_checkstring(L, 1);
  int i;
  ol_settop(L, 1);  /* _LOADED table will be at index 2 */
  ol_getfield(L, ol_REGISTRYINDEX, "_LOADED");
  ol_getfield(L, 2, name);
  if (ol_toboolean(L, -1)) {  /* is it there? */
    if (ol_touserdata(L, -1) == sentinel)  /* check loops */
      olL_error(L, "OLang.RequireError: loop or previous error loading module " ol_QS, name);
    return 1;  /* package is already loaded */
  }
  /* else must load it; iterate over available loaders */
  ol_getfield(L, ol_ENVIRONINDEX, "loaders");
  if (!ol_istable(L, -1))
    olL_error(L, ol_QL("package.loaders") " must be a table");
  ol_pushliteral(L, "");  /* error message accumulator */
  for (i=1; ; i++) {
    ol_rawgeti(L, -2, i);  /* get a loader */
    if (ol_isnil(L, -1))
      olL_error(L, "module " ol_QS " not found:%s",
                    name, ol_tostring(L, -2));
    ol_pushstring(L, name);
    ol_call(L, 1, 1);  /* call it */
    if (ol_isfunction(L, -1))  /* did it find module? */
      break;  /* module loaded successfully */
    else if (ol_isstring(L, -1))  /* loader returned error message? */
      ol_concat(L, 2);  /* accumulate it */
    else
      ol_pop(L, 1);
  }
  ol_pushlightuserdata(L, sentinel);
  ol_setfield(L, 2, name);  /* _LOADED[name] = sentinel */
  ol_pushstring(L, name);  /* pass name as argument to module */
  ol_call(L, 1, 1);  /* run loaded module */
  if (!ol_isnil(L, -1))  /* non-nil return? */
    ol_setfield(L, 2, name);  /* _LOADED[name] = returned value */
  ol_getfield(L, 2, name);
  if (ol_touserdata(L, -1) == sentinel) {   /* module did not set a value? */
    ol_pushboolean(L, 1);  /* use true as result */
    ol_pushvalue(L, -1);  /* extra copy to be returned */
    ol_setfield(L, 2, name);  /* _LOADED[name] = true */
  }
  return 1;
}

/* }====================================================== */



/*
** {======================================================
** 'module' function
** =======================================================
*/
  

static void setfenv (ol_State *L) {
  ol_Debug ar;
  if (ol_getstack(L, 1, &ar) == 0 ||
      ol_getinfo(L, "f", &ar) == 0 ||  /* get calling function */
      ol_iscfunction(L, -1))
    olL_error(L, ol_QL("module") " not called from a ol function");
  ol_pushvalue(L, -2);
  ol_setfenv(L, -2);
  ol_pop(L, 1);
}


static void dooptions (ol_State *L, int n) {
  int i;
  for (i = 2; i <= n; i++) {
    ol_pushvalue(L, i);  /* get option (a function) */
    ol_pushvalue(L, -2);  /* module */
    ol_call(L, 1, 0);
  }
}


static void modinit (ol_State *L, const char *modname) {
  const char *dot;
  ol_pushvalue(L, -1);
  ol_setfield(L, -2, "_M");  /* module._M = module */
  ol_pushstring(L, modname);
  ol_setfield(L, -2, "_NAME");
  dot = strrchr(modname, '.');  /* look for last dot in module name */
  if (dot == NULL) dot = modname;
  else dot++;
  /* set _PACKAGE as package name (full module name minus last part) */
  ol_pushlstring(L, modname, dot - modname);
  ol_setfield(L, -2, "_PACKAGE");
}


static int ll_module (ol_State *L) {
  const char *modname = olL_checkstring(L, 1);
  int loaded = ol_gettop(L) + 1;  /* index of _LOADED table */
  ol_getfield(L, ol_REGISTRYINDEX, "_LOADED");
  ol_getfield(L, loaded, modname);  /* get _LOADED[modname] */
  if (!ol_istable(L, -1)) {  /* not found? */
    ol_pop(L, 1);  /* remove previous result */
    /* try global variable (and create one if it does not exist) */
    if (olL_findtable(L, ol_GLOBALSINDEX, modname, 1) != NULL)
      return olL_error(L, "name conflict for module " ol_QS, modname);
    ol_pushvalue(L, -1);
    ol_setfield(L, loaded, modname);  /* _LOADED[modname] = new table */
  }
  /* check whether table already has a _NAME field */
  ol_getfield(L, -1, "_NAME");
  if (!ol_isnil(L, -1))  /* is table an initialized module? */
    ol_pop(L, 1);
  else {  /* no; initialize it */
    ol_pop(L, 1);
    modinit(L, modname);
  }
  ol_pushvalue(L, -1);
  setfenv(L);
  dooptions(L, loaded - 1);
  return 0;
}


static int ll_seeall (ol_State *L) {
  olL_checktype(L, 1, ol_TTABLE);
  if (!ol_getmetatable(L, 1)) {
    ol_createtable(L, 0, 1); /* create new metatable */
    ol_pushvalue(L, -1);
    ol_setmetatable(L, 1);
  }
  ol_pushvalue(L, ol_GLOBALSINDEX);
  ol_setfield(L, -2, "__index");  /* mt.__index = _G */
  return 0;
}


/* }====================================================== */



/* auxiliary mark (for internal use) */
#define AUXMARK		"\1"

static void setpath (ol_State *L, const char *fieldname, const char *envname,
                                   const char *def) {
  const char *path = getenv(envname);
  if (path == NULL)  /* no environment variable? */
    ol_pushstring(L, def);  /* use default */
  else {
    /* replace ";;" by ";AUXMARK;" and then AUXMARK by default path */
    path = olL_gsub(L, path, ol_PATHSEP ol_PATHSEP,
                              ol_PATHSEP AUXMARK ol_PATHSEP);
    olL_gsub(L, path, AUXMARK, def);
    ol_remove(L, -2);
  }
  setprogdir(L);
  ol_setfield(L, -2, fieldname);
}


static const olL_Reg pk_funcs[] = {
  {"loadlib", ll_loadlib},
  {"seeall", ll_seeall},
  {NULL, NULL}
};


static const olL_Reg ll_funcs[] = {
  {"module", ll_module},
  {"require", ll_require},
  {NULL, NULL}
};


static const ol_CFunction loaders[] =
  {loader_preload, loader_ol, loader_C, loader_Croot, NULL};


olLIB_API int olopen_package (ol_State *L) {
  int i;
  /* create new type _LOADLIB */
  olL_newmetatable(L, "_LOADLIB");
  ol_pushcfunction(L, gctm);
  ol_setfield(L, -2, "__gc");
  /* create `package' table */
  olL_register(L, ol_LOADLIBNAME, pk_funcs);
#if defined(ol_COMPAT_LOADLIB) 
  ol_getfield(L, -1, "loadlib");
  ol_setfield(L, ol_GLOBALSINDEX, "loadlib");
#endif
  ol_pushvalue(L, -1);
  ol_replace(L, ol_ENVIRONINDEX);
  /* create `loaders' table */
  ol_createtable(L, sizeof(loaders)/sizeof(loaders[0]) - 1, 0);
  /* fill it with pre-defined loaders */
  for (i=0; loaders[i] != NULL; i++) {
    ol_pushcfunction(L, loaders[i]);
    ol_rawseti(L, -2, i+1);
  }
  ol_setfield(L, -2, "loaders");  /* put it in field `loaders' */
  setpath(L, "path", ol_PATH, ol_PATH_DEFAULT);  /* set field `path' */
  setpath(L, "cpath", ol_CPATH, ol_CPATH_DEFAULT); /* set field `cpath' */
  /* store config information */
  ol_pushliteral(L, ol_DIRSEP "\n" ol_PATHSEP "\n" ol_PATH_MARK "\n"
                     ol_EXECDIR "\n" ol_IGMARK);
  ol_setfield(L, -2, "config");
  /* set field `loaded' */
  olL_findtable(L, ol_REGISTRYINDEX, "_LOADED", 2);
  ol_setfield(L, -2, "loaded");
  /* set field `preload' */
  ol_newtable(L);
  ol_setfield(L, -2, "preload");
  ol_pushvalue(L, ol_GLOBALSINDEX);
  olL_register(L, NULL, ll_funcs);  /* open lib into global table */
  ol_pop(L, 1);
  return 1;  /* return 'package' table */
}


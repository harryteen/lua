/*
** $Id: ol.c,v 1.160.1.2 2007/12/28 15:32:23 roberto Exp $
** ol stand-alone interpreter
** See Copyright Notice in ol.h
*/


#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ol_c

#include "ol.h"

#include "olauxlib.h"
#include "ollib.h"



static ol_State *globalL = NULL;

static const char *progname = ol_PROGNAME;



static void lstop (ol_State *L, ol_Debug *ar) {
  (void)ar;  /* unused arg. */
  ol_sethook(L, NULL, 0, 0);
  olL_error(L, "OLang.InterruptedError: interrupted!");
}


static void laction (int i) {
  signal(i, SIG_DFL); /* if another SIGINT happens before lstop,
                              terminate process (default action) */
  ol_sethook(globalL, lstop, ol_MASKCALL | ol_MASKRET | ol_MASKCOUNT, 1);
}


static void print_usage (void) {
  fprintf(stderr,
  "Usage: %s [Options] [Script [Args]].\n"
  "Available options are:\n"
  "  -e stat  execute string " ol_QL("stat") "\n"
  "  -l name  require library " ol_QL("name") "\n"
  "  -i       enter interactive mode after executing " ol_QL("script") "\n"
  "  -v       show version information\n"
  "  --       stop handling options\n"
  "  -        execute stdin and stop handling options\n"
  ,
  progname);
  fflush(stderr);
}


static void l_message (const char *pname, const char *msg) {
  if (pname) fprintf(stderr, "%s: ", pname);
  fprintf(stderr, "%s\n", msg);
  fflush(stderr);
}


static int report (ol_State *L, int status) {
  if (status && !ol_isnil(L, -1)) {
    const char *msg = ol_tostring(L, -1);
    if (msg == NULL) msg = "(OLang.ObjectError:  object is not a string)";
    l_message(progname, msg);
    ol_pop(L, 1);
  }
  return status;
}


static int traceback (ol_State *L) {
  if (!ol_isstring(L, 1))  /* 'message' not a string? */
    return 1;  /* keep it intact */
  ol_getfield(L, ol_GLOBALSINDEX, "debug");
  if (!ol_istable(L, -1)) {
    ol_pop(L, 1);
    return 1;
  }
  ol_getfield(L, -1, "traceback");
  if (!ol_isfunction(L, -1)) {
    ol_pop(L, 2);
    return 1;
  }
  ol_pushvalue(L, 1);  /* pass error message */
  ol_pushinteger(L, 2);  /* skip this function and traceback */
  ol_call(L, 2, 1);  /* call debug.traceback */
  return 1;
}


static int docall (ol_State *L, int narg, int clear) {
  int status;
  int base = ol_gettop(L) - narg;  /* function index */
  ol_pushcfunction(L, traceback);  /* push traceback function */
  ol_insert(L, base);  /* put it under chunk and args */
  signal(SIGINT, laction);
  status = ol_pcall(L, narg, (clear ? 0 : ol_MULTRET), base);
  signal(SIGINT, SIG_DFL);
  ol_remove(L, base);  /* remove traceback function */
  /* force a complete garbage collection in case of errors */
  if (status != 0) ol_gc(L, ol_GCCOLLECT, 0);
  return status;
}


static void print_version (void) {
  l_message(NULL, ol_RELEASE "  " ol_COPYRIGHT);
}


static int getargs (ol_State *L, char **argv, int n) {
  int narg;
  int i;
  int argc = 0;
  while (argv[argc]) argc++;  /* count total number of arguments */
  narg = argc - (n + 1);  /* number of arguments to the script */
  olL_checkstack(L, narg + 3, "OLang.ArgumentsError:  too many arguments to script");
  for (i=n+1; i < argc; i++)
    ol_pushstring(L, argv[i]);
  ol_createtable(L, narg, n + 1);
  for (i=0; i < argc; i++) {
    ol_pushstring(L, argv[i]);
    ol_rawseti(L, -2, i - n);
  }
  return narg;
}


static int dofile (ol_State *L, const char *name) {
  int status = olL_loadfile(L, name) || docall(L, 0, 1);
  return report(L, status);
}


static int dostring (ol_State *L, const char *s, const char *name) {
  int status = olL_loadbuffer(L, s, strlen(s), name) || docall(L, 0, 1);
  return report(L, status);
}


static int dolibrary (ol_State *L, const char *name) {
  ol_getglobal(L, "require");
  ol_pushstring(L, name);
  return report(L, docall(L, 1, 1));
}


static const char *get_prompt (ol_State *L, int firstline) {
  const char *p;
  ol_getfield(L, ol_GLOBALSINDEX, firstline ? "_PROMPT" : "_PROMPT2");
  p = ol_tostring(L, -1);
  if (p == NULL) p = (firstline ? ol_PROMPT : ol_PROMPT2);
  ol_pop(L, 1);  /* remove global */
  return p;
}


static int incomplete (ol_State *L, int status) {
  if (status == ol_ERRSYNTAX) {
    size_t lmsg;
    const char *msg = ol_tolstring(L, -1, &lmsg);
    const char *tp = msg + lmsg - (sizeof(ol_QL("<eof>")) - 1);
    if (strstr(msg, ol_QL("<eof>")) == tp) {
      ol_pop(L, 1);
      return 1;
    }
  }
  return 0;  /* else... */
}


static int pushline (ol_State *L, int firstline) {
  char buffer[ol_MAXINPUT];
  char *b = buffer;
  size_t l;
  const char *prmt = get_prompt(L, firstline);
  if (ol_readline(L, b, prmt) == 0)
    return 0;  /* no input */
  l = strlen(b);
  if (l > 0 && b[l-1] == '\n')  /* line ends with newline? */
    b[l-1] = '\0';  /* remove it */
  if (firstline && b[0] == '=')  /* first line starts with `=' ? */
    ol_pushfstring(L, "return %s", b+1);  /* change it to `return' */
  else
    ol_pushstring(L, b);
  ol_freeline(L, b);
  return 1;
}


static int loadline (ol_State *L) {
  int status;
  ol_settop(L, 0);
  if (!pushline(L, 1))
    return -1;  /* no input */
  for (;;) {  /* repeat until gets a complete line */
    status = olL_loadbuffer(L, ol_tostring(L, 1), ol_strlen(L, 1), "=stdin");
    if (!incomplete(L, status)) break;  /* cannot try to add lines? */
    if (!pushline(L, 0))  /* no more input? */
      return -1;
    ol_pushliteral(L, "\n");  /* add a new line... */
    ol_insert(L, -2);  /* ...between the two lines */
    ol_concat(L, 3);  /* join them */
  }
  ol_saveline(L, 1);
  ol_remove(L, 1);  /* remove line */
  return status;
}


static void dotty (ol_State *L) {
  int status;
  const char *oldprogname = progname;
  progname = NULL;
  while ((status = loadline(L)) != -1) {
    if (status == 0) status = docall(L, 0, 0);
    report(L, status);
    if (status == 0 && ol_gettop(L) > 0) {  /* any result to print? */
      ol_getglobal(L, "print");
      ol_insert(L, 1);
      if (ol_pcall(L, ol_gettop(L)-1, 0, 0) != 0)
        l_message(progname, ol_pushfstring(L,
                               "OLang.PrintError:   calling " ol_QL("print") " (%s)",
                               ol_tostring(L, -1)));
    }
  }
  ol_settop(L, 0);  /* clear stack */
  fputs("\n", stdout);
  fflush(stdout);
  progname = oldprogname;
}


static int handle_script (ol_State *L, char **argv, int n) {
  int status;
  const char *fname;
  int narg = getargs(L, argv, n);  /* collect arguments */
  ol_setglobal(L, "arg");
  fname = argv[n];
  if (strcmp(fname, "-") == 0 && strcmp(argv[n-1], "--") != 0) 
    fname = NULL;  /* stdin */
  status = olL_loadfile(L, fname);
  ol_insert(L, -(narg+1));
  if (status == 0)
    status = docall(L, narg, 0);
  else
    ol_pop(L, narg);      
  return report(L, status);
}


/* check that argument has no extra characters at the end */
#define notail(x)	{if ((x)[2] != '\0') return -1;}


static int collectargs (char **argv, int *pi, int *pv, int *pe) {
  int i;
  for (i = 1; argv[i] != NULL; i++) {
    if (argv[i][0] != '-')  /* not an option? */
        return i;
    switch (argv[i][1]) {  /* option */
      case '-':
        notail(argv[i]);
        return (argv[i+1] != NULL ? i+1 : 0);
      case '\0':
        return i;
      case 'i':
        notail(argv[i]);
        *pi = 1;  /* go through */
      case 'v':
        notail(argv[i]);
        *pv = 1;
        break;
      case 'e':
        *pe = 1;  /* go through */
      case 'l':
        if (argv[i][2] == '\0') {
          i++;
          if (argv[i] == NULL) return -1;
        }
        break;
      default: return -1;  /* invalid option */
    }
  }
  return 0;
}


static int runargs (ol_State *L, char **argv, int n) {
  int i;
  for (i = 1; i < n; i++) {
    if (argv[i] == NULL) continue;
    ol_assert(argv[i][0] == '-');
    switch (argv[i][1]) {  /* option */
      case 'e': {
        const char *chunk = argv[i] + 2;
        if (*chunk == '\0') chunk = argv[++i];
        ol_assert(chunk != NULL);
        if (dostring(L, chunk, "=(command line)") != 0)
          return 1;
        break;
      }
      case 'l': {
        const char *filename = argv[i] + 2;
        if (*filename == '\0') filename = argv[++i];
        ol_assert(filename != NULL);
        if (dolibrary(L, filename))
          return 1;  /* stop if file fails */
        break;
      }
      default: break;
    }
  }
  return 0;
}


static int handle_olinit (ol_State *L) {
  const char *init = getenv(ol_INIT);
  if (init == NULL) return 0;  /* status OK */
  else if (init[0] == '@')
    return dofile(L, init+1);
  else
    return dostring(L, init, "=" ol_INIT);
}


struct Smain {
  int argc;
  char **argv;
  int status;
};


static int pmain (ol_State *L) {
  struct Smain *s = (struct Smain *)ol_touserdata(L, 1);
  char **argv = s->argv;
  int script;
  int has_i = 0, has_v = 0, has_e = 0;
  globalL = L;
  if (argv[0] && argv[0][0]) progname = argv[0];
  ol_gc(L, ol_GCSTOP, 0);  /* stop collector during initialization */
  olL_openlibs(L);  /* open libraries */
  ol_gc(L, ol_GCRESTART, 0);
  s->status = handle_olinit(L);
  if (s->status != 0) return 0;
  script = collectargs(argv, &has_i, &has_v, &has_e);
  if (script < 0) {  /* invalid args? */
    print_usage();
    s->status = 1;
    return 0;
  }
  if (has_v) print_version();
  s->status = runargs(L, argv, (script > 0) ? script : s->argc);
  if (s->status != 0) return 0;
  if (script)
    s->status = handle_script(L, argv, script);
  if (s->status != 0) return 0;
  if (has_i)
    dotty(L);
  else if (script == 0 && !has_e && !has_v) {
    if (ol_stdin_is_tty()) {
      print_version();
      dotty(L);
    }
    else dofile(L, NULL);  /* executes stdin as a file */
  }
  return 0;
}


int main (int argc, char **argv) {
  int status;
  struct Smain s;
  ol_State *L = ol_open();  /* create state */
  if (L == NULL) {
    l_message(argv[0], "OLang.StateError:   cannot create state: not enough memory");
    return EXIT_FAILURE;
  }
  s.argc = argc;
  s.argv = argv;
  status = ol_cpcall(L, &pmain, &s);
  report(L, status);
  ol_close(L);
  return (status || s.status) ? EXIT_FAILURE : EXIT_SUCCESS;
}


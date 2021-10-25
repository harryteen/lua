/*
** $Id: ltm.c,v 2.8.1.1 2007/12/27 13:02:25 roberto Exp $
** Tag methods
** See Copyright Notice in ol.h
*/


#include <string.h>

#define ltm_c
#define ol_CORE

#include "ol.h"

#include "olobject.h"
#include "olstate.h"
#include "olstring.h"
#include "oltable.h"
#include "oltm.h"



const char *const olT_typenames[] = {
  "nil", "boolean", "userdata", "number",
  "string", "table", "function", "userdata", "thread",
  "proto", "upval"
};


void olT_init (ol_State *L) {
  static const char *const olT_eventname[] = {  /* ORDER TM */
    "__index", "__newindex",
    "__gc", "__mode", "__eq",
    "__add", "__sub", "__mul", "__div", "__mod",
    "__pow", "__unm", "__len", "__lt", "__le",
    "__concat", "__call"
  };
  int i;
  for (i=0; i<TM_N; i++) {
    G(L)->tmname[i] = olS_new(L, olT_eventname[i]);
    olS_fix(G(L)->tmname[i]);  /* never collect these names */
  }
}


/*
** function to be used with macro "fasttm": optimized for absence of
** tag methods
*/
const TValue *olT_gettm (Table *events, TMS event, TString *ename) {
  const TValue *tm = olH_getstr(events, ename);
  ol_assert(event <= TM_EQ);
  if (ttisnil(tm)) {  /* no tag method? */
    events->flags |= cast_byte(1u<<event);  /* cache this fact */
    return NULL;
  }
  else return tm;
}


const TValue *olT_gettmbyobj (ol_State *L, const TValue *o, TMS event) {
  Table *mt;
  switch (ttype(o)) {
    case ol_TTABLE:
      mt = hvalue(o)->metatable;
      break;
    case ol_TUSERDATA:
      mt = uvalue(o)->metatable;
      break;
    default:
      mt = G(L)->mt[ttype(o)];
  }
  return (mt ? olH_getstr(mt, G(L)->tmname[event]) : olO_nilobject);
}


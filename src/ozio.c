/*
** $Id: olzio.c,v 1.31.1.1 2007/12/27 13:02:25 roberto Exp $
** a generic input stream interface
** See Copyright Notice in ol.h
*/


#include <string.h>

#define olzio_c
#define ol_CORE

#include "ol.h"

#include "ollimits.h"
#include "olmem.h"
#include "olstate.h"
#include "ozio.h"


int olZ_fill (ZIO *z) {
  size_t size;
  ol_State *L = z->L;
  const char *buff;
  ol_unlock(L);
  buff = z->reader(L, z->data, &size);
  ol_lock(L);
  if (buff == NULL || size == 0) return EOZ;
  z->n = size - 1;
  z->p = buff;
  return char2int(*(z->p++));
}


int olZ_lookahead (ZIO *z) {
  if (z->n == 0) {
    if (olZ_fill(z) == EOZ)
      return EOZ;
    else {
      z->n++;  /* olZ_fill removed first byte; put back it */
      z->p--;
    }
  }
  return char2int(*z->p);
}


void olZ_init (ol_State *L, ZIO *z, ol_Reader reader, void *data) {
  z->L = L;
  z->reader = reader;
  z->data = data;
  z->n = 0;
  z->p = NULL;
}


/* --------------------------------------------------------------- read --- */
size_t olZ_read (ZIO *z, void *b, size_t n) {
  while (n) {
    size_t m;
    if (olZ_lookahead(z) == EOZ)
      return n;  /* return number of missing bytes */
    m = (n <= z->n) ? n : z->n;  /* min. between n and z->n */
    memcpy(b, z->p, m);
    z->n -= m;
    z->p += m;
    b = (char *)b + m;
    n -= m;
  }
  return 0;
}

/* ------------------------------------------------------------------------ */
char *olZ_openspace (ol_State *L, Mbuffer *buff, size_t n) {
  if (n > buff->buffsize) {
    if (n < ol_MINBUFFER) n = ol_MINBUFFER;
    olZ_resizebuffer(L, buff, n);
  }
  return buff->buffer;
}



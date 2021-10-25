/*
** $Id: olzio.h,v 1.21.1.1 2007/12/27 13:02:25 roberto Exp $
** Buffered streams
** See Copyright Notice in ol.h
*/


#ifndef olzio_h
#define olzio_h

#include "ol.h"

#include "olmem.h"


#define EOZ	(-1)			/* end of stream */

typedef struct Zio ZIO;

#define char2int(c)	cast(int, cast(unsigned char, (c)))

#define zgetc(z)  (((z)->n--)>0 ?  char2int(*(z)->p++) : olZ_fill(z))

typedef struct Mbuffer {
  char *buffer;
  size_t n;
  size_t buffsize;
} Mbuffer;

#define olZ_initbuffer(L, buff) ((buff)->buffer = NULL, (buff)->buffsize = 0)

#define olZ_buffer(buff)	((buff)->buffer)
#define olZ_sizebuffer(buff)	((buff)->buffsize)
#define olZ_bufflen(buff)	((buff)->n)

#define olZ_resetbuffer(buff) ((buff)->n = 0)


#define olZ_resizebuffer(L, buff, size) \
	(olM_reallocvector(L, (buff)->buffer, (buff)->buffsize, size, char), \
	(buff)->buffsize = size)

#define olZ_freebuffer(L, buff)	olZ_resizebuffer(L, buff, 0)


olI_FUNC char *olZ_openspace (ol_State *L, Mbuffer *buff, size_t n);
olI_FUNC void olZ_init (ol_State *L, ZIO *z, ol_Reader reader,
                                        void *data);
olI_FUNC size_t olZ_read (ZIO* z, void* b, size_t n);	/* read next n bytes */
olI_FUNC int olZ_lookahead (ZIO *z);



/* --------- Private Part ------------------ */

struct Zio {
  size_t n;			/* bytes still unread */
  const char *p;		/* current position in buffer */
  ol_Reader reader;
  void* data;			/* additional data */
  ol_State *L;			/* ol state (for reader) */
};


olI_FUNC int olZ_fill (ZIO *z);

#endif

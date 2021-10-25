/*
** $Id: olundump.h,v 1.37.1.1 2007/12/27 13:02:25 roberto Exp $
** load precompiled ol chunks
** See Copyright Notice in ol.h
*/

#ifndef olundump_h
#define olundump_h

#include "olobject.h"
#include "olzio.h"

/* load one chunk; from lundump.c */
olI_FUNC Proto* olU_undump (ol_State* L, ZIO* Z, Mbuffer* buff, const char* name);

/* make header; from lundump.c */
olI_FUNC void olU_header (char* h);

/* dump one chunk; from ldump.c */
olI_FUNC int olU_dump (ol_State* L, const Proto* f, ol_Writer w, void* data, int strip);

#ifdef olc_c
/* print one chunk; from print.c */
olI_FUNC void olU_print (const Proto* f, int full);
#endif

/* for header of binary files -- this is ol 5.1 */
#define olC_VERSION		0x51

/* for header of binary files -- this is the official format */
#define olC_FORMAT		0

/* size of header of binary files */
#define olC_HEADERSIZE		12

#endif

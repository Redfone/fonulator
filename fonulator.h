/*
   fonulator 3 - foneBRIDGE configuration daemon
   (C) 2005-2007 Redfone Communications, LLC.
   www.red-fone.com

   by Brett Carrington <brettcar@gmail.com>

   Global Header File
*/

/** @file fonulator.h 
 *
 * \brief Global Fonulator Include File
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_NETDB_H
# include <netdb.h>
#endif

#ifdef HAVE_LIBFB_FB_LIB_H
# include <libfb/fb_lib.h>
#endif

/** @def DBG 
 * @brief A macro used to hide debugging messages.
 *
 * To enable the debugging code that is blocked off by this macro the
 * preprocessor symbol DEBUG must be defined. The easiest way to do
 * this is to reconfigure the source with:
 *     CFLAGS="-DDEBUG"  ./configure
 */

#ifdef DEBUG
#define DBG(v)  \
  do		\
    {		\
      v;	\
    } while (0)
#else
#define DBG(v)
#endif

#include "dlist.h"
#include "tokens.h"
#include "tree.h"
#include "state.h"
#include "error.h"
#include "status.h"
#include "dsp.h"


#ifdef HAVE_STDIO_H
# include <stdio.h>
#endif

/** ETHER_ADDR_LEN MAC address length in bytes */
#ifndef ETHER_ADDR_LEN
# define ETHER_ADDR_LEN 6
#endif

T_SPAN *get_span (int num);
bool queryFonebridge (libfb_t * f);
int fb_tdmoectl (libfb_t * f, int state);
bool interactiveReboot (libfb_t * f);
bool simpleReboot (libfb_t * f);

/* keys.c */
FB_STATUS program_key (libfb_t * fb, int slotID, KEY_ENTRY * theKey);

/* flash.c */
FB_STATUS write_file_to_flash (libfb_t * f, FILE * bin, int blk);
FB_STATUS flash_set_gpaklen (libfb_t * f, size_t bytes);
void show_warning ();

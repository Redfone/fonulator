/*
   fonulator 3 - foneBRIDGE configuration daemon
   (C) 2005-2007 Redfone Communications, LLC.
   www.red-fone.com

   by Brett Carrington <brettcar@gmail.com>
*/
/** @file
 * Key/License File Routines
 */

#include "fonulator.h"

#if defined(STDC_HEADERS) || defined(HAVE_STDLIB_H)
# include <stdlib.h>
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif



/** @brief Program a single key entry into a device.
 * 
 * @param fb the libfb context for the device
 * @param slotID an integer represented the flash slot ID to write
 * @param theKey pointer to the KEY_ENTRY structure to write
 */
FB_STATUS
program_key (libfb_t * fb, int slotID, KEY_ENTRY * theKey)
{
  fblib_err ret;

  ret = custom_cmd (fb, DOOF_CMD_KEY_WRITE, slotID, (char *)theKey->customer_key, CUSTOMER_KEY_SZ);

  DBG (libfb_fprint_key (stderr, theKey));

  if (ret != E_SUCCESS)
    return E_FBLIB;

  return E_SUCCESS;
}

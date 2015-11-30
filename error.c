/*
   fonulator 3 - foneBRIDGE configuration daemon
   (C) 2005-2007 Redfone Communications, LLC.
   www.red-fone.com

   by Brett Carrington <brettcar@gmail.com>

   Error Handling
*/
/** @file 
 * Error code translation and display 
 */
#include "fonulator.h"


/** @brief return the string representation of an FB_STATUS code 
 *
 * @param the_error the status code
 * @return its string representation
 */
static char *
fberrno_to_string (FB_STATUS the_error)
{
  switch (the_error)
    {
    case E_SUCCESS:
      return "Success";
    case E_BADVALUE:
      return "Bad Parameter Value";
    case E_BADTOKEN:
      return "Bad or Unknown Configuration Token";
    case E_BADSTATE:
      return "Unexpected Token or State";
    case E_DUPLICATE:
      return "Duplicate Token";
    case E_BADINPUT:
      return "Bad Input";
    case E_SYSTEM:
      return "Unknown System Error";
    case E_REBOOTDSP:
      return "DSP Requires foneBRIDGE Reboot";
    case E_FBLIB:
      return "Internal foneBRIDGE library error";

    }
  return "Unknown";
}

/** @brief print an error message for a given error code
 * 
 * @param message the explanitory part of the message
 * @param the_error the error code that occured 
 */
void
fberror (const char *message, FB_STATUS the_error)
{
  printf ("%s: %s\n", message, fberrno_to_string (the_error));
}

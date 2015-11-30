/*
   fonulator 3 - foneBRIDGE configuration daemon
   (C) 2005-2007 Redfone Communications, LLC.
   www.red-fone.com

   by Brett Carrington <brettcar@gmail.com>

   Error Handling Header File
*/

/** @file
 *
 *  Error code definitions
 */

/**
 *  Enumeration of error codes used internally by the fonulator
 *  code. E_SYSTEM is used for operating system errors. E_FBLIB is
 *  used for errors reported by libfb. All others are internal errors.
 */
typedef enum
{ E_SUCCESS = 0, E_BADVALUE, E_BADTOKEN,
  E_BADSTATE, E_DUPLICATE, E_BADINPUT, E_SYSTEM,
  E_REBOOTDSP, E_FBLIB
}
FB_STATUS;

void fberror (const char *message, FB_STATUS the_error);

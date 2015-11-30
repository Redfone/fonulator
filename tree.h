/*
   fonulator 3 - foneBRIDGE configuration daemon
   (C) 2005-2007 Redfone Communications, LLC.
   www.red-fone.com

   by Brett Carrington <brettcar@gmail.com>
*/
/** @file 
 * Parse Tree Data Structures -- note that none of this is actually a Tree in the formal sense
 */

/** Max token string value length */
#define S_LEN 148

#ifdef HAVE_STDBOOL_H
#include <stdbool.h>
#endif

typedef struct keylist
{
  /** The key data */
  KEY_ENTRY *key;
  /** The next key in the list */
  struct keylist *next;
} T_KEYLIST;

typedef struct token
{
  /** Token Type */
  int token;

  /** Any integer value */
  int ival;
  /** Any string value */
  char sval[S_LEN];
  /** The line number the token was found on */
  int lineno;
}
T_TOKEN;

typedef struct span
{
  /** The number of this span (indexed from 1) */
  int num;

  /** The configuration of this span */
  IDT_LINK_CONFIG config;
  /** Slave if true or master timing is false */
  bool slave;

  /** Run special shorthaul routines if true */
  bool shorthaul;
  /** Run special longhaul routines if true */
  bool longhaul;
  /** Run special dejitter routines if true */
  bool dejitter;
}
T_SPAN;

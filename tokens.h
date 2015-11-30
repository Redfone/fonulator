/*
   fonulator 3 - foneBRIDGE configuration daemon
   (C) 2005-2007 Redfone Communications, LLC.
   www.red-fone.com

   by Brett Carrington <brettcar@gmail.com>

   LEX Token Enumeration
*/

/** @file 
 * List of tokens returned by the (F)LEX parser code 
 */
enum
{ TOK_UNKNOWN = -1, TOK_LEX_EOF = 0, TOK_SPAN = 1, TOK_FRAMING,
  TOK_ENCODING, TOK_FB_CONFIG, TOK_SERVER_CONFIG, TOK_J1,
  TOK_RBS, TOK_CRCMF, TOK_GLOBALS, TOK_PORT, TOK_MAC, TOK_IP,
  TOK_DSP_OFF, TOK_ULAW, TOK_ALAW, TOK_DSP_HEADER, TOK_VOICEA,
  TOK_VOICEB, TOK_DATA, TOK_RANGE, TOK_DIGIT, TOK_LOOPBACK, TOK_PRIO,
  TOK_SLAVE, TOK_LONGHAUL, TOK_SHORTHAUL, TOK_IEC, TOK_DSP_DISABLED,
  TOK_BEGIN_KEY, TOK_END_KEY, TOK_SLOT_ID, TOK_CUSTOMER_KEY, TOK_DEJITTER, 
  TOK_WPLLOFF
};

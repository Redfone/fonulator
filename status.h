/*
   fonulator 3 - foneBRIDGE configuration daemon
   (C) 2005-2007 Redfone Communications, LLC.
   www.red-fone.com

   by Brett Carrington <brettcar@gmail.com>
*/
/** @file
   foneBRIDGE Status Definitions
*/

FB_STATUS statusInitalize (libfb_t * f);
void statusCleanup (void);
void statusDisplay (void);
bool statusHasDSP (void);
bool statusIsIEC (void);
unsigned int statusGetSpans (void);
unsigned int statusGetTransceivers (void);
DOOF_STATIC_INFO *status_get_dsi (void);
bool statusRunPMON (libfb_t * fb);
void statusPrintRegisters (DList * regs);
void pmon_destroy (libfb_PMONRegister * reg);

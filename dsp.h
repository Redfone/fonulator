/*
   fonulator 3 - foneBRIDGE configuration daemon
   (C) 2005-2007 Redfone Communications, LLC.
   www.red-fone.com

   by Brett Carrington <brettcar@gmail.com>

   DSP Definitions
*/
/** @file 
 *
 * DSP configuration function prototypes and definitions 
 */

/** @enum dsp_chantype 
 *
 *   @brief Enumeration of possible DSP channel states.
 */
typedef enum
{ DSP_DATA = 0, DSP_A, DSP_B, DSP_OFF, DSP_MAX }
dsp_chantype;

char *dspchan_to_string (dsp_chantype chan);

FB_STATUS configureDSP (libfb_t * f, DList * list);
void dspconfig_init_userconfig ();
FB_STATUS dspconfig_set_userdigit (dsp_chantype type, int chan);
FB_STATUS dspconfig_set_userrange (dsp_chantype type, int min, int max);

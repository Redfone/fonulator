/*
   fonulator 3 - foneBRIDGE configuration daemon
   (C) 2005-2007 Redfone Communications, LLC.
   www.red-fone.com

   by Brett Carrington <brettcar@gmail.com>

   Very Simple State Machine
*/

/** @file 
 *
 * Global state machine for configuration parser 
 */

/** @enum STATE
 *
 * Possible global states that fonulator can be in. Details:
 * 
 * STATE_GLOBAL: Parsing the [global] block
 * STATE_SPAN:   Parsing a [spanN] block
 * STATE_DSP:    Parsing the [dsp] block
 * (DSPSTATE_WAIT_FOR_VALUE: Internal STATE_DSP state)
 * STATE_RUN:    Running the configuration
 */
typedef enum
{ STATE_NONE = -1, STATE_GLOBAL = 0, STATE_SPAN, STATE_DSP,
  DSPSTATE_WAIT_FOR_VALUE, STATE_RUN
}
STATE;



/** @struct smachine 
 *
 * A global structure the represents some key states
 * in the entire program. Some 'global' data is saved here while other
 * data (like individual spans) are relegated to storage in the T_SPAN
 * linked list.
 */
struct { 
  int span;   /** Tracks which span we're currently parsing */

  /* Parsed configuration */
  int port;
  int companding;

  char *fonebridge;
  char *server;

  /* Various globals */
  unsigned int total_spans;
  bool dspconfig;
  bool iec;			/**< Is this an IEC? */
  bool dspdisabled;		/**< True if we must not touch the DSP */
  FEATURE featset;              /**< The feature set of the device */ 
  bool wpll;                    /*wpll=0 off, wpll=1 on*/

  /** @brief Span priority. 
   *
   * @details 0 will be slaved to telco. [1-3] indicate slaving
   * priority should the span marked 0 become unavailable. All
   * settings mutually exclusive, no duplicates, except the special
   * case of '0' set for every span which means that all spans use
   * internal timing. It is set to -1 by default which is used in
   * validation routines to substitute applicable defaults if needed.
   */
  unsigned int priorities[4];
}
smachine;

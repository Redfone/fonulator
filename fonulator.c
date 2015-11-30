 /*
    fonulator 3 - foneBRIDGE configuration daemon
    (C) 2005-2007 Redfone Communications, LLC.
    www.red-fone.com

    by Brett Carrington <brettcar@gmail.com>
  */
/** @file 
 *
 * Main fonulator routines
 */
/** @mainpage
 * 
 * @section overview_sec Overview 
 *
 * The fonulator software product is concerned with two fundamental
 * tasks. The first is to parse the user's configuration file into a
 * format useful to the computer. The second task is to use the
 * collected information to configure a Redfone product (foneBRIDGE or
 * IEC) over a UDP connection.
 *
 * @section flow_sec Program Flow 
 * 
 * @subsection parsing_sec Parsing
 * Program flow begins by using the libargtable2 library to parse
 * command line options. Except when special operations are selected
 * the default behavior is then to read the selected configuration
 * file and begin parsing. This is accomplished by using the `flex'
 * fast lexical analyzer generator. The lexParser() routine repeatedly
 * calls yylex() which returns the type of the next token in the
 * specified configuration file. A linked list of tokens (and their
 * ancillary data) is stored in token_list.
 * 
 * @subsubsection tokens_sec Tokens
 *
 *
 * Tokens are enumerated in tokens.h. This enumation is used by both
 * the internal fonulator sources and by the flex analyzer description
 * in the tokens.l source. The flex analyzer must be told which
 * actions to take when a token is encountered in a configuration
 * file. Similarly the fonulator program must be told what actions to
 * take for each token. These behaviors are specified in tokens.l and
 * in treeParser() respectively.
 *
 * @subsubsection treeparser_sec  Tree Parser
 *
 * The token_list linked list is complete once the lexical analyzer
 * has finished parsing the input file. The treeParser() routine now
 * iterates over the tokens list, ensuring that:
 *
 * <ul> <li> The configuration file was properly formatted.
 *
 *  <li> Each configuration directive is applied to the respective
 * span or DSP data structures.</ul>
 *
 * @subsection configuration_sec Configuration/Post-Parsing
 * 
 * After parsing completes successfully an attempt is made to connect
 * to the target device. Communication with the target device is made
 * only through the libfb library. Fonulator only uses the UDP
 * functionality of the library. Hence, most functions and almost all
 * calls to the libfb library require we specify the libfb context
 * pointer, `fb'. This pointer is defined on the stack of the main()
 * function and passed to other functions when needed. The actual
 * memory referenced by `fb' is located elsewhere, of course.
 *
 * Various special functions are now called if required by the user,
 * such as rebooting the device or programming it with license
 * files. Otherwise the device is reconfigured, this requires that we
 * have a span_list linked list that has an entry for each physical
 * span on the device and no more. If this is not the case
 * completeSpans() is called or the error condition is reported.
 *
 * If a DSP is available on the target device configureDSP() is
 * run. Ultimately configureFonebridge() is called which runs the
 * actual configuration of the T1/E1 spans, the TDMOE stream (if
 * applicable) and so on.
 */
#include "fonulator.h"

#ifdef HAVE_ARGTABLE2_H
#include <argtable2.h>
#endif

#if defined(STDC_HEADERS) || defined(HAVE_STDLIB_H)
#include <stdlib.h>
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef VERSION
#define SW_VER FONULATOR_VERSION
#endif

/** UDP configuration socket port on foneBRIDGE devices */
#define UDP_CONFIG_PORT 1024
/** default location of the configuration file */
#define DEFAULT_CONFIG "/etc/redfone.conf"

#include "ver.h"

/* flex externals */
extern int yylval;
extern char *yytext;
extern FILE *yyin;
extern int yylineno;
extern int yylex (void);
extern int yylex_destroy (void);

/* fonulator globals */

/** @var state
 *
 * The internal state of the fonulator parsing, configuration and
 * other routines.  See state.h for enumation of possible states.
 */
static STATE state = STATE_NONE;

/** @var token_list
 * The file local pointer to the head of the linked list of tokens.
 */
static DList *token_list = NULL;

/** @var span_list
 * The file local pointer to the head of the linked list of spans.
 */
static DList *span_list = NULL;

/** The maximum number of keys that we can store in flash memory */
#define MAX_KEYS 32
/** An array of keys/licenses parsed for programming to the device. */
static KEY_ENTRY all_keys[MAX_KEYS];
/** For each valid key, k, in all_keys valid_keys[k] is true. */
static int valid_keys[MAX_KEYS];

static bool priorities_valid ();
/** Verbosity level */
int vbose = 0;

/** 
 * @param d hex digit to convert to integer value
 * @return integer representation of d
 */
static int
digit2int (char d)
{
  switch (d)
    {
    case 'F':
    case 'E':
    case 'D':
    case 'C':
    case 'B':
    case 'A':
      return d - 'A' + 10;
    case 'f':
    case 'e':
    case 'd':
    case 'c':
    case 'b':
    case 'a':
      return d - 'a' + 10;
    case '9':
    case '8':
    case '7':
    case '6':
    case '5':
    case '4':
    case '3':
    case '2':
    case '1':
    case '0':
      return d - '0';
    }
  return -1;
}

/**
 * @param s null-terminated string representing a hexadecimal octet
 * @return the integer value of s
 */
static int
hex2int (char *s)
{
  int res;
  int tmp;
  /* Gotta be at least one digit */
  if (strlen (s) < 1)
    return -1;

  /* Can't be more than two */
  if (strlen (s) > 2)
    return -1;
  /* Grab the first digit */
  res = digit2int (s[0]);
  if (res < 0)
    return -1;
  tmp = res;
  /* Grab the next */
  if (strlen (s) > 1)
    {
      res = digit2int (s[1]);
      if (res < 0)
	return -1;
      tmp = tmp * 16 + res;
    }
  return tmp;
}

/**
 * @param hexnum A string in the format "0xABCD" include the 0x. It must be at least 2*(size+1) long.
 * @param size The decoded size of the number, exatly
 * @param store pointer to storage location for the stored number
 *
 * Decodes an ASCII number in hex notation into a big-endian unsigned
 * integer representation.
 */
void
parse_huge_hexnumber (char *hexnum, int size, unsigned char *store)
{
  int i;
  char *position = (hexnum + 2);

  for (i = 0; i < size; i++)
    {
      /* Read a byte, write a byte: for reading in huge hexadeximal numbers */
      char unparsed_byte[3];
      strncpy (unparsed_byte, position, 2);
      if (unparsed_byte[1] == ';')
	unparsed_byte[1] = '\0';
      unparsed_byte[2] = '\0';

      store[i] = hex2int (unparsed_byte);
      position += 2;
    }

}


/** @brief Return a pointer to the T_SPAN structure with number `num'.
 *
 * The span number is indexed from 1. For example, '1' is the first
 * span on a foneBRIDGE and '4' is the last span on a quad foneBRIDGE.
 * 
 * If a span number is not found in the list then it is created and
 * added to the linked list of T_SPAN pointers. This means that a
 * pointer to the right T_SPAN is always returned except if memory is
 * not available to allocate the space. 
 * 
 * @return pointer to the desired T_SPAN. Always creates a new entry
 * if not found. Returns NULL if creation failed.
 */
T_SPAN *
get_span (int num)
{
  T_SPAN *current;
  DListElmt *element;

  /* Find span, or create it */

  element = dlist_head (span_list);
  while (element != NULL)
    {
      current = dlist_data (element);
      if (current->num == num)
	return current;
      element = dlist_next (element);
    }

  if (state == STATE_RUN)
    {
      DBG (printf ("Couldn't find span %d.\n", num));
      return NULL;
    }

  DBG (printf ("Couldn't find span %d, creating.\n", num));

  /* Couldn't find it. Create. */
  element = dlist_tail (span_list);
  current = malloc (sizeof (T_SPAN));
  if (current == NULL)
    {
      perror ("malloc");
      return NULL;
    }
  memset (current, 0, sizeof (T_SPAN));
  current->num = num;
  smachine.total_spans++;
  dlist_ins_next (span_list, element, current);
  return current;
}

/**
 * treeParser goes through the entire list of parsed tokens
 * (token_list) and for each token it executes a specific behavior to
 * that token type. If any behavior fails the entire routine fails,
 * likely because some part of the configuration is invalid.
 *
 * @return success/failure code
 */
FB_STATUS
treeParser (void)
{
  T_TOKEN *current;
  DListElmt *element;
  int current_key = -1;
  bool exit = false;
  FB_STATUS status;

  smachine.wpll=1; //by default the wpll is enabled

  dsp_chantype current_chantype = DSP_DATA;

  element = dlist_head (token_list);
  current = dlist_data (element);

  while (!exit || current != NULL)
    {
      switch (current->token)
	{
	case TOK_GLOBALS:
	  if (state == STATE_NONE)
	    state = STATE_GLOBAL;
	  else
	    {
	      /*
	       * We are in some other state and cannot go
	       * back to global.
	       */
	      return E_BADSTATE;
	    }
	  break;

	case TOK_PORT:
	  if (state == STATE_GLOBAL)
	    {
	      smachine.port = current->ival;
	      DBG (printf ("Set port to %d\n", smachine.port));
	    }
	  else
	    {
	      return E_BADSTATE;
	    }
	  break;

	case TOK_PRIO:
	  if (state == STATE_GLOBAL)
	    {
	      int i;
	      for (i = 0; i < 7; i += 2)
		{
		  smachine.priorities[i / 2] =
		    strtoul (&current->sval[i], NULL, 10);
		}
	    }
	  else
	    return E_BADSTATE;
	  break;

	case TOK_WPLLOFF:
	  if (state == STATE_GLOBAL)
	    {
	       smachine.wpll = 0;
            }
	  else
	    return E_BADSTATE;
	  break;
		
	case TOK_ULAW:
	  smachine.companding = DSP_COMP_TYPE_ULAW;
	  break;
	case TOK_ALAW:
	  smachine.companding = DSP_COMP_TYPE_ALAW;
	  break;
	case TOK_DSP_OFF:
	  smachine.companding = -1;
	  break;
	case TOK_DSP_DISABLED:
	  smachine.companding = -1;
	  smachine.dspdisabled = true;
	  break;

	case TOK_FB_CONFIG:
	  if (state == STATE_GLOBAL)
	    {
	      DBG (printf ("Setting up FB: %s\n", current->sval));
	      smachine.fonebridge = malloc (strlen (current->sval) + 1);

	      if (smachine.fonebridge == NULL)
		{
		  perror ("malloc");
		  return E_SYSTEM;
		}
	      strcpy (smachine.fonebridge, current->sval);
	    }
	  else
	    {
	      return E_BADSTATE;
	    }
	  break;

	case TOK_SERVER_CONFIG:
	  if (state == STATE_GLOBAL)
	    {
	      DBG (printf ("Setting up server: %s\n", current->sval));
	      smachine.server = malloc (1 + strlen (current->sval));
	      if (smachine.server == NULL)
		{
		  perror ("malloc");
		  return E_SYSTEM;
		}
	      strcpy (smachine.server, current->sval);
	    }
	  else
	    {
	      return E_BADSTATE;
	    }
	  break;

	  /* DSP Block Handlers */
	case TOK_DSP_HEADER:
	  if (state == STATE_GLOBAL || state == STATE_SPAN)
	    {
	      state = STATE_DSP;
	      dspconfig_init_userconfig ();
	    }
	  else
	    {
	      return E_BADSTATE;
	    }
	  break;

	case TOK_VOICEA:
	  if (state != STATE_DSP)
	    return E_BADSTATE;
	  state = DSPSTATE_WAIT_FOR_VALUE;
	  current_chantype = DSP_A;
	  break;

	case TOK_VOICEB:
	  if (state != STATE_DSP)
	    return E_BADSTATE;
	  state = DSPSTATE_WAIT_FOR_VALUE;
	  current_chantype = DSP_B;
	  break;

	case TOK_DATA:
	  if (state != STATE_DSP)
	    return E_BADSTATE;
	  state = DSPSTATE_WAIT_FOR_VALUE;
	  current_chantype = DSP_DATA;
	  break;

	case TOK_DIGIT:
	  if (state != DSPSTATE_WAIT_FOR_VALUE)
	    return E_BADSTATE;
	  status = dspconfig_set_userdigit (current_chantype, current->ival);
	  state = STATE_DSP;
	  if (status != E_SUCCESS)
	    return status;
	  break;

	case TOK_RANGE:
	  if (state != DSPSTATE_WAIT_FOR_VALUE)
	    return E_BADSTATE;
	  char *dash = strchr (current->sval, '-');
	  if (dash == NULL)
	    return E_BADINPUT;
	  int min, max;
	  *dash = '\0';
	  min = atoi (current->sval);
	  dash++;
	  max = atoi (dash);

	  status = dspconfig_set_userrange (current_chantype, min, max);
	  state = STATE_DSP;
	  if (status != E_SUCCESS)
	    return status;
	  break;

	case TOK_SPAN:
	  if (state == STATE_GLOBAL)
	    {
	      state = STATE_SPAN;
	    }
	  else if (state != STATE_SPAN)
	    {
	      return E_BADSTATE;
	    }
	  if (smachine.span == current->ival)
	    return E_DUPLICATE;

	  smachine.span = current->ival;

	  DBG (printf ("Starting parse for Span %d\n", smachine.span));
	  break;

	case TOK_FRAMING:
	  if (state == STATE_SPAN)
	    {
	      T_SPAN *s = get_span (smachine.span);	/* get current span */
	      if (!strcmp (current->sval, "cas"))
		{
		  s->config.E1Mode = 1;
		  /*
		   * RBS should never be reset to '0'
		   * elsewhere in parsing routines or
		   * this functionality might break
		   */
		  s->config.rbs_en = 1;
		  s->config.framing = 0;
		}
	      else if (!strcmp (current->sval, "ccs"))
		{
		  s->config.E1Mode = 1;
		  s->config.rbs_en = 0;
		  s->config.framing = 0;
		}
	      else if (!strcmp (current->sval, "esf"))
		{
		  s->config.E1Mode = 0;
		  s->config.framing = 1;
		}
	      else if (!strcmp (current->sval, "sf"))
		{
		  s->config.E1Mode = 0;
		  s->config.framing = 0;
		}
	      else
		{
		  return E_BADVALUE;
		}
	    }
	  else
	    {
	      return E_BADSTATE;
	    }
	  break;

	case TOK_ENCODING:
	  if (state == STATE_SPAN)
	    {
	      T_SPAN *s = get_span (smachine.span);	/* get current span */
	      if (!strcmp (current->sval, "hdb3"))
		{
		  s->config.E1Mode = 1;
		  s->config.encoding = 0;
		}
	      else if (!strcmp (current->sval, "b8zs"))
		{
		  s->config.E1Mode = 0;
		  s->config.encoding = 0;
		}
	      else if (!strcmp (current->sval, "ami"))
		{
		  /* Could be E1 or T1 */
		  s->config.encoding = 1;
		}
	      else
		{
		  return E_BADVALUE;
		}
	    }
	  else
	    {
	      return E_BADSTATE;
	    }
	  break;

	case TOK_CRCMF:
	  if (state == STATE_SPAN)
	    {
	      T_SPAN *s = get_span (smachine.span);	/* get current span */
	      s->config.CRCMF = 1;
	    }
	  else
	    {
	      return E_BADSTATE;
	    }
	  break;

	case TOK_J1:
	  if (state == STATE_SPAN)
	    {
	      T_SPAN *s = get_span (smachine.span);
	      s->config.J1Mode = 1;
	      s->config.E1Mode = 0;
	    }
	  else
	    {
	      return E_BADSTATE;
	    }
	  break;


	case TOK_RBS:
	  if (state == STATE_SPAN)
	    {
	      T_SPAN *s = get_span (smachine.span);	/* get current span */
	      DBG (printf ("RBS on span %d\n", s->num));
	      s->config.rbs_en = 1;
	    }
	  else
	    {
	      return E_BADSTATE;
	    }
	  break;

	case TOK_LOOPBACK:
	  if (state == STATE_SPAN)
	    {
	      T_SPAN *s = get_span (smachine.span);	/* get current span */
	      DBG (printf ("LOOPBACK on span %d\n", s->num));
	      s->config.rlb = 1;
	    }
	  else
	    {
	      return E_BADSTATE;
	    }
	  break;

	case TOK_SLAVE:
	  if (state == STATE_SPAN)
	    {
	      T_SPAN *s = get_span (smachine.span);	/* get current span */
	      DBG (printf ("SLAVE on span %d\n", s->num));
	      s->slave = true;
	    }
	  break;

	case TOK_SHORTHAUL:
	  if (state == STATE_SPAN)
	    {
	      T_SPAN *s = get_span (smachine.span);	/* get current span */
	      if (s->longhaul)
		{
		  fprintf (stderr,
			   "Shorthaul specified on line %d but longhaul already specified for this span.\n",
			   current->lineno);

		  return E_BADVALUE;
		}

	      if (current->ival < 0 || current->ival >= MAX_SHORTLBO)
		{
		  fprintf (stderr,
			   "Invalid shorthaul configuration on line %d!\n",
			   current->lineno);
		  return E_BADVALUE;
		}

	      s->shorthaul = true;
	      s->config.LBO = shortlbo[current->ival] & 0xF;
	    }
	  else
	    {
	      return E_BADSTATE;
	    }
	  break;

	case TOK_LONGHAUL:
	  if (state == STATE_SPAN)
	    {
	      T_SPAN *s = get_span (smachine.span);	/* get current span */
	      if (s->shorthaul)
		{
		  fprintf (stderr,
			   "Longhaul specified on line %d but shorthaul already specified for this span.\n",
			   current->lineno);
		  return E_BADVALUE;
		}
	      DBG (printf ("Adaptive equalizer on span %d\n", s->num));
	      DBG (printf ("Longhaul value: %d\n", current->ival));

	      if (current->ival < 0 || current->ival >= MAX_LONGLBO)
		{
		  fprintf (stderr,
			   "Invalid longhaul configuration on line %d!\n",
			   current->lineno);
		  return E_BADVALUE;
		}

	      s->longhaul = true;
	      s->config.LBO = longlbo[current->ival] & 0xF;
	      s->config.EQ = true;
	    }
	  else
	    {
	      return E_BADSTATE;
	    }
	  break;

	case TOK_DEJITTER:
	  if (state == STATE_SPAN)
	    {
	      T_SPAN *s = get_span (smachine.span);	/* get current span */
	      DBG (printf ("DEJITTER on span %d\n", s->num));
	      s->dejitter = true;
	    }
	  else
	    {
	      return E_BADSTATE;
	    }
	  break;

	case TOK_BEGIN_KEY:
	  break;

	case TOK_SLOT_ID:
	  current_key = strtol (current->sval, NULL, 16);
	  DBG (printf ("Slot ID: %i\n", current_key));
	  if (!(current_key >= 0 && current_key < MAX_KEYS))
	    {
	      fprintf (stderr, "Invalid slot ID for license key!\n");
	      return E_BADTOKEN;
	    }
	  break;

	case TOK_CUSTOMER_KEY:
	  parse_huge_hexnumber (current->sval, CUSTOMER_KEY_SZ,
				all_keys[current_key].customer_key);
	  break;

	case TOK_END_KEY:
	  valid_keys[current_key] = 1;
	  break;

	case TOK_LEX_EOF:
	  exit = true;
	  break;

	case TOK_UNKNOWN:
	default:
	  printf ("Bad token in configuration file on line %d.\n",
		  current->lineno);
	  DBG (printf ("(%s) [%d]\n", current->sval, current->token));
	  return E_BADTOKEN;
	}

      element = dlist_next (element);
      if (element != NULL)
	current = dlist_data (element);
      else
	current = NULL;

    }
  return E_SUCCESS;
}


/** @brief Recursively free a linked list of tokens
 *
 * @param token pointed to a list of tokens to be free'd
 */
void
cleanupToken (T_TOKEN * token)
{
  free (token);
  token = NULL;
}

/**
 *
 * lexParser calls yylex over and over until end of file is
 * reached. Each yylex call has (f)lex parse the next line in the
 * configuration file and return a TOK_* response if applicable.
 *
 * Parsed tokens are added to the linked list of tokens (token_list).
 *
 * @return 0 on success
 */
int
lexParser (void)
{
  int ret;
  T_TOKEN *current = NULL;

  current = malloc (sizeof (T_TOKEN));
  if (current == NULL)
    {
      perror ("malloc");
      return -1;
    }

  if (dlist_ins_next (token_list, NULL, current) < 0)
    {
      fprintf (stderr, "Failed to insert token into linked list.\n");
      return -1;
    }

  do
    {
      T_TOKEN *nextToken = NULL;
      ret = yylex ();

      current->ival = yylval;
      strncpy (current->sval, yytext, S_LEN);
      current->lineno = yylineno;
      current->token = ret;

      nextToken = malloc (sizeof (T_TOKEN));
      if (nextToken == NULL)
	{
	  perror ("malloc");
	  return -1;
	}

      // Insert at list's tail
      if (dlist_ins_next (token_list, dlist_tail (token_list), nextToken) < 0)
	{
	  fprintf (stderr, "Failed to insert token into linked list.\n");
	  return -1;
	}

      current = nextToken;

    }
  while (ret != 0);

  return 0;
}


/** @brief Parse a user-specified MAC address into an unsigned byte array
 *
 * @param mac a colon-separated or  dash-separated MAC address (i.e. 11:22:33:44:55:66)
 * @param dst location to store result (must be at least 6 bytes)
 * @return success/error code
 */
FB_STATUS
detokenify_mac (uint8_t * dst, char *mac)
{
  int i;
  unsigned int scanned[ETHER_ADDR_LEN];

  i = sscanf (mac, "%02X:%02X:%02X:%02X:%02X:%02X",
	      &scanned[0], &scanned[1], &scanned[2],
	      &scanned[3], &scanned[4], &scanned[5]);

  if (i != ETHER_ADDR_LEN)
    {
      /* Try this way */
      i = sscanf (mac, "%02X-%02X-%02X-%02X-%02X-%02X",
		  &scanned[0], &scanned[1], &scanned[2],
		  &scanned[3], &scanned[4], &scanned[5]);
      if (i != ETHER_ADDR_LEN)
	return E_BADVALUE;
    }

  for (i = 0; i < ETHER_ADDR_LEN; i++)
    dst[i] = scanned[i] & 0xFF;

  return E_SUCCESS;
}



/** @brief Configure a device after populating all configurationdata structures 
 * 
 * @param f the libfb context for the device
 * @return success/error code
 */
FB_STATUS
configureFonebridge (libfb_t * f)
{
  unsigned char prio[4];	/* set master=1 or slave=0 mode */
  unsigned char oldprio[4];
  char dest_mac[ETHER_ADDR_LEN];
  IDT_LINK_CONFIG current[IDT_LINKS];
  IDT_LINK_CONFIG new[IDT_LINKS];
  int i, status;
  bool need_update = false;
  bool need_prio_update = false;

  unsigned long long clkselregnew=0x1000;
  unsigned long long clkselregold;
/* 
  clkselregnew[0]=1;
  clkselregnew[1]=0;
  clkselregnew[2]=0;
  clkselregnew[3]=0;
*/
  if (vbose > 0)
    printf ("Detecting current foneBRIDGE link configuration\n");

  status = configcheck_fb_udp (f, current);

  if (status != E_SUCCESS)
    {
      fprintf (stderr,
	       "Unable to detect current foneBRIDGE link configuration.\n");
      return E_SYSTEM;
    }

  if (smachine.iec == 0)
    if (fb_tdmoectl (f, 0) < 0)
      return E_FBLIB;

  if (smachine.featset == FEATURE_2_0 && !priorities_valid ())
    {
      fprintf (stderr,
	       "Invalid priority settings. Only values 0 to 3 are valid, no duplicates. All zeros may be used if internal timing is desired.\n");
      return E_BADVALUE;
    }

//disable_wpll
  
  

  if (!smachine.wpll)
   {
     
     printf("WPLL Disabled\n");
     status = custom_cmd_reply (f, DOOF_CMD_CLKSEL_PIO, 1 , (char*) &clkselregnew, 4, (char*) &clkselregold, 4);
   }
  else
   {
    printf("WPLL Enabled\n");
    status = custom_cmd_reply (f, DOOF_CMD_CLKSEL_PIO, 2 , (char*) &clkselregnew, 4, (char*) &clkselregold, 4);		
   }
  for (i = 0; i < 4; i++)
    {
      T_SPAN *s = get_span (1 + i);
      if (s)
	{
	  if (smachine.featset == FEATURE_PRE_2_0)
	    {
	      /* Default is master */
	      prio[i] = 1;
	      if (s->slave)
		prio[i] = 0;
	    }
	  else if (smachine.featset == FEATURE_2_0)
	    {
	      /* If priorities_valid() succeded above then it is
	       * permissible to merely copy the priorities from the
	       * state machine
	       */
	      prio[i] = smachine.priorities[i];
	    }
	  /* If J1 mode was selected, ensure E1Mode is off */
	  if (s->config.J1Mode)
	    {
	      s->config.E1Mode = 0;
	      s->config.LBO = PULS_J1;
	    }

	  if (memcmp
	      ((void *) &s->config, (void *) &current[i],
	       sizeof (IDT_LINK_CONFIG)) != 0)
	    {
	      if (vbose > 0)
		printf ("Line configurations differ for link %d\n", i + 1);

	      need_update = true;
	    }
	  memcpy ((void *) &new[i], (void *) &s->config,
		  sizeof (IDT_LINK_CONFIG));
	}
      else
	{
	  memcpy ((void *) &new[i], (void *) &current[i],
		  sizeof (IDT_LINK_CONFIG));
	}
    }

  if (!need_update)
    {
      /* 
       * Actually we are getting the current values of the priorities
       * here, not setting them. 
       */
      status = custom_cmd_reply (f, DOOF_CMD_SET_PRIORITY, 0,
				 (char *) prio, 4, (char *) oldprio, 4);
      if (status != E_SUCCESS)
	{
	  PRINT_MAPPED_ERROR_IF_FAIL (status);
	  fprintf (stderr,
		   "fonulator: Priority Control Error (Couldn't get current priorities)\n");
	  return E_SYSTEM;
	}

      for (i = 0; i < 4; i++)
	{
	  if (oldprio[i] != prio[i])
	    need_prio_update = true;
	  if (vbose >= 2)
	    printf ("Span %d: Old priority %d, new priority %d\n", i,
		    oldprio[i], prio[i]);
	}
    }

  if (!smachine.iec)
    {
      /* IEC does not need these operations */
      status = detokenify_mac ((unsigned char *) dest_mac, smachine.server);
      if (status != E_SUCCESS)
	{
	  fprintf (stderr, "TDMoE Destination MAC Invalid\n");
	  return E_SYSTEM;
	}

      if (smachine.port == 0)
	{
	  if (vbose > 0)
	    printf ("No port setting found, using default port '1'.\n");
	  smachine.port = 1;
	}

      status =
	custom_cmd (f, DOOF_CMD_TDMOE_DSTMAC, (smachine.port - 1), dest_mac,
		    6);

      DBG (printf ("TDMoE Set Destination MAC returned: 0x%02X\n", status));

      if (status != E_SUCCESS)
	{
	  fprintf (stderr, "Error setting destination MAC\n");
	  return E_SYSTEM;
	}
    }

  if (need_update)
    {
      if (vbose > 0)
	printf ("Updating foneBRIDGE link configuration\n");

      status = config_fb_udp_linkconfig (f, new);
      if (status != E_SUCCESS)
	{
	  /* libfb currently prints to the user, ugh! */
	  return status;
	}
    }

  if (need_update || need_prio_update)
    {
      char reply[4];

      status =
	custom_cmd_reply (f, DOOF_CMD_SET_PRIORITY, 0xf, (char *) prio, 4,
			  (char *) reply, 4);
      if (status != E_SUCCESS)
	{
	  PRINT_MAPPED_ERROR_IF_FAIL (status);
	  fprintf (stderr, "fonulator: Priority Control Error\n");
	}
    }

  for (i = 0; i < 4; i++)
    {
      T_SPAN *s = get_span (1 + i);
      if (s)
	{
	  /* 0x8 represents dejitter ON, 0x0 is dejitter OFF */
	  uint8_t regvalue = s->dejitter ? 0x8 : 0x0;
	  if (writeidt (f, i, 0x21, regvalue) != FBLIB_ESUCCESS)
	    fprintf (stderr,
		     "fonulator: Write to IDT jitter register 0x21 failed!\n");
	  if (writeidt (f, i, 0x27, regvalue) != FBLIB_ESUCCESS)
	    fprintf (stderr,
		     "fonulator: Write to IDT jitter register 0x27 failed!\n");
	}
    }

  if (smachine.iec == 0)
    if (fb_tdmoectl (f, 1) < 0)
      return E_FBLIB;

  return E_SUCCESS;
}

/** @brief Enable or disable TDMoE transmission
 *
 * @param state 0/1 to disable/enable TDMoE transmission
 * @param f the libfb context for the device
 * @return 0 on success
 *
 * This function only checks if the state is a valid one. It does not
 * check if the device is a foneBRIDGE or an inline echo
 * canceller. Enabling TDMoE on an IEC is not recommended!
 */
int
fb_tdmoectl (libfb_t * f, int state)
{
  int status;
  if (state > 1 || state < 0)
    return -1;

  if (vbose > 0)
    printf ("%s foneBRIDGE TDMoE transmission\n",
	    state ? "Starting" : "Stopping");

  status = custom_cmd (f, DOOF_CMD_TDMOE_TXCTL, state, NULL, 0);
  DBG (printf ("TDMoE Control returned 0x%02X\n", status));
  if (status != E_SUCCESS)
    {
      fprintf (stderr, "fonulator: TDMoE Control Error %d\n", status);
      return -1;
    }
  return 0;
}


/**
 *
 * Many configuration routines expect four T_SPAN's to exist (for span
 * numbers 1 to 4). This function fills in any missing spans with
 * copies of the tail span.
 *
 * @return success/error code
 */
FB_STATUS
completeSpans (void)
{
  DListElmt *element = dlist_head (span_list);
  T_SPAN *current;
  bool missing[] = { true, true, true, true };
  int i;

  if (element == NULL)
    return E_BADSTATE;

  do
    {
      current = dlist_data (element);
      if (current != NULL)
	{
	  missing[current->num - 1] = false;
	  element = dlist_next (element);
	}
    }
  while (element != NULL);

  for (i = 0; i < 4; i++)
    {
      T_SPAN *new;
      if (missing[i])
	{
	  new = malloc (sizeof (T_SPAN));

	  if (new == NULL)
	    {
	      perror ("malloc");
	      return E_SYSTEM;
	    }

	  /* Copy the tail's settings */
	  element = dlist_tail (span_list);
	  current = dlist_data (element);
	  memcpy (new, current, sizeof (T_SPAN));
	  new->num = i + 1;
	  dlist_ins_next (span_list, element, current);
	}
    }
  return E_SUCCESS;
}


/** @brief Recursively free a linked list of spans
 *
 * @param span pointed to a list of spans to be free'd
 */
void
cleanupSpan (T_SPAN * span)
{
  free (span);
  span = NULL;
}



/** @brief free memory used by the state machine */
void
cleanupStateMachine (void)
{
  if (smachine.fonebridge)
    free (smachine.fonebridge);
  if (smachine.server)
    free (smachine.server);
}

/** clean up all data structures, freeing used memory */
void
cleanupAll (void)
{
  dlist_destroy (token_list);
  free (token_list);
  token_list = NULL;

  dlist_destroy (span_list);
  free (span_list);
  span_list = NULL;

  cleanupStateMachine ();
  statusCleanup ();
}


/**
 * Our entry point. The argtable library is consulted to decode the
 * user's selected options.
 *
 * yyin is set to the configuration file and lexParser is called. This
 * reads all the tokens from the configuration file. treeParser is
 * called to execute the desired behavior for each function. Depending
 * on the configuration options and the user's command line options
 * particular behaviors occur (reboot, query, etc) and/or the device
 * is configured by configureFonebridge.
 */
int
main (int argc, char *argv[])
{
  int remotePort, nspans, status = 0;
  char *remoteHost = NULL;

  libfb_t *fb;
  char errstr[LIBFB_ERRBUF_SIZE];

  bool exit_after_free = false;
  int  do_reboot = 0;
  bool change_ip = false;
  bool do_query = false;
  bool do_stats = false;
  bool do_flash_upload = false;
  bool save_config = false;
  bool clear_config = false;
  bool flash_is_gpak = false;
  bool load_keys = false;
  bool dsp_available;

  FILE *cf;
  char *flash_filename = NULL;
  
  char *new_ip = NULL;

  int i;
  int ip_sel=0;

  EPCS_CONFIG epcs;

  T_EPCS_INFO epcs_info;
  DOOF_STATIC_INFO dsi;

  int result;
  unsigned char iptmp[4];

  struct arg_lit *help = arg_lit0 ("hH", "help", "this help information");
  struct arg_lit *reboot = arg_litn ("R", "reboot", 0, 2, "reboot the foneBRIDGE");
  struct arg_lit *clearconfig = arg_lit0 (NULL, "reset-defaults",
					  "reset default configuration on the foneBRIDGE");
  struct arg_file *flashfw =
    arg_file0 (NULL, "upload", "<file>", "upload new firmware image");
  struct arg_lit *gpak = arg_lit0 ("g", "gpak",
				   "specify that the uploaded firmware is a GPAK binary");
  struct arg_lit *saveconfig =
    arg_lit0 (NULL, "write-config", "write configuration to the foneBRIDGE");
  struct arg_lit *loadkeys =
    arg_lit0 (NULL, "load-keys", "write license/keys to the foneBRIDGE");
  struct arg_lit *verbose = arg_litn ("v", "verbose", 0, 2, "verbose output");
  struct arg_lit *query =
    arg_lit0 ("q", "query", "query foneBRIDGE to check availability");
  struct arg_lit *stats =
    arg_lit0 ("s", "stats", "query foneBRIDGE link statistics");
  struct arg_lit *version =
    arg_litn ("V", "version", 0, 2, "get version information");
  struct arg_file *file = arg_file0 (NULL, NULL, "FILE",
				     "config file (default: /etc/redfone.conf)");
  struct arg_str *ip = arg_str0 (NULL, "set-ip", "x.x.x.x", "set new ip");
  
  struct arg_lit *fb2 = arg_lit0 (NULL, "fb2", "specify that ip to be changed is fb2");

  struct arg_end *end = arg_end (5);
  void *argtable[] =
    { help, verbose, query, stats, version, saveconfig, clearconfig, flashfw,
    gpak, /* loadkeys, */ reboot, file, ip, fb2, end
  };

  if (arg_nullcheck (argtable) != 0)
    {
      fprintf (stderr, "argtable: insufficient memory\n");
      exit (1);
    }
  /* Set defaults */
  saveconfig->count = clearconfig->count = loadkeys->count = reboot->count =
    verbose->count = query->count = stats->count = file->count = help->count =
    flashfw->count = gpak->count = version->count = ip->count = fb2->count = 0;
  file->filename[0] = DEFAULT_CONFIG;
  /* End defaults */
  status = arg_parse (argc, argv, argtable);

  if (status != 0 || help->count == 1)
    {
      if (status != 0)
	{
	  arg_print_errors (stdout, end, argv[0]);
	  printf ("\n");
	}
      printf ("usage: fonulator");
      arg_print_syntax (stdout, argtable, "\n");
      printf ("\n");
      arg_print_glossary (stdout, argtable, "\t%-25s %s\n");
      exit_after_free = true;
      status = EXIT_SUCCESS;
    }
  else if (version->count > 0)
    {
      printf ("fonulator %s\n", SW_VER);
      printf ("Copyright (C) 2009 Redfone Communications, LLC.\n");
      printf ("Build Number: %d\n", BUILD_NUM);
      if (version->count > 1)
	{
	  printf ("\n");
	  printf ("Compiled on %s at %s.\n", __DATE__, __TIME__);
	  libfb_printver ();
	}
      exit_after_free = true;
    }
  else if (reboot->count > 0)
    do_reboot = reboot->count;
  else if (ip->count > 0)
    { 
       
       change_ip = true;                             //set flag to change ip        
       ip_sel = 0;                                   //ip_sel=0 to change ip of fb1
       new_ip = malloc (strlen (ip->sval[0]) + 1);   
       strncpy (new_ip, ip->sval[0],                 //copy the ip string to *new_ip 
                   strlen (ip->sval[0]) + 1);
       /*Check if ip to be changed is fb2 */
       if (fb2->count > 0)
	   ip_sel=1;         //ip to be changed is fb2    
        

    }   
  else if (query->count > 0)
    do_query = true;
  else if (stats->count > 0)
    do_stats = true;
  else if (saveconfig->count && clearconfig->count)
    {
      fprintf (stderr, "Invalid command line options. "
	       "Cannot both save and clear configuration.\n");
      status = EXIT_FAILURE;
      exit_after_free = true;
    }
  else if (saveconfig->count > 0)
    save_config = true;
  else if (clearconfig->count > 0)
    clear_config = true;
  else if (flashfw->count > 0)
    {
      flash_filename = malloc (strlen (flashfw->filename[0]) + 1);

      if (gpak->count > 0)
	flash_is_gpak = true;
      if (!flash_filename)
	{
	  perror ("malloc");
	  status = EXIT_FAILURE;;
	  exit_after_free = true;
	}
      else
	{
	  strncpy (flash_filename, flashfw->filename[0],
		   strlen (flashfw->filename[0]) + 1);
	  do_flash_upload = true;
	}
    }
  else if (gpak->count > 0)
    {
      fprintf (stderr, "Invalid command line options. "
	       "GPAK image specified but no upload option specified.\n");
      status = EXIT_FAILURE;
      exit_after_free = true;
    }


  if (verbose->count > 0)
    vbose = verbose->count;

#if 0
  if (loadkeys->count > 0)
    load_keys = true;
#endif

  if (exit_after_free)
    {
      arg_freetable (argtable, sizeof (argtable) / sizeof (argtable[0]));
      exit (status);
    }

  cf = fopen (file->filename[0], "r");

  arg_freetable (argtable, sizeof (argtable) / sizeof (argtable[0]));

  if (cf == NULL)
    {
      fprintf (stderr, "Error opening configuration file.\n");
      perror ("fopen");
      exit (1);
    }

  /* Set up state machine data structure */
  memset (&smachine, 0, sizeof (smachine));
  for (i = 0; i < 4; i++)
    smachine.priorities[i] = -1;

  /* Set up linked list data structures */
  token_list = malloc (sizeof (DList));
  if (token_list == NULL)
    {
      perror ("maloc");
      return -1;
    }

  span_list = malloc (sizeof (DList));
  if (span_list == NULL)
    {
      perror ("maloc");
      return -1;
    }


  dlist_init (token_list, (void (*)(void *)) (cleanupToken));
  dlist_init (span_list, (void (*)(void *)) (cleanupSpan));

  /* Tell flex to look at a file instead of stdin */
  yyin = cf;
  status = lexParser ();
  if (status == E_SUCCESS)
    {
      status = treeParser ();
      if (status != E_SUCCESS)
	{
	  fberror ("treeParser", status);
	  yylex_destroy ();
	  fclose (cf);
	  exit (status);
	}
    }
  else
    {
      printf ("Error parsing configuration file.\n");
      fberror ("lexParser", status);
      yylex_destroy ();
      fclose (cf);
      exit (1);
    }

  yylex_destroy ();
  fclose (cf);

  if ((remoteHost = smachine.fonebridge) == NULL)
    {
      fprintf (stderr,
	       "Configuration file incomplete! No foneBRIDGE IP address.\n");
      exit (EXIT_FAILURE);
    }
  remotePort = UDP_CONFIG_PORT;
  fb = libfb_init (NULL, LIBFB_ETHERNET_OFF, errstr);
  if (fb == NULL)
    {
      fprintf (stderr, "libfb: %s\n", errstr);
      exit (EXIT_FAILURE);
    }
  switch (libfb_connect (fb, remoteHost, remotePort))
    {
    case FBLIB_EERRNO:
      perror ("libfb");
      libfb_destroy (fb);
      exit (EXIT_FAILURE);
      break;
    case FBLIB_EHERRNO:
      herror ("libfb");
      libfb_destroy (fb);
      exit (EXIT_FAILURE);
      break;
    default:
      /* get on with it! */
      break;
    }

  /*********** Socket initialization complete ***********/
  state = STATE_RUN;

  if (vbose > 0)
    printf ("Detecting foneBRIDGE\n");

  status = statusInitalize (fb);
  if (status != E_SUCCESS)
    {
      fberror ("statusInitalize", status);
      libfb_destroy (fb);
      cleanupAll ();
      exit (EXIT_FAILURE);
    }

  if (do_query)
    {
      bool success = queryFonebridge (fb);
      libfb_destroy (fb);
      cleanupAll ();
      exit ((success) ? EXIT_SUCCESS : EXIT_FAILURE);
    }

  if (do_stats)
    {
      bool success = statusRunPMON (fb);
      libfb_destroy (fb);
      cleanupAll ();
      exit ((success) ? EXIT_SUCCESS : EXIT_FAILURE);
    }

  if (do_reboot > 0)
    { 
      bool success;
	
      if (do_reboot > 1)
	{
          success = simpleReboot (fb);
        }
      else
        {
          success = interactiveReboot (fb);
        }

      libfb_destroy (fb);
      cleanupAll ();
      exit ((success) ? EXIT_SUCCESS : EXIT_FAILURE);
    }
  
  if (change_ip > 0)
    {
      bool success;
      udp_get_static_info (fb, &dsi);      
      epcs_info.epcs_blocks = dsi.epcs_blocks;
      if ((udp_read_blk (fb,(epcs_info.epcs_blocks - 2) * 65536,sizeof (EPCS_CONFIG), 
                     (uint8_t *) &epcs) < 0 ))
	{
	  fprintf (stderr, "Error reading configuration block!\n");
          exit (EXIT_FAILURE);
        }
      printf ("Changing ip of fb%d\n",ip_sel+1);
      result = sscanf (new_ip, "%hhu.%hhu.%hhu.%hhu",
                           &iptmp[0], &iptmp[1], &iptmp[2], &iptmp[3]);

      epcs.ip_address[ip_sel] = grab32(iptmp);
      epcs.crc16 =
        crc_16 ((uint8_t *) & epcs, sizeof (EPCS_CONFIG) - 2);
      if (udp_write_to_blk (fb, 0, sizeof (EPCS_CONFIG), (uint8_t *) &epcs) < 0)
        {
          printf ("Error writing config data to flash\n");
          exit (EXIT_FAILURE);
        }
      if (udp_start_blk_write (fb, epcs_info.epcs_blocks - 2))
        {
          printf ("Error executing write blk command\n");
          exit (EXIT_FAILURE);
        }
      printf ("Reboot required\n");
      success = interactiveReboot (fb);          
      libfb_destroy (fb);
      cleanupAll ();
      exit ((success) ? EXIT_SUCCESS : EXIT_FAILURE);
    }
          

  if (load_keys)
    {
      /* In this section we will load security keys and then prompt for reboot. */
      bool success = true;

      int z;
      for (z = 0; z < MAX_KEYS && success == true; z++)
	{
	  /* Build a buffer as follows:
	   *
	   * <feature_id (4)><parm_len(4)><cust_key (32)><parameters (...)>
	   */
	  if (valid_keys[z])
	    {
	      FB_STATUS retval;

	      printf ("Attempting programming of slot %i...", z);
	      fflush (NULL);

	      if ((retval = program_key (fb, z, &all_keys[z])) != E_SUCCESS)
		success = false;

	      printf ("%s\n", (retval == E_SUCCESS) ? "done." : "failed!");
	    }
	}

      if (success)
	success = interactiveReboot (fb);
      else
	fprintf (stderr, "License/Key programming failed!\n");

      cleanupAll ();
      exit ((success) ? EXIT_SUCCESS : EXIT_FAILURE);
    }

  if (do_flash_upload && flash_filename)
    {
      FILE *bin;

      show_warning ();
      bin = fopen (flash_filename, "r");

      if (bin == NULL)
	{
	  fprintf (stderr, "Error opening %s file (%s).\n",
		   flash_is_gpak ? "GPAK" : "firmware", flash_filename);
	  perror ("fopen");
	  cleanupAll ();
	  exit (EXIT_FAILURE);
	}

      if (!smachine.iec && fb_tdmoectl (fb, 0) != 0)
	{
	  fprintf (stderr,
		   "Error disabling TDMoE transmission. Flash operation cancelled.\n");
	  fclose (bin);
	  cleanupAll ();
	  exit (EXIT_FAILURE);
	}

      /* GPAK is located at block 10, firmware is located at block 0. */
      if ((write_file_to_flash (fb, bin, flash_is_gpak ? 10 : 0)) !=
	  E_SUCCESS)
	{
	  fprintf (stderr, "Flash write operation failed.\n");
	  fclose (bin);
	  cleanupAll ();
	  exit (EXIT_FAILURE);
	}

      if (flash_is_gpak)
	{
	  /* Set GPAK length information */
	  struct stat filestat;
	  printf ("Setting GPAK length in flash configuration table.\n");
	  if (fstat (fileno (bin), &filestat) != 0)
	    {
	      fprintf (stderr,
		       "Unable to get length information about GPAK binary!\n");
	      fclose (bin);
	      cleanupAll ();
	      exit (EXIT_FAILURE);
	    }

	  flash_set_gpaklen (fb, filestat.st_size);
	}
      printf ("Flash verification successful. Reboot required.\n");

      bool success = interactiveReboot (fb);

      cleanupAll ();
      exit ((success) ? EXIT_SUCCESS : EXIT_FAILURE);
    }


  smachine.featset = libfb_feature_set (status_get_dsi ());

  if (smachine.featset < FEATURE_PRE_2_0 || smachine.featset >= FEATURE_MAX)
    {
      fprintf (stderr,
	       "Warning! Failed to detect feature set from hardware build number!\n");
      cleanupAll ();
      exit (EXIT_FAILURE);
    }
  else if (vbose >= 2)
    {
      printf ("Found feature set index %d.\n", smachine.featset);
    }

  smachine.iec = statusIsIEC ();

  if (!smachine.iec && smachine.server == NULL)
    {
      fprintf (stderr,
	       "Configuration file incomplete! Missing destination MAC address!\n");
      libfb_destroy (fb);
      cleanupAll ();
      exit (EXIT_FAILURE);
    }

  nspans = statusGetSpans ();
  if (nspans < smachine.total_spans)
    {
      completeSpans ();
    }
  else if (nspans > smachine.total_spans)
    {
      statusDisplay ();
      printf ("However, the configuration requests %u spans.\n",
	      smachine.total_spans);
    }
  else if (vbose > 0)
    {
      statusDisplay ();
    }

  dsp_available = statusHasDSP ();

  printf ("DSP Status: %s\n",
	  (!smachine.dspdisabled
	   && dsp_available) ? "Available" : "Bypassed");

  if (dsp_available)
    {
      if (smachine.iec == 0)
	fb_tdmoectl (fb, 0);

      //Disable TDMoE generation before DSP config
      status = configureDSP (fb, span_list);

      if (status != E_SUCCESS && status != E_REBOOTDSP)
	fberror ("configureDSP", status);

      if (status != E_SUCCESS)
	{
	  libfb_destroy (fb);
	  cleanupAll ();
	  exit (status);
	}
    }
  status = configureFonebridge (fb);
  if (status != E_SUCCESS)
    {
      fberror ("configureFonebridge", status);
      status = EXIT_FAILURE;
    }
  else if (vbose > 0)
    {
      printf ("foneBRIDGE reconfigured!\n");
      status = EXIT_SUCCESS;
    }
  // Remove these for commit
#define DOOF_CMD_PCONFIG_STORE 39	/* Execute a store p_config command */
#define DOOF_CMD_PCONFIG_CLEAR 40	/* Execute a clear on the p_config
					   structure */
  if (clear_config)
    {
      printf ("Clearing foneBRIDGE configuration...");
      status = custom_cmd (fb, DOOF_CMD_PCONFIG_CLEAR, 0, NULL, 0);
      printf ("Done!\n");
    }
  else if (save_config)
    {
      printf ("Saving foneBRIDGE configuration...");
      status = custom_cmd (fb, DOOF_CMD_PCONFIG_STORE, 0, NULL, 0);
      printf ("Done!\n");
    }

  libfb_destroy (fb);
  cleanupAll ();

  exit (status);
}


/** @brief Ask the user to confirm a reboot of the foneBRIDGE.
 *
 * @return true if the board was rebooted.
 *
 * Currently a fixed delay [implemented with sleep(10)] is used to
 * allow the user to kill the process if they mistakenly responded
 * 'yes'.
 */
bool
interactiveReboot (libfb_t * f)
{
  printf ("Would you like to reboot the foneBRIDGE now? (Y/N) ");

  char in = (char) getchar ();
  if (in == 'n' || in == 'N')
    return false;

  else if (in == 'y' || in == 'Y')
    {
      printf ("Resetting foneBRIDGE in 10 seconds...\n");
      sleep (10);
      custom_cmd (f, DOOF_CMD_RESET, 0, NULL, 0);
      return true;
    }
  else
    {
      printf ("Couldn't understand that response. Will not reboot.\n");
      return false;
    }
}

bool
simpleReboot (libfb_t * f)
{
      printf ("Resetting foneBRIDGE in 10 seconds...\n");
      sleep (10);
      custom_cmd (f, DOOF_CMD_RESET, 0, NULL, 0);
      return true;	
}

/** @brief Determine if the priority settings are permitted
 *
 * @return true if valid
 */
static bool
priorities_valid ()
{
  int i;

  bool prio[4];
  bool zero[4];
  unsigned int zerocheck = 0;

  memset (prio, false, sizeof (prio));
  memset (zero, false, sizeof (zero));

  for (i = 0; i < 4; i++)
    {
      /* The priority of the inspected span */
      int cur_prio = smachine.priorities[i];
      if (cur_prio == -1)
	smachine.priorities[i] = i;

      if (cur_prio >= 0 && cur_prio < 4)
	{
	  /* We are a valid priority number. */
	  if (prio[cur_prio] && cur_prio != 0)
	    {
	      /* This priority number already taken and it isn't a
	         zero! */
	      return false;
	    }
	  if (cur_prio == 0)
	    {
	      /* The priority was zero, so we must later check, if we
	         have duplicate zeros, that they are _all_ duplicate
	         zeros. */
	      zerocheck++;
	      zero[i] = true;
	    }

	  /* Mark this priority number as in use. */
	  prio[cur_prio] = true;
	}
    }

  if (zerocheck > 1 && zerocheck != 4)
    return false;

  return true;
}

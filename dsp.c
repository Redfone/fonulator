/*
   fonulator 3 - foneBRIDGE configuration daemon
   (C) 2005-2007 Redfone Communications, LLC.
   www.red-fone.com

   by Brett Carrington <brettcar@gmail.com>

   DSP Implementations
*/

/** @file
 *
 * DSP Configuration Implementation

 * user_config represents the configuration from the user's
 * prespective, the first channel is '1' and the last is '124'. This
 * corresponds to 31 channels times 4 spans. 
 *
 * Internally the DSP has 128 channels and so 4 channels are
 * unused. Our DSP routines are responsible for mapping from the
 * user's perspective with 124 channels to the DSP's perspective with 128 channels.

 */
#include "fonulator.h"

#if defined(STDC_HEADERS) || defined(HAVE_STDLIB_H)
# include <stdlib.h>
#endif


/** @struct dsp_config
 * 
 * This structure represents the 128 channels on the DSP. It is
 * configured into the desired representation (E1, T1, Data, etc) and
 * then programmed to the DSP. This is taken from user_config but is
 * translated into the DSP's perspective.
 */

static struct
{
  dsp_chantype dsp[128];
}
dsp_config;

/** @struct flash_config
 *
 * Global structure holding state of all 128 channels as configured in
 * device currently.
 */
static struct
{
  dsp_chantype dsp[128];
}
flash_config;

/** @struct user_config
 *
 * Global structure holding state of all 128 channels as configured by
 * the user, from their perspective
 */
static struct
{
  dsp_chantype dsp[128];
}
user_config;

/** File-global pointer to GPAK flash configuration. Set by
    dspconfig_get_gpak_flash() */
static GPAK_FLASH_PARMS *gpak_flash;

/** @brief Sets gpak_flash
 *
 * @param f pointer to libfb context
 * @return FB_STATUS code
 *
 * Retrieves the GPAK flash parameters from the target device.
 */
static FB_STATUS
dspconfig_get_gpak_flash (libfb_t * f)
{
  int status;
  if (gpak_flash == NULL)
    {
      gpak_flash = malloc (sizeof (GPAK_FLASH_PARMS));
      if (gpak_flash == NULL)
	{
	  perror ("malloc");
	  return E_SYSTEM;
	}
    }

  memset (gpak_flash, 0, sizeof (GPAK_FLASH_PARMS));
  status = custom_cmd_reply (f, DOOF_CMD_GET_GPAK_FLASH_PARMS, 0, NULL, 0,
			     (char *) gpak_flash, sizeof (GPAK_FLASH_PARMS));
  return status;
}


/** @brief Transforms a dsp_chantype value into its string representation.
 * 
 * @param chan The channel type to translate
 * @return The string representation of chan
 */
char *
dspchan_to_string (dsp_chantype chan)
{
  switch (chan)
    {
    case DSP_DATA:
      return "Data";
    case DSP_A:
      return "Voice A";
    case DSP_B:
      return "Voice B";
    case DSP_OFF:
      return "Channel Off";
    case DSP_MAX:
      return "Invalid";
    }
  return "Unknown";
}


/** @brief Sets dsp_config into its default state for a particular
 * span.
 *
 * This will place the channels associated with a particular span into
 * the correct format for that span type. All voice channels are set
 * to DSP_B. If a PRI D-Channel is encountered it is set to DSP_DATA. 
 *
 * This does not actually program the DSP. It only sets up the
 * dsp_config structure.
 *
 * @param span The span you wish to set up a default DSP configuration for
 * @return FB_STATUS code indicating success or failure 
 */

FB_STATUS
dspconfig_setdefault (T_SPAN * span)
{
  int num = span->num;
  int maxchan, prichan, i;
  if (num < 1 || num > 4)
    return E_BADVALUE;

  maxchan = (span->config.E1Mode) ? 31 : 24;
  prichan = (span->config.E1Mode) ? 16 : 24;

  for (i = 0; i < maxchan; i++)
    {
      int I;

      I = 32 * (num - 1) + (i);

      if (!span->config.rbs_en && (i + 1) == prichan)
	{
	  dsp_config.dsp[I] = DSP_DATA;
	}
      else
	{
	  dsp_config.dsp[I] = DSP_B;
	}
    }

  return E_SUCCESS;
}

/** @brief Return current mask for channels of particular type and span 
 *
 * @param span the span number we are interested in
 * @param type which type of channels we are interested in
 * @return A bitmask representing which channels are currently set to the specified type in the specified span
 */
uint32_t
dspconfig_getmask (int span, dsp_chantype type)
{
  uint32_t mask = 0;
  int i;

  for (i = 0; i < 32; i++)
    {
      int j;
      j = 32 * span + i;
      if (dsp_config.dsp[j] == type)
	mask |= 1 << i;
    }
  return mask;
}


/** @brief fill in the flash_config struct from the native DSP data
    structure */
FB_STATUS
dspconfig_initflash (unsigned char *flashes)
{
  int i;
  for (i = 0; i < 128; i++)
    flash_config.dsp[i] = flashes[i];
  return E_SUCCESS;
}


/** @brief Print the current DSP configuration
 * @param flash prints the flash configuration if true, if false prints the dsp_config configuration
 *
 * If the parameter flash is false then the desired (parsed)
 * configuration is printed. Useful for debugging parsing routines.
 */
FB_STATUS
dspconfig_showconfig (bool flash)
{
  int i;
  for (i = 0; i < 128; i++)
    printf ("%s[%03d] is %s\n", flash ? "flash" : "dsp", i,
	    dspchan_to_string (flash ? flash_config.dsp[i] : dsp_config.
			       dsp[i]));
  return E_SUCCESS;
}

/**
 *
 * @return true if flash_config and dsp_config do not represent the same configuration 
 */
bool
dspconfig_differ ()
{
  int i = 0;
  bool differ = false;
  do
    {
      if (dsp_config.dsp[i] != flash_config.dsp[i])
	differ = true;

      i++;
    }
  while (i < 128 && !differ);
  return differ;
}

/** @brief places dsp_config into default state (all channels off) */
void
dspconfig_init (void)
{
  int i;
  for (i = 0; i < 128; i++)
    dsp_config.dsp[i] = DSP_OFF;
}


/** @brief places user_config into default state (all channels off) */
void
dspconfig_init_userconfig (void)
{
  int i;

  smachine.dspconfig = true;

  for (i = 0; i < 128; i++)
    user_config.dsp[i] = DSP_OFF;
}


/** @brief sets user_config channel `chan' into state `type'
 *
 * @param chan the channel to configure
 * @param type the type to set
 * @return success/error code
 * 
 */
FB_STATUS
dspconfig_set_userdigit (dsp_chantype type, int chan)
{
  if (chan < 1 || chan > 124)
    {
      /* Not a possible user-land channel */
      return E_BADINPUT;
    }
  user_config.dsp[chan] = type;
  return E_SUCCESS;
}

/** @brief sets a range of user_config channels all to the same type
 *
 * All channels in the range {min,max} are set to `type', min and max
 * inclusive.
 *
 * @param type the type to set
 * @param min the lower channel
 * @param max the upper channel
 * @return succes/error code
 */
FB_STATUS
dspconfig_set_userrange (dsp_chantype type, int min, int max)
{
  int i;
  if (min < 1 || min > 123 || max < 2 || max > 124 || min >= max)
    return E_BADINPUT;

  for (i = min; i <= max; i++)
    user_config.dsp[i] = type;
  return E_SUCCESS;
}

/** 
 *
 * This is the 'work-horse' function of the DSP routines. It
 * translates a user perspective configuration into the native DSP
 * perspective. Previous revisions of firmware used different
 * representations. Both are selectable here by using the
 * preprocessing defines.
 * 
 * This must only be called when user_config is completely filled as
 * desired. dsp_config will be populated based on those choices.
 *
 * @return success/failure code
 */
FB_STATUS
dspconfig_user_to_native (void)
{
  bool e1[4];
  register int chan = 0;
  int done_chan = 0;
  int boundaries[] = { 0, 32, 64, 96 };
  register int i;

  for (i = 0; i < 4; i++)
    {
      T_SPAN *s = get_span (i + 1);
      if (s)
	e1[i] = s->config.E1Mode;
      else
	e1[i] = true;
    }
#if 1
  /* Method 1: Fixed boundaries every 32 channels */
  for (i = 0; i < 4; i++)
    {
      register int j;
      dsp_chantype *dsp, *user;
      chan = boundaries[i];	/* We start on a particular boundary */

      /* Previous foneBRIDGE hardware required channel '0' to be
         skipped. This is no longer the case. */
#if 0
      dsp_config.dsp[chan] = DSP_OFF;	/* and always turn that channel off */
      chan++;			/* and start reading the user's settings in on the next channel */
#endif

      user = &user_config.dsp[done_chan + 1];	/* user channels are numbered starting with 1 */
      dsp = &dsp_config.dsp[chan];

      /* Copy the user's settings in. */
      for (j = 0; j < (e1[i] ? 31 : 24); j++)
	{
	  *dsp = *user;
	  user++;
	  dsp++;
	  done_chan++;
	}
    }
#else
  /* Method 2: Boundaries at end of spans, exactly */
  for (i = 0; i < 4; i++)
    {
      register int j;
      dsp_chantype *dsp, *user;

      dsp_config.dsp[chan] = DSP_OFF;	/* as in method 1, turn off the 0th channel */
      chan++;

      user = &user_config.dsp[done_chan + 1];
      dsp = &dsp_config.dsp[chan];

      /* Copy in their settings */
      for (j = 0; j < (e1[i] ? 31 : 24); j++)
	{
	  *dsp = *user;
	  user++;
	  dsp++;
	  done_chan++;
	  chan++;
	}
    }
#endif

  return E_SUCCESS;
}


/** @brief Disables or enables the DSP on a device
 *
 * @param f the libfb context of the device
 * @return success/error code
 */
FB_STATUS
bypassDSP (libfb_t * f, bool enable)
{
  int retval;
  uint8_t param, val = 1;
  param = enable ? DOOF_CMD_TDM_REGCTL_SET : DOOF_CMD_TDM_REGCTL_CLR;
  retval =
    custom_cmd_reply (f, DOOF_CMD_TDM_LB_SEL, param, (char *) &val, 1,
		      (char *) &val, 1);
  DBG (printf ("TDM Reg reads: 0x%X\n", val));
  PRINT_MAPPED_ERROR_IF_FAIL (retval);
  if (retval != FBLIB_ESUCCESS)
    return E_FBLIB;
  return E_SUCCESS;
}


/** @brief configure the DSP on a device
 *
 * Initalizes data structures, populates them, and the configures the
 * DSP
 *
 * @param f the libfb context of the device to configure
 * @param span_head the head of the linked list of T_SPANs 
 * @return success/error code
 */
FB_STATUS
configureDSP (libfb_t * f, DList * list)
{
  int i;
  uint32_t new_type[4];
  T_SPAN *first_span;
  bool need_update = false, need_update_companding = false;

  if (list == NULL || dlist_head (list) == NULL)
    return E_BADINPUT;

  first_span = dlist_data (dlist_head (list));


  /* If the DSP is to be disabled, we enable the bypass and return */
  if (smachine.dspdisabled)
    return bypassDSP (f, true);
  else
    bypassDSP (f, false);


  memset (new_type, 0, sizeof (uint32_t) * 4);

  if (dspconfig_get_gpak_flash (f) != E_SUCCESS)
    {
      printf ("Failed to read current DSP channel configuration.\n");
      return E_SYSTEM;
    }

  /* Read what is in the foneBRIDGE flash */
  dspconfig_initflash (gpak_flash->dsp_chan_type);

  /* Set up defaults, putting unneeded channels OFF first */
  dspconfig_init ();

  for (i = 0; i < statusGetSpans (); i++)
    dspconfig_setdefault (get_span (i + 1));

  /* Copy the user's configuration into the DSP's native channel numbers */
  if (smachine.dspconfig)
    dspconfig_user_to_native ();

  //  dspconfig_showconfig (false);

  /* First set companding if user didn't */
  if (smachine.companding == 0)
    smachine.companding =
      (first_span->config.E1Mode) ? DSP_COMP_TYPE_ALAW : DSP_COMP_TYPE_ULAW;
  else if (smachine.companding == -1)
    {
      /* Set all channels to DSP_DATA */
      memset (&dsp_config, 0, sizeof (dsp_config));
    }

  if (smachine.companding != gpak_flash->dsp_companding_type
      && smachine.companding != -1)
    {
      need_update_companding = true;
      printf ("Companding types differ in flash, update needed.\n");
    }

  if (dspconfig_differ ())
    need_update = true;

  /* Run four updates for each DSP */
  if (need_update)
    {
      dsp_chantype cfg_mode;
      for (cfg_mode = DSP_DATA; cfg_mode < DSP_MAX; cfg_mode++)
	{
	  uint32_t mask[4];
	  printf ("Setting mode %s\n", dspchan_to_string (cfg_mode));
	  for (i = 0; i < 4; i++)
	    {
	      mask[i] = dspconfig_getmask (i, cfg_mode);
	      DBG (printf ("%d: 0x%08X ", i, mask[i]));
	    }
	  DBG (printf ("\n"));
	  if (ec_set_chantype (f, cfg_mode, mask) != E_SUCCESS)
	    {
	      printf ("DSP channel configuration failed in %s mode.\n",
		      dspchan_to_string (cfg_mode));
	      return E_SYSTEM;
	    }
	  /* Succeded one write */
	  printf ("Successfully set %s mode.\n",
		  dspchan_to_string (cfg_mode));
	}
    }

  if (need_update_companding)
    {
      if (custom_cmd (f, DOOF_CMD_EC_SETPARM, DOOF_CMD_EC_SETPARM_COMP_TYPE,
		      (char *) &(smachine.companding), 1) != E_SUCCESS)
	{
	  printf ("Error setting companding type\n");
	  return E_SYSTEM;
	}

      /* Reboot here! */
      printf
	("The foneBRIDGE requires a reset to set the companding type..\n");
      printf
	("You will have to rerun fonulator after the reset is complete.\n");
      interactiveReboot (f);
      return E_REBOOTDSP;
    }

  return E_SUCCESS;
}

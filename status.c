/*
   fonulator 3 - foneBRIDGE configuration daemon
   (C) 2005-2007 Redfone Communications, LLC.
   www.red-fone.com

   by Brett Carrington <brettcar@gmail.com>

   foneBRIDGE Status Routines
*/

/** @file 
 * Status and query routines 
 */
#include "fonulator.h"

#if defined(STDC_HEADERS) || defined(HAVE_STDLIB_H)
# include <stdlib.h>
#endif

/** @var dsi
 *
 * Pointer to the DOOF_STATIC_INFO structure from the device we are
 * configuring or querying.
 */
DOOF_STATIC_INFO *dsi;

extern bool vbose;


/** @brief Print GPAK flash status for each DSP channel 
 * 
 * @param g the GPAK flash parameters 
 */
void
statusPrintGpak (GPAK_FLASH_PARMS * g)
{
  register int i;

  for (i = 0; i < 128; i++)
    {
      char ch;
      dsp_chantype dc = (dsp_chantype) g->dsp_chan_type[i];
      switch (dc)
	{
	case DSP_OFF:
	  ch = 'O';
	  break;
	case DSP_DATA:
	  ch = 'D';
	  break;
	case DSP_A:
	  ch = 'A';
	  break;
	case DSP_B:
	  ch = 'B';
	  break;
	default:
	  ch = 'I';
	  break;
	}
      if ((i % 16) == 0)
	printf ("\n0x%02X: ", i / 16);

      printf ("%c ", ch);
    }
  printf ("\n");
}

/** @brief Query a device and display results
 *
 * @param f the libfb context for the device
 * @return true on success 
 */
bool
queryFonebridge (libfb_t * f)
{
  bool dsp_available;

  /* Must call after statusInitalize() */
  if (dsi == NULL)
    return false;

  if (vbose > 0)
    {
#if 0
      struct tm *time_info = malloc (sizeof (struct tm));
      time_t caltime;
#endif
      IDT_LINK_CONFIG current[IDT_LINKS];
      register int i;

      if (configcheck_fb_udp (f, current) != FBLIB_ESUCCESS)
	return false;

      for (i = 0; i < dsi->spans; i++)
	{
	  printf ("Span %d configured as: ", 1 + i);
	  print_span_mode_idtlink (current[i], stdout);
	}

      printf ("SW ver: %s\n", dsi->sw_ver);
      printf ("SW Compile date: %s\n", dsi->sw_compile_date);
      printf ("SW Build: %d\n", dsi->build_num);

      set_reftime (f);
#if 0
      caltime = ntohl (dsi->epcs_config.mfg_date) + libfb_get_ctime (f);
      time_info = localtime (&caltime);
      printf ("Flash Date: %d/%d/%d %d:%d:%d\n",
	      time_info->tm_mon + 1, time_info->tm_mday,
	      time_info->tm_year + 1900, time_info->tm_hour,
	      time_info->tm_min, time_info->tm_sec);
      caltime = dsi->fpga_timestamp + libfb_get_systime (f);
      time_info = localtime (&caltime);
      printf ("SYSID Timestamp: %02d/%02d/%02d %02d:%02d:%02d\n",
	      time_info->tm_mon + 1, time_info->tm_mday,
	      time_info->tm_year + 1900, time_info->tm_hour,
	      time_info->tm_min, time_info->tm_sec);
#endif
      printf ("MAC Address: ");
      print_mac (dsi->epcs_config.mac_addr);
      printf ("IP Address[0]: ");
      print_ip (dsi->epcs_config.ip_address[0]);
      printf ("IP Address[1]: ");
      print_ip (dsi->epcs_config.ip_address[1]);
    }

  dsp_available = statusHasDSP ();
  printf ("DSP Status: %s\n",
	  (!smachine.dspdisabled
	   && dsp_available) ? "Available" : "Bypassed");

  if (vbose > 1 && dsp_available)
    {
      GPAK_FLASH_PARMS *gpak = malloc (sizeof (GPAK_FLASH_PARMS));
      if (gpak != NULL)
	{
	  memset (gpak, 0, sizeof (GPAK_FLASH_PARMS));
	  if (custom_cmd_reply (f, DOOF_CMD_GET_GPAK_FLASH_PARMS, 0, NULL, 0,
				(char *) gpak,
				sizeof (GPAK_FLASH_PARMS)) == FBLIB_ESUCCESS)
	    statusPrintGpak (gpak);
	  free (gpak);
	}
    }

  return true;
}


/** @brief initalized internal status data structures with current information from the device 
 * 
 * @param f the libfb context for the device
 * @return success/error code
 */
FB_STATUS
statusInitalize (libfb_t * f)
{
  fblib_err status;

  dsi = malloc (sizeof (DOOF_STATIC_INFO));
  if (dsi == NULL)
    {
      perror ("malloc");
      return E_SYSTEM;
    }

  status = udp_get_static_info (f, dsi);
  if (status != E_SUCCESS)
    {
      if (status == FBLIB_ETIMEDOUT)
	fprintf (stderr,
		 "Connection to device timed out! Check network or device power.\n");
      else
	PRINT_MAPPED_ERROR_IF_FAIL (status);
      return E_FBLIB;
    }
  return E_SUCCESS;
}

/** @brief free resources used by status data structures */
void
statusCleanup (void)
{
  if (dsi)
    free ((void *) dsi);
}


/** @brief display minimal statistics about the device */
void
statusDisplay (void)
{
  printf ("Found a foneBRIDGE with %i spans on %i transceivers.\n",
	  dsi->spans, dsi->devices);
}

/**
 * @return true if device is an inline echo canceller (IEC)
 */
bool
statusIsIEC (void)
{
  if (dsi->epcs_config.cfg_flags & 1)
    return true;
  return false;
}

/**
 * @return the number of spans on the device
 */
unsigned int
statusGetSpans (void)
{
  return dsi->spans;
}

/**
 * @return the number of transceivers on the device
 */
unsigned int
statusGetTransceivers (void)
{
  return dsi->devices;
}

/**
 * @return true if device has a supported DSP 
 */
bool
statusHasDSP (void)
{
  if (dsi->gpak_config.max_channels == 128)
    return true;
  return false;
}

/**
 * @return the DSI pointer (encapsulation)
 */
DOOF_STATIC_INFO *
status_get_dsi (void)
{
  return dsi;
}

/**  @brief PMON data structure setup
 * 
 * Builds a linked list of PMON register data structures based on
 * the span configuration given.
 *
 * @param the link configuration
 * @return a linked list of indirect register data structures
 */
DList *
statusSetupPMON (IDT_LINK_CONFIG * link)
{
  const libfb_PMONRegister *registerList;
  libfb_PMONRegister onereg;
  DList *list = malloc (sizeof (DList));
  int i = 0;

  if (list == NULL)
    {
      perror ("malloc");
      return NULL;
    }

  dlist_init (list, (void (*)(void *)) (pmon_destroy));

  if (link->E1Mode)
    {
      registerList = libfb_regs_E1;
    }
  else if (link->framing)
    {
      registerList = libfb_regs_T1ESF;
    }
  else
    {
      registerList = libfb_regs_T1SF;
    }

  onereg = registerList[i];
  while (onereg.length_bits > 0)
    {
      libfb_PMONRegister *datareg = malloc (sizeof (libfb_PMONRegister));
      if (datareg)
	{
	  memcpy (datareg, &onereg, sizeof (libfb_PMONRegister));

	  datareg->data = malloc (datareg->length_bytes);
	}

      if (datareg->data != NULL)
	{
	  dlist_ins_next (list, dlist_tail (list), datareg);
	}

      onereg = registerList[++i];
    }
  return list;
}

/** @brief Free a PMON register
 *
 * @param reg the register data structure to free
 */
void
pmon_destroy (libfb_PMONRegister * reg)
{
  if (reg->data != NULL)
    free (reg->data);
  free (reg);
  reg = NULL;
}

/** @brief Statistics query entry point
 *
 * Self-contained routine sets up PMON data structures, queries
 * device, and prints results.
 *
 * @param the device context for which statistics are desired
 * @return true on success
 */
bool
statusRunPMON (libfb_t * fb)
{
  int i;
  IDT_LINK_CONFIG links[IDT_LINKS];
  fblib_err ret;

  unsigned int devices = statusGetTransceivers ();
  unsigned int spans = statusGetSpans () / devices;;

  DList **spanregs;

  /* spanregs is a dynamically allocated array of DLists. For example:
   *  if there are two spans there will be spanregs[0] and
   *  spanregs[1].
   */

  spanregs = calloc (spans * devices, sizeof (DList *));
  if (spanregs == NULL)
    {
      perror ("calloc");
      return false;
    }

  /* First we must get the current link configurations */
  ret = configcheck_fb_udp (fb, links);
  if (ret != E_SUCCESS)
    {
      fberror ("statusRunPMON", ret);
      return false;
    }

  for (i = 0; i < spans * devices; i++)
    {
      spanregs[i] = statusSetupPMON (&links[i]);
    }

  for (i = 0; i < devices; i++)
    {
      int span;
      /* Send UPDAT transition and then process each register */
      for (span = 0; span < spans; span++)
	{
	  DListElmt *elmt = NULL;
	  DList *currentregs = NULL;
	  unsigned int span_index = (i * spans) + span;

	  /* Force registers to fetch updated counter */
	  libfb_updat_pmon (fb, span_index);

	  /* Get the list for the current span */
	  currentregs = spanregs[span_index];

	  /* Process each register */
	  elmt = dlist_head (currentregs);
	  while (elmt != NULL)
	    {
	      int byte;
	      libfb_PMONRegister *reg = dlist_data (elmt);
	      for (byte = 0; byte < reg->length_bytes; byte++)
		libfb_readidt_pmon (fb, span_index, reg->first_address + byte,
				    &reg->data[byte]);

	      elmt = dlist_next (elmt);
	    }

	  printf ("Span %d Statistics\n-----------------\n", span_index + 1);
	  statusPrintRegisters (currentregs);
	  if ((span_index + 1) < IDT_LINKS)
	    printf ("\n");
	}
    }
  return true;
}


/** @brief print a linked list of PMON registers
 * 
 * @param regs a DList of PMONRegister data
 */
void
statusPrintRegisters (DList * regs)
{
  DListElmt *elmt = dlist_head (regs);
  while (elmt != NULL)
    {
      int byte;
      libfb_PMONRegister *datareg = dlist_data (elmt);

      printf ("\t(%s) %-*s : ",
	      datareg->name, (int) (40 - strlen (datareg->name)),
	      datareg->longname);

      uint16_t data = 0;
      uint8_t mask = 0xFF;
      for (byte = 0; byte < datareg->length_bytes; byte++)
	{
	  if (datareg->length_bytes == 1)
	    mask >>= 8 - datareg->length_bits;
	  else if (datareg->length_bytes == (byte + 1))
	    mask >>= 8 - (datareg->length_bits - 8 * byte);

	  data |= (datareg->data[byte] & mask) << (8 * byte);
	}
      printf ("%d\n", data);


      elmt = dlist_next (elmt);
    }
}

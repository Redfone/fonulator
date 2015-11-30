#include "fonulator.h"

extern DOOF_STATIC_INFO *dsi;

/** @brief Compare two files for differences
 * 
 * Both files must be open already and seek'd to the starting point
 * the comparison should begin at.
 *
 * @param file1 the file to compare with file2
 * @param file2 the file to compare with file1
 * @return 0 if the files are the same, and -1 if they are different
 */
int
diff_file (FILE * file1, FILE * file2)
{
  unsigned char *buf1, *buf2;
  int x, y, z, retval = 0;
  int blk = 0;

  buf1 = malloc (EPCS_BLK_SIZE);
  buf2 = malloc (EPCS_BLK_SIZE);

  if (!buf1 || !buf2)
    {
      perror ("Error allocating buffers");
      return -1;
    }

  while (1)
    {
      x = fread ((void *) buf1, 1, EPCS_BLK_SIZE, file1);
      y = fread ((void *) buf2, 1, EPCS_BLK_SIZE, file2);

      if (x != y)
	{
	  retval = -1;
	  break;
	}
      if (!x)
	break;

      printf ("Checking block %d\n", blk);

      for (z = 0; z < y; z++)
	{
	  if (buf1[z] != buf2[z])
	    {
	      printf ("Error verifying block %d, offset %d\n", blk, z);
	      retval = -1;
	      break;
	    }
	}

      blk++;

    }

  free (buf1);
  free (buf2);

  return retval;
}



/**
 *
 * @param f the device context
 * @param bin the file to store the data in, it must already be opened for writing
 * @param address the address to read from
 * @param len the number of bytes to read
 * @return an error code if applicable
 */
FB_STATUS
read_flash_to_file (libfb_t * f, FILE * bin, int address, int len)
{
  while (len > 0)
    {
      uint8_t buffer[256];
      if (udp_read_blk (f, address, (len > 256) ? 256 : len, buffer) !=
	  FBLIB_ESUCCESS)
	{
	  fprintf (stderr, "Unable to read flash data from device!\n");
	  return E_FBLIB;
	}
      fwrite (buffer, 1, (len > 256) ? 256 : len, bin);
      len -= 256;
      address += 256;
    }
  return E_SUCCESS;
}

/**
 *
 * @param f the device context
 * @param bin the file to write, it must already be opened for reading
 * @param blk the block number to begin the write
 * @return an error code if applicable
 */
FB_STATUS
write_file_to_flash (libfb_t * f, FILE * bin, int blk)
{
  uint8_t *payload;
  int bytes, len, orig_blk;
  FILE *tmp;
  FB_STATUS result;

  orig_blk = blk;

  payload = malloc (EPCS_BLK_SIZE);
  if (payload == NULL)
    {
      fprintf (stderr,
	       "Unable to allocate enough memeory for flash upload operation.\n");
      perror ("malloc");
      return E_SYSTEM;
    }

  bytes = len = 0;
  while ((bytes = fread (payload, 1, EPCS_BLK_SIZE, bin)) > 0)
    {
      int i;

      len += bytes;
      printf ("Read in %d bytes, Block %d\n", bytes, blk);

      for (i = 0; i < EPCS_BLK_SIZE; i += 256)
	{
	  if (udp_write_to_blk (f, i, 256, payload + i) != FBLIB_ESUCCESS)
	    {
	      fprintf (stderr,
		       "Error writing to block %d at offset %d (0x%X)\n", blk,
		       i, i);
	      free (payload);
	      return E_FBLIB;
	    }
	}

      if (udp_start_blk_write (f, blk) != FBLIB_ESUCCESS)
	{
	  fprintf (stderr, "Error executing write on block %d\n", blk);
	  free (payload);
	  return E_FBLIB;
	}

      blk++;
    }

  printf ("Starting flash verification\n");

  /* Verify success of flashing */
  tmp = tmpfile ();
  if (tmp == NULL)
    {
      perror ("tmpfile");
      free (payload);
      return E_SYSTEM;
    }

  free (payload);
  if ((result =
       read_flash_to_file (f, tmp, orig_blk * EPCS_BLK_SIZE,
			   len)) != E_SUCCESS)
    {
      fprintf (stderr, "Error reading flash to check results!\n");
      fclose (tmp);
      return result;
    }

  rewind (bin);
  rewind (tmp);

  if (diff_file (tmp, bin) != 0)
    {
      fprintf (stderr, "Comparision between input and flash failed!\n");
      fclose (tmp);
      return E_FBLIB;
    }

  fclose (tmp);
  return E_SUCCESS;
}


void
show_warning ()
{
  printf ("**************************************************\n");
  printf ("* WARNING: Any interruption in network service\n");
  printf ("* to the target WILL cause flash corruption\n");
  printf ("* and will render the target unusable. Do NOT\n");
  printf ("* remove power or the network connection\n");
  printf ("* while updating firmware.\n");
  printf ("**************************************************\n");
  printf ("Press return to continue...\n");

  getchar ();
}

/** @brief Modify the EPCS flash info to reflect a new GPAK file length
 *
 *  Must be called after statusInitalize()
 * 
 *  @param f the device context
 *  @param bytes the file length in bytes
 *  @return an error code, if applicable
 */
FB_STATUS
flash_set_gpaklen (libfb_t * f, size_t bytes)
{
  int epcs_blk, epcs_location;
  EPCS_CONFIG current;

  if (dsi == NULL)
    return E_BADSTATE;

  epcs_blk = dsi->epcs_blocks - 2;
  epcs_location = epcs_blk * 65536;

  if (udp_read_blk
      (f, epcs_location, sizeof (EPCS_CONFIG),
       (uint8_t *) & current) != FBLIB_ESUCCESS)
    return E_FBLIB;

  /* Don't do any writes if the lengths are the same */
  if (current.gpak_len != bytes)
    {
      current.gpak_len = bytes;
      current.crc16 =
	crc_16 ((uint8_t *) & current, sizeof (EPCS_CONFIG) - 2);
      if (udp_write_to_blk (f, 0, sizeof (EPCS_CONFIG), (uint8_t *) & current)
	  != FBLIB_ESUCCESS)
	return E_FBLIB;
      if (udp_start_blk_write (f, epcs_blk) != FBLIB_ESUCCESS)
	return E_FBLIB;
    }

  return E_SUCCESS;
}

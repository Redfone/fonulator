#if HAVE_CONFIG_H
# include <config.h>
#endif

#undef malloc

#if HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

void *malloc ();

/* Allocate an N-byte block of memory from the heap. If N is zero, allocate a 1-byte block. */

void *
rpl_malloc (size_t n)
{
  if (n == 0)
    n = 1;
  return malloc (n);
}

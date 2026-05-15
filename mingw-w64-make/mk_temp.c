
/* Sample from example in GNU Make Manual
 * (https://www.gnu.org/software/make/manual/html_node/Loaded-Object-Example.html#Loaded-Object-Example)
 * with minor edits for error reporting.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <gnumake.h>

int plugin_is_GPL_compatible;

char *
gen_tmpfile ( const char *nm, unsigned int argc, char **argv )
{
  int fd;

  /* Compute the size of the filename and allocate space for it.  */
  int len = strlen ( argv[0] ) + 6 + 1;
  char *buf = gmk_alloc ( len );

  strcpy ( buf, argv[0] );
  strcat ( buf, "XXXXXX" );

  fd = mkstemp ( buf );
  if ( fd >= 0 ) {
    /* Don't leak the file descriptor.  */
    close ( fd );
  } else {
    /* Failure.  */
    fprintf ( stderr, "%s:%d: error: mkstemp(%s) failed: %s\n"
              , __FILE__, __LINE__, buf, strerror ( errno ) );
    gmk_free ( buf );
    buf = NULL;
  }

  return buf;
}

int
mk_temp_gmk_setup (const gmk_floc *floc)
{
  const char* funcnm = "mk-temp";

  fprintf(stderr, "%s:%d(%s): Registering function with make name '%s', implemented in '%s'.\n",
          floc->filenm, floc->lineno, __FILE__, funcnm, "mk_temp.dll");

  /* Register the function with make name "mk-temp".  */
  gmk_add_function ( funcnm, gen_tmpfile, 1, 1, 1 );
  return 1;
}

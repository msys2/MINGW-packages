/*
      .Some useful path tools.
   Written by Ray Donnelly in 2014.
   Licensed under CC0. No warranty.
 */

#ifndef PATHTOOLS_H
#define PATHTOOLS_H

#include <unistd.h>
#if defined(__APPLE__)
#include <stdlib.h>
#else
#include <malloc.h>
#endif
#include <stdio.h>

char * malloc_copy_string(char const * original);

/* In-place replaces any '\' with '/' and any '//' with '/' */
void sanitise_path(char * path);

/* Uses a host OS specific function to determine the path of the executable,
   if IMPLEMENT_SYS_GET_EXECUTABLE_PATH is defined, otherwise uses argv0. */
int get_executable_path(char const * argv0, char * result, ssize_t max_size);

/* Where possible, in-place removes occourances of '.' and 'path/..' */
void simplify_path(char * path);

/* Allocates (via malloc) and returns the path to get from from to to. */
char * get_relative_path(char const * from, char const * to);

/* Allocates (via malloc) and returns a relocated path from a single Unix path.
   This function makes large assumptions regarding PREFIX and is therefore very
   much an MSYS2-only function. It operates by removing the first folder of the
   input and final folder of the program executable then appending the input to
   that.
*/
char const * msys2_get_relocated_single_path(char const * unix_path);

/* Allocates (via malloc) and for each ':' delimited Unix sub-path,  returns the
   result of applying the procedure detailed for msys2_get_relocated_single_path
   on that Unix sub-path with the results joined up again with a ';' delimiter.
   It implements the same logic in msys2_get_relocated_single_path to reduce the
   the number of mallocs.
*/
char * msys2_get_relocated_path_list(char const * paths);

#endif /* PATHTOOLS_H */

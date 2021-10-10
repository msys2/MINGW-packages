/*
      .Some useful path tools.
   Written by Ray Donnelly in 2014.
   Licensed under CC0. No warranty.
 */

#ifndef MSYS2_RELOCATE_H
#define MSYS2_RELOCATE_H

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

#endif /* MSYS2_RELOCATE_H */

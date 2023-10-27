/*
      .Some useful path tools.
        .ASCII only for now.
   .Written by Ray Donnelly in 2014.
   .Licensed under CC0 (and anything.
  .else you need to license it under).
      .No warranties whatsoever.
  .email: <mingw.android@gmail.com>.
 */

#ifndef PATHTOOLS_PRIVATE_H
#define PATHTOOLS_PRIVATE_H

#include <stddef.h>
#include <stdint.h>

/* In-place replaces any '\' with '/' and any '//' with '/' */
void sanitise_path(char * path);

#endif /* PATHTOOLS_PRIVATE_H */

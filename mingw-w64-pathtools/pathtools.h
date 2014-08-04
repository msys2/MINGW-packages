/*
      .Some useful path tools.
   Written by Ray Donnelly in 2014.
   Licensed under CC0. No warranty.
 */

#ifndef PATHTOOLS_H
#define PATHTOOLS_H

#if defined(__APPLE__)
#include <stdlib.h>
#else
#include <malloc.h>
#endif
#include <limits.h>
#include <stdio.h>
#include <string.h>
#if defined(__linux__) || defined(__CYGWIN__) || defined(__MSYS__)
#include <alloca.h>
#endif
#include <unistd.h>

/* If you don't define this, then get_executable_path()
   can only use argv[0] which will often not work well */
#define IMPLEMENT_SYS_GET_EXECUTABLE_PATH

#if defined(IMPLEMENT_SYS_GET_EXECUTABLE_PATH)
#if defined(__linux__) || defined(__CYGWIN__) || defined(__MSYS__)
/* Nothing needed, unistd.h is enough. */
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#elif defined(_WIN32)
#define WIN32_MEAN_AND_LEAN
#include <windows.h>
#include <psapi.h>
#endif
#endif /* defined(IMPLEMENT_SYS_GET_EXECUTABLE_PATH) */

/* These functions are used to support relocation.*/
char * malloc_copy_string (char const * original);
char * sanitise_path(char * path);

/* Returns the malloc'ed string. Caller should free using free(void*) */
//static char * malloc_copy_string (char const * original);

/* Uses a host OS specific function to determine the path of the executable,
   if IMPLEMENT_SYS_GET_EXECUTABLE_PATH is defined, otherwise uses argv0. */
int get_executable_path(char const * argv0, char * result, ssize_t max_size);

/* Where possible, removes occourances of '.' and 'path/..' */
void simplify_path (char * path);

/* mallocs and returns the path to get from from to to. Caller should free using free(void*) */
char * get_relative_path (char const * from, char const * to);

/* returns relocated path from single unix path. Caller should free using free(void*) */
const char * get_relocated_single_path(char const * unix_path);

#endif 

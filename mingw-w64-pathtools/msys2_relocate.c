/*
      .Some useful path tools.
   Written by Ray Donnelly in 2014.
   Licensed under CC0. No warranty.
 */

#include "pathtools.h"
#if defined(_WIN32)
/* All this just for MAX_PATH */
#define WIN32_MEAN_AND_LEAN
#include <windows.h>
#else
#include <limits.h>
#endif

char const *
msys2_get_relocated_single_path(char const * unix_path)
{
  char * unix_part = (char *) strip_n_prefix_folders (unix_path, 1);
  char win_part[MAX_PATH];
  get_executable_path (NULL, &win_part[0], MAX_PATH);
  strip_n_suffix_folders (&win_part[0], 2); /* 2 because the file name is present. */
  char * new_path = (char *) malloc (strlen (unix_part) + strlen (win_part) + 1);
  strcpy (new_path, win_part);
  strcat (new_path, unix_part);
  return new_path;
}

char *
msys2_get_relocated_path_list(char const * paths)
{
  char win_part[MAX_PATH];
  get_executable_path (NULL, &win_part[0], MAX_PATH);
  strip_n_suffix_folders (&win_part[0], 2); /* 2 because the file name is present. */

  char **arr = NULL;

  size_t count = split_path_list (paths, ':', &arr);
  int result_size = 1 + (count - 1); /* count - 1 is for ; delim. */
  size_t i;
  for (i = 0; i < count; ++i)
  {
    arr[i] = (char *) strip_n_prefix_folders (arr[i], 1);
    result_size += strlen (arr[i]) + strlen (win_part);
  }
  char * result = (char *) malloc (result_size);
  if (result == NULL)
  {
    return NULL;
  }
  result[0] = '\0';
  for (i = 0; i < count; ++i)
  {
    strcat (result, win_part);
    strcat (result, arr[i]);
    if (i != count-1)
    {
#if defined(_WIN32)
      strcat (result, ";");
#else
      strcat (result, ":");
#endif
    }
  }
  free ((void*)arr);
  return result;
}

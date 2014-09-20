/*
      .Some useful path tools.
        .ASCII only for now.
   .Written by Ray Donnelly in 2014.
   .Licensed under CC0 (and anything.
  .else you need to license it under).
      .No warranties whatsoever.
  .email: <mingw.android@gmail.com>.
 */

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

#include "pathtools.h"

char *
malloc_copy_string(char const * original)
{
  char * result = (char *) malloc (sizeof (char*) * strlen (original)+1);
  if (result != NULL)
  {
    strcpy (result, original);
  }
  return result;
}

void
sanitise_path(char * path)
{
  size_t path_size = strlen (path);

  /* Replace any '\' with '/' */
  char * path_p = path;
  while ((path_p = strchr (path_p, '\\')) != NULL)
  {
    *path_p = '/';
  }
  /* Replace any '//' with '/' */
  path_p = path;
  while ((path_p = strstr (path_p, "//")) != NULL)
  {
    memmove (path_p, path_p + 1, path_size--);
  }
  return;
}

char *
get_relative_path(char const * from_in, char const * to_in)
{
  size_t from_size = (from_in == NULL) ? 0 : strlen (from_in);
  size_t to_size = (to_in == NULL) ? 0 : strlen (to_in);
  size_t max_size = (from_size + to_size) * 2 + 4;
  char * scratch_space = (char *) alloca (from_size + 1 + to_size + 1 + max_size + max_size);
  char * from;
  char * to;
  char * common_part;
  char * result;
  size_t count;

  /* No to, return "./" */
  if (to_in == NULL)
  {
    return malloc_copy_string ("./");
  }

  /* If alloca failed or no from was given return a copy of to */
  if (   from_in == NULL
      || scratch_space == NULL )
  {
    return malloc_copy_string (to_in);
  }

  from = scratch_space;
  strcpy (from, from_in);
  to = from + from_size + 1;
  strcpy (to, to_in);
  common_part = to + to_size + 1;
  result = common_part + max_size;
  simplify_path (from);
  simplify_path (to);

  result[0] = '\0';

  size_t match_size_dirsep = 0;  /* The match size up to the last /. Always wind back to this - 1 */
  size_t match_size = 0;         /* The running (and final) match size. */
  size_t largest_size = (from_size > to_size) ? from_size : to_size;
  int to_final_is_slash = (to[to_size-1] == '/') ? 1 : 0;
  char from_c;
  char to_c;
  for (match_size = 0; match_size < largest_size; ++match_size)
  {
    /* To simplify the logic, always pretend the strings end with '/' */
    from_c = (match_size < from_size) ? from[match_size] : '/';
    to_c =   (match_size <   to_size) ?   to[match_size] : '/';

    if (from_c != to_c)
    {
      if (from_c != '\0' || to_c != '\0')
      {
        match_size = match_size_dirsep;
      }
      break;
    }
    else if (from_c == '/')
    {
      match_size_dirsep = match_size;
    }
  }
  strncpy (common_part, from, match_size);
  common_part[match_size] = '\0';
  from += match_size;
  to += match_size;
  size_t ndotdots = 0;
  char const* from_last = from + strlen(from) - 1;
  while ((from = strchr (from, '/')) && from != from_last)
  {
    ++ndotdots;
    ++from;
  }
  for (count = 0; count < ndotdots; ++count)
  {
    strcat(result, "../");
  }
  if (strlen(to) > 0)
  {
    strcat(result, to+1);
  }
  /* Make sure that if to ends with '/' result does the same, and
     vice-versa. */
  size_t size_result = strlen(result);
  if ((to_final_is_slash == 1)
      && (!size_result || result[size_result-1] != '/'))
  {
    strcat (result, "/");
  }
  else if (!to_final_is_slash
           && size_result && result[size_result-1] == '/')
  {
    result[size_result-1] = '\0';
  }

  return malloc_copy_string (result);
}

void
simplify_path(char * path)
{
  ssize_t n_toks = 1; /* in-case we need an empty initial token. */
  ssize_t i, j;
  size_t tok_size;
  size_t in_size = strlen (path);
  int it_ended_with_a_slash = (path[in_size - 1] == '/') ? 1 : 0;
  char * result = path;
  sanitise_path(result);
  char * result_p = result;

  do
  {
    ++n_toks;
    ++result_p;
  } while ((result_p = strchr (result_p, '/')) != NULL);

  result_p = result;
  char const ** toks = (char const **) alloca (sizeof (char const*) * n_toks);
  n_toks = 0;
  do
  {
    if (result_p > result)
    {
      *result_p++ = '\0';
    }
    else if (*result_p == '/')
    {
      /* A leading / creates an empty initial token. */
      toks[n_toks++] = result_p;
      *result_p++ = '\0';
    }
    toks[n_toks++] = result_p;
  } while ((result_p = strchr (result_p, '/')) != NULL);

  /* Remove all non-leading '.' and any '..' we can match
     with an earlier forward path (i.e. neither '.' nor '..') */
  for (i = 1; i < n_toks; ++i)
  {
    int removals[2] = { -1, -1 };
    if ( strcmp (toks[i], "." ) == 0)
    {
      removals[0] = i;
    }
    else if ( strcmp (toks[i], ".." ) == 0)
    {
      /* Search backwards for a forward path to collapse.
         If none are found then the .. also stays. */
      for (j = i - 1; j > -1; --j)
      {
        if ( strcmp (toks[j], "." )
          && strcmp (toks[j], ".." ) )
        {
          removals[0] = j;
          removals[1] = i;
          break;
        }
      }
    }
    for (j = 0; j < 2; ++j)
    {
      if (removals[j] >= 0) /* Can become -2 */
      {
        --n_toks;
        memmove (&toks[removals[j]], &toks[removals[j]+1], (n_toks - removals[j])*sizeof (char*));
        --i;
        if (!j)
        {
          --removals[1];
        }
      }
    }
  }
  result_p = result;
  for (i = 0; i < n_toks; ++i)
  {
    tok_size = strlen(toks[i]);
    memcpy (result_p, toks[i], tok_size);
    result_p += tok_size;
    if ((!i || tok_size) && ((i < n_toks - 1) || it_ended_with_a_slash == 1))
    {
      *result_p = '/';
      ++result_p;
    }
  }
  *result_p = '\0';
}

/* Returns actual_to by calculating the relative path from -> to and
   applying that to actual_from. An assumption that actual_from is a
   dir is made, and it may or may not end with a '/' */
char const *
get_relocated_path (char const * from, char const * to, char const * actual_from)
{
  char const * relative_from_to = get_relative_path (from, to);
  char * actual_to = (char *) malloc (strlen(actual_from) + 2 + strlen(relative_from_to));
  return actual_to;
}

int
get_executable_path(char const * argv0, char * result, ssize_t max_size)
{
  char * system_result = (char *) alloca (max_size);
  ssize_t system_result_size = -1;
  ssize_t result_size = -1;

  if (system_result != NULL)
  {
#if defined(IMPLEMENT_SYS_GET_EXECUTABLE_PATH)
#if defined(__linux__) || defined(__CYGWIN__) || defined(__MSYS__)
    system_result_size = readlink("/proc/self/exe", system_result, max_size);
#elif defined(__APPLE__)
    uint32_t bufsize = (uint32_t)max_size;
    if (_NSGetExecutablePath(system_result, &bufsize) == 0)
    {
      system_result_size = (ssize_t)bufsize;
    }
#elif defined(_WIN32)
    unsigned long bufsize = (unsigned long)max_size;
    system_result_size = GetModuleFileNameA(NULL, system_result, bufsize);
    if (system_result_size == 0 || system_result_size == (ssize_t)bufsize)
    {
      /* Error, possibly not enough space. */
      system_result_size = -1;
    }
    else
    {
      /* Early conversion to unix slashes instead of more changes
         everywhere else .. */
      char * winslash;
      system_result[system_result_size] = '\0';
      while ((winslash = strchr (system_result, '\\')) != NULL)
      {
        *winslash = '/';
      }
    }
#else
#warning "Don't know how to get executable path on this system"
#endif
#endif /* defined(IMPLEMENT_SYS_GET_EXECUTABLE_PATH) */
  }
  /* Use argv0 as a default in-case of failure */
  if (system_result_size != -1)
  {
    strncpy (result, system_result, system_result_size);
    result[system_result_size] = '\0';
  }
  else
  {
    if (argv0 != NULL)
    {
      strncpy (result, argv0, max_size);
      result[max_size-1] = '\0';
    }
    else
    {
      result[0] = '\0';
    }
  }
  result_size = strlen (result);
  return result_size;
}

char const *
strip_n_prefix_folders(char const * path, size_t n)
{
  if (path == NULL)
  {
    return NULL;
  }

  if (path[0] != '/')
  {
    return path;
  }

  char const * last = path;
  while (n-- && path != NULL)
  {
    last = path;
    path = strchr (path + 1, '/');
  }
  return (path == NULL) ? last : path;
}

void
strip_n_suffix_folders(char * path, size_t n)
{
  if (path == NULL)
  {
    return;
  }
  while (n--)
  {
    if (strrchr (path + 1, '/'))
    {
      *strrchr (path + 1, '/') = '\0';
    }
    else
    {
      return;
    }
  }
  return;
}

size_t
split_path_list(char const * path_list, char split_char, char *** arr)
{
  size_t path_count;
  size_t path_list_size;
  char const * path_list_p;

  path_list_p = path_list;
  if (path_list == NULL || path_list[0] == '\0')
  {
    return 0;
  }
  path_list_size = strlen (path_list);

  path_count = 0;
  do
  {
    ++path_count;
    ++path_list_p;
  }
  while ((path_list_p = strchr (path_list_p, split_char)) != NULL);

  /* allocate everything in one go. */
  char * all_memory = (char *) malloc (sizeof (char *) * path_count + strlen(path_list) + 1);
  if (all_memory == NULL)
    return 0;
  *arr = (char **)all_memory;
  all_memory += sizeof (char *) * path_count;

  path_count = 0;
  path_list_p = path_list;
  char const * next_path_list_p = 0;
  do
  {
    next_path_list_p = strchr (path_list_p, split_char);
    if (next_path_list_p != NULL)
    {
      ++next_path_list_p;
    }
    size_t this_size = (next_path_list_p != NULL)
                       ? next_path_list_p - path_list_p - 1
                       : &path_list[path_list_size] - path_list_p;
    memcpy (all_memory, path_list_p, this_size);
    all_memory[this_size] = '\0';
    (*arr)[path_count++] = all_memory;
    all_memory += this_size + 1;
  } while ((path_list_p = next_path_list_p) != NULL);

  return path_count;
}

char *
get_relocated_path_list(char const * from, char const * to_path_list)
{
  char exe_path[MAX_PATH];
  char * temp;
  get_executable_path (NULL, &exe_path[0], sizeof (exe_path) / sizeof (exe_path[0]));
  if ((temp = strrchr (exe_path, '/')) != NULL)
  {
    temp[1] = '\0';
  }

  char **arr = NULL;
  /* Ask Alexey why he added this. Are we not 100% sure
     that we're dealing with unix paths here? */
  char split_char = ':';
  if (strchr (to_path_list, ';'))
  {
    split_char = ';';
  }
  size_t count = split_path_list (to_path_list, split_char, &arr);
  int result_size = 1 + (count - 1); /* count - 1 is for ; delim. */
  size_t exe_path_size = strlen (exe_path);
  size_t i;
  /* Space required is:
     count * (exe_path_size + strlen (rel_to_datadir))
     rel_to_datadir upper bound is:
     (count * strlen (from)) + (3 * num_slashes (from))
     + strlen(arr[i]) + 1.
     .. pathalogically num_slashes (from) is strlen (from)
     (from = ////////) */
  size_t space_required = (count * (exe_path_size + 4 * strlen (from))) + count - 1;
  for (i = 0; i < count; ++i)
  {
    space_required += strlen (arr[i]);
  }
  char * scratch = (char *) alloca (space_required);
  if (scratch == NULL)
    return NULL;
  for (i = 0; i < count; ++i)
  {
    char * rel_to_datadir = get_relative_path (from, arr[i]);
    scratch[0] = '\0';
    arr[i] = scratch;
    strcat (scratch, exe_path);
    strcat (scratch, rel_to_datadir);
    simplify_path (arr[i]);
    size_t arr_i_size = strlen (arr[i]);
    result_size += arr_i_size;
    scratch = arr[i] + arr_i_size + 1;
  }
  char * result = (char *) malloc (result_size);
  if (result == NULL)
  {
    return NULL;
  }
  result[0] = '\0';
  for (i = 0; i < count; ++i)
  {
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

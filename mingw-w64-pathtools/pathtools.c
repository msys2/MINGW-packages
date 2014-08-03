/*
      .Some useful path tools.
   Written by Ray Donnelly in 2014.
   Licensed under CC0. No warranty.
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

static char *
malloc_copy_string (char const * original)
{
  char * result = (char *) malloc (sizeof(char*) * strlen(original)+1);
  if (result != NULL)
  {
    strcpy (result, original);
  }
  return result;
}

static char *
sanitise_path(char * path)
{
  size_t path_size = strlen(path);

  /* Replace any '\' with '/' */
  char * path_p = path;
  while ((path_p = strchr(path_p, '\\')) != NULL)
  {
    *path_p = '/';
  }
  /* Replace any '//' with '/' */
  path_p = path;
  while ((path_p = strstr(path_p, "//")) != NULL)
  {
    memmove(path_p, path_p + 1, path_size--);
  }
  return path;
}

void
sanitise_path_debug(char const * path, char const * expected)
{
  char * path_copy = (char *) alloca (strlen(path)+1);
  strcpy (path_copy, path);
  path_copy = sanitise_path (path_copy);

  int ok = (strcmp(path_copy, expected) == 0) ? 1 : 0;
  if (ok)
  {
    printf ("PASS: %s cleans up to %s\n", path, path_copy);
  }
  else
  {
    printf ("FAIL: %s cleans up to %s, should be %s\n", path, path_copy, expected);
    _exit(1);
  }
}

char *
get_relative_path (char const * from, char const * to)
{
  size_t from_size = strlen (from);
  size_t to_size = strlen (to);
  size_t max_size = (from_size + to_size) * 2 + 4;
  char * common_part = (char *) alloca (max_size);
  char * result = (char *) alloca (max_size);
  size_t count;

  /* If either alloca failed, no from was given, from is not absolute
     or either to or from contains '.' then return a copy of to; it's
     the best we can do in this instance (though we could call a path
     normalization function for some of these conditions).
   */
  if (common_part == NULL
      || result == NULL
      || from == NULL
      || from[0] != '/'
      || strchr(to, '.') != NULL
      || strchr(from, '.') != NULL)
  {
    return malloc_copy_string (to);
  }

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
  while ((from = strchr(from, '/')) && from != from_last)
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
simplify_path (char * path)
{
  ssize_t n_toks = 1; /* in-case we need an empty initial token. */
  ssize_t i, j;
  size_t tok_size;
  size_t in_size = strlen (path);
  int it_ended_with_a_slash = (path[in_size - 1] == '/') ? 1 : 0;
  char * result = path;
  char * result_p = sanitise_path(result);

  do
  {
    ++n_toks;
    ++result_p;
  } while ((result_p = strchr(result_p, '/')) != NULL);

  result_p = result;
  char const ** toks = (char const **) alloca (sizeof(char const*) * n_toks);
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
  } while ((result_p = strchr(result_p, '/')) != NULL);

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
        memmove (&toks[removals[j]], &toks[removals[j]+1], (n_toks - removals[j])*sizeof(char*));
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

void
simplify_path_debug (const char * input, const char * expected)
{
  char * input_copy = malloc_copy_string (input);
  if ( input_copy == NULL )
  {
    _exit(1);
  }
  simplify_path (input_copy);
  int ok = (strcmp(input_copy, expected) == 0) ? 1 : 0;
  if (ok)
  {
    printf ("PASS: %s simplifies to %s\n", input, input_copy);
  }
  else
  {
    printf ("FAIL: %s simplifies to %s, should be %s\n", input, input_copy, expected);
    _exit(1);
  }
  free ((void *)input_copy);
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

void
get_relative_path_debug (char const * from, char const * to, char const * expected)
{
  char const * result = get_relative_path (from, to);
  if ( result == NULL )
  {
    _exit(1);
  }
  else
  {
    int ok = (strcmp(result, expected) == 0) ? 1 : 0;
    if (ok)
    {
      printf ("PASS: %s to %s is %s\n", from, to, result);
    }
    else
    {
      printf ("FAIL: %s to %s is %s, should be %s\n", from, to, result, expected);
      _exit(1);
    }
    free ((void *)result);
  }
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
    if (system_result_size == 0 || system_result_size == bufsize)
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

const char *
get_relocated_single_path(char const * unix_path)
{
  char * unix_part = (char *) strip_n_prefix_folders (unix_path, 1);
  char win_part[MAX_PATH];
  get_executable_path (NULL, &win_part[0], MAX_PATH);
  strip_n_suffix_folders (&win_part[0], 2); /* 2 because the file name is present. */
  char * new_path = (char *) malloc (strlen (unix_part) + strlen (win_part) + 1);
  strcpy(new_path, win_part);
  strcat(new_path, unix_part);
  return new_path;
}

#define X509_PRIVATE_DIR "/mingw64/ssl/private"
const char *
X509_get_default_private_dir(void)
{
#ifdef __MINGW32__
  return get_relocated_single_path (X509_PRIVATE_DIR);
#else
  return (X509_PRIVATE_DIR);
#endif
}

int main(int argc, char *argv[])
{
#define BINDIR  "/mingw64/bin"
#define DATADIR "/mingw64/share"

  char exe_path[PATH_MAX];
  get_executable_path (argv[0], &exe_path[0], sizeof(exe_path)/sizeof(exe_path[0]));
  printf ("executable path is %s\n", exe_path);

  char * rel_to_datadir = get_relative_path (BINDIR, DATADIR);
  if (strrchr (exe_path, '/') != NULL)
  {
     strrchr (exe_path, '/')[1] = '\0';
  }
  strcat (exe_path, rel_to_datadir);
  simplify_path (&exe_path[0]);
  printf("real path of DATADIR is %s\n", exe_path);

  if (argc >= 2)
  {
    get_relative_path_debug (argv[argc-2], argv[argc-1], 0);
  }
  get_relative_path_debug ("/a/b/c/d",                "/a/b/c",            "..");
  get_relative_path_debug ("/a/b/c/d/",               "/a/b/c/",           "../");
  get_relative_path_debug ("/",                       "/",                 "/");
  get_relative_path_debug ("/a/testone/c/d",          "/a/testtwo/c",      "../../../testtwo/c");
  get_relative_path_debug ("/a/testone/c/d/",         "/a/testtwo/c/",     "../../../testtwo/c/");
  get_relative_path_debug ("/home/part2/part3/part4", "/work/proj1/proj2", "../../../../work/proj1/proj2");

  simplify_path_debug ("a/b/..", "a");
  simplify_path_debug ("a/b/c/../../", "a/");
  simplify_path_debug ("a/../a/..",    "");
  simplify_path_debug ("../a/../a/",   "../a/");

  simplify_path_debug ("./././",     "./");
  simplify_path_debug ("/test/",     "/test/");
  simplify_path_debug (".",          ".");
  simplify_path_debug ("..",         "..");
  simplify_path_debug ("../",        "../");
  simplify_path_debug ("././.",      ".");
  simplify_path_debug ("../..",      "../..");
  simplify_path_debug ("/",          "/");
  simplify_path_debug ("./test/",    "./test/");
  simplify_path_debug ("./test",     "./test");
  simplify_path_debug ("/test",      "/test");
  simplify_path_debug ("../test",    "../test");
  simplify_path_debug ("../../test", "../../test");
  simplify_path_debug ("../test/..", "..");
  simplify_path_debug (".././../",   "../../");

  sanitise_path_debug ("C:\\windows\\path", "C:/windows/path");
  sanitise_path_debug ("", "");
  sanitise_path_debug ("\\\\", "/");

  char const * win_path = X509_get_default_private_dir ();
  printf ("%s -> %s\n", X509_PRIVATE_DIR, win_path);
  free ((void *)win_path);

  return 0;
}

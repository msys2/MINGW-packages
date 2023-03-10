/*
      .Some useful path tools.
   Written by Ray Donnelly in 2014.
   Licensed under CC0. No warranty.
 */

#include <limits.h>

#include "pathtools.h"
#include "msys2_relocate.h"

void
sanitise_path_debug(char const * path, char const * expected)
{
  char * path_copy = (char *) alloca (strlen(path)+1);
  strcpy (path_copy, path);
  sanitise_path (path_copy);

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




#define X509_PRIVATE_DIR "/mingw64/ssl/private"

const char *
X509_get_default_private_dir(void)
{
#if defined(__MINGW32__)
  static char stored_path[PATH_MAX];
  static int stored = 0;
  if (stored == 0)
  {
    char const * relocated = msys2_get_relocated_single_path (X509_PRIVATE_DIR);
    strncpy (stored_path, relocated, PATH_MAX);
    stored_path[PATH_MAX-1] = '\0';
    free ((void *)relocated);
    stored = 1;
  }
  return stored_path;
#else
  return (X509_PRIVATE_DIR);
#endif
}

#define TRUST_PATHS "/mingw64/etc/pki/ca-trust/source:/mingw64/share/pki/ca-trust-source"
#define SINGLE_PATH_LIST "/mingw64"

int main(int argc, char *argv[])
{
#define BINDIR  "/mingw64/bin"
#define DATADIR "/mingw64/share"

  char exe_path[PATH_MAX];
  get_executable_path (argv[0], &exe_path[0], sizeof (exe_path) / sizeof (exe_path[0]));
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
  get_relative_path_debug (NULL,                      NULL,                              "./");
  get_relative_path_debug ("/mingw64/bin",            "/mingw64/etc/pkcs11/pkcs11.conf", "../etc/pkcs11/pkcs11.conf");
  get_relative_path_debug ("/a/b/c/d",                "/a/b/c",                          "..");
  get_relative_path_debug ("/a/b/c/d/",               "/a/b/c/",                         "../");
  get_relative_path_debug ("/",                       "/",                               "/");
  get_relative_path_debug ("/a/testone/c/d",          "/a/testtwo/c",                    "../../../testtwo/c");
  get_relative_path_debug ("/a/testone/c/d/",         "/a/testtwo/c/",                   "../../../testtwo/c/");
  get_relative_path_debug ("/home/part2/part3/part4", "/work/proj1/proj2",               "../../../../work/proj1/proj2");

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
  sanitise_path_debug ("\\\\", "//");
  sanitise_path_debug ("a\\\\", "a/");

  char const * win_path = X509_get_default_private_dir ();
  printf ("%s -> %s\n", X509_PRIVATE_DIR, win_path);

  char * trusts = msys2_get_relocated_path_list (TRUST_PATHS);
  printf ("%s -> %s\n", TRUST_PATHS, trusts);
  free ((void*)trusts);

  char * single = msys2_get_relocated_path_list (SINGLE_PATH_LIST);
  printf ("%s -> %s\n", SINGLE_PATH_LIST, single);
  free ((void*)single);
  
  char *multi = get_relocated_path_list(BINDIR, TRUST_PATHS);
  printf ("Source pathlist: %s \n", TRUST_PATHS);
  printf ("Real pathlist: %s\n", multi);
  free ((void*)multi);
  return 0;
}

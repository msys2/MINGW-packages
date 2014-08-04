/*
      .Some useful path tools.
   Written by Ray Donnelly in 2014.
   Licensed under CC0. No warranty.
 */

#include "pathtools.h"

#define X509_PRIVATE_DIR "/mingw64/ssl/private"

const char *
X509_get_default_private_dir(void)
{
#if defined(__MINGW32__)
  static char * stored_path[PATH_MAX];
  static int stored = 0;
  if (stored == 0)
  {
    char const * relocated = get_relocated_single_path (X509_PRIVATE_DIR);
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

  return 0;
}

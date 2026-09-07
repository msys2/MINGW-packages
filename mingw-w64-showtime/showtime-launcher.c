#include <windows.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

int
wmain (int argc, wchar_t **argv)
{
  wchar_t module_path[MAX_PATH];
  wchar_t python_path[MAX_PATH];
  wchar_t script_path[MAX_PATH];
  wchar_t *last_separator;
  wchar_t **child_argv;
  DWORD path_length;
  int result;

  path_length = GetModuleFileNameW (NULL, module_path, MAX_PATH);
  if (path_length == 0 || path_length >= MAX_PATH)
    return EXIT_FAILURE;

  last_separator = wcsrchr (module_path, L'\\');
  if (last_separator == NULL)
    return EXIT_FAILURE;
  *last_separator = L'\0';

  if (swprintf (python_path, MAX_PATH, L"%ls\\pythonw.exe", module_path) < 0 ||
      swprintf (script_path, MAX_PATH, L"%ls\\showtime", module_path) < 0)
    return EXIT_FAILURE;

  child_argv = calloc ((size_t) argc + 2, sizeof (*child_argv));
  if (child_argv == NULL)
    return EXIT_FAILURE;

  child_argv[0] = python_path;
  child_argv[1] = script_path;
  for (int index = 1; index < argc; index++)
    child_argv[index + 1] = argv[index];

  result = _wspawnv (_P_WAIT, python_path, (const wchar_t * const *) child_argv);
  free (child_argv);

  return result < 0 ? EXIT_FAILURE : result;
}

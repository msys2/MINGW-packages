/*
 * git-wrapper - replace cmd\git.cmd with an executable
 *
 * Copyright (C) 2012 Pat Thoyts <patthoyts@users.sourceforge.net>
 */

#define STRICT
#define WIN32_LEAN_AND_MEAN
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <stdio.h>
#include <wchar.h>
#include <stdbool.h>

/* Let the system deal with it if the path is too long */
#undef MAX_PATH
#define MAX_PATH 4096

static WCHAR msystem_bin[64];

static void print_error(LPCWSTR prefix, DWORD error_number)
{
	LPWSTR buffer = NULL;
	DWORD count = 0;

	count = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER
			| FORMAT_MESSAGE_FROM_SYSTEM
			| FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, error_number, LANG_NEUTRAL,
			(LPTSTR)&buffer, 0, NULL);
	if (count < 1)
		count = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER
				| FORMAT_MESSAGE_FROM_STRING
				| FORMAT_MESSAGE_ARGUMENT_ARRAY,
				L"Code 0x%1!08x!",
				0, LANG_NEUTRAL, (LPTSTR)&buffer, 0,
				(va_list*)&error_number);
	fwprintf(stderr, L"%s: %s", prefix, buffer);
	LocalFree((HLOCAL)buffer);
}


static void find_exe_realpath(LPWSTR exepath, int count) {
	WCHAR mdexe[MAX_PATH], realexe[MAX_PATH];
	HANDLE hFile = INVALID_HANDLE_VALUE;
	DWORD dwlen = 0;
	/* get the installation location */
	GetModuleFileNameW(NULL, mdexe, MAX_PATH);
	hFile = CreateFileW(mdexe,                 /* file to open */
			    GENERIC_READ,          /* open for reading */
			    FILE_SHARE_READ,       /* share for reading */
			    NULL,                  /* default security */
			    OPEN_EXISTING,         /* existing file only */
			    FILE_ATTRIBUTE_NORMAL, /* normal file */
			    NULL);                 /* no attr. template */
	if (hFile == INVALID_HANDLE_VALUE) {
		wcscpy(exepath, mdexe);
		return;
	}
	dwlen = GetFinalPathNameByHandleW(hFile, realexe, MAX_PATH,
					  VOLUME_NAME_DOS);
	if (!dwlen || dwlen >= MAX_PATH) {
		wcscpy(exepath, mdexe);
	} else {
		size_t offset = 0;
		size_t length = dwlen;

		/*
		 * Strip the \\?\ prefix (that was added to the "final path
		 * name" by GetFinalPathNameByHandleW()).
		 */
		if (!wcsncmp(realexe, L"\\\\?\\", 4)) {
			/*
			 * For network paths, GetFinalPathNameByHandleW()
			 * actually adds a \\?\UNC prefix (and turns the
			 * leading double backslash into a single one)...
			 */
			if (wcsncmp(realexe + 4, L"UNC\\", 4))
				offset = 4;
			else {
				/*
				 * Skip \\?\UNC and re-instate the double
				 * backslash starting the network path.
				 */
				offset = 7;
				*(exepath++) = L'\\';
			}
			length -= offset;
		}

		wcsncpy(exepath, realexe + offset, length);
		exepath[length] = L'\0';
	}
	CloseHandle(hFile);
}

static void my_path_append(LPWSTR list, LPCWSTR path, size_t alloc)
{
	size_t len1 = wcslen(list), len2 = wcslen(path);
	int need_dir_sep = len1 && list[len1 - 1] != L'\\';
	LPWSTR sep;

	if (len1 + need_dir_sep + len2 + 1 > alloc &&
	    (sep = wcschr(list, L';')))
		/* silently remove the last path element if too long */
		*sep = L'\0';
	else {
		if (need_dir_sep)
			list[len1++] = L'\\';
		wcscpy(list + len1, path);
	}
}

static int running_on_arm64 = -1;

static int is_running_on_arm64_hardware()
{
	if (running_on_arm64 >= 0)
		return running_on_arm64;

	USHORT process_machine = 0;
	USHORT native_machine = 0;

	/* Note: IsWow64Process2 is only available in Windows 10 1511+ */
	BOOL (WINAPI* IsWow64Process2)(HANDLE, PUSHORT, PUSHORT) =
		(BOOL (WINAPI *)(HANDLE, PUSHORT, PUSHORT))
		GetProcAddress(GetModuleHandle(L"kernel32"), "IsWow64Process2");

	running_on_arm64 = IsWow64Process2 &&
		IsWow64Process2(GetCurrentProcess(), &process_machine, &native_machine) &&
		native_machine == 0xaa64;

	return running_on_arm64;
}

static int is_running_on_arm64_msystem(LPWSTR top_level_path, LPWSTR msystem_bin)
{
	int ret=0;
	size_t len = wcslen(top_level_path);

	/* Does /clangarm64/bin exist? */
	my_path_append(top_level_path, L"clangarm64/bin", MAX_PATH);
	if (_waccess(top_level_path, 0) != -1) {
		wcscpy(msystem_bin, L"clangarm64/bin");
		ret=1;
	}
	top_level_path[len] = L'\0';
	return ret;
}

static inline int is_running_on_arm64(LPWSTR top_level_path, LPWSTR msystem_bin)
{
	return is_running_on_arm64_hardware() && is_running_on_arm64_msystem(top_level_path, msystem_bin);
}

static int is_system32_path(LPWSTR path)
{
	WCHAR system32[MAX_PATH];

	return GetSystemDirectoryW(system32, MAX_PATH) && !_wcsicmp(system32, path);
}

static void setup_environment(LPWSTR top_level_path, int full_path)
{
	WCHAR msystem[64];
	LPWSTR path2 = NULL;
	int len;

	/* Set MSYSTEM */
	if (running_on_arm64 > 0) {
		swprintf(msystem, sizeof(msystem),
				L"CLANGARM64");
	} else {
		swprintf(msystem, sizeof(msystem),
				L"MINGW%d", (int)sizeof(void*) * 8);
	}

	SetEnvironmentVariable(L"MSYSTEM", msystem);

	/* if not set, set PLINK_PROTOCOL to ssh */
	if (!GetEnvironmentVariable(L"PLINK_PROTOCOL", NULL, 0))
		SetEnvironmentVariable(L"PLINK_PROTOCOL", L"ssh");

	/*
	 * set HOME to %HOMEDRIVE%%HOMEPATH% or %USERPROFILE%
	 * With roaming profiles: HOMEPATH is the roaming location and
	 * USERPROFILE is the local location
	 */
	if (!GetEnvironmentVariable(L"HOME", NULL, 0)) {
		LPWSTR e = NULL;
		len = GetEnvironmentVariable(L"HOMEPATH", NULL, 0);
		if (len) {
			DWORD attr, drvlen = GetEnvironmentVariable(L"HOMEDRIVE", NULL, 0);
			e = (LPWSTR)malloc(sizeof(WCHAR) * (drvlen + len));
			drvlen = GetEnvironmentVariable(L"HOMEDRIVE", e, drvlen);
			GetEnvironmentVariable(L"HOMEPATH", e + drvlen, len);
			/* check if the path exists */
			attr = is_system32_path(e) ?
				/* If HOMEDRIVE/HOMEPATH points to C:/Windows/system32, ignore it */
				INVALID_FILE_ATTRIBUTES : GetFileAttributesW(e);
			if (attr != INVALID_FILE_ATTRIBUTES
					&& (attr & FILE_ATTRIBUTE_DIRECTORY))
				SetEnvironmentVariable(L"HOME", e);
			else
				len = 0; /* use USERPROFILE */
			free(e);
		}

		if (len == 0) {
			len = GetEnvironmentVariable(L"USERPROFILE", NULL, 0);
			if (len != 0) {
				e = (LPWSTR)malloc(len * sizeof(WCHAR));
				GetEnvironmentVariable(L"USERPROFILE", e, len);
				SetEnvironmentVariable(L"HOME", e);
				free(e);
			}
		}
	}

	/* extend the PATH */
	len = GetEnvironmentVariable(L"PATH", NULL, 0);
	len = sizeof(WCHAR) * (len + 4 * MAX_PATH);
	path2 = (LPWSTR)malloc(len);
	wcscpy(path2, top_level_path);
	if (!full_path)
		my_path_append(path2, L"cmd;", len);
	else {
		my_path_append(path2, msystem_bin, len);
		if (_waccess(path2, 0) != -1) {
			/* We are in an MSys2-based setup */
			int len2 = GetEnvironmentVariable(L"HOME", NULL, 0);

			wcscat(path2, L";");
			wcscat(path2, top_level_path);
			my_path_append(path2, L"usr\\bin;", len);
			if (len2 + 6 < MAX_PATH) {
				GetEnvironmentVariable(L"HOME",
						path2 + wcslen(path2), len2);
				my_path_append(path2, L"bin;", len);
			}
		}
		else {
			/* Fall back to MSys1 paths */
			wcscpy(path2, top_level_path);
			my_path_append(path2, L"bin;", len);
			wcscat(path2, top_level_path);
			my_path_append(path2, L"mingw\\bin;", len);
		}
	}
	GetEnvironmentVariable(L"PATH", path2 + wcslen(path2),
				(len / sizeof(WCHAR)) - wcslen(path2));
	SetEnvironmentVariable(L"PATH", path2);
	free(path2);

}

/*
 * Fix up the command line to call git.exe
 * We have to be very careful about quoting here so we just
 * trim off the first argument and replace it leaving the rest
 * untouched.
 */
static LPWSTR fixup_commandline(LPWSTR exepath, LPWSTR *exep, int *wait,
	LPCWSTR prefix_args, int prefix_args_len, int is_git_command,
	int skip_arguments, int append_quote_to_cmdline)
{
	int wargc = 0;
	LPWSTR cmd = NULL, cmdline = NULL;
	LPWSTR *wargv = NULL, p = NULL;
	size_t alloc;

	cmdline = GetCommandLineW();
	wargv = CommandLineToArgvW(cmdline, &wargc);
	alloc = sizeof(WCHAR) * (wcslen(cmdline) + prefix_args_len + 1
				 + MAX_PATH + append_quote_to_cmdline);
	cmd = (LPWSTR)malloc(alloc);
	if (prefix_args) {
		if (is_git_command)
			_swprintf(cmd, L"\"%s\\%s\" %.*s", exepath, L"git.exe",
					prefix_args_len, prefix_args);
		else
			_swprintf(cmd, L"%.*s", prefix_args_len, prefix_args);

	}
	else
		wcscpy(cmd, L"git.exe");

	/* skip wargv[0], append the remaining arguments */
	++skip_arguments;
	if (skip_arguments < wargc) {
		LPWSTR space = wcschr(cmd, L' '), quote;
		int i;

		for (i = 0, p = cmdline; p && *p && i < skip_arguments; i++) {
			if (i)
				while (isspace(*p))
					p++;
			if (*p == L'"')
				p++;
			p += wcslen(wargv[i]);
			if (*p == L'"')
				p++;
			while (*p && !isspace(*p))
				p++;
		}

		if (*p == L' ' && p[wcsspn(p, L" ")] == L'"' && space - cmd >= 8 &&
		    !wcsncmp(space - 8, L"\\cmd.exe /K \"", 13)) {
			/*
			 * In git-cmd.exe, we automatically prepend a doskey
			 * definition to disallow git.exe to be picked up from
			 * the current directory. So the command line has the
			 * form:
			 *
			 * <system32>\cmd.exe /K
			 *     "doskey git=^"<git>\cmd\git.exe $*^""
			 *
			 * That works as long as there is no additional command
			 * i.e. as long as `git-cmd.exe` is called without an
			 * argument specifying a command.
			 *
			 * When an additional command-line argument is appended,
			 * it will be mistaken as part of that doskey command.
			 *
			 * To fix that, we need to insert a `&&` (to tell
			 * cmd.exe that the doskey command is done and now
			 * comes the next command to execute), i.e. the command
			 * line for `git-cmd.exe <arg>` should look like this:
			 *
			 * <system32>\cmd.exe /K
			 *     "doskey git=^"<git>\cmd\git.exe $*^" && <arg>"
			 */
			alloc += sizeof(WCHAR) * 3;
			cmd = realloc(cmd, alloc);
			quote = cmd + wcslen(cmd) - 1;
			if (*quote == L'"')
				wcscpy(quote, L" && ");
			while (*p == L' ')
				p++;
			p++;
		}

		alloc += sizeof(WCHAR) * wcslen(p);
		cmd = realloc(cmd, alloc);
		wcscat(cmd, p);
	}

	if (append_quote_to_cmdline) {
		alloc += sizeof(WCHAR);
		cmd = realloc(cmd, alloc);
		wcscat(cmd, L"\"");
	}

	if (wargc > 1 && !wcscmp(wargv[1], L"gui"))
		*wait = 0;

	LocalFree(wargv);

	return cmd;
}

static int strip_prefix(LPWSTR str, int *len, LPCWSTR prefix)
{
	LPWSTR start = str;
	do {
		if (str - start > *len)
			return 0;
		if (!*prefix) {
			*len -= str - start;
			memmove(start, str,
				sizeof(WCHAR) * (wcslen(str) + 1));
			return 1;
		}
	} while (*str++ == *prefix++);
	return 0;
}

static void extract_first_arg(LPWSTR command_line, LPWSTR exepath, LPWSTR buf)
{
	LPWSTR *wargv;
	int wargc;

	wargv = CommandLineToArgvW(command_line, &wargc);
	if (wargc < 1) {
		fwprintf(stderr, L"Invalid command-line: '%s'\n", command_line);
		exit(1);
	}
	if (*wargv[0] == L'\\' ||
			(isalpha(*wargv[0]) && wargv[0][1] == L':'))
		wcscpy(buf, wargv[0]);
	else {
		wcscpy(buf, exepath);
		my_path_append(buf, wargv[0], MAX_PATH);
	}
	LocalFree(wargv);
}

#define alloc_nr(x) (((x)+16)*3/2)

static LPWSTR expand_variables(LPWSTR buffer, size_t alloc)
{
	LPWSTR buf = buffer;
	size_t len = wcslen(buf), move_len;

	for (;;) {
		LPWSTR atat = wcsstr(buf, L"@@"), atat2;
		WCHAR save;
		int env_len, delta;

		if (!atat)
			break;

		atat2 = wcsstr(atat + 2, L"@@");
		if (!atat2)
			break;

		*atat2 = L'\0';
		atat2 += 2;
		env_len = GetEnvironmentVariable(atat + 2, NULL, 0);
		delta = env_len - 1 - (atat2 - atat);
		if (len + delta >= alloc) {
			LPWSTR buf2;
			alloc = alloc_nr(alloc);
			if (alloc <= len + delta)
				alloc = len + delta + 1;
			if (buf != buffer)
				buf2 = realloc(buf, sizeof(WCHAR) * alloc);
			else {
				buf2 = malloc(sizeof(WCHAR) * alloc);
				if (buf2)
					memcpy(buf2, buf, sizeof(WCHAR)
							* (len + 1));
			}
			if (!buf2) {
				fwprintf(stderr,
					L"Substituting '%s' results in too "
					L"large a command-line\n", atat + 2);
				exit(1);
			}
			atat += buf2 - buf;
			atat2 += buf2 - buf;
			buf = buf2;
		}
		move_len = sizeof(WCHAR) * (len + 1 - (atat2 - buf));
		if (delta > 0)
			memmove(atat2 + delta, atat2, move_len);
		len += delta;
		save = atat[env_len - 1 + (delta < 0 ? -delta : 0)];
		GetEnvironmentVariable(atat + 2, atat, env_len);
		if (delta < 0)
			memmove(atat2 + delta, atat2, move_len);
		atat[env_len - 1] = save;
	}

	return buf;
}

static void set_app_id(LPWSTR app_id)
{
	HMODULE shell32;
	HRESULT (*set_app_id)(LPWSTR app_id);

	shell32 = LoadLibrary(L"shell32.dll");
	if (!shell32)
		return;
	set_app_id = (void *) GetProcAddress(shell32,
			"SetCurrentProcessExplicitAppUserModelID");
	if (!set_app_id)
		return;
	if (!SUCCEEDED(set_app_id(app_id)))
		print_error(L"warning: could not set app ID", GetLastError());
}

static int configure_via_resource(LPWSTR basename, LPWSTR exepath, LPWSTR exep,
	LPCWSTR *prefix_args, int *prefix_args_len,
	int *is_git_command, LPWSTR *working_directory, int *full_path,
	int *skip_arguments, int *allocate_console, int *show_console,
	int *append_quote_to_cmdline)
{
	int i, id, minimal_search_path, needs_a_console, no_hide, wargc;
	int append_quote;
	LPWSTR *wargv;
	WCHAR *app_id;

#define BUFSIZE 65536
	static WCHAR buf[BUFSIZE];
	LPWSTR buf2 = buf;
	int len;

	for (id = 0; ; id++) {
		minimal_search_path = 0;
		needs_a_console = 0;
		no_hide = -1;
		append_quote = 0;
		app_id = NULL;
		len = LoadString(NULL, id, buf, BUFSIZE);

		if (!len) {
			if (!id)
				return 0; /* no resources found */

			fwprintf(stderr, L"Need a valid command-line; "
				L"Edit the string resources accordingly\n");
			exit(1);
		}

		if (len >= BUFSIZE) {
			fwprintf(stderr,
				L"Could not read resource (too large)\n");
			exit(1);
		}

		for (;;) {
			if (strip_prefix(buf, &len, L"MINIMAL_PATH=1 "))
				minimal_search_path = 1;
			else if (strip_prefix(buf, &len, L"ALLOC_CONSOLE=1 "))
				needs_a_console = 1;
			else if (strip_prefix(buf, &len, L"SHOW_CONSOLE=1 "))
				no_hide = 1;
			else if (strip_prefix(buf, &len, L"APPEND_QUOTE=1 "))
				append_quote = 1;
			else if (strip_prefix(buf, &len, L"APP_ID=")) {
				LPWSTR space = wcschr(buf, L' ');
				size_t app_id_len = space - buf;
				if (!space) {
					len -= 7;
					memmove(buf, buf + 7,
							len * sizeof(WCHAR));
					break;
				}
				app_id = wcsdup(buf);
				app_id[app_id_len] = L'\0';
				len -= app_id_len + 1;
				memmove(buf, buf + app_id_len + 1,
						len * sizeof(WCHAR));
			}
			else
				break;
		}

		buf[len] = L'\0';

		if (!id)
			SetEnvironmentVariable(L"EXEPATH", exepath);

		buf2 = expand_variables(buf, BUFSIZE);

		extract_first_arg(buf2, exepath, exep);

		if (_waccess(exep, 0) != -1)
			break;
		fwprintf(stderr,
			L"Skipping command-line '%s'\n('%s' not found)\n",
			buf2, exep);
		if (app_id)
			free(app_id);
	}

	*prefix_args = buf2;
	*prefix_args_len = wcslen(buf2);

	*is_git_command = 0;
	wargv = CommandLineToArgvW(GetCommandLineW(), &wargc);
	for (i = 1; i < wargc; i++) {
		if (!wcscmp(L"--no-cd", wargv[i]))
			*working_directory = NULL;
		else if (!wcscmp(L"--cd-to-home", wargv[i]))
			*working_directory = (LPWSTR) 1;
		else if (!wcsncmp(L"--cd=", wargv[i], 5))
			*working_directory = wcsdup(wargv[i] + 5);
		else if (!wcscmp(L"--minimal-search-path", wargv[i]))
			minimal_search_path = 1;
		else if (!wcscmp(L"--no-minimal-search-path", wargv[i]))
			minimal_search_path = 0;
		else if (!wcscmp(L"--needs-console", wargv[i]))
			needs_a_console = 1;
		else if (!wcscmp(L"--no-needs-console", wargv[i]))
			needs_a_console = 0;
		else if (!wcscmp(L"--hide", wargv[i]))
			no_hide = 0;
		else if (!wcscmp(L"--no-hide", wargv[i]))
			no_hide = 1;
		else if (!wcscmp(L"--append-quote", wargv[i]))
			append_quote = 1;
		else if (!wcscmp(L"--no-append-quote", wargv[i]))
			append_quote = -1;
		else if (!wcsncmp(L"--command=", wargv[i], 10)) {
			LPWSTR expanded;

			wargv[i] += 10;
			expanded = expand_variables(wargv[i], wcslen(wargv[i]));
			if (expanded == wargv[i])
				expanded = wcsdup(expanded);

			extract_first_arg(expanded, exepath, exep);

			*prefix_args = expanded;
			*prefix_args_len = wcslen(*prefix_args);
			*skip_arguments = i;
			break;
		}
		else if (!wcsncmp(L"--app-id=", wargv[i], 9)) {
			free(app_id);
			app_id = wcsdup(wargv[i] + 9);
		}
		else
			break;
		*skip_arguments = i;
	}
	if (minimal_search_path)
		*full_path = 0;
	if (needs_a_console)
		*allocate_console = 1;
	if (no_hide >= 0)
		*show_console = no_hide;
	if (append_quote)
		*append_quote_to_cmdline = append_quote == 1;
	if (app_id)
		set_app_id(app_id);
	LocalFree(wargv);

	return 1;
}

static void initialize_top_level_path(LPWSTR top_level_path, LPWSTR exepath,
		LPWSTR msystem_bin, int strip_count)
{
	wcscpy(top_level_path, exepath);

	while (strip_count) {
		if (strip_count < 0) {
			int len = wcslen(top_level_path);
			if (is_running_on_arm64(top_level_path, msystem_bin))
				return;
			my_path_append(top_level_path, msystem_bin, MAX_PATH);
			if (_waccess(top_level_path, 0) != -1) {
				/* We are in an MSys2-based setup */
				top_level_path[len] = L'\0';
				return;
			}
			top_level_path[len] = L'\0';
			my_path_append(top_level_path, L"mingw\\bin", MAX_PATH);
			if (_waccess(top_level_path, 0) != -1) {
				/* We are in an MSys-based setup */
				top_level_path[len] = L'\0';
				return;
			}
			top_level_path[len] = L'\0';
			if (!(++strip_count)) {
				fwprintf(stderr, L"Top-level not found: %s\n",
					exepath);
				exit(1);
			}
		}

		if (!PathRemoveFileSpec(top_level_path)) {
			fwprintf(stderr, L"Invalid executable path: %s\n",
					exepath);
			ExitProcess(1);
		}

		if (strip_count > 0)
			--strip_count;
	}
	/* Only enable ARM64 support if <top-level>/arm64/bin/ exists */
	is_running_on_arm64(top_level_path, msystem_bin);
}

static void maybe_read_config(LPWSTR top_level_path)
{
	WCHAR config_path[MAX_PATH];
	char line[1024];
	FILE *f;

	wcscpy(config_path, top_level_path);
	wcsncat(config_path, L"\\etc\\git-bash.config", MAX_PATH - wcslen(top_level_path) - 1);
	f = _wfopen(config_path, L"rt");
	if (!f)
		return;

	while (fgets(line, sizeof(line), f)) {
		int len = strlen(line);

		/* trim trailing newline */
		if (len > 0 && line[len - 1] == '\n')
			if (--len > 0 && line[len - 1] == '\r')
				len--;
		line[len] = '\0';

		if (!memcmp(line, "MSYS=", 5)) {
			int len2 = GetEnvironmentVariableA("MSYS", NULL, 0);

			if (!len2)
				SetEnvironmentVariableA("MSYS", line + 5);
			else {
				char *combined = malloc(len - 5 + 1 + len2 + 1);
				strcpy(combined, line + 5);
				combined[len - 5] = ' ';
				GetEnvironmentVariableA("MSYS", combined + len - 5 + 1, len2);
				SetEnvironmentVariableA("MSYS", combined);
				free(combined);
			}
		}
	}

	fclose(f);
}

int main(void)
{
	int r = 1, wait = 1, prefix_args_len = -1, needs_env_setup = 1,
		is_git_command = 1, full_path = 1, skip_arguments = 0,
		allocate_console = 0, show_console = -1,
		append_quote_to_cmdline = 0;
	WCHAR exepath[MAX_PATH], exe_bup[MAX_PATH], exe[MAX_PATH], top_level_path[MAX_PATH];
	LPWSTR cmd = NULL, exep = exe, basename;
	LPWSTR working_directory = NULL;
	LPCWSTR prefix_args = NULL;

	/* Determine MSys2-based Git path. */
	swprintf(msystem_bin, sizeof(msystem_bin),
		L"mingw%d\\bin", (int) sizeof(void *) * 8);
	*top_level_path = L'\0';

	/* get the installation location */
	/* GetModuleFileName(NULL, exepath, MAX_PATH); */
	find_exe_realpath(exepath, MAX_PATH);
	wcscpy(exe_bup, exepath);
	if (!PathRemoveFileSpec(exepath)) {
		fwprintf(stderr, L"Invalid executable path: %s\n", exepath);
		ExitProcess(1);
	}
	basename = exepath + wcslen(exepath) + 1;
	if (configure_via_resource(basename, exepath, exep,
			&prefix_args, &prefix_args_len,
			&is_git_command, &working_directory,
			&full_path, &skip_arguments, &allocate_console,
			&show_console, &append_quote_to_cmdline)) {
		/* do nothing */
	}
	else if (!wcsicmp(basename, L"git-lfs.exe")) {
		initialize_top_level_path(top_level_path, exepath, msystem_bin, 1);

		/* set the default exe module */
		wcscpy(exe, top_level_path);
		my_path_append(exe, msystem_bin, MAX_PATH);
		my_path_append(exe, L"git-lfs.exe", MAX_PATH);
	}
	else if (!wcsicmp(basename, L"scalar.exe")) {
		initialize_top_level_path(top_level_path, exepath, msystem_bin, 1);

		needs_env_setup = 1;

		/* set the default exe module */
		wcscpy(exe, top_level_path);
		my_path_append(exe, msystem_bin, MAX_PATH);
		my_path_append(exe, basename, MAX_PATH);
	}
	else if (!wcsicmp(basename, L"git-gui.exe") ||
			!wcsicmp(basename, L"gitk.exe")) {
		static WCHAR buffer[BUFSIZE];
		const WCHAR *infix = L"libexec\\git-core";
		wait = 0;
		allocate_console = 1;
		initialize_top_level_path(top_level_path, exepath, msystem_bin, 1);

		/* set the default exe module */
		wcscpy(exe, top_level_path);
		my_path_append(exe, msystem_bin, MAX_PATH);
		my_path_append(exe, L"wish.exe", MAX_PATH);
		basename[wcslen(basename) - 4] = L'\0';
		if (!wcsicmp(basename, L"gitk"))
			infix = L"bin";
		if (_waccess(exe, 0) != -1)
			swprintf(buffer, BUFSIZE,
				L"\"%s\" \"%s\\%.*s\\%s\"",
				exe, top_level_path,
				wcslen(msystem_bin) - 4, msystem_bin, infix);
		else {
			wcscpy(exe, top_level_path);
			my_path_append(exe, L"mingw\\bin\\wish.exe", MAX_PATH);
			swprintf(buffer, BUFSIZE,
				L"\"%s\" \"%s\\mingw\\%s\"",
				exe, top_level_path, infix);
		}
		is_git_command = 0;
		my_path_append(buffer, basename, BUFSIZE);
		wcsncat(buffer, L" --", BUFSIZE - wcslen(buffer) - 1);
		prefix_args = buffer;
		prefix_args_len = wcslen(buffer);
	}
	else if (!wcsicmp(basename, L"git-remote-http.exe")) {
		is_git_command = 0;
		needs_env_setup = 0;

		/* Call remote-https */
		wcscpy(exe, exepath);
		my_path_append(exe, L"git-remote-https.exe", MAX_PATH);
	}
	else if (!wcsnicmp(basename, L"git-", 4)) {
		is_git_command = 1;
		needs_env_setup = 0;

		/* Call a builtin */
		prefix_args = basename + 4;
		prefix_args_len = wcslen(prefix_args) - 4;
		if (!wcsicmp(prefix_args + prefix_args_len - 4, L".exe"))
			prefix_args_len -= 4;

		/* set the default exe module */
		wcscpy(exe, exepath);
		my_path_append(exe, L"git.exe", MAX_PATH);
	}
	else if (!wcsicmp(basename, L"git.exe")) {
		initialize_top_level_path(top_level_path, exepath, msystem_bin, 1);

		/* set the default exe module */
		wcscpy(exe, top_level_path);
		my_path_append(exe, msystem_bin, MAX_PATH);
		my_path_append(exe, L"git.exe", MAX_PATH);
		if (_waccess(exe, 0) == -1) {
			wcscpy(exe, top_level_path);
			my_path_append(exe, L"bin\\git.exe", MAX_PATH);
		}
	}

	else if (!wcsicmp(basename, L"sh.exe") || !wcsicmp(basename, L"ash.exe")) {
		is_git_command = 0;
		needs_env_setup = 0;

		/* Call BusyBox' ash */
		prefix_args = L"sh";
		prefix_args_len = wcslen(prefix_args);

		wcscpy(exe, exepath);
		my_path_append(exe, L"busybox.exe", MAX_PATH);
		if (_waccess(exe, 0) == -1) {
			initialize_top_level_path(top_level_path, exepath, msystem_bin, -4);

			wcscpy(exe, top_level_path);
			my_path_append(exe, msystem_bin, MAX_PATH);
			my_path_append(exe, L"busybox.exe", MAX_PATH);
			if (_waccess(exe, 0) == -1) {
				fwprintf(stderr, L"Could not find '%ls'\n", exe);
				exit(1);
			}
		}
	}

	if (needs_env_setup) {
		if (!top_level_path[0])
			initialize_top_level_path(top_level_path, exepath,
					msystem_bin, -4);

		setup_environment(top_level_path, full_path);
		maybe_read_config(top_level_path);
	}
	cmd = fixup_commandline(exepath, &exep, &wait,
		prefix_args, prefix_args_len, is_git_command, skip_arguments,
		append_quote_to_cmdline);

	if (working_directory == (LPWSTR)1) {
		int len = GetEnvironmentVariable(L"HOME", NULL, 0);

		if (len) {
			working_directory = malloc(sizeof(WCHAR) * len);
			GetEnvironmentVariable(L"HOME", working_directory, len);
		}
	}

	if (!wcscmp(exe_bup, exep)) {
		fwprintf(stderr, L"BUG (fork bomb): %s\n", exep);
		ExitProcess(1);
	}

	{
		STARTUPINFO si;
		PROCESS_INFORMATION pi;
		DWORD creation_flags = CREATE_UNICODE_ENVIRONMENT;
		HANDLE console_handle;
		BOOL br = FALSE;
		ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
		ZeroMemory(&si, sizeof(STARTUPINFO));
		si.cb = sizeof(STARTUPINFO);

		if (allocate_console || show_console > 0)
			creation_flags |= CREATE_NEW_CONSOLE;
		else if ((console_handle = CreateFile(L"CONOUT$", GENERIC_WRITE,
				FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL, NULL)) !=
				INVALID_HANDLE_VALUE)
			CloseHandle(console_handle);
		else {
#define STD_HANDLE(field, id) si.hStd##field = GetStdHandle(STD_##id); if (!si.hStd##field) si.hStd##field = INVALID_HANDLE_VALUE
			STD_HANDLE(Input, INPUT_HANDLE);
			STD_HANDLE(Output, OUTPUT_HANDLE);
			STD_HANDLE(Error, ERROR_HANDLE);
			si.dwFlags = STARTF_USESTDHANDLES;


			creation_flags |= CREATE_NO_WINDOW;
		}
		if (show_console >= 0) {
			si.dwFlags |= STARTF_USESHOWWINDOW;
			si.wShowWindow = show_console ? SW_SHOW : SW_HIDE;
		}

		br = CreateProcess(/* module: null means use command line */
				exep,
				cmd,  /* modified command line */
				NULL, /* process handle inheritance */
				NULL, /* thread handle inheritance */
					/* handles inheritable? */
				allocate_console ? FALSE : TRUE,
				creation_flags,
				NULL, /* environment: use parent */
				working_directory, /* use parent's */
				&si, &pi);
		if (br) {
			if (wait) {
				/*
				 * Ignore Ctrl+C: the called process needs
				 * to handle this event correctly, then we
				 * quit, too.
				 */
				SetConsoleCtrlHandler(NULL, TRUE);
				WaitForSingleObject(pi.hProcess, INFINITE);
				SetConsoleCtrlHandler(NULL, FALSE);
			}
			if (!GetExitCodeProcess(pi.hProcess, (DWORD *)&r))
				print_error(L"error reading exit code",
					GetLastError());
			CloseHandle(pi.hProcess);
		}
		else {
			print_error(L"error launching git", GetLastError());
			r = 1;
		}
	}

	free(cmd);

	ExitProcess(r);
}

int wmain(int wargc, const wchar_t **wargv)
{
	return main();
}

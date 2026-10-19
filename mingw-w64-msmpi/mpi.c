#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>


#define CMDLINE_MAX 32768 // CreateProcess accepts at most 32767 characters + NUL


static char* quote_new(const char* a) {
	char* q = malloc(2*strlen(a) + 3);
	char* o = q;
	size_t bs = 0, bare = (*a != 0);
	if(!q) return NULL;
	for(const char* p = a; bare && *p; ++p)
		if(*p == ' ' || *p == '\t' || *p == '"') bare = 0;
	if(bare) {
		strcpy(q, a);
		return q;
	}
	*o++ = '"';
	for(const char* p = a; *p; ++p) {
		if(*p == '\\') {
			++bs;
			continue;
		}
		if(*p == '"') bs = bs*2 + 1;
		while(bs) {
			*o++ = '\\';
			--bs;
		}
		*o++ = *p;
	}
	for(bs *= 2; bs; --bs) *o++ = '\\';
	*o++ = '"';
	*o = '\0';
	return q;
}


static char* command_line_new(char** args) {
	size_t size = 0;
	for(size_t x = 0; args[x]; ++x) {
		char* q = quote_new(args[x]);
		if(!q) return NULL;
		size += strlen(q) + 1;
		free(q);
	}
	char* line = malloc(size);
	if(!line) return NULL;
	char* o = line;
	for(size_t x = 0; args[x]; ++x) {
		char* q = quote_new(args[x]);
		if(!q) { free(line); return NULL; }
		if(x) *o++ = ' ';
		strcpy(o, q);
		o += strlen(q);
		free(q);
	}
	*o = '\0';
	return line;
}


int main(int argc, char** argv) {
	char* self;
	for(size_t n = MAX_PATH;; n *= 2) {
		size_t r;
		self = malloc(n);
		if(!self) return 1;
		r = GetModuleFileNameA(NULL, self, (DWORD)n);
		if(r == 0) return 1;
		if(r < n) break;
		free(self);
	}
	{
		// strip the executable name and the bin/ directory to get the root directory
		size_t x = strlen(self);
		while(self[x] != '\\' && self[x] != '/') --x;
		--x;
		while(self[x] != '\\' && self[x] != '/') --x;
		self[x] = '\0';
	}
	// force forward slash as a file name separator
	for(char* p = self; *p; ++p) if(*p == '\\') *p = '/';
	char* lpath = malloc(CMDLINE_MAX); if(!lpath) abort();
	snprintf(lpath, CMDLINE_MAX, "-L%s/lib", self);
	char* ipath = malloc(CMDLINE_MAX); if(!ipath) abort();
	snprintf(ipath, CMDLINE_MAX, "-I%s/include", self);
	char* buffer = NULL;
	char* compiler = DRV;
	size_t envsz = GetEnvironmentVariable(ENV, NULL, 0);
	if(envsz > 0) {
		buffer = malloc(envsz); if(!buffer) abort();
		if(GetEnvironmentVariable(ENV, buffer, envsz) > 0) compiler = buffer;
	}
	#define N 5 // must be in sync with the actual maximum number of added arguments + NULL
	char** args = malloc((argc + N) * sizeof(*args)); if(!args) abort();
	int show = (argc == 2 && strcmp(argv[1], "-show") == 0);
	size_t i = 0;
	args[i++] = compiler;
	args[i++] = ipath;
	if(!show) for(size_t x = 1; x < argc; ++x) args[i++] = argv[x];
	args[i++] = lpath;
	args[i++] = "-l:libmsmpi.dll.a";
#if defined(FC)
	args[i++] = "-l:libmsmpifec.dll.a";
#endif
	args[i] = NULL;
	if(show) {
		for(size_t x = 0; args[x]; ++x) {
			char* qa = quote_new(args[x]);
			printf("%s ", qa);
		}
		printf("\n");
		fflush(stdout);
		return 0;
	} else {
		char* line = command_line_new(args);
		STARTUPINFOA si;
		PROCESS_INFORMATION pi;
		DWORD result = (DWORD)-1;
		if(!line) return 1;
		memset(&si, 0, sizeof(si));
		si.cb = sizeof(si);
		if(!CreateProcess(NULL, line, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
			fprintf(stderr, "can not run '%s'.\n", args[0]);
			return 1;
		}
		WaitForSingleObject(pi.hProcess, INFINITE);
		GetExitCodeProcess(pi.hProcess, &result);
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
		return result;
	}
}

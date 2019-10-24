#include <time.h>
#include <stdio.h>
#include <assert.h>
#include <process.h>
#include <windows.h>
#include <shellapi.h>
#include <inttypes.h>

#ifdef _WIN64
#define BIT "64"
#else
#define BIT "32"
#endif

#define BUFFER_SIZE 32767

uint32_t hash( uint32_t a) {
    a = (a ^ 61) ^ (a >> 16);
    a = a + (a << 3);
    a = a ^ (a >> 4);
    a = a * 0x27d4eb2d;
    a = a ^ (a >> 15);
    return a;
}

int main(int argc, char** argv, char** envp) {
	int i = strlen(_pgmptr)-1;
	while(_pgmptr[i] != '\\') --i; /* Wipe executable name from path, c:\dir\a.exe --> c:\dir\ */
	char* root = calloc(i+1, sizeof(char)); assert(root);
	strncpy(root, _pgmptr, i);
	char* tmpdir = malloc(BUFFER_SIZE*sizeof(char)); assert(tmpdir);
	char* buffer = malloc(BUFFER_SIZE*sizeof(char)); assert(buffer);
	GetTempPath(BUFFER_SIZE-1, buffer);
	// Create unique temp dir
	srand((unsigned)time(NULL));
	do {snprintf(tmpdir, BUFFER_SIZE, "%s\\buildtests%d", buffer, (unsigned short)hash(rand()));} while(!CreateDirectory(tmpdir, NULL));
	// Fix environment to set to new temp dir
	for(i = 0; envp[i]; ++i) {
		#undef T
		#define T "TMP="
		if(strncmp(T, envp[i], strlen(T)) == 0) {
			char* env = malloc(BUFFER_SIZE*sizeof(char)); assert(env);
			snprintf(env, BUFFER_SIZE, "%s%s", T, tmpdir);
			envp[i] = env;
			continue;
		}
		#undef T
		#define T "TEMP="
		if(strncmp(T, envp[i], strlen(T)) == 0) {
			char* env = malloc(BUFFER_SIZE*sizeof(char)); assert(env);
			snprintf(env, BUFFER_SIZE, "%s%s", T, tmpdir);
			envp[i] = env;
			continue;
		}
#if 1
		// Augment Windows-specific Path variable with root directory
		// When run from within the MSYS2's MingGW shell the PATH variable is augmented automatically
		#define T "Path="
		if(strncmp(T, envp[i], strlen(T)) == 0) {
			char* env = malloc(BUFFER_SIZE*sizeof(char)); assert(env);
			strcpy(buffer, &envp[i][strlen(T)]);
			snprintf(env, BUFFER_SIZE, "%s%s;%s", T, root, buffer);
			envp[i] = env;
			printf("%s", env);
			continue;
		}
#endif
	}
	// Path remover executable
	char* rmpath = malloc(BUFFER_SIZE*sizeof(char)); assert(rmpath);
	snprintf(rmpath, BUFFER_SIZE, "%s\\..\\libexec\\rmpath.exe", root);
	// Construct args
	char** args = malloc(BUFFER_SIZE*sizeof(char*)); assert(args);
	// Set path to tclsh executable
	char* tclsh = malloc(BUFFER_SIZE*sizeof(char)); assert(tclsh);
	snprintf(tclsh, BUFFER_SIZE, "%s\\tclsh.exe", root);
	i = 0;
	args[i++] = tclsh;
	// Set path to init script
	char* all_tcl = malloc(BUFFER_SIZE*sizeof(char)); assert(all_tcl);
	snprintf(all_tcl, BUFFER_SIZE, "%s\\..\\share\\tests\\all.tcl", root);
	args[i++] = all_tcl;
	// Copy remaining args
	for(int ii = 1; ii < argc; ++ii) {
		args[i++] = argv[ii];
	}
	args[i] = NULL;
#ifndef NDEBUG
	puts("\n");
	for(i = 0; envp[i]; ++i) printf("%s\n", envp[i]);
	printf("\n_spawn(_P_WAIT): ");
	for(i = 0; args[i]; ++i) printf("%s ", args[i]);
	puts("\n");
	fflush(stdout);
#endif
	// Execute tests
	int c = _spawnve(_P_WAIT, tclsh, args, envp);
	// Fire up cleaning process
#ifndef NDEBUG
	printf("\n_spawn(_P_NOWAIT): %s %s\n", rmpath, tmpdir);
	fflush(stdout);
#endif
	_spawnl(_P_NOWAIT, rmpath, rmpath, tmpdir, NULL);
	return c;
}	

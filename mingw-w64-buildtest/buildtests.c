#include <time.h>
#include <stdio.h>
#include <assert.h>
#include <process.h>
#include <windows.h>
#include <shellapi.h>
#include <inttypes.h>

#define BUFFER_SIZE 4096

static uint32_t hash( uint32_t a) {
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
	int cleanup = 1; // Should staging directory be deleted after the tests execution.
	// Copy remaining args
	for(int ii = 1; ii < argc; ++ii) {
		// Detect and remove flags meaningless to the tcltest.
		if(strcmp("-nocleanup", argv[ii]) == 0) {
#ifndef NDEBUG
			printf("*** Staging directory cleanup disabled with -nocleanup command-line option\n");
#endif
			cleanup = 0;
			continue;
		}
		args[i++] = argv[ii];
	}
	args[i] = NULL;
	// Filter environment to set to new temp & path.
	i = 0;
	char** envs = malloc(BUFFER_SIZE*sizeof(char*)); assert(envs);
	for(int ii = 0; envp[ii]; ++ii) {
		#undef T
		#define T "TMP="
		if(strncmp(T, envp[ii], strlen(T)) == 0) {
			char* env = malloc(BUFFER_SIZE*sizeof(char)); assert(env);
			snprintf(env, BUFFER_SIZE, "%s%s", T, tmpdir);
			envs[i++] = env;
			continue;
		}
		#undef T
		#define T "TEMP="
		if(strncmp(T, envp[ii], strlen(T)) == 0) {
			char* env = malloc(BUFFER_SIZE*sizeof(char)); assert(env);
			snprintf(env, BUFFER_SIZE, "%s%s", T, tmpdir);
			envs[i++] = env;
			continue;
		}
		// Native Windows cmd and MinGW shell set different path variables.
		// Augment whatever path variable found.
		#undef T
		#define T "PATH="
		if(strncmp(T, envp[ii], strlen(T)) == 0) {
			char* env = malloc(BUFFER_SIZE*sizeof(char)); assert(env);
			strncpy(buffer, envp[i] + strlen(T), BUFFER_SIZE);
			snprintf(env, BUFFER_SIZE, "%s%s;%s", T, root, buffer);
			envs[i++] = env;
			continue;
		}
		#undef T
		#define T "Path="
		if(strncmp(T, envp[ii], strlen(T)) == 0) {
			char* env = malloc(BUFFER_SIZE*sizeof(char)); assert(env);
			strncpy(buffer, envp[i] + strlen(T), BUFFER_SIZE);
			snprintf(env, BUFFER_SIZE, "%s%s;%s", T, root, buffer);
			envs[i++] = env;
			continue;
		}
		//
		envs[i++] = envp[ii];
	}
	{
		char* clup = malloc(BUFFER_SIZE*sizeof(char)); assert(clup);
		snprintf(clup, BUFFER_SIZE, "BUILDTEST_CLEANUP=%d", cleanup);
		envs[i++] = clup;
	}
	envs[i] = NULL;
#ifndef NDEBUG
	puts("\n");
	for(i = 0; envs[i]; ++i) printf("%s\n", envs[i]);
	printf("\n_spawn(_P_WAIT): ");
	for(i = 0; args[i]; ++i) printf("%s ", args[i]);
	puts("\n");
	fflush(stdout);
#endif
	// Execute tests.
	int c = _spawnve(_P_WAIT, tclsh, args, envs);
	// Fire up cleaning process.
	if(cleanup) {
#ifndef NDEBUG
		printf("\n_spawn(_P_NOWAIT): %s %s\n", rmpath, tmpdir);
		fflush(stdout);
#endif
		_spawnl(_P_NOWAIT, rmpath, rmpath, tmpdir, NULL);
	} else {
#ifndef NDEBUG
		printf("\nPreserving staging directory %s\n", tmpdir);
		fflush(stdout);
#endif
	}
	return c;
}	

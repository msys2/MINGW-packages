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

#if 0
int main(int argc, char** argv) {
	int i = strlen(_pgmptr)-1;
	while(_pgmptr[i] != '\\') --i;
	char* root = calloc(i+1, sizeof(char)); assert(root);
	strncpy(root, _pgmptr, i);
	char* stage = calloc(BUFFER_SIZE, sizeof(char)); assert(stage);
	char* temp = calloc(BUFFER_SIZE, sizeof(char)); assert(temp);
	GetTempPath(BUFFER_SIZE-1, temp);
	srand((unsigned)time(NULL));
	do {snprintf(stage, BUFFER_SIZE, "%swhpc%d", temp, (unsigned short)hash(rand()));} while(!CreateDirectory(stage, NULL));
	SetEnvironmentVariable("TMP", stage);
	SetEnvironmentVariable("TEMP", stage);
	char* tclsh = calloc(BUFFER_SIZE, sizeof(char)); assert(tclsh);
	snprintf(tclsh, BUFFER_SIZE, "\"%s\\tclkit.exe\"", root);
	char* init_tcl = calloc(BUFFER_SIZE, sizeof(char)); assert(init_tcl);
	snprintf(init_tcl, BUFFER_SIZE, "\"%s\\share\\tests\\all.tcl\"", root);
	char* root_dir = calloc(BUFFER_SIZE, sizeof(char)); assert(root_dir);
	snprintf(root_dir, BUFFER_SIZE, "\"%s\"", root);
	char* stage_dir = calloc(BUFFER_SIZE, sizeof(char)); assert(stage_dir);
	snprintf(stage_dir, BUFFER_SIZE, "\"%s\"", stage);
	_spawnlp(_P_WAIT, "cmd.exe", "/c", "title", "WHPC"BIT" self check test", "&&", tclsh, whpctest, BIT, root_dir, stage_dir, NULL);
	char** files = {stage, NULL};
	SHFILEOPSTRUCT op;
	op.hwnd = NULL;
	op.wFunc = FO_DELETE;
	op.pFrom = files;
	op.pTo = NULL;
	op.fFlags = FOF_NOCONFIRMATION|FOF_SILENT;
	do {Sleep(3000);} while(SHFileOperation(&op));
}
#endif


int main(int argc, char** argv, char** envp) {
	int i = strlen(_pgmptr)-1;
	while(_pgmptr[i] != '\\') --i; /* Wipe executable name from path, c:\dir\a.exe --> c:\dir\ */
	char* root = calloc(i+1, sizeof(char)); assert(root);
	strncpy(root, _pgmptr, i);
	char* tmpdir = malloc(BUFFER_SIZE*sizeof(char)); assert(tmpdir);
	char* buffer = malloc(BUFFER_SIZE*sizeof(char)); assert(buffer);
	GetTempPath(BUFFER_SIZE-1, buffer);
	srand((unsigned)time(NULL));
	// Create unique temp dir
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
	}
	// Path remover executable
	char* rmpath = malloc(BUFFER_SIZE*sizeof(char)); assert(rmpath);
	snprintf(rmpath, BUFFER_SIZE, "%s\\..\\libexec\\rmpath.exe", root);
	// Construct args
	i = 0;
	char** args = malloc(BUFFER_SIZE*sizeof(char*)); assert(args);
	// Set path to tclsh executable
	char* tclsh = malloc(BUFFER_SIZE*sizeof(char)); assert(tclsh);
	snprintf(tclsh, BUFFER_SIZE, "%s\\tclsh.exe", root);
	args[i++] = tclsh;
	// Set path to init script
	char* all_tcl = malloc(BUFFER_SIZE*sizeof(char)); assert(all_tcl);
	snprintf(all_tcl, BUFFER_SIZE, "%s\\..\\share\\tests\\all.tcl", root);
	args[i++] = all_tcl;
	// Copy remaining args
	for(int argvi = 1; argvi < argc; ++argvi) {
		args[i++] = argv[argvi];
	}
	args[i] = NULL;
	// Execute tests
	int c = _spawnve(_P_WAIT, tclsh, args, envp);
	// Fire up cleaning process
	_spawnl(_P_NOWAIT, rmpath, rmpath, tmpdir, NULL);
	return c;
}	

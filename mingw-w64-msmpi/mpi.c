#include <stdio.h>
#include <process.h>
#include <windows.h>

#if defined(CC)
	#define C "cc.exe"
	#define E "MPICC"
#elif defined(CXX)
	#define C "c++.exe"
	#define E "MPICXX"
#elif defined(FC)
	#define C "gfortran.exe"
	#define E "MPIFORT"
#else
	#error
#endif

int main(int argc, char** argv) {
	int s = strlen(argv[0]);
	while(argv[0][s] != '\\' && argv[0][s] != '/') --s; --s;
	while(argv[0][s] != '\\' && argv[0][s] != '/') --s;
	argv[0][s] = '\0';
        // Force forward slash as a file name separator
        s = strlen(argv[0]);
        while(s >= 0) if(argv[0][s--] == '\\') argv[0][s+1] = '/';
#define SZ 32767	
	char* lpath = malloc(SZ*sizeof(char));
	snprintf(lpath, SZ, "-L%s/lib", argv[0]);
	char* ipath = malloc(SZ*sizeof(char));
	snprintf(ipath, SZ, "-I%s/include", argv[0]);
	char* comp = malloc(SZ*sizeof(char));
	char** args = malloc(SZ*sizeof(char*));
	int show = (argc == 2 && strcmp(argv[1], "-show") == 0);
	int i = 0;
	args[i++] = GetEnvironmentVariable(E, comp, SZ) > 0 ? comp : C;
	args[i++] = ipath;
#ifdef FC
	// Workarounds for the GFORTRAN 10 increased strictness
	args[i++] = "-fallow-invalid-boz";
	args[i++] = "-fallow-argument-mismatch";
#endif
	if(!show) for(int x = 1; x < argc; ++x) args[i++] = argv[x];
	args[i++] = lpath;
	args[i++] = "-lmsmpi";
	args[i] = NULL;
	if(show) {
		for(int x = 0; args[x]; ++x) printf("%s ", args[x]);
		printf("\n");
		fflush(stdout);
		return 0;
	} else {
		return _spawnvp(_P_WAIT, args[0], (const char* const*)args);
	}
};

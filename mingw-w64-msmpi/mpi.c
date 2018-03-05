#include <stdio.h>
#include <process.h>
#include <windows.h>

#if defined(CC)
	#define C "gcc.exe"
	#define E "MPICC"
#elif defined(CXX)
	#define C "g++.exe"
	#define E "MPICXX"
#elif defined(FC)
	#define C "gfortran.exe"
	#define E "MPIFC"
#else
	#error
#endif

int main(int argc, char** argv) {
	int s = strlen(argv[0]);
	while(argv[0][s] != '\\' && argv[0][s] != '/') --s; --s;
	while(argv[0][s] != '\\' && argv[0][s] != '/') --s;
	char slash = argv[0][s];
	argv[0][s] = '\0';
#define SZ 32767	
	char* lpath = malloc(SZ*sizeof(char));
	snprintf(lpath, SZ, "-L%s%clib", argv[0], slash);
	char* ipath = malloc(SZ*sizeof(char));
	snprintf(ipath, SZ, "-I%s%cinclude", argv[0], slash);
	char* comp = malloc(SZ*sizeof(char));
	char** args = malloc(SZ*sizeof(char*));
	int show = (argc == 2 && strcmp(argv[1], "-show") == 0);
	int i = 0;
	args[i++] = GetEnvironmentVariable(E, comp, SZ) > 0 ? comp : C;
	args[i++] = ipath;
#ifdef FC
	args[i++] = "-fno-range-check";
#else	
	args[i++] = "-include"; args[i++] = "inttypes.h";
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
		/*if(GetEnvironmentVariable("MSMPIDRVFLAGS", comp, SZ) > 0 && strcmp(comp, "-v") == 0) {
			for(int x = 0; args[x]; ++x) printf("%s ", args[x]);
			printf("\n");
			fflush(stdout);
		}*/
		return _spawnvp(_P_WAIT, args[0], args);
	}
};
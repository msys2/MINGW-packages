#define _GNU_SOURCE 1
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/cygwin.h>
#include <unistd.h>

#define iswinabspath(p) (isalpha((unsigned)*(p)) && (p)[1] == ':' && \
			 ((p)[2] == '/' || (p)[2] == '\\'))

int main(int argc, char **argv)
{
	char newcmd[260] = "/usr/bin/";
	char *newargv[argc + 1];
	newargv[0] = strcat (newcmd, basename (argv[0]));
	for (int i = 1; i < argc; ++i)
	{
		if (iswinabspath (argv[i]))
			newargv[i] = cygwin_create_path (CCP_WIN_A_TO_POSIX, argv[i]);
		else
			newargv[i] = argv[i];
	}
	newargv[argc] = NULL;
	execv(newargv[0], newargv);
	return 42;
}

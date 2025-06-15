#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/cygwin.h>
#include <sys/errno.h>
#include <unistd.h>

#ifndef LLVM_CONFIG_PATH
#  define LLVM_CONFIG_PATH "/usr/bin/llvm-config"
#endif

int main (int argc, char ** argv) {
	int filter_output = 0;
	for (int i = 1; i < argc && !filter_output; ++i)
		if (strcmp(argv[i], "--ldflags") == 0)
			filter_output = 1;

	argv[0] = LLVM_CONFIG_PATH;
	if (!filter_output) {
		execv(argv[0], argv);
		return 42;
	}
	char *cmd = (char *)malloc(32768);
	if (!cmd)
		return ENOMEM;
	char *p = stpcpy(cmd, argv[0]);
	for (int i = 1; i < argc; ++i) {
		int n = sprintf(p, " '%s'", argv[i]);
		for (char *q = memchr(p + 2, '\'', n - 3); q; q = memchr(q, '\'', n - (q - p) - 1)) {
			memmove(q + 3, q, n - (q - p));
			*(q + 1) = '\\';
			*(q + 2) = '\'';
			q += 4;
			n += 3;
		}
		p += n;
	}
	FILE *fh = popen(cmd, "r");
	char *line = NULL;
	size_t n = 0;
	ssize_t read;

	while ((read = getline(&line, &n, fh)) != -1) {
		for (char *tok = strtok(line, " \n"); tok; tok = strtok(NULL, " \n")) {
			if (strncmp(tok, "-L", 2) == 0) {
				char *path = cygwin_create_path (CCP_POSIX_TO_WIN_A, tok + 2);
				p = path;
				while ((p = strchr (p, '\\')))
					*p = '/';
				fputs("-L", stdout);
				fputs(path, stdout);
				fputc(' ', stdout);
				free(path);
			} else {
				fputs(tok, stdout);
				fputc(' ', stdout);
			}
		}
		fputc('\n', stdout);
	}
	free (line);
	return pclose (fh);
}

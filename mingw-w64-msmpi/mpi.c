#include <winsock2.h>
#include <windows.h>
#include <stdarg.h>
#include <uv.h>


static char* printf_new(char* format, ...) {
  va_list vas;
  size_t size = 1<<16;
  char* buffer;
  while(1) {
    buffer = malloc(size*sizeof(char)); if(!buffer) abort();
    va_start(vas, format);
    int r = vsnprintf(buffer, size, format, vas);
    va_end(vas);
    if(r < 0) abort();
    if(r < size) break;
    size *= 2;
    free(buffer);
  }
  return buffer;
}


static int result = 1;
static void on_exit(uv_process_t *request, int64_t status, int signal) {
  result = signal != 0 ? 128 + signal : status;
  uv_close((uv_handle_t*)request, NULL);
}


int main(int argc, char **argv) {

  char* root; // mingw prefix directory
  char* bin; // module's directory

  {
    size_t size;
    char* buffer;
    size = 1<<16;
    while(1) {
      root = malloc(size*sizeof(char)); if (!root) abort();
      size_t r = GetModuleFileName(NULL, root, size);
      if(r == 0) abort();
      if(r < size) {
        char* c = root;
        // forward sweep: replace all backward slashes with forwards
        while(*c) {
          if(*c == '\\') *c = '/';
          ++c;
        }
        // *c == 0
        // backward sweep: chop trailing segments
        while(c >= root && *c != '/') --c; *c = '\0'; // chop module name
        bin = strdup(root); // look for the compiler in this wrapper's directory
        while(c >= root && *c != '/') --c; // chop module's immediate directory
        if(c > root) {
          *c = '\0';
          break;
        }
        abort(); // module is in a disk's root directory or malformed module's path
      }
      size *= 2;
      free(root);
    }
  }

  #define ARGV_SIZE 1024

  size_t s = 0;
  char* args[ARGV_SIZE];

  #define RUN (1u<<0)
  #define SHOW_COMPILE (1u<<1)
  #define SHOW_LINK (1u<<2)
  #define SHOW_DRIVER (1u<<3)
  #define SHOW_ARGS (1u<<4)

  unsigned mode = RUN | SHOW_DRIVER | SHOW_COMPILE | SHOW_ARGS | SHOW_LINK;

  if(argc == 2) {
    if(strcmp(argv[1], "-show") == 0) mode = SHOW_DRIVER | SHOW_COMPILE | SHOW_LINK; else
    if(strcmp(argv[1], "-show:compile") == 0) mode = SHOW_DRIVER | SHOW_COMPILE; else
    if(strcmp(argv[1], "-show:link") == 0) mode = SHOW_DRIVER | SHOW_LINK; else
    if(strcmp(argv[1], "-showme") == 0) mode = SHOW_COMPILE | SHOW_LINK; else
    if(strcmp(argv[1], "-showme:compile") == 0) mode = SHOW_COMPILE; else
    if(strcmp(argv[1], "-showme:link") == 0) mode = SHOW_LINK;
  }
  
  if(mode & SHOW_DRIVER) {
	  char* buffer = malloc(MAX_PATH*sizeof(char)); if(!buffer) abort();
    args[s++] = GetEnvironmentVariable(ENV, buffer, MAX_PATH) > 0 ? buffer : printf_new("%s/%s", bin, DRV);
  }
  
  if(mode & SHOW_COMPILE) {
    args[s++] = printf_new("-I%s/include", root);
  }

  // inject wrapper's arguments
  if(mode & SHOW_ARGS) {
    for(size_t v = 1; v < argc; ++v) args[s++] = argv[v];
  }

  if(mode & SHOW_LINK) {
    args[s++] = printf_new("-L%s/lib", root);
    args[s++] = "-l:libmsmpi.dll.a";
#ifdef FC
    args[s++] = "-l:libmsmpifec.dll.a";
#endif
}

  args[s++] = NULL;

  if(s > ARGV_SIZE) abort();

  if(!(mode & RUN)) {
    for(char** p = args; *p; ++p) {
      fprintf(stdout, "'%s' ", *p); // TODO
    }
    fputs("\n", stdout);
    return 0;
  }

#ifndef NDEBUG
    for(char** p = args; *p; ++p) {
      fprintf(stderr, "'%s' ", *p); // TODO
    }
    fputs("\n", stderr);
#endif

  uv_loop_t* loop = uv_default_loop();

  uv_process_t spawned;
  uv_process_options_t options = {0};

  options.args = args;
  options.file = args[0];

  options.exit_cb = on_exit;

  uv_stdio_container_t stdio[3];
  for(int i = 0; i < 3; ++i) {
    stdio[i].flags = UV_INHERIT_FD;
    stdio[i].data.fd = i;
  }
  options.stdio = stdio;
  options.stdio_count = 3;

  int r;
  if ((r = uv_spawn(loop, &spawned, &options))) {
      fprintf(stderr, "process spawn error: %s\n", uv_strerror(r));
      return 1;
  }

  uv_run(loop, UV_RUN_DEFAULT);

  return result;
}



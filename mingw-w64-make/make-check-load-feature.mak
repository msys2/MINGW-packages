# Check whether GNU Make's dll load feature
# - is unlocked and compiled into $(MAKE)
# - works when using the compiled import library libgnumake-1.dll.a

# Check if our make prog is compiled with activated load feature.
ifeq ($(filter load,$(.FEATURES)),)
  $(error FAILED ...GNU Make feature to load a dll not active in executable '$(MAKE)')
else
  $(warning PASSED ... GNU Make feature to load a dll active in executable '$(MAKE)'.)
endif

# If loading the dll fails, make will try to build the
# target 'mk_temp.dll' and restart from scratch again.
-load mk_temp.dll

# Run and check the new make function 'mk-temp'.
.PHONY: all
  TXT_ERROR  = FAILED  Temporary file not created
  TXT_PASSED = PASSED  Temporary file created.
all:
	$(if $(mk-temp tmpfile.),$(warning $(TXT_PASSED)),$(error $(TXT_ERROR)))

ifeq ($(LIBS),)
  LIBS       = -lgnumake-1
endif

%.dll: %.c
	$(warning Compiling $@.)
	gcc -shared -fPIC $(CFLAGS) -o $@ $< $(LIBS)

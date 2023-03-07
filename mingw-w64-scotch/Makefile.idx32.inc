EXE		=
LIB		= .a
OBJ		= .o

MAKE		= make
AR		= ar
ARFLAGS	= -ruv
CAT		= cat
CCS		= $(CC)
CCP		= mpicc
CCD		= $(CC)
CFLAGS += -DCOMMON_RANDOM_FIXED_SEED -DSCOTCH_RENAME -Drestrict=__restrict -DCOMMON_PTHREAD -DSCOTCH_PTHREAD -DCOMMON_FILE_COMPRESS_GZ -DCOMMON_OS_WINDOWS
CLIBFLAGS	=
LDFLAGS	= -lz -pthread
CP		= cp
FLEX = flex
LN		= ln
MKDIR		= mkdir
MV		= mv
RANLIB		= ranlib
BISON = bison

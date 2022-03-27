#
# diStorm3 (Linux Port)
#

LIB_NAME	= libdistorm3
SHARED_EXT	= .dll
STATIC_EXT	= .a
IMPORT_EXT	= ${SHARED_EXT}${STATIC_EXT}

TARGET	=  ${LIB_NAME}${SHARED_EXT}
STATICLIB	= ${LIB_NAME}${STATIC_EXT}
IMPORTLIB	= ${LIB_NAME}${IMPORT_EXT}
COBJS	= ../../src/mnemonics.o ../../src/textdefs.o ../../src/prefix.o ../../src/operands.o ../../src/insts.o ../../src/instructions.o ../../src/distorm.o ../../src/decoder.o
CC	= gcc
CFLAGS	= -O2 -Wall -DSUPPORT_64BIT_OFFSET -DDISTORM_STATIC
WINDRES	= windres

INSTALL_PREFIX = /usr/local

all: clib

clean:
	/bin/rm -rf ../../src/*.o ${TARGET} ../../${STATICLIB}  ../../${IMPORTLIB} ./../*.o

resource.o: resource.rc
	$(WINDRES) --include-dir=. resource.rc $@

clib: ${COBJS}
	${CC} ${CFLAGS} ${VERSION} ${COBJS} -shared -o ${TARGET} -Wl,--out-implib,${IMPORTLIB}
	ar rs ${STATICLIB} ${COBJS}

install: ${TARGET}
	mkdir ${DESTDIR}${INSTALL_PREFIX}
	mkdir ${DESTDIR}${INSTALL_PREFIX}/bin
	mkdir ${DESTDIR}${INSTALL_PREFIX}/lib
	install ${TARGET} ${DESTDIR}${INSTALL_PREFIX}/bin
	install ${IMPORTLIB} ${DESTDIR}${INSTALL_PREFIX}/lib
	install ${STATICLIB} ${DESTDIR}${INSTALL_PREFIX}/lib

.c.o:
	${CC} ${CFLAGS} ${VERSION} -c $< -o $@


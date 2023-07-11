# GCC makefile for WinSparkle
# Copyright (C) 2015, 2016 Renato Silva
# This file is licensed under the same terms as WinSparkle

all: static dll

translations:
	@for file in ../translations/*.po; do\
		msgfmt $$file -o "$${file/%po/mo}"; \
	done
	windres ../translations/translations.rc -O coff -o translations.res

objects:
	$(CC) -std=c++11 -c -mthreads -D_WIN32_WINNT=0x600 -DUNICODE -D_UNICODE -DBUILDING_WIN_SPARKLE appcast.cpp          -I../include
	$(CC) -std=c++11 -c -mthreads -D_WIN32_WINNT=0x600 -DUNICODE -D_UNICODE -DBUILDING_WIN_SPARKLE appcontroller.cpp    -I../include
	$(CC) -std=c++11 -c -mthreads -D_WIN32_WINNT=0x600 -DUNICODE -D_UNICODE -DBUILDING_WIN_SPARKLE dll_api.cpp          -I../include
	$(CC) -std=c++11 -c -mthreads -D_WIN32_WINNT=0x600 -DUNICODE -D_UNICODE -DBUILDING_WIN_SPARKLE dllmain.cpp          -I../include
	$(CC) -std=c++11 -c -mthreads -D_WIN32_WINNT=0x600 -DUNICODE -D_UNICODE -DBUILDING_WIN_SPARKLE download.cpp         -I../include
	$(CC) -std=c++11 -c -mthreads -D_WIN32_WINNT=0x600 -DUNICODE -D_UNICODE -DBUILDING_WIN_SPARKLE error.cpp            -I../include
	$(CC) -std=c++11 -c -mthreads -D_WIN32_WINNT=0x600 -DUNICODE -D_UNICODE -DBUILDING_WIN_SPARKLE settings.cpp         -I../include
	$(CC) -std=c++11 -c -mthreads -D_WIN32_WINNT=0x600 -DUNICODE -D_UNICODE -DBUILDING_WIN_SPARKLE signatureverifier.cpp -I../include
	$(CC) -std=c++11 -c -mthreads -D_WIN32_WINNT=0x600 -DUNICODE -D_UNICODE -DBUILDING_WIN_SPARKLE threads.cpp          -I../include
	$(CC) -std=c++11 -c -mthreads -D_WIN32_WINNT=0x600 -DUNICODE -D_UNICODE -DBUILDING_WIN_SPARKLE updatechecker.cpp    -I../include
	$(CC) -std=c++11 -c -mthreads -D_WIN32_WINNT=0x600 -DUNICODE -D_UNICODE -DBUILDING_WIN_SPARKLE ui.cpp               -I../include $(shell wx-config-3.2 --cflags)
	$(CC) -std=c++11 -c -mthreads -D_WIN32_WINNT=0x600 -DUNICODE -D_UNICODE -DBUILDING_WIN_SPARKLE updatedownloader.cpp -I../include $(shell wx-config-3.2 --cflags)

static: objects
	ar rcs libwinsparkle.a *.o

dll: objects translations
	$(CC) -shared -mthreads -o libwinsparkle.dll *.o translations.res -Wl,--out-implib,libwinsparkle.dll.a -lstdc++ $(shell wx-config-3.2 --libs core) -lexpat -lcrypto -lssl -lcrypt32 -lwininet -lversion -lole32 -loleaut32 -lrpcrt4 -luuid

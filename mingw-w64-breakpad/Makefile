CXX=g++
WINDRES=windres
CXXFLAGS=-DUNICODE -D_UNICODE
LDFLAGS=-mwindows -municode

OBJS=abstract_class.o crash_generation_app.o resource.o
LIBS=-lbreakpad_client

all: crash_generation_app.exe

resource.o: resource.rc small.ico crash_generation_app.ico
	$(WINDRES) resource.rc $@

crash_generation_app.exe: $(OBJS)
	$(CXX) -o $@ $(OBJS) $(LIBS) $(LDFLAGS)

clean:
	${RM} ${OBJS}

PREFIX = i686-w64-mingw32-
CC      = $(PREFIX)gcc
STRIP   = $(PREFIX)strip

# struct packing is broken in old mingw gcc versions without no-ms-bitfields
CFLAGS  = -O3 -Wall -Wno-parentheses -Wno-format-overflow -fno-strict-aliasing -mno-ms-bitfields
LDFLAGS = -shared

EXE_OBJS = framestep.o
DLL_OBJS = dll.o

EXE_LIBS = -lshlwapi -mwindows
DLL_LIBS = -luser32 -lgdi32 -lwinmm

all: framestep.exe framestep.dll

.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f $(EXE_OBJS) framestep.exe $(DLL_OBJS) framestep.dll

framestep.exe: $(EXE_OBJS)
	$(CC) -o $@ $(EXE_OBJS) $(EXE_LIBS)
	$(STRIP) -s $@

framestep.dll: $(DLL_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(DLL_OBJS) $(DLL_LIBS)
	$(STRIP) -s $@
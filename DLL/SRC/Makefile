CC = gcc
LDFLAGS = -Wl,--nxcompat,--no-seh,--large-address-aware,-s,--disable-forceinteg,--subsystem,console
LDFLAGS_DEBUG = -Wl,--nxcompat,--no-seh,--large-address-aware,--disable-reloc-section,--disable-forceinteg,--disable-dynamicbase,--subsystem,console
CFLAGS = -Wall -Wextra -m32 -m80387 -maes -march=i686 -mfpmath=sse -mmmx -msse -msse2 -msse3 -msse4 -msse4.1 -mwindows -s -O2 -fomit-frame-pointer -ftree-vectorize -DNDEBUG
CFLAGS_DEBUG = -Wall -Wextra -m32 -m80387 -maes -march=i686 -mfpmath=sse -mmmx -msse -msse2 -msse3 -msse4 -msse4.1 -mwindows -O0 -g -DDEBUG

.PHONY: all
all: run.dll hook.exe inject.dll

run.dll: run.c
	$(CC) $(CFLAGS) -shared $< -o $@ $(LDFLAGS)

hook.exe: hook.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

inject.dll: inject.c
	$(CC) $(CFLAGS) -shared $< -o $@ $(LDFLAGS)

.PHONY: debug_all debug_run debug_hook debug_inject
debug_all: debug_run debug_hook debug_inject

debug_run: run.c
	$(CC) $(CFLAGS_DEBUG) -shared $< -o run.dll $(LDFLAGS_DEBUG)

debug_hook: hook.c
	$(CC) $(CFLAGS_DEBUG) $< -o hook.exe $(LDFLAGS_DEBUG)

debug_inject: inject.c
	$(CC) $(CFLAGS_DEBUG) -shared $< -o inject.dll $(LDFLAGS_DEBUG)

clean:
	rm -f run.dll hook.exe inject.dll

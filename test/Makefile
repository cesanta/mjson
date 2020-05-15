all: test
CFLAGS ?= -g -W -Wall -I../src
GCOVCMD ?= true

ifeq ($(shell uname -s),Darwin)
ifeq ($(CC),clang)
	EXTRA = -coverage
	GCOVCMD = gcov unit_test.c ; curl -s https://codecov.io/bash | /bin/bash
endif
endif

test: ../src/mjson.h unit_test.c
	$(CC) unit_test.c -std=c99 $(CFLAGS) $(EXTRA) -o unit_test && $(DEBUGGER) ./unit_test
	g++ -g -x c++ unit_test.c $(CFLAGS) -o unit_test && ./unit_test
	@test "$(GCOVCMD)" == true || $(GCOVCMD)

PDIR ?= $(realpath $(CURDIR)/..)
VC98 = docker run -v $(PDIR):$(PDIR) -w $(CURDIR) docker.io/mgos/vc98
VCFLAGS = /nologo /W3 /O1 /I../src
vc98: unit_test.c ../src/mjson.h
	$(VC98) wine cl unit_test.c $(VCFLAGS) $(TFLAGS) /Fe$@.exe
	$(VC98) wine $@.exe

clean:
	rm -rf unit_test *.exe *.obj *.dSYM

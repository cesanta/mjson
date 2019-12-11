all: test
CFLAGS ?= -g -W -Wall 
GCOVCMD ?= true

ifeq ($(shell uname -s),Darwin)
ifeq ($(CC),clang)
	EXTRA = -coverage
	GCOVCMD = gcov unit_test.c ; bash <(curl -s https://codecov.io/bash)
endif
endif

test: mjson.h unit_test.c
	$(CC) unit_test.c -std=c99 $(CFLAGS) $(EXTRA) -o unit_test && $(DEBUGGER) ./unit_test
	g++ -g -x c++ unit_test.c $(CFLAGS) -o unit_test && ./unit_test
	$(GCOVCMD)

VC98 = docker run -v $(CURDIR):$(CURDIR) -w $(CURDIR) docker.io/mgos/vc98
VCFLAGS = /nologo /W4 /O1
vc98: unit_test.c mjson.h
	$(VC98) wine cl unit_test.c /DINCLUDE_MJSON_C $(TFLAGS) /Fe$@.exe
	$(VC98) wine $@.exe

clean:
	rm -rf unit_test *.exe *.obj *.dSYM

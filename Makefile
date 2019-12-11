all: test
CFLAGS ?= -g -W -Wall 

ifeq ($(shell uname -s),Darwin)
ifeq ($(CC),clang)
	CFLAGS += -coverage
	GCOV = gcov
endif
endif

test: mjson.h unit_test.c
	$(CC) unit_test.c -std=c99 $(CFLAGS) -o unit_test && $(DEBUGGER) ./unit_test
	g++ -g -x c++ unit_test.c $(CFLAGS) -o unit_test && ./unit_test

VC98 = docker run -v $(CURDIR):$(CURDIR) -w $(CURDIR) docker.io/mgos/vc98
VCFLAGS = /nologo /W4 /O1
vc98: unit_test.c mjson.h
	$(VC98) wine cl unit_test.c /DINCLUDE_MJSON_C $(TFLAGS) /Fe$@.exe
	$(VC98) wine $@.exe

clean:
	rm -rf unit_test *.exe *.obj *.dSYM

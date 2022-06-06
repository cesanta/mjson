DEFS = -DMJSON_ENABLE_MERGE=1 -DMJSON_ENABLE_PRETTY=1
WARN ?= -W -Wall -Wextra -Werror -Wshadow -Wdouble-promotion -fno-common -Wconversion
CFLAGS ?= $(WARN) -g3 -Os -I../src $(DEFS)
GCOVCMD ?= true
CWD ?= $(realpath $(CURDIR))
ROOT ?= $(realpath $(CWD)/../../..)
DOCKER ?= docker run --rm -e Tmp=. -e WINEDEBUG=-all -v $(ROOT):$(ROOT) -w $(CWD)
VCFLAGS = /nologo /W3 /O1 /I../src /MD $(DEFS) $(TFLAGS)
SRC = unit_test.c ../src/mjson.c

ifeq ($(shell uname -s),Darwin)
ifeq ($(CC),clang)
	EXTRA = -coverage
	GCOVCMD = gcov unit_test.c ; curl -s https://codecov.io/bash | /bin/bash
endif
endif

all: test linux arm armlinux vc98 vc2017 vc22

test: ../src/mjson.h ../src/mjson.c unit_test.c
	$(CC) $(SRC) $(CFLAGS) $(EXTRA) -o unit_test && $(RUN) ./unit_test
	$(CXX) -g -x c++ $(SRC) $(CFLAGS) -o unit_test && $(RUN) ./unit_test
	@test -z "$(GCOVCMD)" || $(GCOVCMD)

linux: test
linux: CFLAGS += -O2 -fsanitize=address,undefined,shift,null,return,bounds,alignment,object-size,bool,enum -static-libasan
linux: CC = $(DOCKER) mdashnet/cc2 cc
linux: CXX = $(DOCKER) mdashnet/cc2 c++
linux: RUN = $(DOCKER) mdashnet/cc2

armlinux: test
armlinux: CC = $(DOCKER) mdashnet/armlinux cc
armlinux: CXX = $(DOCKER) mdashnet/armlinux c++
armlinux: RUN = $(DOCKER) mdashnet/armlinux

arm: unit_test.c ../src/mjson.h ../src/mjson.c
	$(DOCKER) mdashnet/armgcc arm-none-eabi-gcc $(SRC) $(CFLAGS) --specs=nosys.specs -o unit_test

vc98: unit_test.c ../src/mjson.h ../src/mjson.c
	$(DOCKER) mdashnet/vc98 wine cl $(SRC) $(VCFLAGS) /Fe$@.exe
	$(DOCKER) mdashnet/vc98 wine $@.exe

vc22: unit_test.c ../src/mjson.h ../src/mjson.c
	$(DOCKER) mdashnet/vc22 wine64 cl $(SRC) $(VCFLAGS) /Fe$@.exe
	$(DOCKER) mdashnet/vc22 wine64 $@.exe

vc2017: unit_test.c ../src/mjson.h ../src/mjson.c
	$(DOCKER) mdashnet/vc2017 wine64 cl $(SRC) $(VCFLAGS) /Fe$@.exe
	$(DOCKER) mdashnet/vc2017 wine64 $@.exe

mingw: unit_test.c ../src/mjson.h ../src/mjson.c
	$(DOCKER) mdashnet/mingw x86_64-w64-mingw32-gcc $(SRC) $(CFLAGS) -Wno-float-conversion -o $@.exe
	$(DOCKER) mdashnet/vc22 wine64 $@.exe

arduino:
	rm -rf /tmp/ArduinoDeviceShadowExample
	cp -r ../examples/ArduinoDeviceShadowExample /tmp/
	cp ../* /tmp/ArduinoDeviceShadowExample
	/Applications/Arduino.app/Contents/MacOS/Arduino --verify /tmp/ArduinoDeviceShadowExample/ArduinoDeviceShadowExample.ino

clean:
	rm -rf unit_test *.exe *.obj *.dSYM

PROG = main

LIBS = -lm -ldl

ifeq ($(type),vplotter)
	MACHINE_TYPE = -D__VPLOTTER__
else
	MACHINE_TYPE = 
endif

# define the C compiler to use
ifeq ($(MAKECMDGOALS),pi_cc)
	CC = arm-linux-gnueabihf-gcc
	MAIN = sketchy-server
	PROG = main
	CFLAGS = -W -Wall -Wno-missing-field-initializers -pthread -O0 $(CFLAGS_EXTRA) $(MACHINE_TYPE) $(MODULE_CFLAGS) -DMG_ENABLE_SSL -lssl -lcrypto
	SOURCES = $(PROG).c mongoose/mongoose.c ../sketchy_shared/sketchy-ipc.c ../sketchy_driver/Config.c ../sketchy_driver/inih/ini.c
	INCLUDES = -I../sketchy_shared -I../sketchy_driver -I../sketchy_driver/inih -I../sketchy_driver/nanosvg -I/tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian-x64/arm-linux-gnueabihf/include
else
	MAKECMDGOALS = default
	CC = gcc
	MAIN = sketchy-server
	PROG = main
	CFLAGS = -W -Wall -Wno-missing-field-initializers -pthread -O0 $(CFLAGS_EXTRA) $(MACHINE_TYPE) $(MODULE_CFLAGS) -DMG_ENABLE_SSL -lssl -lcrypto
	SOURCES = $(PROG).c mongoose/mongoose.c ../sketchy_shared/sketchy-ipc.c ../sketchy_driver/Config.c ../sketchy_driver/inih/ini.c
	INCLUDES = -I../sketchy_shared -I../sketchy_driver -I../sketchy_driver/inih -I../sketchy_driver/nanosvg 
endif
OBJS = $(SOURCES:.c=.o)
.PHONY: depend clean test pi_cc

$(MAKECMDGOALS):$(MAIN)
	@echo sketchy server has been compiled

$(MAIN): $(OBJS)
	$(CC) -o $(MAIN) $(OBJS) $(CFLAGS) $(INCLUDES) $(LIBS)

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<  -o $@

clean:
	rm -rf $(MAIN) *.exe *.dSYM *.obj *.exp *.o *.lib mongoose/*.o ../sketchy_shared/*.o ../sketchy_driver/inih/*.o ../sketchy_driver/Config.o

install:
	install sketchy-server ../build
	install index.html ../build
	install manifest.ini ../build/job

depend: $(SRCS)
	makedepend $(INCLUDES) $^

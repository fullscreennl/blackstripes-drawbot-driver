# define the C compiler to use
CC = gcc

# define library paths in addition to /usr/lib
#   if I wanted to include libraries not in /usr/lib I'd specify
#   their path using -Lpath, something like:
LFLAGS = -Llua-5.2.3 -L../sketchy_shared
CFLAGS_EXTRA = -I/tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian-x64/arm-linux-gnueabihf/include
# define any libraries to link into executable:
#   if I want to link in libraries (libx.so or libx.a) I use the -llibname 
#   option, something like (this will link in libmylib.so and libm.so:
LIBS = -lm -llua -ldl

ifeq ($(type),vplotter)
	MACHINE_TYPE = -D__VPLOTTER__
else
	MACHINE_TYPE = 
endif

ifeq ($(MAKECMDGOALS),t)
	MAIN = test
	CFLAGS = -Wall -g -D__TEST__ $(MACHINE_TYPE)
	INCLUDES = -I../sketchy_shared
	SRCS = test.c FSNumber.c FSArray.c FSObject.c Point.c Model.c Step.c sketchy.c SpeedManager.c lodepng/lodepng.c Preview.c inih/ini.c Config.c ../sketchy_shared/sketchy-ipc.c
else ifeq ($(MAKECMDGOALS),pi)
	MAIN = sketchy-driver
	CFLAGS = -Wall -g -D__PI__ $(MACHINE_TYPE)
	INCLUDES = -I/usr/include/xenomai -Ibcm2835-1.25 -Ilua-5.2.3 -I../sketchy_shared
	LFLAGS = -L/usr/lib/xenomai -Llua-5.2.3 -L../sketchy_shared
	LIBS = -lnative -lxenomai -lm -llua -ldl
	SRCS = main.c FSNumber.c FSArray.c FSObject.c Point.c Model.c Step.c bcm2835-1.25/bcm2835.c SpeedManager.c sketchy.c inih/ini.c Config.c ../sketchy_shared/sketchy-ipc.c
else ifeq ($(MAKECMDGOALS),pi_cc)
	CC =  arm-linux-gnueabihf-gcc
	MAIN = sketchy-driver
	CFLAGS = -Wall -g -D__PI__ $(MACHINE_TYPE) $(CFLAGS_EXTRA)
	INCLUDES = -I/usr/include/xenomai -Ibcm2835-1.25 -Ilua-5.2.3 -I../sketchy_shared
	LFLAGS = -L/usr/lib/xenomai -Llua-5.2.3 -L../sketchy_shared
	LIBS = -lnative -lxenomai -lm -llua -ldl
	SRCS = main.c FSNumber.c FSArray.c FSObject.c Point.c Model.c Step.c bcm2835-1.25/bcm2835.c SpeedManager.c sketchy.c inih/ini.c Config.c ../sketchy_shared/sketchy-ipc.c
else ifeq ($(MAKECMDGOALS),preview)
	MAIN = sketchy-preview
	CFLAGS = -Wall -g -D__SIM__ $(MACHINE_TYPE)
	INCLUDES = -I../sketchy_shared
	SRCS = main.c FSNumber.c FSArray.c FSObject.c Point.c Model.c Step.c SpeedManager.c sketchy.c lodepng/lodepng.c Preview.c inih/ini.c Config.c ../sketchy_shared/sketchy-ipc.c
else
	MAKECMDGOALS = default
	MAIN = sketchy-driver
	CFLAGS = -Wall -g -D__SIM__ $(MACHINE_TYPE)
	INCLUDES = -I../sketchy_shared
	SRCS = main.c FSNumber.c FSArray.c FSObject.c Point.c Model.c Step.c SpeedManager.c sketchy.c lodepng/lodepng.c Preview.c inih/ini.c Config.c ../sketchy_shared/sketchy-ipc.c
endif


# define the C object files 
#
# This uses Suffix Replacement within a macro:
#   $(name:string1=string2)
#         For each word in 'name' replace 'string1' with 'string2'
# Below we are replacing the suffix .c of all words in the macro SRCS
# with the .o suffix
#
OBJS = $(SRCS:.c=.o)

#
# The following part of the makefile is generic; it can be used to 
# build any executable just by changing the definitions above and by
# deleting dependencies appended to the file from 'make depend'
#

.PHONY: depend clean test pi pi_cc preview

$(MAKECMDGOALS):$(MAIN)
		@echo sketchy has been compiled

$(MAIN):$(OBJS) 
		$(CC) $(CFLAGS) $(INCLUDES) -o $(MAIN) $(OBJS) $(LFLAGS) $(LIBS)

# this is a suffix replacement rule for building .o's from .c's
# it uses automatic variables $<: the name of the prerequisite of
# the rule(a .c file) and $@: the name of the target of the rule (a .o file) 
# (see the gnu make manual section about automatic variables)
.c.o:
		$(CC) $(CFLAGS) $(INCLUDES) -c $<  -o $@

clean:
		$(RM) *.o *~ sketchy test sketchy-driver sketchy-preview

install:
		install sketchy-driver ../build

installp:
		install sketchy-preview ../build

depend: $(SRCS)
		makedepend $(INCLUDES) $^

# DO NOT DELETE THIS LINE -- make depend needs it

##
CC=gcc
##
##
RM=rm
##
##  these are compile time constructs, that affect the running of the game.
CFLAGS = -DNEW_RENT -DLEVEL_LOSS -DNEWEXP -DGROUP_NAMES

##  these are architecture dependent flags
ifeq ($(OS),Windows_NT)
	# noop for now
else
	SHLIB_LINK += -L$(AUTOV8_OUT)
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Darwin)
		CFLAGS += -g -Werror -Wall
		LDFLAGS ?= -g -Werror -Wall
		CC = clang
	endif
	ifeq ($(UNAME_S),Linux)
		CFLAGS += -g -Wall -fPIC
		LDFLAGS ?= -g -Wall -lcrypt -fPIC
	endif
endif

LD=$(CC)

#TITANFLAGS = -g -43 -vector_c -safe=ptrs -DTITAN

##
##
##  For lower exp loss at lower levels -DNEWEXP
##
##  For level loss for really low exp -DLEVEL_LOSS
##
##  To make some optimizations, you need to add things to the line above.
##  to use hash tables for rooms  (More cpu, less storage), add
##  -DHASH
##
##  To get rid of tracking (lower cpu)
##  -DNOTRACK
##
##  To limit items above a specific rent cost
##  -DLIMITED_ITEMS
##
##  To force new players to be authorized
##  -DPLAYER_AUTH
##
##  To add some new debugging stuff (mostly limited to malloc_debug)
##  -DDEBUG
##  To add some commands for locking out specific hosts
##  -DSITELOCK
##
## To lock out oak.grove.iup.edu from boot time.
## -DLOCKGROVE
##
##  To save the world (objects and mobs) -DSAVE_WORLD
##
##  To modify some commands to make item duplication near impossible:
##  -DNODUPLICATES
##
##  To have each 58+ god restricted to one site, or set of sites
##  -DIMPL_SECURITY
##
##  To use BSD memory allocation/deallocation routines instead of the
##  native memory routines:
##  -DKLUDGE_MEM
##
##  If your system doesn't have built in functions of strdup() and
##  strstr(), then:
##  -DKLUDGE_STRING
##

HEADERFILES = structs.h utils.h comm.h interpreter.h db.h
##
OBJS = comm.o act.comm.o act.info.o act.move.o act.obj1.o act.obj2.o \
	act.off.o act.other.o act.social.o act.wizard.o handler.o \
	db.o interpreter.o utility.o spec_assign.o shop.o limits.o mobact.o \
	fight.o modify.o weather.o spells1.o spells2.o spell_parser.o \
	reception.o constants.o spec_procs.o signals.o board.o magic.o \
	magic2.o skills.o Opinion.o Trap.o magicutils.o multiclass.o hash.o \
	Sound.o Heap.o spec_procs2.o magic3.o security.o spec_procs3.o \
        create.o bsd.o parser.o intrinsics.o

SRCS = comm.c act.comm.c act.info.c act.move.c act.obj1.c act.obj2.c \
	act.off.c act.other.c act.social.c act.wizard.c handler.c \
	db.c interpreter.c utility.c spec_assign.c shop.c limits.c mobact.c \
	fight.c modify.c weather.c spells1.c spells2.c spell_parser.c \
	reception.c constants.c spec_procs.c signals.c board.c magic.c \
	magic2.c skills.c Opinion.c Trap.c magicutils.c multiclass.c hash.c \
	Sound.c Heap.c spec_procs2.c magic3.c security.c spec_procs3.c \
        create.c bsd.c parser.c intrinsics.c

all : dmserver

dmserver : ${OBJS}
## if you want to add debugging stuff, uncomment the following, and comment
##  out the line after that
##	$(LD) -o $@ -g ${OBJS} /local/lib/gnu/gcc-gnulib /usr/lib/debug/malloc.o
	$(LD) -o $@ -g ${OBJS} $(LDFLAGS)
##	$(LD) -o $@  ${OBJS}
##

depend: .depend

.depend: $(SRCS) $(TEST_SRCS)
	$(RM) -f ./.depend
	$(CC) $(CFLAGS) -MM $^>>./.depend

clean:
	$(RM) -f $(OBJS) dmserver

include .depend

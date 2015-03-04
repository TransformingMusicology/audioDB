HELP2MAN=help2man
GENGETOPT=gengetopt
GSL_INCLUDE=$(shell pkg-config --cflags gsl)
LIBGSL=$(shell pkg-config --libs gsl)
LIBAUDIODB=$(shell pkg-config --libs audioDB)

TESTDIRS=tests

PREFIX=/usr/local
EXEC_PREFIX=$(PREFIX)
BINDIR=$(EXEC_PREFIX)/bin
INCLUDEDIR=$(PREFIX)/include
MANDIR=$(PREFIX)/share/man

OBJS=index.o cmdline.o audioDB.o common.o

EXECUTABLE=audioDB

override CFLAGS+=-g -O3

# set to generate profile (gprof) and coverage (gcov) info
#override CFLAGS+=-fprofile-arcs -ftest-coverage -pg

# set to DUMP hashtables on QUERY load
#override CFLAGS+=-DLSH_DUMP_CORE_TABLES

ifeq ($(shell uname),Linux)
override CFLAGS+=-D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
endif

ifeq ($(shell uname),Darwin)
ifeq ($(shell sysctl -n hw.optional.x86_64),1)
override CFLAGS+=-arch x86_64
endif
endif

.PHONY: all clean test install

all: $(EXECUTABLE) $(EXECUTABLE).1

$(EXECUTABLE).1: $(EXECUTABLE)
	$(HELP2MAN) ./$(EXECUTABLE) > $(EXECUTABLE).1

HELP.txt: $(EXECUTABLE)
	./$(EXECUTABLE) --help > HELP.txt

cmdline.c cmdline.h: gengetopt.in
	$(GENGETOPT) -e <gengetopt.in

%.o: %.cpp audioDB.h audioDB_API.h cmdline.h reporter.h ReporterBase.h lshlib.h
	$(CXX) -c $(CFLAGS) $(GSL_INCLUDE) -Wall  $<

cmdline.o: cmdline.c cmdline.h
	$(CC) -c $(CFLAGS) $<

$(EXECUTABLE): $(OBJS)
	$(CXX) -o $(EXECUTABLE) $(CFLAGS) $^ $(LIBGSL) $(LIBAUDIODB)

tags:
	ctags *.cpp *.h

testclean: 
	-for d in $(TESTDIRS); do (cd $$d && sh ./clean.sh); done

clean: testclean
	-rm cmdline.c cmdline.h cmdline.o
	-rm adb.*
	-rm HELP.txt
	-rm $(EXECUTABLE) $(EXECUTABLE).1 $(OBJS)
	-rm xthresh
	-rm tags

distclean: clean
	-rm *.o
	-rm -rf audioDB.dump

test: $(EXECUTABLE)
	for d in $(TESTDIRS); do (cd $$d && sh ./run-tests.sh); done

xthresh: xthresh.c
	$(CC) -o $@ $(CFLAGS) $(GSL_INCLUDE) $(LIBGSL) $<

install: $(EXECUTABLE)
	mkdir -m755 -p $(BINDIR) $(MANDIR)/man1
	install -m755 $(EXECUTABLE) $(BINDIR)
	install -m644 $(EXECUTABLE).1 $(MANDIR)/man1

SRC=src
INCLUDE=include
BUILD_DIR=build
HELP2MAN=help2man
GENGETOPT=gengetopt
GSL_INCLUDE=$(shell pkg-config --cflags gsl)
LIBGSL=$(shell pkg-config --libs gsl)
ADB_INCLUDE=$(shell pkg-config --cflags audioDB)
LIBAUDIODB=$(shell pkg-config --libs audioDB)

TESTDIRS=tests

PREFIX=/usr/local
BINDIR=$(PREFIX)/bin
INCLUDEDIR=$(PREFIX)/include
MANDIR=$(PREFIX)/share/man

_OBJS=index.o audioDB.o common.o
OBJS=$(patsubst %,$(BUILD_DIR)/%,$(_OBJS))

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

all: $(BUILD_DIR) $(EXECUTABLE) $(EXECUTABLE).1

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(EXECUTABLE).1: $(EXECUTABLE)
	$(HELP2MAN) ./$(BUILD_DIR)/$(EXECUTABLE) > $(BUILD_DIR)/$(EXECUTABLE).1

$(BUILD_DIR)/HELP.txt: $(EXECUTABLE)
	./$(BUILD_DIR)/$(EXECUTABLE) --help > $(BUILD_DIR)/HELP.txt

$(SRC)/cmdline.c $(INCLUDE)/cmdline.h: gengetopt.in
	$(GENGETOPT) --header-output-dir=$(INCLUDE) --src-output-dir=$(SRC) -e <gengetopt.in

$(OBJS): $(BUILD_DIR)/%.o: $(SRC)/%.cpp $(INCLUDE)/audioDB.h $(INCLUDE)/reporter.h $(INCLUDE)/ReporterBase.h #$(ADB_INCLUDE)/audioDB/audioDB_API.h $(BUILD_DIR)/cmdline.h  $(ADB_INCLUDE)/audioDB/lshlib.h
	$(CXX) -o $@ -c $(CFLAGS) $(GSL_INCLUDE) $(ADB_INCLUDE) -I$(INCLUDE) -Wall $<

$(BUILD_DIR)/%.o: $(SRC)/%.cpp $(INCLUDE)/audioDB.h $(INCLUDE)/reporter.h $(INCLUDE)/ReporterBase.h #$(ADB_INCLUDE)/audioDB/audioDB_API.h $(BUILD_DIR)/cmdline.h $(ADB_INCLUDE)/audioDB/reporter.h $(ADB_INCLUDE)/audioDB/ReporterBase.h $(ADB_INCLUDE)/audioDB/lshlib.h
	$(CXX) -c $(CFLAGS) $(GSL_INCLUDE) $(ADB_INCLUDE) -I$(INCLUDE) -Wall  $<

$(BUILD_DIR)/cmdline.o: $(SRC)/cmdline.c $(INCLUDE)/cmdline.h
	$(CC) -o $(BUILD_DIR)/cmdline.o -c $(CFLAGS) $(ADB_INCLUDE) -I$(INCLUDE) $<

$(EXECUTABLE): $(BUILD_DIR)/cmdline.o $(OBJS)
	$(CXX) -o $(BUILD_DIR)/$(EXECUTABLE) $(CFLAGS) $^ $(LIBGSL) $(LIBAUDIODB)

tags:
	ctags $(SRC)/*.cpp $(INCLUDE)/*.h

testclean: 
	-for d in $(TESTDIRS); do (cd $$d && sh ./clean.sh); done

clean: testclean
	-rm $(SRC)/cmdline.c $(INCLUDE)/cmdline.h $(BUILD_DIR)/cmdline.o
	-rm adb.*
	-rm $(BUILD_DIR)/HELP.txt
	-rm $(BUILD_DIR)/$(EXECUTABLE) $(BUILD_DIR)/$(EXECUTABLE).1 $(OBJS)
	-rm $(BUILD_DIR)/xthresh
	-rm tags
	-rmdir build

distclean: clean
	-rm $(BUILD_DIR)/*.o
	-rm -rf audioDB.dump

test: $(EXECUTABLE)
	for d in $(TESTDIRS); do (cd $$d && sh ./run-tests.sh); done

xthresh: $(SRC)/xthresh.c
	$(CC) -o $(BUILD_DIR)/$@ $(CFLAGS) $(GSL_INCLUDE) $(ADB_INCLUDE) $(LIBGSL) $<

install: $(EXECUTABLE)
	mkdir -m755 -p $(BINDIR) $(MANDIR)/man1
	install -m755 $(BUILD_DIR)/$(EXECUTABLE) $(BINDIR)
	install -m644 $(BUILD_DIR)/$(EXECUTABLE).1 $(MANDIR)/man1

# get the target for the compiler
target = $(strip $(shell $(CC) -dumpmachine))

# DOS and Windows executables should have the .exe extension.
# Other operating systems should be extension-less.

CC ?= gcc

ifeq ($(findstring mingw32,$(target)),mingw32)
EXENAME = acc.exe
else
ifeq ($(findstring djgpp,$(target)),djgpp)
EXENAME = acc.exe
else
EXENAME = acc
endif
endif

CFLAGS ?= -O2 -Wall -W
LDFLAGS ?= -s
VERNUM = 154

OBJS = \
	acc.o     \
	error.o   \
	misc.o    \
	parse.o   \
	pcode.o   \
	strlist.o \
	symbol.o  \
	token.o

SRCS = \
	acc.cpp		\
	error.cpp	\
	misc.cpp	\
	parse.cpp	\
	pcode.cpp	\
	strlist.cpp	\
	symbol.cpp	\
	token.cpp	\
	common.h	\
	error.h		\
	misc.h		\
	parse.h		\
	pcode.h		\
	strlist.h	\
	symbol.h	\
	token.h		\
	Makefile	\
	acc.dsp		\
	acc.dsw

ACS = \
	Headers/zcommon.acs		\
	Headers/zdefs.acs		\
	Headers/zspecial.acs	\
	Headers/zprops.acs		\
	Headers/zstruct.acs		\
	Headers/ztypes.acs		\
	Headers/Actor.acs		\
	Headers/Tid.acs			\
	Headers/Fixed.acs

$(EXENAME) : $(OBJS)
	$(CC) $(OBJS) -o $(EXENAME) $(LDFLAGS)

acc.o: acc.cpp \
	common.h \
	error.h \
	misc.h \
	parse.h \
	pcode.h \
	strlist.h \
	symbol.h \
	token.h \
	

error.o: error.cpp \
	common.h \
	error.h \
	misc.h \
	token.h \
	

misc.o: misc.cpp \
	common.h \
	error.h \
	misc.h \
	

parse.o: parse.cpp \
	common.h \
	error.h \
	misc.h \
	parse.h \
	pcode.h \
	strlist.h \
	symbol.h \
	token.h \
	

pcode.o: pcode.cpp \
	common.h \
	error.h \
	misc.h \
	pcode.h \
	strlist.h \
	

strlist.o: strlist.cpp \
	common.h \
	error.h \
	misc.h \
	pcode.h \
	strlist.h \
	

symbol.o: symbol.cpp \
	common.h \
	error.h \
	misc.h \
	pcode.h \
	symbol.h \
	parse.h \
	

token.o: token.cpp \
	common.h \
	error.h \
	misc.h \
	pcode.h \
	symbol.h \
	token.h \
	parse.h \


clean:
	rm -f $(OBJS) $(EXENAME)

# These targets can only be made with MinGW's make and not DJGPP's, because
# they use Win32 tools.

zipsrc: $(SRCS)
	kzip /y acc$(VERNUM)-src.zip $(SRCS) "End User License ACC Source Code.doc"

zipbin: $(EXENAME) $(ACS)
	kzip /y acc$(VERNUM).zip $(EXENAME) $(ACS)

zipwbin: Release/acc.exe $(ACS)
	kzip /y acc$(VERNUM)win.zip Release/acc.exe $(ACS)

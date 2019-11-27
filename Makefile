.POSIX:

## This really shouldn't be overridden
PROJECT ?= nombre
## Invoke with -DDVCS=git to use the git functions instead
DVCS ?= fossil
## Set the suffixes to catch all .c and .o files
.SUFFIXES = .c .o

## Switch this to use whatever C99 compliant compiler you have/prefer
## there should not be any OS or compiler dependant code in this project
## NOTE: This is Clang 8.0.0 at the time of starting this project
CC = clang-devel
LD = ld.lld-devel
STD = c99

## Additional flags used in "debug" builds
.ifdef DEBUG
DBG = -g3 -ggdb -DBUILD_DEBUG
.else
DBG = 
.endif

## Flags used to build under GCC
## Generic flags
LIBS = -L/usr/local/lib -L/usr/lib -lsqlite3 #-lpthread -lc
INCS = -I/usr/local/include -I/usr/include
## List of *.c files to build
SRCS = nombre.c initdb.c dbverify.c parsecmd.c subnom.c
SRCDIR = src/

## TODO: Figure this out
.if ${CC:Mclang-devel} || ${CC:Mclang}
## Flags used to build under Clang/LLVM
## Not all flags exist or mean the same for all compilers
WARN = -Wextra -Wall -Wparentheses -Weverything -pedantic
LDFLAGS= --icf=safe -z relro  -z now  -z combreloc
CFLAGS = -std=${STD} -c -Oz -fpic -fpie -fPIC -fPIE \
							 -fvectorize -fstack-protector -fstrict-enums -fstrict-return -fstack-protector-strong \
							 -fmerge-all-constants -fstack-protector-all -Qn -fstrict-aliasing \
							 -ffunction-sections -fdata-sections
.elif (${CC:Mgcc})
## TODO: Fill out with GCC specific flags
WARN = -Wextra -pedantic -Wall
LDFLAGS= -z combreloc -z relro -z now -fpie -fpic --gc-sections
CFLAGS = -std=${STD} -Os -fpic -fpie -fPIC -fPIE -fstack-protector-all -fstack-protector-strong -fstrict-aliasing \
				 -ffunction-sections -fdata-sections
.else
## Restrict to the options that should be available nearly everywhere
CFLAGS = -Os -std=${STD} -fpic -fpie -fPIC -fPIE
LDFLAGS= -z relro -z now
.endif ## End of compiler checks

## Now ensure the compiler and linker arguments include the appropriate path info
CFLAGS += ${INCS}
#CFLAGS += ${LIBS}
LDFLAGS+= ${LIBS}

## Add the debug flags to the CFLAGS for the system
.ifdef DEBUG
CFLAGS += ${DBG}
.endif 

## These variables control where the binary actually gets installed
## The name of the binary
TARGET = nombre
## The binary gets installed to ${PREFIX}${DESTDIR}
PREFIX ?= "${HOME}"
DESTDIR = /bin

## Sticky bit, RWX for owner, RX for group
## leading 0 to ensure that printf(1) sees it as an octal number
DIRMODE = 01750 

## Read and execute for all
BINMODE = 0755

## Usable make targets
TARGETS = "debug install uninstall check run test"
DVCS_TARGETS = "commit push pull status"
CTRL_TARGETS = "config help clean purge"

## The "help" flag for the binary to run usage()
HELP = -h

help:
	@printf "Build system configuration for %s:\n" ${PROJECT}
	@printf "\tValid targets: %s\n" ${TARGETS}
	@printf "\tDVCS targets: %s\n" ${DVCS_TARGETS}
	@printf "\tControl targets: %s\n" ${CTRL_TARGETS}
	@printf "\nCompilation settings:\n"
	@printf "\tSources: %s\n" "${SRCS}"
	@printf "\tInstall to: %s\n" "${PREFIX}${DESTDIR}"
	@printf "\tDirectory Permissions: %04o\n" ${DIRMODE}
	@printf "\tPermissions: %04o\n" ${BINMODE}
	@printf "\tCompiler: %s\n" "${CC}"
	@printf "\tCFLAGS: %s\n" "${CFLAGS}"
	@printf "\tLDFLAGS: %s\n" "${LDFLAGS}"
	@printf "\nTo change these settings run \`make config\` or \`%s Makefile\`\n" "${EDITOR}"

## Currently only set to work with clang-devel
check: ${SRCS}
	@clang-tidy-devel -checks=* $?

debug: mkdest bin/nombre
	@echo "[${.TARGET}]: Working in ${.CURDIR}"
	(cd "${.CURDIR}/bin" && install -vm ${BINMODE} ${TARGET} ${PREFIX}${DESTDIR})
	${PREFIX}${DESTDIR}/${TARGET} ${HELP}

install: mkdest bin/nombre
	@echo "[${.TARGET}]: Working in ${.CURDIR}"
	@strip -s bin/${TARGET}
	@install -vm ${BINMODE} bin/${TARGET} ${PREFIX}${DESTDIR}
	${PREFIX}${DESTDIR}/${TARGET} ${HELP}
	@echo "Hit ^C in the next 5 seconds to prevent bootstrapping the database!"
	@sleep 5
	@${PREFIX}${DESTDIR}/${TARGET} -Ii nombre.sql

## Create the installation directory
mkdest: dirs
	@echo "[${.TARGET}]: Working in ${.CURDIR}"
	@mkdir -pm ${DIRMODE} ${PREFIX}${DESTDIR}

## Create an intermediate build location,
## including the test directory
dirs:
	@echo "[${.TARGET}]: Working in ${.CURDIR}"
	@mkdir -pm 1750 ${.CURDIR}/bin
	@mkdir -pm 1750 ${.CURDIR}/obj
	@mkdir -pm 1750 ${.CURDIR}/src
	@mkdir -pm 1750 ${.CURDIR}/test

## Build the object files
objects: ${.OBJDIR}/nombre.o ${.OBJDIR}/initdb.o ${.OBJDIR}/dbverify.o ${.OBJDIR}/parsecmd.o ${.OBJDIR}/subnom.o
	@echo "[${.TARGET}]: Built object files"

${.OBJDIR}/nombre.o: ${SRCDIR}nombre.c
	@echo "[${.TARGET}]: Compiling ${.OODATE}"
	@$(CC) ${INCS} ${CFLAGS} -o ${.TARGET} ${.ALLSRC}

${.OBJDIR}/initdb.o: ${SRCDIR}initdb.c
	@echo "[${.TARGET}]: Compiling ${.OODATE}"
	@$(CC) ${INCS} ${CFLAGS} -o ${.TARGET} ${.ALLSRC}

${.OBJDIR}/dbverify.o: ${SRCDIR}dbverify.c
	@echo "[${.TARGET}]: Compiling ${.OODATE}"
	@$(CC) ${INCS} ${CFLAGS} -o ${.TARGET} ${.ALLSRC}

${.OBJDIR}/parsecmd.o: ${SRCDIR}parsecmd.c
	@echo "[${.TARGET}]: Compiling ${.OODATE}"
	@$(CC) ${INCS} ${CFLAGS} -o ${.TARGET} ${.ALLSRC}

${.OBJDIR}/subnom.o: ${SRCDIR}subnom.c
	@echo "[${.TARGET}]: Compiling ${.OODATE}"
	@$(CC) ${INCS} ${CFLAGS} -o ${.TARGET} ${.ALLSRC}

## Link the object files
bin/nombre: objects obj/nombre.o obj/initdb.o obj/dbverify.o obj/parsecmd.o obj/subnom.o
	@echo "[${.TARGET}]: In ${.CURDIR}, Linking ${.ALLSRC:M*.o}"
	(cd ${.CURDIR} && ${LD} ${LIBS} ${LDFLAGS} -o ${.TARGET} ${.ALLSRC:M*.o})

push:
	@gitsync -r ${PROJECT} -n master

commit:
	@(cd ${.CURDIR} && ${DVCS} commit)

status:
	@echo "[${.TARGET}]: In ${.CURDIR}"
	@(cd ${.CURDIR} && ${DVCS} status)

diff:
	@(cd ${.CURDIR} && ${DVCS} diff)

config:
	@$(EDITOR) ${.CURDIR}/Makefile

uninstall:
	@echo "This target is not yet ready"

purge:
	@echo "This target is not yet ready"

clean:
	@echo "[${.TARGET}]: Cleaning up build objects..."
	@rm -vf ${.CURDIR}/obj/*
	@rm -vf ${.CURDIR}/bin/*

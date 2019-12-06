.POSIX:

include config.mk

## This really shouldn't be overridden
PROJECT = nombre
## Invoke with -DDVCS=git to use the git functions instead
DVCS ?= fossil
## Set the suffixes to catch all .c and .o files
.SUFFIXES = .c .o

## The standard used for the codebase
STD = c99

## List of *.c files to build
SRCS = nombre.c initdb.c dbverify.c parsecmd.c subnom.c
HEADERS = $(SRCS:.c=.h)
OBJ = $(SRCS:.c=.o)

### Restrict to the options that should be available nearly everywhere
#CFLAGS = -Os -std=${STD} -fpic -fpie -fPIC -fPIE
#LDFLAGS= -z relro -z now

## Now ensure the compiler and linker arguments include the appropriate path info
CFLAGS += ${INCS}
LDFLAGS+= ${LIBS} ${LINKTO}

## Add the debug flags, if unset will not change CFLAGS
CFLAGS += ${DBG}

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

## Build the object files
.c.o:
	$(CC) ${CFLAGS} -c $< -o ${<:.c=.o}

nombre.o: ${HEADERS}
initdb.o: nombre.h initdb.h
parsecmd.o: nombre.h parsecmd.h
subnom.o: nombre.h initdb.h parsecmd.h subnom.h
dbverify.o: nombre.h dbverify.h

$(PROJECT): $(OBJ)
	$(CC) -o $@ $? -fuse-ld=${LD} ${LDFLAGS}

help:
	@echo "Objects: $(OBJ)"
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
	@printf "\tLinker: %s\n" "${LD}"
	@printf "\tCFLAGS: %s\n" "${CFLAGS}"
	@printf "\tLDFLAGS: %s\n" "${LDFLAGS}"
	@printf "\nTo change these settings run \`make config\` or \`%s config.mk\`\n" "${EDITOR}"

## Currently only set to work with clang-devel
check: ${SRCS}
	@clang-tidy-devel -checks=* $?

build: mkdest nombre
	@echo "[${@}]: Working in ${PWD}"
	install -vm ${BINMODE} ${TARGET} ${PREFIX}${DESTDIR}
	${PREFIX}${DESTDIR}/${TARGET} ${HELP}

install: mkdest nombre
	@echo "[${@}]: Working in ${PWD}"
	@strip -s ${TARGET}
	@install -vm ${BINMODE} ${TARGET} ${PREFIX}${DESTDIR}
	${PREFIX}${DESTDIR}/${TARGET} ${HELP}
	@echo "Hit ^C in the next 5 seconds to prevent bootstrapping the database!"
	@sleep 5
	@${PREFIX}${DESTDIR}/${TARGET} -Ii nombre.sql

## Create the installation directory
mkdest: dirs
	@echo "[${@}]: Ensuring ${PREFIX}${DESTDIR} exists"
	@mkdir -pm ${DIRMODE} ${PREFIX}${DESTDIR}

## Create an intermediate build location,
## including the test directory
dirs:
	@echo "[${@}]: Working in ${PWD}"
	@mkdir -pm 1750 ${PWD}/test

push:
	@gitsync -r ${PROJECT} -n master

commit:
	@(cd ${PWD} && ${DVCS} commit)

status:
	@echo "[${@}]: In ${PWD}"
	@(cd ${PWD} && ${DVCS} status)

diff:
	@(cd ${PWD} && ${DVCS} diff)

config:
	@$(EDITOR) ${PWD}/config.mk

uninstall: ;
	@echo "[$@]: Deleting ${PREFIX}${DESTDIR}/${PROJECT}"
	@rm -f ${PREFIX}${DESTDIR}/${PROJECT}

purge:
	@echo "This target is not yet ready"

clean:
	@echo "[${@}]: Cleaning up build objects..."
	@rm -vf ${PWD}/obj/*
	@rm -vf ${PWD}/bin/*
	@rm -f ${PWD}/${PROJECT}

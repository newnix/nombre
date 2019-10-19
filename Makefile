.POSIX:

## This really shouldn't be overridden
PROJECT ?= nombre
## Invoke with -DDVCS=git to use the git functions instead
DVCS ?= fossil

## Switch this to use whatever C99 compliant compiler you have/prefer
## there should not be any OS or compiler dependant code in this project
## NOTE: This is Clang 8.0.0 at the time of starting this project
CC = clang-devel
STD = c99

## Additional flags used in "debug" builds
DBG = -ggdb -fsanitize-cfi-cross-dso

## Flags used to build under Clang/LLVM
## Not all flags exist or mean the same for all compilers
CLANG_WARN = -Wextra -Wall -Wparentheses -Weverything -pedantic
CLANG_LINK = -fuse-ld=lld-devel -Wl,-v,--gc-sections,--icf=all
CLANG_CFLAGS = -std=${STD} -Oz -fpic -fpie -fPIC -fPIE -z relro -z combreloc -z now -pipe \
							 -fvectorize -fstack-protector -fstrict-enums -fstrict-return -fstack-protector-strong \
							 -fmerge-all-constants -fstack-protector-all -Qn -fstrict-aliasing
CLANG_FLAGS = ${CLANG_CFLAGS} ${CLANG_WARN} ${CLANG_LINK}

## Flags used to build under GCC
## Generic flags
LIBS = -L/usr/local/lib -L/usr/lib -lsqlite3
INCS = -I/usr/local/include -I/usr/include
## List of *.c files to build
SRCS = nombre.c initdb.c dbverify.c parsecmd.c subnom.c

## These variables control where the binary actually gets installed
## The name of the binary
TARGET = nombre
## The binary gets installed to ${PREFIX}${DESTDIR}
PREFIX ?= "${HOME}"
DESTDIR = /bin

## Sticky bit, RWX for owner, RX for group
DIRMODE ?= 1750 

## Read and execute for all
BINMODE ?= 0555

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
	@printf "\tClang: %s\n" "${CLANG_FLAGS}"
	@printf "\tInstall to: %s\n" "${PREFIX}${DESTDIR}"
	@printf "\tCompiler: %s\n" "${CC}"
	@printf "\nTo change these settings run \`make config\` or \`%s Makefile\`\n" "${EDITOR}"

check: ${SRCS}
	@clang-tidy-devel -checks=* $?

debug: mkdest ${SRCS}
	@$(CC) -DBUILD_DEBUG ${LIBS} ${INCS} ${DBG} ${CLANG_FLAGS} ${SRCS} -o ${TARGET}
	@install -vm ${BINMODE} ${TARGET} ${PREFIX}${DESTDIR}
	${PREFIX}${DESTDIR}/${TARGET} ${HELP}

install: mkdest ${SRCS}
	@$(CC) ${CLANG_FLAGS} ${SRCS} -o ${TARGET}
	@strip -s ${TARGET}
	@install -vm ${BINMODE} ${TARGET} ${PREFIX}${DESTDIR}
	${PREFIX}${DESTDIR}/${TARGET} ${HELP}

## Create the installation directory
mkdest: 
	@mkdir -pm ${DIRMODE} ${PREFIX}${DESTDIR}

push:
	@gitsync -r ${PROJECT} -n master

commit:
	@${DVCS} commit

status: 
	@${DVCS} status

diff:
	@${DVCS} diff

config:
	@$(EDITOR) Makefile

uninstall:
	@echo "This target is not yet ready"

purge:
	@echo "This target is not yet ready"

clean:
	@echo "This target is not yet ready"

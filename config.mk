## Set up some variables for use in building nombre
## Used as a means of working around Makefile limitations and portability concerns

## Uncomment to force a debug build
#DBG = -g3 -NOMBRE_DEBUG

## Set the library and include paths
INCS = -I/usr/include -I/usr/local/include
LIBS = -L/usr/lib -L/usr/local/lib

## Library linkages
LINKTO = -lc -lsqlite3 -lpthread

## Define where the binary will be installed
## Default is ${HOME}/bin
PREFIX = ${HOME}
DESTDIR = /bin

## Comment out to not use Clang
include clang-opts.mk

## Uncomment to use GCC
#include gcc-opts.mk

## Uncomment for "failsafe" settings
#include failsafe.mk

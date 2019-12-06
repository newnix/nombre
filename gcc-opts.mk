## Compiler and Linker settings for using GCC
CC = gcc
LD = ld

WRN = -pedantic -Wno-alignment -Werror -Wall
CFLAGS = ${WRN} -std=${STD} -Os -fpic -fpie -pipe -fPIC -fPIE -fstack-protector-all -fstack-protector-strong \
				 -fstrict-aliasing -ffunction-sections -fdata-sections
LDFLAGS = -z combreloc -z now -z relro -fpie -fpic --icf=safe

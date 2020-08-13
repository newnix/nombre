## Compilation and Linking settings when using Clang/LLVM
CC = clang
LD = lld

WRN = -Wextra -Wall -Weverything -Wdeprecated -pedantic -Werror -Wno-padded -Wno-cast-qual -Wno-unused-command-line-argument\
			-Wno-disabled-macro-expansion
CFLAGS = ${WRN} ${MACRO_OVERRIDES} -std=${STD} -Oz -fpic -fpie -pipe -fPIC -fPIE -fvectorize -fstack-protector \
				 -fstrict-enums -fstrict-return -fstack-protector-strong -fmerge-all-constants -fstack-protector-all \
				 -Qn -fstrict-aliasing -ffunction-sections -fdata-sections -fuse-ld=${LD}
LDFLAGS = -z relro -z now -z combreloc #--icf=safe

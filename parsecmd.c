/*
 * Copyright (c) 2019, Exile Heavy Industries
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the disclaimer
 * below) provided that the following conditions are met:
 * 
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 * 
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * 
 * * Neither the name of the copyright holder nor the names of its contributors may be used
 *   to endorse or promote products derived from this software without specific
 *   prior written permission.
 * 
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS
 * LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef NOMBRE_PARSECMD_H
#include "parsecmd.h"
#endif
#ifndef NOMBRE_H
#include "nombre.h"
#endif

extern char *__progname;
extern char **environ;
extern bool dbg;

int
parsecmd(nomcmd * restrict cmdbuf, const char * restrict arg) {
	int retc;
	retc = 0;

	/* Define a list of valid command strings */
	const char *cmdstrs[] = { "def", "add", "key", "ver", "imp", "exp", "src", "dmp", "upd", "vqy", "cts", "grp" }; /* "Short" */
	const char *cmdstr_long[] = { "define", "adddef", "keyword", "verify", "import", "export", "srcadd", "dumpdb", "update", "vquery", "catscn", "grpcmd" }; /* "Long" */

	if (dbg) {
		NOMDBG("Entering with cmdbuf = %p, arg = %p\n", (const void *)cmdbuf, (const void *)arg);
	}

	/* A NULL argument should not be possible */
	if (arg == NULL || cmdbuf == NULL) {
		NOMERR("%s\n", "Given invalid arguments!");
		retc = BADARGS;
	}

	/* 
	 * Since we can be sure that we don't have a NULL pointer, check the string size 
	 * iterator value is never more than 11, so use whatever the fastest option is for an 8bit value
	 * Loops are capped at valid commands aside from the group value, as that requires extra logic,
	 * the group commands are assumed to be among the least frequent, and as such will only be checked 
	 * after all single-value commands are exhausted.
	 */
	if (strnlen(arg, (size_t)6) <= 3) {
		for (register int_fast8_t i = 0; ((i < (CMDCOUNT - 1)) && (retc != 0)) ; i++) {
			retc = memcmp(arg, cmdstrs[i], 3);
			cmdbuf->command = (retc == 0) ? (0x01 << i) : 0;
		}
		if (memcmp(arg, cmdstrs[CMDCOUNT], 3) == 0) {
			cmdbuf->command |= grpcmd;
			retc = grpcmd;
		}
	} else {
		for (register int_fast8_t i = 0; ((i < (CMDCOUNT - 1)) && (retc == 0)); i++) {
			retc = memcmp(arg, cmdstr_long[i], 6);
			cmdbuf->command = (retc == 0) ? (0x01 << i) : 0;
		}
		if (memcmp(arg, cmdstr_long[CMDCOUNT], 6) == 0) {
			cmdbuf->command |= grpcmd;
			retc = grpcmd;
		}
	}

	if (dbg) {
		NOMDBG("Returning %d to caller with cmdbuf->command = %d\n", retc, cmdbuf->command);
	}
	return(retc);
}

int
nombre_lookup(nomcmd * restrict cmdbuf, const char ** restrict args) {
	int retc;
	retc = 0;

	if (dbg) {
		NOMDBG("Entering with cmdbuf = %p, args = %p\n", (void *)cmdbuf, (const void *)args);
	}
	if ((cmdbuf == NULL) || (args == NULL)) {
		NOMERR("%s\n", "Invalid Arguments!");
		retc = BADARGS;
	}
	if (dbg) {
		NOMDBG("Returning %d to caller\n", retc);
	}
	return(retc);
}

int
nombre_newdef(nomcmd * restrict cmdbuf, const char ** restrict args) {
	int retc;
	retc = 0;

	if ((cmdbuf == NULL) || (args == NULL)) {
		NOMERR("%s\n", "Invalid Arguments!");
		retc = BADARGS;
	}
	return(retc);
}

int
nombre_addsrc(nomcmd * restrict cmdbuf, const char ** restrict args) {
	int retc;
	retc = 0;

	if ((cmdbuf == NULL) || (args == NULL)) {
		NOMERR("%s\n", "Invalid Arguments!");
		retc = BADARGS;
	}
	return(retc);
}

int
nombre_vquery(nomcmd * restrict cmdbuf, const char ** restrict args) {
	int retc;
	retc = 0;

	if ((cmdbuf == NULL) || (args == NULL)) {
		NOMERR("%s\n", "Invalid Arguments!");
		retc = BADARGS;
	}
	return(retc);
}

int
nombre_ksearch(nomcmd * restrict cmdbuf, const char ** restrict args) {
	int retc;
	retc = 0;

	if ((cmdbuf == NULL) || (args == NULL)) {
		NOMERR("%s\n", "Invalid Arguments!");
		retc = BADARGS;
	}
	return(retc);
}

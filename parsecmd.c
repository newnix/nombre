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

static inline bool isgrp(const nomcmd * restrict cmd);
static inline void upcase(char * restrict str);

int
parsecmd(nomcmd * restrict cmdbuf, const char * restrict arg) {
	int retc;
	size_t arglen;
	retc = -1;
	arglen = 0;

	/* Define a list of valid command strings */
	const char *cmdstrs[] = { "def", "add", "key", "ver", "imp", "exp", "src", "dmp", "upd", "vqy", "cts", "grp" }; /* "Short" */
	const char *cmdstr_long[] = { "define", "adddef", "keyword", "verify", "import", "export", "srcadd", "dumpdb", "update", "vquery", "catscn", "grpcmd" }; /* "Long" */

	if (dbg) {
		NOMDBG("Entering with cmdbuf = %p, arg = %p, strnlen(%s) = %lu\n", (const void *)cmdbuf, (const void *)arg, arg, strnlen(arg,(size_t)6));
	}

	/* A NULL argument should not be possible */
	if (arg == NULL || cmdbuf == NULL) {
		NOMERR("%s\n", "Given invalid arguments!");
		retc = BADARGS;
	}
	arglen = strnlen(arg, (size_t)3);

	/* 
	 * Since we can be sure that we don't have a NULL pointer, check the string size 
	 * iterator value is never more than 11, so use whatever the fastest option is for an 8bit value
	 * Loops are capped at valid commands aside from the group value, as that requires extra logic,
	 * the group commands are assumed to be among the least frequent, and as such will only be checked 
	 * after all single-value commands are exhausted.
	 * XXX: These loops can probably be collapsed, but doing so is not a priority at this time
	 */
	if (arglen == 3) {
		for (register int_fast8_t i = 0; ((i < (CMDCOUNT - 1)) && (retc != 0)) ; i++) {
			retc = memcmp(arg, cmdstrs[i], arglen);
			if (dbg) { 
				NOMDBG("i = %d, retc = %d, cmdbuf->command = %u, (0x01 << %d) = %X (%u)\n", i, retc, cmdbuf->command, i, (0x01 << i), (0x01 << i)); 
			}
			if (retc == 0) {
				cmdbuf->command |= (0x01 << i);
			}
		}
		if (memcmp(arg, cmdstrs[CMDCOUNT], arglen) == 0) {
			if (dbg) {
				NOMDBG("Detected group command with arg = %s\n", arg);
			}
			cmdbuf->command |= grpcmd;
			retc = grpcmd;
		}
	} else {
		for (register int_fast8_t i = 0; ((i < (CMDCOUNT - 1)) && (retc != 0)); i++) {
			retc = memcmp(arg, cmdstr_long[i], arglen);
			if (dbg) { 
				NOMDBG("i = %d, retc = %d, cmdbuf->command = %u, (0x01 << %d) = %X (%u)\n", i, retc, cmdbuf->command, i, (0x01 << i), (0x01 << i)); 
			}
			if (retc == 0) {
				cmdbuf->command |= (0x01 << i);
			}
		}
		if (memcmp(arg, cmdstr_long[CMDCOUNT], arglen) == 0) {
			if (dbg) {
				NOMDBG("Detected group command with arg = %s\n", arg);
			}
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

	if (isgrp(cmdbuf)) {
		/* Use group logic */
		strlcpy(cmdbuf->defdata[NOMBRE_DBCATG], *args, (size_t)DEFLEN); args++;
		strlcpy(cmdbuf->defdata[NOMBRE_DBTERM], *args, (size_t)DEFLEN); args++;
		retc = snprintf(cmdbuf->gensql, (size_t)PATHMAX, "SELECT meaning FROM definitions WHERE term LIKE(\'%s\') AND category=(SELECT id FROM categories WHERE name LIKE(\'%s\'));",
				cmdbuf->defdata[NOMBRE_DBTERM], cmdbuf->defdata[NOMBRE_DBCATG]);
	} else {
		/* Expected to be normal path */
		strlcpy(cmdbuf->defdata[NOMBRE_DBTERM], *args, (size_t)DEFLEN); args++;
		retc = snprintf(cmdbuf->gensql, (size_t)PATHMAX, "SELECT meaning FROM definitions WHERE term LIKE(\'%s\');",
				cmdbuf->defdata[NOMBRE_DBTERM]);
	}
	if (retc > 0) {
		retc ^= retc;
	}
	if (dbg) {
		NOMDBG("Returning %d to caller\n", retc);
	}
	return(retc);
}

int
nombre_newdef(nomcmd * restrict cmdbuf, const char ** restrict args) {
	int retc;
	char defstr[DEFLEN];
	retc = 0;

	if (dbg) {
		NOMDBG("Entering with cmdbuf = %p, args = %p\n", (void *)cmdbuf, (const void *)*args);
	}
	if ((cmdbuf == NULL) || (args == NULL)) {
		NOMERR("%s\n", "Invalid Arguments!");
		retc = BADARGS;
	} else {
		/* Explicitly zero local character array */
		memset(defstr, 0, (size_t)DEFLEN);
	}
	/* Return status truncation OK due to length limitations */
	retc = (int)strlcpy(cmdbuf->defdata[NOMBRE_DBTERM], *args, (size_t)DEFLEN); args++;
	upcase(cmdbuf->defdata[NOMBRE_DBTERM]);

	if (isgrp(cmdbuf)) {
		strlcpy(cmdbuf->defdata[NOMBRE_DBCATG], *args, (size_t)DEFLEN); args++;
		/* Flatten the rest of the argument vector */
		for (register int written = 0; *args != NULL && retc > 0; args++) {
			retc = snprintf(&defstr[written -1], (size_t)(DEFLEN - written), (written > 0) ? " %s" : "%s", *args);
			written += retc;
		}
		if (dbg) {
			NOMDBG("Flattened arguments to \"%s\"\n", defstr);
		}
		retc = snprintf(cmdbuf->gensql, (size_t)PATHMAX, 
				"INSERT INTO definitions VALUES (\'%s\', \'%s\', (SELECT id FROM categories WHERE name LIKE(\'%s\')));",
				cmdbuf->defdata[NOMBRE_DBTERM], cmdbuf->defdata[NOMBRE_DBCATG], defstr);
	} else {
		for (register int written = 0; *args != NULL && retc > 0; args++) {
			if (dbg) {
				NOMDBG("written = %d, *args = %s, defstr = %s\n", written, *args, defstr);
			}
			retc = snprintf(&defstr[written], (size_t)(DEFLEN - written), (written > 0) ? " %s" : "%s", *args);
			written += retc;
			//written = (written > 0) ? written - 1 : written;
		}
		if (dbg) {
			NOMDBG("Flattened arguments to \"%s\"\n", defstr);
		}
		retc = snprintf(cmdbuf->gensql, (size_t)PATHMAX, 
				"INSERT INTO definitions VALUES (\'%s\', \'%s\', \'-1\');",
				cmdbuf->defdata[NOMBRE_DBTERM], defstr);
	}
	retc = (retc > 0) ? 0 : retc;
	if (dbg) {
		NOMDBG("Returing %d to caller with gensql = %s\n", retc, cmdbuf->gensql);
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

/* 
 * This function is never called except after validating that we have a 
 * non-NULL pointer to the command structure
 */
static inline bool
isgrp(const nomcmd * restrict cmd) {
	return(((cmd->command & grpcmd) == grpcmd) ? true : false);
}

/* 
 * Simple in-place modification of a string 
 * capitalize all letters detected
 * Simply unset the 6th bit if we find a letter.
 * NOTE: This method only works for ASCII text, will need 
 * to be refactored to support UTF-8 in the future
 */
static inline void
upcase(char * restrict str) {
	register uint_fast16_t i = 0;
	for (; str[i] != 0; i++) {
		str[i] = (str[i] >= 'a' && str[i] <= 'z') ? str[i] ^ 0x20 : str[i];
	}
}

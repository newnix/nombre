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


#include <err.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#ifndef NOMBRE_H
#include "nombre.h"
#endif
#ifndef NOMBRE_PARSECMD_H
#include "parsecmd.h"
#endif
#ifndef NOMBRE_SUBNOM_H
#include "subnom.h"
#endif

extern char *__progname;
extern char **environ;
extern bool dbg;

/* 
 * This function handles the handoff to other functions as needed to build the appropriate SQL 
 * statements to do what the user asked of us. As a manner of convention, the 
 * group directive must be present before any other commands.
 */
int
buildcmd(nomcmd * restrict cmdbuf, const char ** restrict argstr) {
	int retc;
	uint_fast32_t andmask;
	andmask = retc = 0;

	if ((cmdbuf == NULL) || (argstr == NULL)) {
		retc = BADARGS;
	} else {
		/* Since we can't be sure we have a valid  database connection at this time, open one */
		if (cmdbuf->dbcon == NULL) {
			/* Likely use the functions in initdb.h to connect */
		}
	}

	retc = parsecmd(cmdbuf, *argstr);
	++argstr;
	if (cmdbuf->command == grpcmd) {
		retc = parsecmd(cmdbuf, *argstr);
	}
	/* 
	 * Set our andmask to unset the 30th bit 
	 * called functions will be able to check for this bit at entry
	 */
	if ((cmdbuf->command & grpcmd) == grpcmd) {
		andmask = (uint_fast32_t)(~grpcmd);
	} else {
		andmask = (uint32_t)(~0); /* Should be INT_MAX */
	}
	/* Now that we know we have a good database connection, determine what we need to do next */
	switch (cmdbuf->command & andmask) {
		case (lookup):
			break;
		case (define):
			break;
		case (search):
			break;
		case (verify):
			break;
		case (import):
			break;
		case (export):
			break;
		case (dumpdb):
			break;
		case (addsrc):
			break;
		case (update):
			break;
		case (vquery):
			break;
		case (catscn):
			break;
		/* We hit an unexpected value */
		default:
			break;
	}
	return(retc);
}

int
runcmd(nomcmd * restrict cmdbuf) {
	int retc;
	retc = 0;
	if (dbg) {
		NOMDBG("Entering with cmdbuf = %p\n", (void *)cmdbuf);
	}
	if (cmdbuf == NULL) {
		NOMERR("%s", "Given invalid input!\n");
		retc = BADARGS;
	}
	if (dbg) {
		NOMDBG("Returning %d to caller\n", retc);
	}
	return(retc);
}

int
nomdb_dump(const nomcmd * restrict cmdbuf) {
	int retc;
	retc = 0;
	if (dbg) {
		NOMDBG("Entering with cmdbuf = %p\n", (void *)cmdbuf);
	}
	if (cmdbuf == NULL) {
		NOMERR("%s", "Given invalid input!\n");
		retc = BADARGS;
	}
	if (dbg) {
		NOMDBG("Returning %d to caller\n", retc);
	}
	return(retc);
}

int
nomdb_impt(nomcmd * restrict cmdbuf) {
	int retc;
	retc = 0;
	if (dbg) {
		NOMDBG("Entering with cmdbuf = %p\n", (void *)cmdbuf);
	}
	if (cmdbuf == NULL) {
		NOMERR("%s", "Given invalid input!\n");
		retc = BADARGS;
	}
	if (dbg) {
		NOMDBG("Returning %d to caller\n", retc);
	}
	return(retc);
}

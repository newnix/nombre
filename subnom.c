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
#ifndef NOMBRE_INITDB_H
#include "initdb.h"
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
	uint32_t andmask;
	retc = 0;
	andmask = (unsigned int)(~grpcmd);

	if (dbg) {
		NOMDBG("Entering with cmdbuf = %p, argstr = %p, andmask = %X\n", (void *)cmdbuf, (const void *)argstr, andmask);
	}
	if ((cmdbuf == NULL) || (argstr == NULL)) {
		retc = BADARGS;
	} else {
		/* Since we can't be sure we have a valid  database connection at this time, open one */
		if (cmdbuf->dbcon == NULL) {
			/* Likely use the functions in initdb.h to connect */
			if ((retc = nom_getdbn(cmdbuf->filedata[NOMBRE_DBFILE])) == NOM_OK) {
				retc = nom_dbconn(cmdbuf);
			}
		}
	}

	retc = parsecmd(cmdbuf, *argstr++);
	if (cmdbuf->command == grpcmd) {
		retc = parsecmd(cmdbuf, *argstr);
		/* Only increment again if there's a good subcommand given */
		if (retc == NOM_OK) {
			argstr++;
		}
	}
	/* 
	 * Set our andmask to unset the 30th bit 
	 * called functions will be able to check for this bit at entry
	 */

	if (dbg) {
		NOMDBG("cmdbuf->command = %X (%u), andmask = %X (%u), (cmdbuf->command & andmask) = %X (%u)\n", 
				cmdbuf->command, cmdbuf->command, andmask, andmask, (cmdbuf->command & andmask), (cmdbuf->command & andmask));
	}
	/* Now that we know we have a good database connection, determine what we need to do next */
	switch (cmdbuf->command & andmask) {
		case (lookup):
			retc = nombre_lookup(cmdbuf, argstr);
			break;
		/*
		 * This may be a bit deceptively named, but I'm sticking with it for now. 
		 * It's meant to be interpreted in the sense of the user defining a term, not 
		 * the user looking for a term's definition
		 */
		case (define):
			retc = nombre_newdef(cmdbuf, argstr);
			break;
		case (search):
			retc = nombre_ksearch(cmdbuf, argstr);
			break;
		case (verify):
			break;
		case (import):
			break;
		case (export):
			break;
		case (dumpdb):
			retc = nombre_dbdump(cmdbuf);
			break;
		case (addsrc):
			break;
		case (update):
			break;
		case (vquery):
			break;
		case (catscn):
			break;
		/* 
		 * Assume the user just didn't type "def" 
		 * Push the pointer back to the first argument in the string
		 */
		default:
			retc = nombre_lookup(cmdbuf, --argstr);
			break;
	}
	if (retc == 0) {
		retc = runcmd(cmdbuf, (int)strlen(cmdbuf->gensql));
	}
	if (dbg) {
		NOMDBG("Returning %d to caller\n", retc);
	}
	return(retc);
}

int
runcmd(nomcmd * restrict cmdbuf, int genlen) {
	int retc;
	const char *sqltail; 
	sqlite3_stmt *stmt;
	retc = 0;
	sqltail = NULL; stmt = NULL;

	if (dbg) {
		NOMDBG("Entering with cmdbuf = %p, genlen = %d, cmdbuf->gensql = %p\n", (void *)cmdbuf, genlen, (void *)cmdbuf->gensql);
	}
	if (cmdbuf == NULL || cmdbuf->gensql == NULL) {
		NOMERR("%s", "Given invalid input!\n");
		retc = BADARGS;
	}
	if (dbg) {
		NOMDBG("Attempting to run generated SQL = %s\n", cmdbuf->gensql);
	}

	if ((retc = sqlite3_prepare_v2(cmdbuf->dbcon, cmdbuf->gensql, genlen, &stmt, &sqltail)) != SQLITE_OK) {
		NOMERR("Error compiling SQL (%s)!\n", sqlite3_errstr(sqlite3_errcode(cmdbuf->dbcon)));
	}
	/* 
	 * Determine how to best proceed with processing the statement based on
	 * the value of cmdbuf->command
	 */
	switch (cmdbuf->command & (unsigned int)(~grpcmd)) {
		case (lookup):
			retc = sqlite3_step(stmt);
			if (retc == SQLITE_DONE) {
				fprintf(stdout,"%s: unknown\n", cmdbuf->defdata[NOMBRE_DBTERM]);
				return(retc ^= retc);
			}
			for (register uint_fast16_t i = 1; retc == SQLITE_ROW; i++, retc = sqlite3_step(stmt)) {
				if (i > 1) {
					fprintf(stdout,"  #%d: %s\n",  i, sqlite3_column_text(stmt,0));
				} else {
					fprintf(stdout,"%s: %s\n", cmdbuf->defdata[NOMBRE_DBTERM], sqlite3_column_text(stmt,0));
				}
			}
			retc ^= retc;
			break;
		case (define):
			retc = sqlite3_step(stmt);
			if (retc == SQLITE_DONE) {
				sqlite3_finalize(stmt);
				if ((cmdbuf->command & grpcmd) == grpcmd) {
					fprintf(stdout,"Added definition for %s/%s\n",cmdbuf->defdata[NOMBRE_DBCATG], cmdbuf->defdata[NOMBRE_DBTERM]);
				} else {
					fprintf(stdout,"Added definition for %s\n", cmdbuf->defdata[NOMBRE_DBTERM]);
				}
				retc ^= retc;
			}
			break;
		case (dumpdb):
			fprintf(stdout,"Here's what I know:\n");
			retc = sqlite3_step(stmt);
			for (register uint_fast16_t i = 1; retc == SQLITE_ROW; i++, retc = sqlite3_step(stmt)) {
				fprintf(stdout,"  (%s/%s): %s\n", sqlite3_column_text(stmt,0), sqlite3_column_text(stmt,1), sqlite3_column_text(stmt,2));
			}
			if (retc != SQLITE_DONE) {
				NOMERR("Error processing command! (%s)\n", sqlite3_errstr(sqlite3_errcode(cmdbuf->dbcon)));
			} else {
				retc ^= retc;
			}
			sqlite3_finalize(stmt);
			break;
		case (search):
			fprintf(stdout,"Found the following matches:\n");
			retc = sqlite3_step(stmt);
			for (; retc == SQLITE_ROW; retc = sqlite3_step(stmt)) {
				fprintf(stdout,"  %s: %s\n", sqlite3_column_text(stmt,0), sqlite3_column_text(stmt,1));
			}
			if (retc != SQLITE_DONE) {
				NOMERR("Error processing command! (%s)\n", sqlite3_errstr(sqlite3_errcode(cmdbuf->dbcon)));
			} else {
				retc ^= retc;
			}
			sqlite3_finalize(stmt);
			break;
		default:
			/* Should not be reachable */
			NOMERR("%s\n", "It should not be possible to reach this code!");
			sqlite3_finalize(stmt);
			return(retc);
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

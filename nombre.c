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
/* Needed for uint8_t and such */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#ifdef LINUX
#include <bsd/string.h>
#endif /* end LINUX */
#include <unistd.h>

/* 
 * Use header guards to only pull in information if not already defined 
 * this inversion on convention will prevent repeated NAMEI calls and reading files
 * where possible.
 */
#ifndef NOMBRE_H
#include "nombre.h"
#endif
#ifndef NOMBRE_INITDB_H
#include "initdb.h"
#endif
#ifndef NOMBRE_SUBNOM_H
#include "subnom.h"
#endif
#ifndef NOMBRE_PARSECMD_H
#include "parsecmd.h"
#endif
/* Define mneonics for the flag values */
#define HELPME 0x01
#define DBINIT 0x02
#define INTSQL 0x04
#define DBTEST 0x08

/* Declare extern/global vars */
extern char *__progname;
extern char **environ;
extern bool dbg;

/* Hopefully this is correct */
int cook(uint8_t * restrict flags, nomcmd * restrict cmdbuf, const char ** restrict argstr);
inline static void usage(void);

#ifdef BUILD_DEBUG
bool dbg = true;
#else 
bool dbg = false;
#endif 
/* 
 * The layout for uint8_t flags is as follows:
 * 0 0 0 0 0 0 0 0
 * ===============
 * | | | | | | | \- Help
 * | | | | | | \- Reserved
 * | | | | | \- Initialize database
 * | | | | \- Reserved
 * | | | \- Initialization SQL
 * | | \- Self-test
 * | \- Reserved
 * \- Reserved
 */

int
main(int ac, char **av) {
	int retc, ch;
	uint8_t flags;
	/* Ensure all pointer members are initialized as NULL */
	nomcmd cmd = { .dbcon = NULL };
	ch = retc = 0;
	flags = 0;

	/* Initialize the SQLite3 library */
	sqlite3_initialize();
	opterr ^= opterr;
	while ((ch = getopt(ac, av, "d:i:f:vDIh")) != -1) {
		switch (ch) {
			case 'h':
				flags |= HELPME;
				break;
			case 'd':
				strlcpy(cmd.filedata[0], optarg, (size_t)PATHMAX);
				break;
			case 'i':
				flags |= INTSQL;
				strlcpy(cmd.filedata[1], optarg, (size_t)PATHMAX);
				break;
			case 'f':
				strlcpy(cmd.filedata[2], optarg, (size_t)PATHMAX);
				break;
			case 'v':
				flags |= DBTEST;
				break;
			case 'I':
				flags |= DBINIT;
				break;
			case 'D':
				dbg = true;
				break;

			/* This may need to be redone later */
			case '?':
				BADFLAG(av[(optind - 1)]);
				break;
			default:
				break;
		}
	}

	/* Update the argument counter and vector pointer to the end of processed arguments */
	ac -= optind;
	av += optind;

	retc = cook(&flags, &cmd, (const char **)av);
	if (cmd.dbcon != NULL) {
		sqlite3_close_v2(cmd.dbcon);
	}
	/* All SQLite3 objects should be deallocated before this point */
	sqlite3_shutdown();
	return(retc);
}

inline static void 
usage(void) {
	fprintf(stdout,"%s: A simple, local definition database\n", __progname);
	fprintf(stdout,"\t%s [-DIv] -d database -i initfile -f I/O file [subcommand] term...\n"
			"\t  -D Enable run-time debug printouts\n"
			"\t  -I Initialize the database\n"
			"\t  -v Perform a verification test on the database\n"
			"\t  -i Initialization SQL script to use (only useful with -I)\n"
			"\t  -d The location of the nombre database (default: %s%s%s)\n"
			"\t  -f Use the given file for import/export operations\n\n"
			"Subcommands:\n"
			"\t(def)ine: Look up a definition\n"
			"\t(add)def: Add a new definition to the database\n"
			"\t(key)word: Perform a keyword search on saved entries\n"
			,__progname, "~", NOMBRE_DB_DIRECT, NOMBRE_DB_NAME);

	return; /* Gracefully return to caller */
}

/*
 * No real processing of **argstr is done in this function, it's only 
 * available here to be passed into the relevant functions further down the stack
 */
int
cook(uint8_t * restrict flags, nomcmd * restrict cmdbuf, const char ** restrict argstr) {
	int retc;
	retc = 0;

	if (dbg) {
		NOMDBG("Current flag setting: %u\n", *flags);
	}

	/* Test the status of the bits in *flags */
	if ((*flags & HELPME) == HELPME) {
		usage();
		return(retc);
	} else {
		switch (*flags & (uint8_t)(0x8F)) {
			/*
			 * Initialize the database, but use the default construction method or environmental variable
			 */
			case (DBINIT|INTSQL):
				if ((retc = nom_getdbn(cmdbuf->filedata[0])) == NOM_OK) {
					retc = nom_initdb(cmdbuf->filedata[0], cmdbuf->filedata[1], cmdbuf);
				} else {
					NOMERR("%s\n", "Failed to get database name!\n");
				}
				break;
			/*
			 * No behaviour changing flags passed, default behaviour
			 */
			default: 
				retc = buildcmd(cmdbuf, argstr);
				break;
		}
	}

	if (dbg){
		NOMDBG("Returning %d to caller\n", retc);
	}
	return(retc);
}

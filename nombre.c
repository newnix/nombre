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
#ifndef NOMBRE_PARSECMD_H
#include "parsecmd.h"
#endif
/* Define mneonics for the flag values */
#define HELPME 0x01
#define DBFILE 0x02
#define DBINIT 0x04
#define IOFILE 0x08
#define INTSQL 0x10
#define DBTEST 0x20
#define RUNDBG 0x40

/* Some return values for simple things */
#define INITOK_CUSTOM 0x01
#define INITOK_DEFAULT 0x02

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
 * | | | | | | \- Database file
 * | | | | | \- Initialize database
 * | | | | \- Import/Export file
 * | | | \- Initialization SQL
 * | | \- Self-test
 * | \- "Debug" 
 * \- Reserved
 */

int
main(int ac, char **av) {
	int retc, ch;
	uint8_t flags;
	nomcmd cmd;
	ch = retc = 0;
	flags = 0;

	while ((ch = getopt(ac, av, "d:i:f:vDIh")) != -1) {
		switch (ch) {
			case 'h':
				flags |= HELPME;
				break;
			case 'd':
				flags |= DBFILE;
				strlcpy(cmd.filedata[0], optarg, (size_t)PATHMAX);
				break;
			case 'i':
				flags |= INTSQL;
				strlcpy(cmd.filedata[1], optarg, (size_t)PATHMAX);
				break;
			case 'f':
				flags |= IOFILE;
				strlcpy(cmd.filedata[2], optarg, (size_t)PATHMAX);
				break;
			case 'v':
				flags |= DBTEST;
				break;
			case 'I':
				flags |= DBINIT;
				break;
			case 'D':
				flags |= RUNDBG;
				dbg = true;
				break;

			/* This may need to be redone later */
			case '?':
				BADFLAG(ch);
				break;
			default:
				break;
		}
	}

	/* Update the argument counter and vector pointer to the end of processed arguments */
	ac -= optind;
	av += optind;

	retc = cook(&flags, &cmd, (const char **)av);

	/* XXX: Move this into cook() */
	switch (retc) {
		case INITOK_CUSTOM:
			return(nom_initdb(cmd.filedata[0], cmd.filedata[1]));
		case INITOK_DEFAULT:
			if ((retc = nom_getdbn(cmd.filedata[0])) == 0) {
				retc = nom_initdb(cmd.filedata[0], cmd.filedata[1]);
				return(retc);
			} else {
				return(retc);
			}
		default:
			return(retc);
	}
}

inline static void 
usage(void) {
	fprintf(stdout,"%s: A simple, local definition database\n", __progname);
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

	/* Test the status of the bits in *flags */
	if (((*flags & HELPME) == HELPME) || (*flags == HELPME)) {
		usage();
		return(retc);
	} else {
		if (dbg) {
			fprintf(stderr, "DBG: %s [%s:%u] %s: Current flag setting: %u\n", __progname, __FILE__, __LINE__, __func__, *flags);
		}
		switch (*flags & (0x8FFF)) {
			case (DBFILE|DBINIT|INTSQL):
				return(INITOK_CUSTOM); /* We basically discard the argstr and bootstrap the database as defined in the files */
				break;
			default: /* Default case is to assume we're performing some action via subcommands */
				retc = parsecmd(cmdbuf, argstr);
				break;
		}
	}
	return(retc);
}

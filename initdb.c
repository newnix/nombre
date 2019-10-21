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

#ifndef NOMBRE_H
#include "nombre.h"
#endif
#ifndef NOMBRE_INITDB_H
#include "initdb.h"
#endif

extern char *__progname;
extern char **environ;
extern bool dbg;

int
nom_getdbn(char * restrict dbnamebuf) {
	int retc;
	char *dbprefix;
	dbprefix = NULL;
	retc = 0;

	/* It should not be possible to call this function with a NULL dbnamebuf, but JIC... */
	if (dbnamebuf != NULL && *dbnamebuf == 0) {
		/* No database name was written, check environment */
		if ((dbprefix = getenv(NOMBRE_ENV_VAR)) != NULL) {
			retc = snprintf(dbnamebuf, (PATHMAX - 1), "%s", dbprefix);
			dbnamebuf[retc] = 0;
			retc ^= retc;
		/* Environmental variable not found, construct default */
		} else if ((dbprefix = NOMBRE_DB_PREFIX()) != NULL) {
			retc = snprintf(dbnamebuf, (PATHMAX - 1), "%s%s%s", dbprefix, NOMBRE_DB_DIRECT, NOMBRE_DB_NAME);
			dbnamebuf[retc] = 0; /* Ensure NUL terminator */
			retc ^= retc;
		} 
		/* No real else condition, just assume we were given a valid name */
	}

	return(retc);
}

int
nom_initdb(const char * restrict dbname, const char * restrict initsql) {
	int retc;
	retc = 0;

	/* Validate input arguments */
	if ((dbname == NULL) || (initsql == NULL)) {
		fprintf(stderr,"[ERR] %s [%s:%u] %s: Given invalid NULL input, returning to caller!\n", __progname, __FILE__, __LINE__, __func__);
		retc = BADARGS;
	}

	return(retc);
}

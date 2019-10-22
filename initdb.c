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
#include <unistd.h>
/* Needed for stat(2) */
#include <sys/stat.h>
#include <sys/types.h>

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
	size_t dblen;
	dblen = 0;
	retc = 0;

	/* Validate input arguments, though it should not be possible to pass a NULL pointer */
	if ((dbname == NULL) || (initsql == NULL)) {
		fprintf(stderr,"[ERR] %s [%s:%u] %s: Given invalid NULL input, returning to caller!\n", __progname, __FILE__, __LINE__, __func__);
		retc = BADARGS;
	}

	/* Validate that the database directory exists and is writeable */
	dblen = strlen(dbname);
	/* XXX: Should probably have a better mnemonic available for this situation */
	if ((retc = nom_dirtest(dbname, dblen)) != NOM_OK) {
		/* 
		 * Directory does not currently exist, create with mask as defined 
		 * retc should now be the number of characters backtracked to reach the directory
		 */
		retc = nom_mkdirs(dbname, (dblen - retc));
	}

	return(retc);
}

int
nom_dirtest(const char * dbname, const size_t dbnamelen) {
	int retc, dirsep;
	char dbdir[PATHMAX];
	struct stat dbdirstat;
	unsigned int fstest;
	uid_t userid;
	gid_t grpid;
	retc = 0;
	userid = 0;
	grpid = 0;
	fstest = 0;
	/* This cast should be fine, as the max buffer size is less than INT_MAX */
	dirsep = (int)dbnamelen;
	
	/* Should not be possible to get a NULL pointer, but test anyway */
	if ((dbname == NULL) || (dbnamelen == 0)) {
		fprintf(stderr,"[ERR] %s [%s:%u] %s: Given invalid input!\n", __progname, __FILE__, __LINE__, __func__);
		/* XXX: This should not be a positive integer to report errors */
		retc = BADARGS;
		return(retc);
	}
	/* Trace back to the directory separator */
	while (dirsep --> 0) {
		if ((dbname[dirsep] ^ DIRSEP) == 0) {
			strlcpy(dbdir, dbname, (size_t)dirsep);
			userid = getuid();
			/* Group info tests could be improved, currently just using the primary GID */
			grpid = getgid();

			/* Now validate the direcory given actually exists and is writeable by the current user */
			if ((retc = stat(dbdir, &dbdirstat)) != 0) {
				fprintf(stderr,"[ERR] %s [%s:%u] %s: %s\n", __progname, __FILE__, __LINE__, __func__, strerror(errno));
				retc = errno;
				return(retc);
			} else {
				/* Run some tests againt the returned directory structure */
				if ((dbdirstat.st_uid == userid) && (dbdirstat.st_mode == UMODE)) {
					/* Probably fine, but validate permissions anyway */
					fstest = UDIR_OK;
				} else if ((dbdirstat.st_gid == grpid) && (dbdirstat.st_mode == GMODE)) { 
					/* Check for usable group permissions */
					fstest = GDIR_OK;
				}
			}
			/* The directory permissions appear to be OK, can continue with making the nombre DB */
			if ((fstest == UDIR_OK) || (fstest == GDIR_OK)) {
				retc = NOM_OK;
				return(retc);
			}
			/* 
			 * TODO: Add a branch for testing the parent directory as well 
			 * Since the value passed is significantly larger than the 
			 * required value, investigate adding a sentinel bit to allow recursion
			 */
		}
	}
	/* If we reach the end, there's no parent directory to check, use CWD */
	retc = NOM_USECWD;
	return(retc);
}

int
nom_mkdirs(const char * dbname, const size_t diroffset) {
	int retc;
	retc = 0;
	
	/* Validate inputs */
	if ((dbname == NULL) || (diroffset == 0)) {
		fprintf(stderr,"[ERR] %s [%s:%u] %s: Given invalid input!\n", __progname, __FILE__, __LINE__, __func__);
		/* XXX: This should not be a positive integer to report errors */
		retc = BADARGS;
	}
	return(retc);
}

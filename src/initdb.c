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
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
/* Needed for mmap(2) */
#include <sys/mman.h>
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

/* 
 * nom_getdbn()
 * If the buffer is not already full, attempt to find the name
 * of the database by looking up the environmental variable "NOMBREDB"
 * or construct the default database path name and write to the buffer.
 */
int
nom_getdbn(char * restrict dbnamebuf) {
	int retc;
	char *dbprefix;
	dbprefix = NULL;
	retc = 0;

	if (dbg) {
		NOMDBG("Entering with dbnamebuf = %p (%s)\n", (void *)dbnamebuf, dbnamebuf);
	}
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
	if (dbg) {
		NOMDBG("Returinng %d to caller\n", retc);
	}
	return(retc);
}

int
nom_initdb(const char * restrict dbname, const char * restrict initsql, nomcmd *cmdbuf) {
	int retc;
	size_t dblen;
	dblen = 0;
	retc = 0;
	
	if (dbg) {
		NOMDBG("Entering with dbname = %s, initsql = %s, cmdbuf = %p\n", dbname, initsql, (void *)cmdbuf);
	}

	/* Validate input arguments, though it should not be possible to pass a NULL pointer */
	if ((dbname == NULL) || (initsql == NULL)) {
		NOMERR("Given invalid input, returning %d to caller!\n", BADARGS);
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
		retc = nom_mkdirs(dbname, (dblen - (size_t)retc));
	} else {
		if ((retc = sqlite3_initialize()) != SQLITE_OK) {
			NOMERR("%s Returning %d to caller\n","Unable to ititialize SQLite3 library!", retc);
			return(retc);
		}
		retc = sqlite3_open_v2(dbname, &cmdbuf->dbcon, SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE|SQLITE_OPEN_FULLMUTEX|SQLITE_OPEN_PRIVATECACHE, NULL);
		if (retc == SQLITE_OK) {
			/* Run the initialization SQL script */
			retc = run_initsql(cmdbuf);
		} else {
			/* Error in opening the database */
			NOMERR("Unable to open %s (%s)!\n", dbname, sqlite3_errstr(retc));
		}
	}

	/* Close the database if it was created */
	if (retc == NOM_OK) {
		sqlite3_close(cmdbuf->dbcon);
	}
	if (dbg) {
		NOMDBG("Returning %d to caller\n", retc);
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

	if (dbg) {
		NOMDBG("Entering with dbname = %s, dbnamelen = %lu\n", dbname, dbnamelen);
	}
	
	/* Should not be possible to get a NULL pointer, but test anyway */
	if ((dbname == NULL) || (dbnamelen == 0)) {
		NOMERR("%s","Invalid input! Baliing out!\n");
		retc = BADARGS;
		return(retc);
	}

	/* Trace back to the directory separator */
	while (dirsep --> 0) {
		if ((dbname[dirsep] ^ DIRSEP) == 0) {
			strlcpy(dbdir, dbname, (size_t)(dirsep + 1)); /* Add 1 to copy length, attempt to avoid truncation */
			userid = getuid();
			/* Group info tests could be improved, currently just using the primary GID */
			grpid = getgid();
			break;
		}
	}

	if (dbg) {
		NOMDBG("%s%s\n", "Detected directory: ", dbdir);
	}
	/* Now validate the direcory given actually exists and is writeable by the current user */
	if ((retc = stat(dbdir, &dbdirstat)) != 0) {
		NOMERR("%s: %s!\n", dbdir, strerror(errno));
		retc = errno;
		return(retc);
	} else {
		if (dbg) {
			NOMDBG("Directory info: %s: %u, %s:%o\n", "UID", dbdirstat.st_uid, "Mode", dbdirstat.st_mode);
		}
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
		if (dbg) {
			NOMDBG("Returning %d to caller\n", retc);
		}
		return(retc);
	}
	/* 
	 * TODO: Add a branch for testing the parent directory as well 
	 * Since the value passed is significantly larger than the 
	 * required value, investigate adding a sentinel bit to allow recursion
	 */
	/* If we reach the end, there's no parent directory to check, use CWD */
	retc = NOM_USECWD;

	if (dbg) {
		NOMDBG("Returning %d to caller\n", retc);
	}
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

int
run_initsql(const nomcmd * cmdbuf) {
	int retc, sqlfd;
	size_t sqllen;
	char *sqlmap, *sqlend;
	const char *sqltail;
	struct stat sqlstat;
	sqlite3_stmt *stmt;
	retc = 0;
	sqllen = 0;
	sqlmap = NULL; sqlend = NULL; sqltail = NULL;
	stmt = NULL;

	if (dbg) {
		NOMDBG("Entering with dbhandle = %p, dbname = %s, sqlfile = %s\n", 
				(void *)cmdbuf->dbcon, cmdbuf->filedata[NOMBRE_DBFILE], cmdbuf->filedata[NOMBRE_INITSQL]);
	}
	/* Validate non-null pointer */
	if (cmdbuf == NULL) {
		fprintf(stderr, "[ERR] %s [%s:%u] %s: Invalid command structure!\n", __progname, __FILE__, __LINE__, __func__);
		retc = BADARGS;
		return(retc);
	}

	if ((retc = stat(cmdbuf->filedata[NOMBRE_INITSQL], &sqlstat)) != NOM_OK) {
		/* Unable to continue at this point, bailing out */
		NOMERR("Fatal Error: %s\n", strerror(errno));
		return(retc);
	} else {
		/* Ideally validate that we can read the file, but for now it's important to grab the size */
		sqllen = (size_t)sqlstat.st_size;
	}

	/* This test is different, as we expect sqlfd to be a nonzero positive integer */
	if ((sqlfd = open(cmdbuf->filedata[NOMBRE_INITSQL], O_RDONLY|O_NONBLOCK|O_EXLOCK)) < 0) {
		/* Should not be possible after testing for existence via stat(2) */
		NOMERR("Unable to open %s! (%s)\n", cmdbuf->filedata[NOMBRE_INITSQL], strerror(errno));
		return(NOM_FIO_FAIL);
	}

	if (dbg) {
		NOMDBG("sqlfd = %d\n", sqlfd);
	}
	/* Now create an mmap(2)'d buffer for the file */
	if ((sqlmap = mmap(NULL, (size_t)sqlstat.st_size, PROT_READ, MAP_NOSYNC|MAP_PRIVATE, sqlfd, (off_t)0)) == MAP_FAILED) {
		NOMERR("Unable to map %s! (%s)\n", cmdbuf->filedata[NOMBRE_INITSQL], strerror(errno));
		/* Clean up before bailing */
		close(sqlfd);
		return(errno);
	} else {
		/* Success, now prop up the end point so we can loop easily */
		sqlend = sqlmap + sqllen;
		/* Inform the kernel how we intend to use the mmaped file */
		/* XXX: MADV_NOSYNC may not be portable, fortunately not necessary for functionality */
		posix_madvise(sqlmap, (size_t)sqllen, POSIX_MADV_SEQUENTIAL|MADV_NOSYNC|POSIX_MADV_WILLNEED);
		retc = sqlite3_prepare_v2(cmdbuf->dbcon, sqlmap, -1, &stmt, (const char **)&sqltail);
	}

	/* This requires that we've actually compiled a SQL statement before entering the loop */
	for (; sqlmap < sqlend; sqlmap = (char * const)sqltail) {
		/* TODO: Add a check for possible error conditions */
		retc = sqlite3_step(stmt);
		retc = sqlite3_finalize(stmt);
		retc = sqlite3_prepare_v2(cmdbuf->dbcon, sqlmap, -1, &stmt, (const char **)&sqltail);
		if (dbg) {
			NOMDBG("Current Context: %.16s\n", sqlmap);
		} else {
			fprintf(stdout,".");
		}
	}
	fprintf(stdout,"\n");
	if (retc != SQLITE_OK && retc != SQLITE_DONE) {
		/* Likely erroneous end to the loop */
		NOMWRN("Exited with non-OK status! (%s)\n", sqlite3_errmsg(cmdbuf->dbcon));
	} else {
		fprintf(stdout, "\n%s should now be ready to use!\n", cmdbuf->filedata[NOMBRE_DBFILE]);
	}
	/* Clean up before we exit */
	/* Reset the pointerfor our mmap'd data */
	sqlmap = (sqlend - sqllen);
	munmap(sqlmap, (size_t)sqllen);
	close(sqlfd);
	return(retc);
}

int
nom_dbconn(nomcmd *cmdbuf) {
	int retc;
	retc = 0;
	
	if (dbg) {
		NOMDBG("Entering with cmdbuf = %p, dbfile = %s\n", (void *)cmdbuf, cmdbuf->filedata[NOMBRE_DBFILE]);
	}
	if (cmdbuf == NULL) {
		NOMERR("%s\n", "Invalid Parameters!");
		retc = BADARGS;
	} else {
		if ((retc = sqlite3_open_v2(cmdbuf->filedata[NOMBRE_DBFILE], &cmdbuf->dbcon, SQLITE_OPEN_READWRITE|SQLITE_OPEN_NOMUTEX|SQLITE_OPEN_PRIVATECACHE, NULL)) != SQLITE_OK) {
			/* Something has gone wrong */
			cmdbuf->dbcon = NULL;
			NOMERR("Could not connect to database \"%s\" (%s)!\n", cmdbuf->filedata[NOMBRE_DBFILE], sqlite3_errstr(retc));
		} else {
			retc ^= retc;
		}
	}
	if (dbg) {
		NOMDBG("Returning %d to caller with cmdbuf->dbcon = %p\n", retc, (void *)cmdbuf->dbcon);
	}
	return(retc);
}

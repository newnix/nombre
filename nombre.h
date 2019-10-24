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

#define NOMBRE_H
#include <stdint.h>
#include <sqlite3.h>

/* 
 * Default location for the database file to live 
 * should evaluate to ~/.local/nombre.db
 * can be overridden by the '-d' flag
 */
#define NOMBRE_DB_PREFIX() getenv("HOME")
/* Set the default mode to 1750 */
#define NOMBRE_DB_DIRECT_MODE S_ISVTX|S_IRWXU|S_IRGRP|S_IXGRP
#define NOMBRE_DB_DIRECT "/.local/"
#define NOMBRE_DB_NAME "nombre.db"
#define NOMBRE_ENV_VAR "NOMBREDB"
#define DIRSEP  '/'

/* Some buffer size settings */
#define BUFSIZE 4096
#define PATHMAX 1024
#define DEFLEN  512

/* Some general return mnemonics */
#define NOM_OK  0x00
#define BADARGS 0xFF /* Should evaluate to -1 when used as a signed integer */
#define NOM_USECWD 0xFE /* Should evaluate to -2 */

/* Simple not implemented message */
#define NOTIMP(a) fprintf(stderr,"-%c is not yet implemented!\n",a)

/* Simple message that the flag isn't recognized */
#define BADFLAG(a) fprintf(stderr,"[ERR] %s: \'%s\' is not a valid flag!\n",__progname, a)

/* Status reporting macros */
#define NOMDBG(fstr, ...) fprintf(stderr, "[DBG] %s [%s:%u] %s: " fstr, __progname, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define NOMINF(fstr, ...) fprintf(stderr, "[INF] %s [%s:%u] %s: " fstr, __progname, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define NOMWRN(fstr, ...) fprintf(stderr, "[WRN] %s [%s:%u] %s: " fstr, __progname, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define NOMERR(fstr, ...) fprintf(stderr, "[ERR] %s [%s:%u] %s: " fstr, __progname, __FILE__, __LINE__, __func__, __VA_ARGS__)

/* Create a "new" type called "byte" */
typedef uint8_t byte;

/* Define the values of the subcommonds */
typedef enum subcom_t {
	lookup = 0,  /* Most likely command, look up given term */
	define = 1,  /* Add a definition */
	search = 2,  /* Perform a keyword search */
	verify = 4,  /* Run validation tests */
	initdb = 5,  /* Initialize database */
	import = 6,  /* Import definitions from file */
	export = 7,  /* Export definitons to file */
	dumpdb = 8,  /* Dump contents to stdout */
	addsrc = 9,  /* Add an entry for the definition source */
	update = 10, /* Update a definition */
	vquery = 11, /* Lookup with sources */
	catscn = 12  /* Dump the definitions for the given category to stdout */
} subcom;

/* 
 * Define data structure for command parsing 
 * It's a bit larger than I'd like, but there only ever needs to be one per invocation
 * and then we can just pass the pointer around.
 */
typedef struct nombre_cmd_t {
	subcom command;
	sqlite3 *dbcon; /* database connection */
	char filedata[3][PATHMAX]; /* File argument array holder */
	/* assume we're using ASCII for now, full UTF-8 will be a stretch goal */
	char defdata[2][DEFLEN]; /* Fold term/category into single 2D member */
	char *definition;  /* Collected definition (input only) */
	char *gensql; /* Generated SQL statement */
} nomcmd;

/* 
 * Add macros to help with array indexing
 */
#define NOMBRE_DBFILE 0x00
#define NOMBRE_INITSQL 0x01
#define NOMBRE_IOFILE 0x02
#define NOMBRE_DBTERM 0x00
#define NOMBRE_DBCATG 0x01

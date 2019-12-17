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

#define PARSE_SHORT 3

extern char *__progname;
extern char **environ;
extern bool dbg;

static inline bool isgrp(const nomcmd * restrict cmd);
static inline void upcase(char * restrict str);

int
parsecmd(nomcmd * restrict cmdbuf, const char * restrict arg) {
	int retc;
	size_t argsz, arglen;
	retc = -1;
	argsz = arglen = 0;

	/* Define a list of valid command strings */
	const char *cmd[][CMDCOUNT] = { 
		{ "def", "add", "key", "lst", "new", "imp", "exp", "src", "upd", "vqy", "cts", "grp" }, /* "Short" */
		{ "define", "adddef", "keyword", "list", "new", "import", "export", "srcadd", "update", "vquery", "catscn", "grpcmd" } /* "Long" */
	};

	if (dbg) {
		NOMDBG("Entering with cmdbuf = %p, arg = %p, strnlen(%s) = %lu\n", (const void *)cmdbuf, (const void *)arg, arg, strnlen(arg,(size_t)6));
	}

	/* A NULL argument should not be possible */
	if (arg == NULL || cmdbuf == NULL) {
		NOMERR("%s\n", "Given invalid arguments!");
		retc = BADARGS;
	}
	/* Use arglen to determine which index to iterate over */
	argsz = ((arglen = strnlen(arg, ((size_t)PARSE_SHORT * 2))) == PARSE_SHORT) ? 0 : 1;

	/* 
	 * Since we can be sure that we don't have a NULL pointer, check the string size 
	 * iterator value is never more than 11, so use whatever the fastest option is for an 8bit value
	 * Loops are capped at valid commands aside from the group value, as that requires extra logic,
	 * the group commands are assumed to be among the least frequent, and as such will only be checked 
	 * after all single-value commands are exhausted.
	 */
	for (register int_fast8_t i = 0; ((i < (CMDCOUNT - 1)) && (retc != 0)) ; i++) {
		if ((retc = memcmp(arg, cmd[argsz][i], arglen)) == 0) {
			cmdbuf->command |= (0x01 << i);
		}
		if (dbg) { 
			NOMDBG("i = %d, retc = %d, cmd[%lu][%d] = %s, cmdbuf->command = %u, (0x01 << %d) = %X (%u)\n", 
					i, retc, argsz, i, cmd[argsz][i], cmdbuf->command, i, (0x01 << i), (0x01 << i)); 
		}
		/* 
		 * If we get the "new" subcommand without the group subcommand,
		 * set to lookup instead.
		 */
		if (cmdbuf->command == new && (! isgrp(cmdbuf))) {
			NOMINF("Invalid subcommand: \"new\" must be preceeded by \"%s\"! Using default behaviour...\n", cmd[argsz][CMDCOUNT-1]); 
			cmdbuf->command = lookup;
			retc = NOM_INVALID;
			break; /* Force exiting the loop early */
		} else { 
			if (cmdbuf->command != unknown && retc == 0) {
				break;
			}
		}
	}
	/* Explicitly check for the group subcommand modifier */
	if (memcmp(arg, cmd[argsz][CMDCOUNT - 1], arglen) == 0) {
		if (dbg) {
			NOMDBG("Detected group command with arg = %s\n", arg);
		}
		cmdbuf->command |= grpcmd;
		retc = grpcmd;
	}
	/* If the command is still unset at this time, force it to "lookup" */
	cmdbuf->command = (cmdbuf->command == unknown) ? lookup : cmdbuf->command;

	if (dbg) {
		NOMDBG("Returning %d to caller with cmdbuf->command = %d\n", retc, cmdbuf->command);
	}
	return(retc);
}

int
nombre_lookup(nomcmd * restrict cmdbuf, const char ** restrict args) {
	int retc;
	retc = NOM_OK;

	if (dbg) {
		NOMDBG("Entering with cmdbuf = %p, args = %p\n", (void *)cmdbuf, (const void *)*args);
	}
	if ((cmdbuf == NULL) || (*args == NULL)) {
		NOMERR("%s\n", "Invalid Arguments!");
		retc = BADARGS;
	} else {
		if (isgrp(cmdbuf)) {
			/* Use group logic */
			memccpy(cmdbuf->defdata[NOMBRE_DBCATG], *args, 0, (size_t)DEFLEN); args++;
			memccpy(cmdbuf->defdata[NOMBRE_DBTERM], *args, 0, (size_t)DEFLEN); args++;
			retc = snprintf(cmdbuf->gensql, (size_t)PATHMAX, "SELECT meaning FROM definitions WHERE term LIKE(\'%s\') AND category=(SELECT id FROM categories WHERE name LIKE(\'%s\'));",
					cmdbuf->defdata[NOMBRE_DBTERM], cmdbuf->defdata[NOMBRE_DBCATG]);
		} else {
			/* Expected to be normal path */
			memccpy(cmdbuf->defdata[NOMBRE_DBTERM], *args, 0, (size_t)DEFLEN); args++;
			retc = snprintf(cmdbuf->gensql, (size_t)PATHMAX, "SELECT meaning FROM definitions WHERE term LIKE(\'%s\');",
					cmdbuf->defdata[NOMBRE_DBTERM]);
		}
	}
	/* Assume we wrote what was intended and clear the return code. */
	retc = (retc > 0) ? retc ^ retc : retc;
	if (dbg) {
		NOMDBG("Returning %d to caller\n", retc);
	}
	return(retc);
}

int
nombre_newdef(nomcmd * restrict cmdbuf, const char ** restrict args) {
	int retc;
	char defstr[DEFLEN];
	retc = 1; /* Start with retc nonzero to enter loops properly */

	if (dbg) {
		NOMDBG("Entering with cmdbuf = %p, args = %p\n", (void *)cmdbuf, (const void *)*args);
	}
	if ((cmdbuf == NULL) || (*args == NULL)) {
		NOMERR("%s\n", "Invalid Arguments!");
		retc = BADARGS;
	} else {
		/* Explicitly zero local character array */
		memset(defstr, 0, (size_t)DEFLEN);
	}

	if (isgrp(cmdbuf)) {
		memccpy(cmdbuf->defdata[NOMBRE_DBCATG], *args, 0, (size_t)DEFLEN); args++;
		upcase(cmdbuf->defdata[NOMBRE_DBCATG]);
		/* Only if the new value of *args is non-null! */
		if (*args != NULL) {
			memccpy(cmdbuf->defdata[NOMBRE_DBTERM], *args, 0, (size_t)DEFLEN); args++;
			upcase(cmdbuf->defdata[NOMBRE_DBTERM]);
			/* Flatten the rest of the argument vector */
			for (register int written = 0; *args != NULL && retc > 0; args++) {
				retc = snprintf(&defstr[written], (size_t)(DEFLEN - written), (written > 0) ? " %s" : "%s", *args);
				written += retc;
			}
			if (dbg) {
				NOMDBG("Flattened arguments to \"%s\"\n", defstr);
			}
			retc = snprintf(cmdbuf->gensql, (size_t)PATHMAX, 
					"INSERT INTO definitions VALUES (\'%s\', \'%s\', (SELECT id FROM categories WHERE name LIKE(\'%s\')));",
					cmdbuf->defdata[NOMBRE_DBTERM], defstr, cmdbuf->defdata[NOMBRE_DBCATG]);
		}
		retc = BADARGS;
		NOMERR("Invalid number of arguments for %s!\n", __func__);
	} else {
		memccpy(cmdbuf->defdata[NOMBRE_DBTERM], *args, 0, (size_t)DEFLEN); args++;
		upcase(cmdbuf->defdata[NOMBRE_DBTERM]);
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

	if (dbg) {
		NOMDBG("Entering with cmdbuf = %p, args = %p\n", (void *)cmdbuf, (void *)*args);
	}
	if ((cmdbuf == NULL) || (*args == NULL)) {
		NOMERR("%s\n", "Invalid Arguments!");
		retc = BADARGS;
	} else {
		if (isgrp(cmdbuf)) {
			/* Copy the group info if it exists */
			memccpy(cmdbuf->defdata[NOMBRE_DBCATG], *args, 0, (size_t)DEFLEN); args++;
			memccpy(cmdbuf->defdata[NOMBRE_DBTERM], *args, 0, (size_t)DEFLEN); /* Should now be out of arguments */
			retc = snprintf(cmdbuf->gensql, (size_t)PATHMAX, "SELECT term, meaning FROM definitions WHERE meaning LIKE(\'%%%s%%\') AND category=(SELECT id FROM categories WHERE name LIKE(\'%s\'));",
					cmdbuf->defdata[NOMBRE_DBTERM], cmdbuf->defdata[NOMBRE_DBCATG]);
		} else {
			memccpy(cmdbuf->defdata[NOMBRE_DBTERM], *args, 0, (size_t)DEFLEN);
			retc = snprintf(cmdbuf->gensql, (size_t)PATHMAX, "SELECT term, meaning FROM definitions WHERE meaning LIKE(\'%%%s%%\');",
					cmdbuf->defdata[NOMBRE_DBTERM]);
		}
	}
	retc = (retc > 0) ? retc ^ retc : retc;
	if (dbg) {
		NOMDBG("Returning %d to caller, with cmdbuf->gensql = %s\n", retc, cmdbuf->gensql);
	}
	return(retc);
}

/* 
 * The group modifier for this function depends on the presence of 
 * a group name. If no name is given, we will list the currently defined groups,
 * otherwise, we use the group id as an output filter for the database listing
 */
int
nombre_dbdump(nomcmd * restrict cmdbuf, const char ** restrict args) {
	int retc;
	retc = 0;

	if (dbg) {
		NOMDBG("Entering with cmdbuf = %p\n", (void *)cmdbuf);
	}

	/* A NULL pointer is acceptable as the 'args' value */
	if (cmdbuf == NULL) {
		NOMERR("%s\n", "Invalid arguments!\n");
		retc = BADARGS;
	}
	if (retc == NOM_OK) {
		if (isgrp(cmdbuf)) {
			if (*args != NULL) {
				retc = snprintf(cmdbuf->gensql, (size_t)DEFLEN, "SELECT c.name, d.term, d.meaning FROM categories AS c JOIN definitions AS d"
						" ON c.id = d.category WHERE d.category = (SELECT id FROM categories WHERE name LIKE(\'%s\')) ORDER BY 2 DESC;", *args);
			} else {
				retc = snprintf(cmdbuf->gensql, (size_t)DEFLEN, "%s", "SELECT  id, short, nlong FROM category_verbose ORDER BY 1 DESC;");
			}
		} else {
			/* Precision loss is acceptable as the given write limit is well under INT_MAX */
			retc = snprintf(cmdbuf->gensql, (size_t)DEFLEN, "%s", "SELECT c.name, d.term, d.meaning FROM categories AS c JOIN definitions AS d ON c.id = d.category ORDER BY 1,2 DESC;");
		}
		/* Clear the counter values from string operations prior to returning */
		retc = (retc > NOM_OK) ? retc ^ retc : retc;
	}
	
	if (dbg) {
		NOMDBG("Returinng %d to caller with cmdbuf->gensql= %s\n", retc, cmdbuf->gensql);
	}
	return(retc);
}

/* 
 * At this point, the argument vector should only have the new group name,
 * the verbose name, and optionally the group description. 
 * If we only get the new group label, we'll add in some boilerplate data, but the 
 * id is always generated as "SELECT MAX(id)+1 FROM categories;" in a subquery.
 */
int
nombre_newgrp(nomcmd * restrict cmdbuf, const char ** restrict args) {
	int retc;
	retc = 0;
	if (dbg) {
		NOMDBG("Entering with cmdbuf = %p, args = %p\n", (void *)cmdbuf, (const void *)*args);
	}
	if (cmdbuf == NULL || *args == NULL) {
		NOMERR("%s\n", "Invalid parameters!");
		retc = BADARGS;
	} else {
		/* Work on parsing out the new data */
		memccpy(cmdbuf->defdata[NOMBRE_DBCATG], *args, 0, (size_t)DEFLEN); args++;
		/* 
		 * The group data should be dot delimited to reduce conflicts with other values,
		 * so an example would look like:
		 * nombre grp new SHORT.Sharthand
		 * would create a new group called "SHORT" with the description of "Shorthand"
		 */
		if (*args == NULL) {
			retc = NOM_INCOMPLETE;
		}
	}
	if (dbg) {
		NOMDBG("Returning %d to caller with gensql = %s\n", retc, cmdbuf->gensql);
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

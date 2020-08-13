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

static inline int extract_defstring(const char * restrict sql, char * restrict defstr);
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
		{ "def", "add", "key", "del", "lst", "new", "imp", "exp", "src", "upd", "vqy", "cts", "grp" }, /* "Short" */
		{ "define", "adddef", "keyword", "delete", "list", "new", "import", "export", "srcadd", "update", "vquery", "catscn", "grpcmd" } /* "Long" */
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
 * Delete the given term/group from the database based on the 
 * presence of the group flag. This will require some sort of modification or 
 * check against terms using the given group. Most likely just resetting them
 * to "unknown".
 */
int
nombre_delete(nomcmd * restrict cmdbuf, const char ** argstr) {
	int retc;
	retc = NOM_OK;
	if (cmdbuf == NULL || *argstr == NULL) {
		NOMERR("%s\n","Invalid arguments!");
		retc = NOM_INVALID;
		return(retc);
	}
	memccpy(cmdbuf->defdata[NOMBRE_DBTERM], *argstr, 0, (size_t)DEFLEN); argstr++;
	upcase(cmdbuf->defdata[NOMBRE_DBTERM]);
	if (isgrp(cmdbuf)) {
		/* TODO: Add group logic */
	} else {
		/* XXX: Will not handle the altdefs table in this state */
		retc = snprintf(cmdbuf->gensql, (size_t)DEFLEN, "DELETE FROM definitions WHERE term=\'%s\';", cmdbuf->defdata[NOMBRE_DBTERM]);
		retc = (retc > 0) ? retc ^ retc: retc; /* Set to 0 for assumed success */
	}
	return(retc);
}

/* 
 * At this point, the argument vector should only have the new group name,
 * the verbose name, and optionally the group description. 
 * If we only get the new group label, we'll add in some boilerplate data, but the 
 * id is always generated as "SELECT MAX(id)+1 FROM categories;" in a subquery.
 * The "short" name will be limited to 5 characters, while the "long" name 
 * and optional description are only limited by the size of the DEFLEN (512)
 * this should allow for significantly larger values than most uses would need.
 */
int
nombre_newgrp(nomcmd * restrict cmdbuf, const char ** restrict args) {
	int retc;
	char delim, *dptr;
	retc = 0;
	delim = '.';
	dptr = NULL;
	if (dbg) {
		NOMDBG("Entering with cmdbuf = %p, args = %p\n", (void *)cmdbuf, (const void *)*args);
	}
	if (cmdbuf == NULL || *args == NULL) {
		NOMERR("%s\n", "Invalid parameters!");
		retc = BADARGS;
	} else {
		/* 
		 * Work on parsing out the new data
		 * The group data should be dot delimited to reduce conflicts with other values,
		 * so an example would look like:
		 * nombre grp new SHORT.Sharthand
		 * would create a new group called "SHORT" with the description of "Shorthand"
		 * though it should also be possible in the future to allow the user to define their
		 * field delimiter during invocation to allow greater customization.
		 */
		dptr = memccpy(cmdbuf->defdata[NOMBRE_DBCATG], *args, delim, (size_t)DEFLEN); 
		if (dbg) {
			NOMDBG("%p = %s\n", (void *)dptr, dptr);
		}
		/* 
		 * This should get us the "short" name in defdata[NOMBRE_DBCATG] 
		 * from there, we just need to check the length of the argument to verify a valid
		 * short name and potentially additional information for a long name and description
		 */
		if (*(args + 1) == NULL && dptr == NULL) { /* End of input AND no delimiter was found */
			retc = NOM_INCOMPLETE;
			NOMERR("Captured %s, but expected more data!\n", cmdbuf->defdata[NOMBRE_DBCATG]);
		} else if (*(args + 1) != NULL && dptr == NULL) { /* More input, but no delimiter found */
			NOMDBG("Captured %s, but found no delimiter before next term (%s)\n",
					cmdbuf->defdata[NOMBRE_DBCATG], *(args + 1));
		} else if (*(args +1) != NULL && dptr != NULL) { /* More input, found delimiter */
			NOMDBG("Captured %s, assumed long name is \"%s\", with description of %s\n",
					cmdbuf->defdata[NOMBRE_DBCATG], (*args + strlen(cmdbuf->defdata[NOMBRE_DBCATG])), *(args + 1));
		} else { /* End of input, delimiter found */
			NOMDBG("Captured %s, assuming long name is \"%s\"\n",
					cmdbuf->defdata[NOMBRE_DBCATG], (*args+strlen(cmdbuf->defdata[NOMBRE_DBCATG])));
		}
	}
	if (dbg) {
		NOMDBG("Returning %d to caller with gensql = %s\n", retc, cmdbuf->gensql);
	}
	return(retc);
}

/* 
 * Generate the appropriate SQL for inserting an alternate definition
 */
int
nombre_altdef(nomcmd * restrict cmdbuf) {
	int retc;
	char defstr[DEFLEN];
	retc = 0;

	if (dbg) {
		NOMDBG("Entering with cmbduf = %p\n", (void *)cmdbuf);
	}

	/* This should not be possible! */
	if (cmdbuf == NULL) {
		retc = NOM_INVALID;
		NOMERR("%s","Invalid arguments! Bailing out...");
	} else {
		/* Obviously, we're here due to a constraint issue, so we can optimize out a few other checks */
		if (extract_defstring(cmdbuf->gensql, defstr) < 0) {
			NOMERR("%s","Something went wrong extracting the definition!\n");
			return(NOM_FAIL);
		} else {
			if (dbg) {
				NOMDBG("Deleting invalid SQL at %p\n", (void *)cmdbuf->gensql);
			}
			memset(cmdbuf->gensql, 0, (size_t)PATHMAX);
		}
		if (isgrp(cmdbuf)) {
			retc = snprintf(cmdbuf->gensql, (size_t)PATHMAX, "INSERT INTO altdefs VALUES (\'%s\',"
						 "(SELECT (SELECT MAX(defno) FROM altdefs WHERE term like \'%s\' + 1) IS NOT NULL OR 1),"
						 "\'%s\', (SELECT id FROM categories WHERE name ilike \'%s\');",
						 cmdbuf->defdata[NOMBRE_DBTERM], cmdbuf->defdata[NOMBRE_DBTERM], cmdbuf->defdata[NOMBRE_DBCATG], defstr);
		} else {
			retc = snprintf(cmdbuf->gensql, (size_t)PATHMAX, "INSERT INTO altdefs VALUES (\'%s\',"
					"(SELECT SELECT MAX(defno) FROM altdefs WHERE term like \'%s\') + 1 or 1) IS NOT NULL OR 1),"
					"\'%s\', -1);", cmdbuf->defdata[NOMBRE_DBTERM], cmdbuf->defdata[NOMBRE_DBTERM], defstr);
		}
	}
	if (dbg) {
		NOMDBG("Returning %d to caller with regenerated SQL: %s\n", retc, cmdbuf->gensql);
	}
	retc = (retc > 0) ? retc ^ retc : retc;
	return(retc);
}

/*
 * This function is meant explicitly to grab the definition string from an already generated query 
 */
static inline int
extract_defstring(const char * restrict sql, char * restrict defstr) {
	register char *i, *j, *defstart;
	register int_fast8_t retc;
	/* XXX: I don't remember why I allocated j, so this will need to be refactored a bit later */
	i = NULL; j = NULL; defstart = NULL;
	retc = 0;

	if (dbg) {
		NOMDBG("Entering with sql = %s, defstr = %p\n", sql, (void *)defstr);
	}
	/* This should not be possible as both are arrays with static allocation */
	if (sql == NULL || defstr == NULL) {
		NOMERR("%s","This should not be possible, passed invalid arguments!");
		return(NOM_INVALID);
	}
	for (i = (char *)&sql[33], defstart = defstr; *i != 0; i++) {
		if (*i == ',') {
			j = i+2; /* Should be the start of the definition string */
			i = j;
		} else if (j != NULL) {
			if (*i == 0x27) { break; } /* Exit loop after hitting the next apostrophe */
			*defstr = *i; defstr++;
			retc++; /* Keep count of how many bytes have been copied */
		}
	}
	*defstr = 0; /* Ensure NUL termination */
	defstr = defstart;
	if (retc > 0 && dbg) {
		/* Success, hopefully */
		NOMDBG("Copied %d characters: %s\n", retc, defstart);
	} else {
		NOMINF("Extracted definition as: %s\n", defstr);
		retc ^= retc;
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

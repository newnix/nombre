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

#define NOMBRE_INITDB_H

/* Needed for 'size_t' datatype */
#include <stdlib.h>

#ifndef NOMBRE_H
#include "nombre.h"
#endif

/* Initdb specific mnemonics */
#define UID_OK 0x01
#define GID_OK 0x02
#define URW_OK 0x04
#define GRW_OK 0x08
#define UMODOK 0x16
#define GMODOK 0x32
/* Seems that the stat struct holds an extra field in the mode member */
#define UMODE  (S_IRWXU|S_IFDIR)
/* OK to foll on group permissions if we have RWX */
#define GMODE  (S_IRWXG|S_IFDIR)
/* Two possible success conditions */
#define UDIR_OK (UID_OK|URW_OK)
#define GDIR_OK (GID_OK|GRW_OK)

int nom_getdbn(char * restrict dbnamebuf);
int nom_dbconn(nomcmd *cmdbuf);
int nom_testdbpath(const char * restrict dbname);
int nom_initdb(const char * restrict dbname, const char * restrict initsql, nomcmd *cmdbuf);
int nom_dirtest(const char * restrict dbname, const size_t dbanmelen);
int nom_mkdirs(const char * restrict dbname, const size_t diroffset);
int run_initsql(const nomcmd * cmdbuf);

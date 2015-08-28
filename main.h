/**
 * Copyright (C) 2001 Vasili Gavrilov
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __MAIN_H
#define __MAIN_H

#include <getopt.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <math.h>
#include <sys/stat.h>
/* #include <sys/uio.h> */  /* we do not use locks here */
#include <sys/types.h>
#include <sys/file.h>

#define MAX_NAME 256 /* 1st field in PED, this is identificator or a name */
#define DEFAULT_SNIPS_FILE "snips"
#define RSID_MAX 32
#define POSITION_MAX 32
#define COLUMNS_IN_SNIPS_FILE 4

struct Record{
	char rsid[RSID_MAX + 1];
	char chromo[2 + 1];
	char position[POSITION_MAX + 1];
};

/* forward references */
static void wrong_args(void);
static void help(void);
static int parse_params(int argc, char **argv);
static int process(char *filename);
static char* header(void);
static int readline(FILE* fp);
static int read_genome(FILE* fp, FILE* fdw);
static int read_biallele(FILE* fp, char* biallele, int *allelesNumInPed);
static int deserialize(char *line, int* snipsLineNum);
static int read_mapping_line(FILE* mp, int* snipsLineNum);
static void create_filepath(char* filename, char* filepath);

#endif

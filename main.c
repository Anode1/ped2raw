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

#include "main.h"

/* We process line by line, so using globals for saving all the line data - for
 * simplicity and performance, so sorry if you are a purist of OO :) */
static struct Record record;
struct Record * const rec=&record;
static char *filename;
static char *mapfile;
static char *output_dir;
static int lineNum=0;
static char FIELDS_DELIMITER='\t';
static char OUTPUT_DELIMITER='\t';
static char *ext = ".txt"; /* extension for files */
int debug = 0; /* default is 0 = no debug */
int trace = 0; /* extremely verbose logging (into the log file) - veeeery slow */

#ifndef UNIT_TEST

int main(int argc, char **argv){

	int exit_code;	

	if((exit_code = parse_params(argc, argv)))
		return exit_code; /* not 0 - if something wrong or help */

	if((exit_code = process(filename))){
		fprintf(stderr, "Can't process\n");
		return exit_code;
	}

	return exit_code;
}
#endif

/* 
  Prints help message if something wrong with arguments
 */
static void wrong_args(){
	fprintf(stderr, "Wrong arguments. Try 'is -h' for reference.\n");
}


static int parse_params(int argc, char **argv){

	int c;
	while ((c = getopt(argc, argv, "i:s:o:hdt")) != -1){
		switch (c){
			case 'i':
				filename=optarg;
				break;
			case 's':
				mapfile=optarg;
				break;
			case 'o':
				output_dir=optarg;
				break;
			case 'h':
				help();
				return 1; /* help - no need in continuing: exit */
			case 'd':
				debug = 1;
				break;
			case 't':
				trace = 1;
				break;				
			default :
				wrong_args();
				return -1; /* wrong args -> exit */
		}/* switch */
	}/* while (all args) */

	int i;
	for(i=0 ; optind < argc; ++optind){
		/* if(debug)printf("argv[%d]='%s'\n", optind, argv[optind]); */
		if(i==0)
			filename=argv[optind];
		i++;
	}
	
	if(!filename){
		help();
		return -1;	 
	}
	
	if(!mapfile){
		mapfile=DEFAULT_SNIPS_FILE;
	}
	
	return 0;
}


static int process(char* filename){

	FILE *fp;
	int error_code=0;

	if (strcmp(filename, "-") == 0){
		fp = stdin;
	}
	else if((fp = fopen(filename, "r")) == NULL){
		fprintf(stderr, "Can't read file %s: %s\n", filename, strerror(errno));
		return -1;
    }

	while(readline(fp)==0){
	}

	if(debug)
		printf("%d lines processed\n", lineNum);

	if(fp!=stdin)
		fclose(fp);
	
	return error_code;
}


/**
  Returns:
  -1 - if we need to terminate (EOF or mapping is not readable)
  0 - if the line was processed correctly
 */
static int readline(FILE * fp){
	char c; /* char red */
	char name[MAX_NAME+1]; /* name of file */
	char output_filename[1024];
	int i; /* reused in few places */
	FILE *fdw = NULL;
	int ret = 0;

	if((c=getc(fp))==EOF)
		return -1;

	lineNum++;

	while(c=='\r')
		c=getc(fp);

	if(c=='\n')
		return 0;

	/* read name */
	i = 0;
	while(1){
		name[i++]=c;
		if(i==MAX_NAME){ /* out of buffer - cut the name */
			name[i]='\0';
			break;
		}

		if((c=getc(fp))==EOF)
			return -1;
		if(c=='\n')
			return 0; /* ignore - not complete line */

		while(c=='\r')
			c=getc(fp);

		if(c==FIELDS_DELIMITER){
			name[i] = '\0';
	       	break;
		}
	}

	/* skip the rest, 5 tabs, until genome. Exit in case of EOL and EOL */
	for(i=0; i<5; i++){
		while(1){
			c=getc(fp);
			while(c=='\r')
				c=getc(fp);
			if(c==EOF)
				return -1;
			if(c=='\n')
				return 0;
			if(c==FIELDS_DELIMITER){
				break;
			}
		}
	}

	/* Here we have genome ahead, so we can create the file */
	create_filepath(name, output_filename);

	if(debug) printf("%s\n", output_filename);

	if((fdw = fopen(output_filename, "w")) == NULL){
		fprintf(stderr, "Can't open output file %s. Please create target directory and verify write permissions\n", output_filename);
		return -1;
	}

	if(fputs(header(), fdw)<0){
		fprintf(stderr, "Can't write into file: %s\n", strerror(errno));
		return -1;
	}
	else{
		ret = read_genome(fp, fdw);
	}

	fclose(fdw);
	return ret;
}


static void create_filepath(char* filename, char* output_filename){
	char* cur;
	int len;
	cur=output_filename;
	if(output_dir){
		len = strlen(output_dir);
		memcpy(output_filename, output_dir, len);
		cur+=len;

		*cur='/';
		cur+=1;

		len = strlen(filename);
		memcpy(cur, filename, len);
		cur+=len;
	}
	else{
		len = strlen(filename);
		memcpy(output_filename, filename, len);
		cur+=len;
	}

	len = strlen(ext);
	memcpy(cur, ext, len);
	cur+=len;
	*cur='\0';

	/* strcat would be easier but we have it already :) */
}


/*
  Returns:
  -1 - EOF or error (i.e. we need to terminate lines processing
  0 - EOL (this function goes until EOL)
 */
static int read_genome(FILE* fp, FILE* fdw){

	char biallele[2];
	FILE *mp; /* template list aka mapping file */
	int res=0; /* result of write */
	int snipsLineNum=0; /* for reporting wrong lines */
	int allelesNumInPed=0;

	if((mp = fopen(mapfile, "r")) == NULL){
		fprintf(stderr, "Can't read snips list file %s: %s\n", mapfile, strerror(errno));
		return -1;
	}

	while(1){
		if(read_biallele(fp, biallele, &allelesNumInPed)!=0){
			break; /* EOF or EOL in PED - nothing to write */
		}
		allelesNumInPed++;

		/* here we have good biallele and write it to the output */
		/* take template data from mapping */
		if(read_mapping_line(mp, &snipsLineNum)!=0){
			fprintf(stderr, "WARN: Line %d: Number of valid alleles in SNP map (%d is in total) is less than number of alleles in PED file (%d)\n", lineNum, snipsLineNum, allelesNumInPed);
			res = -1;
			break; /* here we do not know what snip/chromo/position to write */
		}

		/* at this point we have Record filled */

		fputs(rec->rsid, fdw);
		fputc(OUTPUT_DELIMITER, fdw);
		fputs(rec->chromo, fdw);
		fputc(OUTPUT_DELIMITER, fdw);
		fputs(rec->position, fdw);
		fputc(OUTPUT_DELIMITER, fdw);
		fwrite(biallele, sizeof(char), sizeof(biallele), fdw);

		if(fputc('\n', fdw)!='\n'){ /* verify that disk not filled out */
			fprintf(stderr, "Can't write into file: %s\n", strerror(errno));
			res = -1;
			break;
		}
	}

	/* check if there are still mappings - this means that PED is not complete */
	if(read_mapping_line(mp, &snipsLineNum)==0){
		fprintf(stderr, "WARN: Line %d: PED is incomplete, number of valid records in SNP map (%d is in total) is bigger than number of alleles in PED file (%d)\n", lineNum, snipsLineNum, allelesNumInPed);
		res = -1;
	}

	fclose(mp);
	return res;
}


/*
  Returns:
  -1 - EOF or corrupted allele becaues of EOL
  0 - if success
 */
static int read_biallele(FILE* fp, char* biallele, int *allelesNumInPed){

	/* Actually - it is better to write it using strchr. I'm just lazy
	 * right now, sorry */

	char c;

	c=getc(fp); /* 1 allele */
	if(c==EOF || c=='\n' || c=='\r'){
		return -1;
	}

	while(c==FIELDS_DELIMITER){ /* skip delimiter (or more) */
		c=getc(fp);
	}

	biallele[0]=c;

	c=getc(fp); /* space */
	if(c==EOF || c=='\n' || c=='\r'){
		return -1;
	}
	if(c!=' ')
		printf("WARN: Alleles in position %d in line %d are delimited by %c instead of space\n", *allelesNumInPed, lineNum, c);

	c=getc(fp); /* 2 allele */
	if(c==EOF || c=='\n' || c=='\r'){
		return -1;
	}

	biallele[1]=c;

	return 0;
}


/**
  Returns:
  0 - if everything is fine and record is written
  -1 - if EOF
 */
static int read_mapping_line(FILE* mp, int* snipsLineNum){

	char line[128]; /* mapping is controlled by us, so it's safe */
	int len;
	int lineSize;

	/* no need in init - we overwriting with \0 at the end always
	memset(rec->rsid, 0, sizeof(rec->rsid));
	memset(rec->chromo, 0, sizeof(rec->chromo));
	memset(rec->position, 0, sizeof(rec->position));
    */

	/* read until we'll find a valid snip */
    while(fgets(line, lineSize=sizeof(line), mp) != NULL){

    	(*snipsLineNum)++;

        if(line[lineSize-1] == '\n')
            line[lineSize-1] = '\0';

    	len = strlen(line);
    	if(len == 0){
    		printf("WARN: line %d is empty in snips -- ignored\n", *snipsLineNum);
    		continue;
    	}
    	if(line[len-1] == '\n')
    		line[len-1] = '\0';

    	/* skip comments - if present */
    	if(line[0]=='#')
    		continue;

    	if(deserialize(line, snipsLineNum)!=0){ /* incomplete snips line */
    		continue;
    	}

    	return 0; /* res returned 0, i.e. Record is populated */
  	}

	return -1; /* here we are if line is NULL, i.e. at the EOF */
}


/*
  Returns:
  0 - if everything is fine and record is written
  -1 - if not completed line - skipped with warning
*/
static int deserialize(char *line, int* snipsLineNum){

	char *delim;
	int curCol=0;
	delim = strtok (line," \t");


	while(delim != NULL){
		curCol++;
		if(curCol == 1)
			strcpy(rec->chromo, delim);
       	else if(curCol == 2)
       		strcpy(rec->rsid, delim);
       	else if(curCol==3){
       	}
       	else if(curCol==4){
       		strcpy(rec->position, delim);
       	}
		delim = strtok(NULL, " \t");
	}
	if(curCol < COLUMNS_IN_SNIPS_FILE){
		printf("WARN: line %d has less columns (%d) than necessary (%d) -- skipping line\n", *snipsLineNum, curCol, COLUMNS_IN_SNIPS_FILE);
		return -1;
	}

	/*printf(">%s %s %s<\n", rec->rsid, rec->chromo, rec->position);*/
	return 0;
}

/* Why do custom tokenizer - if the standard one exists?
 * static int deserialize(char *line, int* snipsLineNum){

	int curCol;
	char* delim;
	char* curStart;

	curStart = line;

	for(curCol = 0; curCol < COLUMNS_IN_SNIPS_FILE; curCol++){

		delim = strchr(curStart, SNIPS_FILE_FILEDS_DELIMITER);

		if(delim == NULL){
			if(curCol < COLUMNS_IN_SNIPS_FILE-1){
				printf("WARN: line %ld has less columns (%d) than necessary (%d) -- skipping line\n", *snipsLineNum, curCol, COLUMNS_IN_SNIPS_FILE);
				return -1;
			}
			else{
				strcpy(rec->position, curStart);
			}
			break;
		}

		*delim = '\0';

       	if(curCol == 0)
       		strcpy(rec->chromo, curStart);
       	else if(curCol == 1)
       		strcpy(rec->rsid, curStart);
       	else if(curCol==2){
       	}
       	else if(curCol==3){
       		strcpy(rec->position, curStart);
       	}

   		curStart=delim+1;
	}

	return 0;
}
*/

/* 
  Prints detailed help screen
 */
static void help(){
	
	fprintf(stdout, "\n\
ped2raw conversion utility transforms PED file to raw format \n\
Copyright (C) 2011 Vasili Gavrilov (vgavrilov at users.sourceforge.net\n\
--\n\
This software comes with ABSOLUTELY NO WARRANTY. For details, see\n\
the enclosed file COPYING for license information (AGPL). If you\n\
did not receive this file, see http://www.gnu.org/licenses/agpl.txt.\n\
--\n\
Version 1.0\n\
\n\
SYNOPSIS\n\
	ped2raw <input_file>\n\
	ped2raw -i <input_file> -s <snips_file>\n\
	ped2raw <input_file> -s <snips_file>\n\
	(for more examples see below)\n\
OPTIONS:\n\
    -i         Input PED file (filename or path)\n\
    -s         Pass snips list (aka mapping file)\n\
    -o         Pass output directory for new files\n\
    -h         Print this help screen (the same as without arguments)\n\
    -d         Debug\n\
	\n\
EXAMPLES:\n\
   ped2raw TestFile.ped\n\
\n\
   ped2raw \"Testing 0607 Data.ped\" (using default provided snips)\n\
   ped2raw i=tests/test_short.ped -s tests/snips_short\n\
   ped2raw -i input.ped -o /home/user/output_dir -s /some/path/snips_map\n");
}


/*
  Prints header into raw output file
 */
static char* header(){

	return
"# This data file generated by ped2raw tool\n\
#\n\
# This file resembles 23andme format, so the following is description from its header:\n\
#\n\
# Fields are TAB-separated\n\
# Each line corresponds to a single SNP.  For each SNP, we provide its identifier\n\
# (an rsid or an internal id), its location on the reference human genome, and the\n\
# genotype call oriented with respect to the plus strand on the human reference\n\
# sequence.     We are using reference human assembly build 36.  Note that it is possible\n\
# that data downloaded at different times may be different due to ongoing improvements\n\
# in our ability to call genotypes. More information about these changes can be found at:\n\
# https://www.23andme.com/you/download/revisions/\n\
#\n\
# More information on reference human assembly build 36:\n\
# http://www.ncbi.nlm.nih.gov/projects/mapview/map_search.cgi?taxid=9606&build=36\n\
#\n\
# rsid    chromosome    position    genotype\n\
";

}


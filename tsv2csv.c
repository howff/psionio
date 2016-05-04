/* > tsv2csv.c
 * 1.02 arb Tue Sep 21 14:41:28 BST 2004 - Added NumCols option, thanks to
 *      suggestion by RCG <rguevara@mcanet.com.ar>
 * 1.01 arb Thu Aug  8 10:10:03 BST 2002 - bigger LINESIZE
 * 1.00 arb Wed May 23 12:12:23 BST 2001
 */

static char SCCSid[] = "@(#)tsv2csv.c     1.02 (C) 2001 arb TSV to CSV";

/*
 * Convert TSV (Tab Separated Values) to CSV (Comma Separated Values).
 * eg. input file:
 * AB	CD	D E F	GH
 * gives output file:
 * "AB","CD","D E F","GH"
 * See also the csv2tsv program.
 *
 * Copyright 2004 Andrew Brooks <arb@sat.dundee.ac.uk>
 */

/*
 * Bugs:
 *
 * Everything is enclosed in quotes, including numbers.  Is that standard?
 * European numbers may include the comma character so it is safe this way.
 *
 * Not sure of the proper way of quoting a double quote character in a string.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "psionio.h"

char *prog;
char *usage = "usage: %s [NumCols] < file.tsv > file.csv\n"
	"Converts TSV (Tab Separated Values) to CSV (Comma Separated Values).\n"
	"Pipe TSV to standard input using < or |\n"
	"Capture CSV on standard output using > or |\n"
	"Specify number of columns if it is necessary to have a fixed amount.\n";
char *options="";

#define LINESIZE 8192

int
main(int argc, char *argv[])
{
	FILE *fpin = stdin, *fpout = stdout;
	char line[LINESIZE];
	int linelen, linecount, fieldcount, numcols=0;
	char *str, *quote;

	prog = argv[0];
	if (argc <= 1)
	{
		fprintf(stderr, usage, prog);
		exit(1);
	}
	if (argc > 1)
		numcols = atoi(argv[1]);
	line[LINESIZE-1]='\0';
	linecount=0;
	while (fgets(line, LINESIZE-1, fpin))
	{
		linecount++;
		linelen = strlen(line);
		if (linelen > LINESIZE-2)
		{
			line[60]='\0';
			fprintf(stderr, "%s: line %d too long - not converted\n(%s...)\n", prog, linecount, line);
			continue;
		}
		/* Remove newline characters from end of line */
		while (line[linelen-1] == A_CR || line[linelen-1] == A_LF)
			line[--linelen] = '\0';
		/* Search for tab characters in line */
		fieldcount = 0;
		str = strctok(line, '\t');
		while (str)
		{
			/* Stop if no more fields and fixed number of cols not needed */
			/* or if number of cols needed has been reached. */
			if ((!str && !numcols) || fieldcount >= numcols)
				break;
			/* Print field separator if needed */
			fieldcount++;
			if (fieldcount > 1)
				printf(",");
			/* Print starting quote */
			printf("\"");
			/* If line contains quotes they need escaping */
			if (str != NULL)
				quote = strchr(str, '"');
			while (quote)
			{
				/* Remove quote, print line upto quote, print escape */
				*quote = '\0';
				printf("%s\\\"", str);
				str = quote+1;
				quote = strchr(str, '"');
			}
			/* Print rest of line */
			if (str != NULL)
				printf("%s", str);
			/* Print finishing quote */
			printf("\"");
			str = strctok(NULL, '\t');
		}
		printf("\n");
	}
	return(0);
}

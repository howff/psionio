/* > csv2tsv.c
 * 1.01 arb Thu Aug  8 10:10:03 BST 2002 - bigger LINESIZE
 * 1.00 arb Wed May 23 12:12:23 BST 2001
 */
static char SCCSid[] = "@(#)csv2tsv.c     1.01 (C) 2001 arb CSV to TSV";

/*
 * Bugs:
 *
 * Not sure of the proper way of quoting a double quote character in a string.
 *
 * Can become confused by malformed CSV files, eg. those with unmatched quotes.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "psionio.h"

char *prog;
char *usage = "usage: %s\nCSV to standard input, TSV to standard output\n";
char *options="";

#define LINESIZE 8192

int
main(int argc, char *argv[])
{
	FILE *fpin = stdin, *fpout = stdout;
	char line[LINESIZE];
	int linelen, linecount, fieldcount, newfield, quotechar, endquote, quotedfield;
	char *ptr;

	prog = argv[0];
	if (argc != 1)
	{
		fprintf(stderr, usage, prog);
		exit(1);
	}
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
		while (line[linelen-1] == A_CR || line[linelen-1] == A_LF)
			line[--linelen] = '\0';
		fieldcount = 0;  /* not used */
		newfield = 1;    /* set when next character is start of new field */
		quotechar = 0;   /* set when next character is quoted */
		endquote = 0;    /* set when next char is comma field separator */
		quotedfield = 0; /* set if field had a start quote */
		for (ptr = line; *ptr; ptr++)
		{
			/* Skip " at start of field */
			if (newfield && *ptr == '"') { newfield = 0; quotedfield = 1; continue; }
			
			/* Remember whether field started with a quote or not */
			if (newfield) { newfield = 0; quotedfield = 0; }

			/* Skip \ quotechar */
			if (!quotechar && *ptr == '\\') { quotechar = 1; continue; }

			/* Output quoted character */
			if (quotechar) { putchar(*ptr); quotechar = 0; continue; }

			/* Skip end quote */
			if (!endquote && *ptr == '"') { endquote = 1; continue; }

			/* Field may not be quoted */
			if (!quotedfield && *ptr == ',') { endquote=1; }

			/* Field separator becomes tab */
			if (endquote && *ptr == ',') { *ptr = '\t'; endquote = 0; newfield = 1; }
			putchar(*ptr);
		}
		printf("\n");
	}
	return(0);
}

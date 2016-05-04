/* > tsv2spr.c Copyright 1997 Andrew Brooks <arb@sat.dundee.ac.uk>
 * 1.01 arb Wed Sep  8 10:27:21 BST 1999 - bug fix recognising numbers/strings.
 * 1.00 arb Mon Mar 17 12:14:33 GMT 1997
 * Reads TSV from stdin and write SPR to stdout.
 * Usage: tsv2spr [-h]
 *  -h : add default descriptor for common options like tab size, etc. as well
 *       as the field labels (not yet implemented).
 */

static char SCCSid[] = "@(#)tsv2spr.c     1.01 (C) 1997 arb TSV to SPR";

/*
 * Configuration
 * Define cell font, or use -1 for default.
 */
#define SPR_CELL_FONT -1

/*
 * Bugs:
 */

/*
 * Notes:
 * Limited to number (int/float) and string cell types.
 * Empty cells are not written to save space, but they could be written
 * as explicitly empty cells.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include "psionio.h"

/* Program limits */
#define MAXLINE 4096

/* Types */
typedef enum { cell_empty, cell_int, cell_float, cell_string } celltype_t;

/* Global variables */
char *prog;
char *usage = "usage: %s [-h] [-v]\nInput on stdin, output to stdout.\n-h\tadds defaults header (wrap, tab size etc) - NYI\n-v\tverbose\n";
char *options="hv";
int verbose=0;


/*
 * Calculate the size of a cell given its type (and optional string value).
 * The font is 0 to 3 (or -1 if no font byte is required).
 * Length of strings is truncated to 255 to fit into a qstr.
 */
int
SPR_CellSize(celltype_t celltype, int font, char *str)
{
	int size = 0;
	int len = 0;

	if (str)
	{
		len = strlen(str);
		if (len > 255)
			len = 255;
	}
	switch (celltype)
	{
		case cell_empty:  size = 6; break;
		case cell_int:    size = 6 + sizeof(Word); break;
		case cell_float:  size = 6 + sizeof(Real); break;
		case cell_string: size = 6 + 1 + len; break;
		default: fprintf(stderr,"%s: error: unknown cell type\n",prog);exit(1);
	}
	if (font >= 0)
		size++;
	return(size);
}


/*
 * Write SPR file header
 */
int
SPR_WriteFileHeader(FILE *fp)
{
	writecstr(fp, SPR_IDSTRING);
	writelong(fp, 0); /* IDSTRING padding */
	writeword(fp, SPR_VERCREAT);
	writeword(fp, SPR_UKOFFSET);
	writeword(fp, SPR_VEROPL);
	return(0);
}


/*
 * Write a cell reference.
 * XXX At the moment only handles direct cell references, not relative
 * etc. as would be required for formulas.
 */
int
SPR_writecellref(FILE *fp, int row, int col)
{
	if (row < 0 || row > SPR_MAXROW)
	{
		fprintf(stderr, "%s: row number %d is out of range\n", prog, row);
		return(-1); /* or exit? */
	}
	if (col < 0 || col > SPR_MAXCOL)
	{
		fprintf(stderr, "%s: column number %d is out of range\n", prog, row);
		return(-1); /* or exit? */
	}
	writeword(fp, col);
	writeword(fp, row);
	return(0);
}

int
SPR_writerangeref(FILE *fp, int left, int top, int right, int bottom)
{
	int rc;
	rc = SPR_writecellref(fp, left, top);
	if (rc == 0) rc = SPR_writecellref(fp, right, bottom);
	return(rc);
}


/*
 * Write SPR file descriptor records describing preferences etc
 */
int
SPR_WriteHeaderRecords(FILE *fp)
{
	fprintf(stderr,"%s: Writing of default header not yet implemented, sorry.\n", prog);
	return(0);
}


/*
 * Read TSV records and write to SPR file
 */
int
SPR_WriteRecords(FILE *fpin, FILE *fpout)
{
	char line[MAXLINE];
	char *ptr, *endstr;
	unsigned char cell_contents, cell_format;
	double num;
	Word word;
	char sep = '\t';
	int size;
	int row = -1, column = 0;
	celltype_t cell;

	while (fgets(line, MAXLINE, fpin))
	{
		line[strlen(line)-1] = '\0';
		column = 0;
		row++;
		/* write cells */
		ptr=strctok(line, sep);
		while (ptr)
		{
			if (strlen(ptr) == 0)
				cell = cell_empty;
			else
			{
				num = strtod(ptr, &endstr);
				if (endstr == ptr || *endstr != '\0')
					cell = cell_string;
				else
				{
					if ((fabs(num) == num) && (fabs(num) < SPR_MAXINT))
					{
						word = num;
						cell = cell_int;
					}
					else
						cell = cell_float;
				}
			}
			if (cell == cell_empty)
			{
				column++;
				ptr=strctok(NULL, sep);
				continue;
			}
			size = SPR_CellSize(cell, SPR_CELL_FONT, ptr);
			writeword(fpout, SPR_RECORD_CELL);
			writeword(fpout, size);
			if (verbose) fprintf(stderr, "string '%s' is %d bytes, size is %d\n",ptr,strlen(ptr), size);
			SPR_writecellref(fpout, row, column);
			cell_contents = SPR_TEXT_ALIGNLEFT + SPR_NUMBER_ALIGNRIGHT;
			cell_format = SPR_CELLFORMAT_GENERAL + SPR_CELLFORMAT_SPECIAL;
			switch (cell)
			{
				case cell_empty:
					/* blank cells not written, see above */
					cell_contents |= SPR_CELL_BLANK;
					writebyte(fpout, cell_contents);
					writebyte(fpout, cell_format);
					break;
				case cell_int:
					cell_contents |= SPR_CELL_WORD;
					writebyte(fpout, cell_contents);
					writebyte(fpout, cell_format);
					writeword(fpout, word);
					if (SPR_CELL_FONT >= 0) writebyte(fpout, SPR_CELL_FONT);
					break;
				case cell_float:
					cell_contents |= SPR_CELL_REAL;
					writebyte(fpout, cell_contents);
					writebyte(fpout, cell_format);
					writereal(fpout, num);
					if (SPR_CELL_FONT >= 0) writebyte(fpout, SPR_CELL_FONT);
					break;
				case cell_string:
					cell_contents |= SPR_CELL_TEXT;
					writebyte(fpout, cell_contents);
					writebyte(fpout, cell_format);
					writeqstr(fpout, ptr);
					if (SPR_CELL_FONT >= 0) writebyte(fpout, SPR_CELL_FONT);
					break;
				default: /* cannot happen */
					break;
			}
			column++;
			ptr=strctok(NULL, sep);
		}
	}
	return(0);
}


/*
 * Main
 */
int
main(int argc, char *argv[])
{
	FILE *fpin = stdin;
	FILE *fpout = stdout;
	int c;
	int header = 0;

#ifdef PC /* Edwin.Klement@Dresdner-Bank.com */
	setmode(fileno(fpout), O_BINARY);
#endif
	prog = argv[0];
	GETOPT(c,options)
	{
		case 'h': header = 1; break;
		case 'v': verbose = 1; break;
		case '?': fprintf(stderr,usage,prog); exit(1);
	}
	SPR_WriteFileHeader(fpout);
	if (header)
		SPR_WriteHeaderRecords(fpout);
	SPR_WriteRecords(fpin, fpout);
	fclose(fpin);
	fclose(fpout);
	return(0);
}

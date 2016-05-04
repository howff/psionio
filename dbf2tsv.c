/* > dbf2tsv.c Copyright 1996 Andrew Brooks <arb@sat.dundee.ac.uk>
 * 1.04 arb Mon Aug 25 13:00:31 BST 1997 - detect 'compatible' databases; long
 *      strings are only allowed in databases containing all string fields.
 * 1.03 arb Thu Mar 20 14:29:28 GMT 1997 - support records with > 32 fields,
 *      plus undocumented extra bytes in header and footer text sub-records.
 *      Thanks to Edwin Klement <34e6b7@dresdner-bank.de>
 * 1.02 arb Mon Sep 30 13:51:10 BST 1996 - support continued qstr fields.
 * 1.01 arb Mon Sep 16 11:16:08 BST 1996 - added verbose and debug options.
 * 1.00 arb Sun Sep 15 15:36:28 BST 1996 - 
 * Extract DBF contents and write to stdout in TSV format.
 * Usage: dbf2tsv [-v] [-d] file.dbf
 *  -v : verbose (displays most useful records)
 *  -d : debug (displays all records with offsets and lengths)
 *       note that offsets shown are for start of data (record starts at n-2)
 */

static char SCCSid[] = "@(#)dbf2tsv.c     1.04 (C) 1996 arb DBF to TSV";

/*
 * Bugs:
 */

/*
 * To do:
 * Forced line feeds in qstr fields.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "psionio.h"

/* Global variables */
static char *prog;
static char *usage = "usage: %s [-v] [-d] file.dbf\n";
static int verbose = 0;
static int debug = 0;
static int fieldtypes[32];
static int numfields;
static int compatible = 0;

#define SkipBytes(F,N) { int n=N; while(n--) getc(F); }

/*
 * Read Database file header
 */
int
DBF_ReadFileHeader(FILE *fp)
{
	char *id;
	Word hdrsize;

	id = readcstr(fp);
	if (strcmp(id, DBF_IDSTRING))
	{
		fprintf(stderr,"%s: not a database file\n",prog);
		exit(1);
	}
	free(id);
	readword(fp);
	hdrsize = readword(fp);
	readword(fp);
	hdrsize -= 22;
	while (hdrsize--) getc(fp);
	return(0);
}


/*
 * Display field information, ie. field types
 * Set 'compatible' flag if database contains only string fields.
 */
int
DBF_DisplayFieldInfo(FILE *fpin, int recsize)
{
	int c;
	int i = 0;
	int allstrings = 1;

	while (recsize>0)
	{
		c = getc(fpin);
		if (c==EOF) break;
		fieldtypes[i++] = c;
		recsize--;
	}
	numfields = i;
	for (i=0; i<numfields; i++)
	{
		if (verbose) fprintf(stderr, "Field %2d is of type %s\n", i, DBF_FieldTypeStr[fieldtypes[i]]);
		if (fieldtypes[i] != DBF_FIELD_QSTR)
			allstrings = 0;
	}
	if (allstrings)
	{
		compatible = 1;
		if (debug) fprintf(stderr, "File is compatible with Data application.\n");
	}
	return(0);
}


/*
 * Display print setup record (inside wrd/dbf files)
 */
int
DBF_DisplayPrintSetupRecord(FILE *fpin, int recsize)
{
	Byte b;
	Word w;
	Long l;

	if (verbose) fprintf(stderr, "Printer setup: unknown\n");
	/* Always 58 bytes */
	w = readword(fpin); /* page width */
	w = readword(fpin); /* page height */
	w = readword(fpin); /* left margin */
	w = readword(fpin); /* top margin */
	w = readword(fpin); /* width of printing area */
	w = readword(fpin); /* height of printing area */
	w = readword(fpin); /* header offset */
	w = readword(fpin); /* footer offset */
	w = readword(fpin); /* paper orientation */
	w = readword(fpin); /* unknown */
	w = readword(fpin); /* first page to print */
	w = readword(fpin); /* last page to print */
	w = readword(fpin); /* header font */
	b = readbyte(fpin); /* header style */
	b = readbyte(fpin); /* unused */
	w = readword(fpin); /* header font size */
	b = readbyte(fpin); /* header alignment */
	b = readbyte(fpin); /* header on first page */
	w = readword(fpin); /* footer font */
	b = readbyte(fpin); /* footer style */
	b = readbyte(fpin); /* unused */
	w = readword(fpin); /* footer font size */
	b = readbyte(fpin); /* footer alignment */
	b = readbyte(fpin); /* footer on first page */
	w = readword(fpin); /* page number of first page */
	w = readword(fpin); /* number of pages */
	w = readword(fpin); /* page number style */
	w = readword(fpin); /* base font */
	b = readbyte(fpin); /* base style */
	b = readbyte(fpin); /* unused */
	w = readword(fpin); /* base font size */
	b = readbyte(fpin); /* paper size */
	b = readbyte(fpin); /* widows/orphans allowed */
	l = readlong(fpin); /* unused */
	return(0);
}


/*
 * Display descriptor sub-records
 */
int
DBF_DisplayDescriptorRecord(FILE *fpin, int recsize)
{
	Byte b;
	Word w;
	char *s;
	int subtype, subsize, tmp;

	/*fprintf(stderr,"Skipping Descriptor record\n");*/
	/*SkipBytes(fpin, recsize);*/
	while (recsize)
	{
		readrecordtype(fpin, &subsize, &subtype);
		recsize -= 2;
		if (feof(fpin)) break;
		switch (subtype)
		{
			case DBF_SUBRECORD_TABSIZE:
				w = readword(fpin);
				if (verbose) fprintf(stderr, "Tabsize: %d\n", w);
				break;
			case DBF_SUBRECORD_FIELDLABELS:
				tmp = subsize;
				while (subsize)
				{
					s = readqstr(fpin);
					subsize -= strlen(s)+1;
					if (verbose) fprintf(stderr, "Field label: %s\n", s);
					free(s);
				}
				subsize = tmp;
				break;
			case DBF_SUBRECORD_DISPLAY:
				b = readbyte(fpin);
				if (verbose)
				{
					fprintf(stderr, "Wrap is %s\n", b&2 ? "on" : "off");
					fprintf(stderr, "Labels are %svisible\n", b&4 ? "" : "in");
					fprintf(stderr, "Status window: %d\n", (b&24)>>3);
					fprintf(stderr, "Zoom level: %d\n", (b&96)>>5);
				}
				b = readbyte(fpin);
				break;
			case DBF_SUBRECORD_PRINTER:
				DBF_DisplayPrintSetupRecord(fpin, subsize);
				break;
			case DBF_SUBRECORD_PRINTERDRIVER:
				b = readbyte(fpin);
				s = readcstr(fpin);
				if (verbose) fprintf(stderr, "Printer driver: %s model %d\n", s, b);
				free(s);
				break;
			case DBF_SUBRECORD_HEADER:
				s = readcstr(fpin);
				if (verbose) fprintf(stderr, "Header text: %s\n", s);
				/* Extra bytes in header record? (EK) */
				tmp = strlen(s) + 1;
				if (tmp < subsize)
					SkipBytes(fpin, subsize - tmp);
				free(s);
				break;
			case DBF_SUBRECORD_FOOTER:
				s = readcstr(fpin);
				if (verbose) fprintf(stderr, "Footer text: %s\n", s);
				/* Extra bytes in footer record? (EK) */
				tmp = strlen(s) + 1;
				if (tmp < subsize)
					SkipBytes(fpin, subsize - tmp);
				free(s);
				break;
			case DBF_SUBRECORD_DIAMOND:
				b = readbyte(fpin);
				if (verbose) fprintf(stderr, "Diamond list: Find %sabled\n", b?"en":"dis");
				b = readbyte(fpin);
				if (verbose) fprintf(stderr, "Diamond list: Change %sabled\n", b?"en":"dis");
				b = readbyte(fpin);
				if (verbose) fprintf(stderr, "Diamond list: Add %sabled\n", b?"en":"dis");
				break;
			case DBF_SUBRECORD_SEARCH:
				w = readword(fpin);
				if (verbose) fprintf(stderr,"Search start field: %d\n", w);
				w = readword(fpin);
				if (verbose) fprintf(stderr,"Search end field: %d\n", w);
				break;
			case DBF_SUBRECORD_AGENDA:
			default:
				SkipBytes(fpin, subsize);
				break;
		}
		recsize -= subsize;
	}
	return(0);
}


/*
 * Display a record
 */
int
DBF_DisplayRecord(FILE *fpin, int recsize)
{
	int i;
	Word w;
	Long l;
	Real r;
	char *q;
	FILE *fpout = stdout;

	for (i=0; i<numfields; i++)
	{
		if (recsize == 0) break;
		if (i && (fieldtypes[i]!=DBF_FIELD_QSTR)) fprintf(fpout, "\t");
		switch (fieldtypes[i])
		{
			case DBF_FIELD_WORD:
				w = readword(fpin);
				fprintf(fpout, "%d", w);
				recsize -= DBF_SIZE_WORD;
				break;
			case DBF_FIELD_LONG:
				l = readlong(fpin);
				fprintf(fpout, "%ld", l);
				recsize -= DBF_SIZE_LONG;
				break;
			case DBF_FIELD_REAL:
				r = readreal(fpin);
				fprintf(fpout, "%f", r);
				recsize -= DBF_SIZE_REAL;
				break;
			case DBF_FIELD_QSTR:
				q = readqstr(fpin);
				if (i && (!compatible || (*q != DBF_FIELD_QSTR_CONTINUATION)))
					fprintf(fpout, "\t");
				fprintf(fpout, "%s", (compatible && (*q==DBF_FIELD_QSTR_CONTINUATION)) ? q+1 : q);
				recsize -= strlen(q)+1;
				free(q);
				break;
		}
	}
	/* Any extra fields are qstr (EK) */
	while (recsize > 0)
	{
		q = readqstr(fpin);
		if (i && (!compatible || (*q != DBF_FIELD_QSTR_CONTINUATION)))
			fprintf(fpout, "\t");
		fprintf(fpout, "%s", (compatible && (*q==DBF_FIELD_QSTR_CONTINUATION)) ? q+1 : q);
		recsize -= strlen(q)+1;
		free(q);
	}
	fprintf(fpout, "\n");
	return(0);
}


/*
 * Read database
 */
int
DBF_ReadDatabase(FILE *fpin, FILE *fpout)
{
	int recsize, rectype;

	DBF_ReadFileHeader(fpin);
	while (!feof(fpin))
	{
		/*fprintf(stderr,"0x%04lx\t", ftell(fpin));*/
		readrecordtype(fpin, &recsize, &rectype);
		if (feof(fpin)) break;
		switch (rectype)
		{
			case DBF_RECORD_FIELDINFO:
				if (debug) fprintf(stderr,"Found field info record, type %d, length %d at %lx\n",rectype,recsize,ftell(fpin));
				DBF_DisplayFieldInfo(fpin, recsize);
				break;
			case DBF_RECORD_DESCRIPTOR:
				if (debug) fprintf(stderr,"Found descriptor record, type %d, length %d at %lx\n",rectype,recsize,ftell(fpin));
				DBF_DisplayDescriptorRecord(fpin, recsize);
				break;
			case DBF_RECORD_VOICEDATA:
				if (debug) fprintf(stderr,"Skipping voice data record, type %d, length %d at %lx\n",rectype,recsize,ftell(fpin));
				SkipBytes(fpin,recsize);
				break;
			case DBF_RECORD_RESERVED:
				if (debug) fprintf(stderr,"Skipping reserved record, type %d, length %d at %lx\n",rectype,recsize,ftell(fpin));
				SkipBytes(fpin,recsize);
				break;
			case DBF_RECORD_PRIVATESTART:    /* total of 4 private types */
			case DBF_RECORD_PRIVATESTART+1:
			case DBF_RECORD_PRIVATESTART+2:
			case DBF_RECORD_PRIVATEEND:
				if (debug) fprintf(stderr,"Skipping private record type %d, length %d at %lx\n",rectype,recsize,ftell(fpin));
				SkipBytes(fpin,recsize);
				break;
			case DBF_RECORD_DELETED:
				if (debug) fprintf(stderr,"Skipping deleted record, type %d, length %d at %lx\n",rectype,recsize,ftell(fpin));
				SkipBytes(fpin,recsize);
				break;
			case DBF_RECORD_DATA:
			default:
				if (debug) fprintf(stderr,"Found data record, type %d, length %d at %lx\n",rectype,recsize,ftell(fpin));
				/*SkipBytes(fpin,recsize);*/
				DBF_DisplayRecord(fpin, recsize);
				break;
		}
	}
	return(0);
}


int
main(int argc, char *argv[])
{
	char *dbffile = NULL;
	FILE *fpin;
	FILE *fpout = stdout;
	int i;

	prog = argv[0];
	for (i=1; i<argc && dbffile == NULL; i++)
	{
		if (!strcmp(argv[i], "-v")) verbose = 1;
		else if (!strcmp(argv[i], "-d")) debug = 1;
		else dbffile = argv[i];
	}
	if (dbffile == NULL) { fprintf(stderr,usage,prog); exit(1); }
	fpin = fopen(dbffile, "rb");
	if (fpin == NULL) { fprintf(stderr,"%s: cannot open %s\n",prog,dbffile); exit(1); }
	DBF_ReadDatabase(fpin, fpout);
	fclose(fpin);
	fclose(fpout);
	return(0);
}

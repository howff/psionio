/* > splitdbf.c
 * 1.00 arb
 * Split DBF file into parts for more reliable transfer
 * Usage: splitdbf [-s size NYI] file.dbf
 */

static char SCCSid[] = "@(#)splitdbf.c    1.00 (C) 1997 arb Split DBF";

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "psionio.h"

/*
 * Note:
 * Simply splits raw records (and writes a basic file header
 * on each part). No interpretation of records is done, so
 * things like field titles and setting will only appear in
 * one of the parts.
 */

#define K 1024

char *prog;

int
main(int argc, char *argv[])
{
	FILE *fpin;
	char *file;
	int partsize = 0;
	int recordsize = 0;
	int maxsize = 120 * K;

	prog = argv[0];
	file = argv[1];
	fpin = fopen(file, "r");
	if (fpin == NULL) exit(1);

	ReadHeader();
	while (1)
	{
		ReadRecord();
		if (recordsize==0) break;
		if (partsize + recordsize > maxsize)
		{
			NewPart();
			/*
			CloseFile
			OpenFile
			WriteHeader
			*/
		}
		WriteRecord();
	}
	return(0);
}

#if 0




/* Global variables */
static char *prog;
static char *usage = "usage: %s [-v] [-d] file.dbf\n";
static int verbose = 0;
static int debug = 0;
static int fieldtypes[32];
static int numfields;

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
 */
int
DBF_DisplayFieldInfo(FILE *fpin, int recsize)
{
	int c;
	int i = 0;

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
	}
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
				SkipBytes(fpin, subsize);
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
				fprintf(fpout, "%lf", r);
				recsize -= DBF_SIZE_REAL;
				break;
			case DBF_FIELD_QSTR:
				q = readqstr(fpin);
				if (i && (*q != DBF_FIELD_QSTR_CONTINUATION))
					fprintf(fpout, "\t");
				fprintf(fpout, "%s", *q==DBF_FIELD_QSTR_CONTINUATION ? q+1 : q);
				recsize -= strlen(q)+1;
				free(q);
				break;
		}
	}
	/* Any extra fields are qstr (EK) */
	while (recsize > 0)
	{
		q = readqstr(fpin);
		if (i && (*q != DBF_FIELD_QSTR_CONTINUATION))
			fprintf(fpout, "\t");
		fprintf(fpout, "%s", *q==DBF_FIELD_QSTR_CONTINUATION ? q+1 : q);
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
	fpin = fopen(dbffile, "r");
	if (fpin == NULL) { fprintf(stderr,"%s: cannot open %s\n",prog,dbffile); exit(1); }
	DBF_ReadDatabase(fpin, fpout);
	fclose(fpin);
	fclose(fpout);
	return(0);
}

#endif /* 0 */

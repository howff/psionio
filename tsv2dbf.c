/* > tsv2dbf.c Copyright 1996 Andrew Brooks <arb@sat.dundee.ac.uk>
 * 1.06 arb Mon Aug 25 13:12:28 BST 1997 - only write continuation strings in
 *      compatible mode (ie. if all fields are string type).
 * 1.05 arb Tue Jul  1 14:28:24 BST 1997 - set types for compatible mode
 * 1.04 arb Thu Jun 26 14:04:42 BST 1997 - write multiple data types; defs read
 *      from end of each field label.
 * 1.03 arb Wed Jun 25 11:18:12 BST 1997 - write continuation fields for long
 *      strings.
 * 1.02 arb Mon Sep 16 15:55:39 BST 1996 - bug fix: length of too-long strings
 * 1.01 arb Mon Sep 16 11:53:05 BST 1996 - added descriptor options
 * 1.00 arb Thu Aug 22 14:19:52 BST 1996
 * Reads TSV from stdin and write DBF to stdout.
 * First line of TSV is field name strings, unless -r is given.
 * Data type is in brackets immediately after each label name.
 * Usage: tsv2dbf [-c] [-l] [-h|-r]
 *  -c : compatibility mode (for Data application), writes numfields=32
 *       and ignores field type declarations (all fields are strings).
 *       Strings can be longer than 254 characters.
 *  -r : raw data, no descriptor added. No field labels are added to the DBF
 *       file and thus the first line of the input is not treated specially.
 *  -l : add descriptor giving field labels as read from first record of input.
 *       This is the default.
 *  -h : add default descriptor for common options like tab size, etc. as well
 *       as the field labels.
 */

static char SCCSid[] = "@(#)tsv2dbf.c     1.06 (C) 1996 arb TSV to DBF";

/*
 * Bugs:
 * Raw mode assumes 32 fields but should count number in first record.
 */

/*
 * Notes:
 * Writes all labels even if last few are empty.
 * Does not attempt to guess compatible mode because other database reading
 * programs may not expect continuation characters.  If you want that feature
 * though you may define DETECT_COMPATIBLE below.  Note that this auto-detection
 * only happens if you are reading a header (ie. not raw -r mode).
 */
/*#define DETECT_COMPATIBLE*/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include "psionio.h"

/* Program limits */
#define MAXLINE 4096
#define MAXFIELDS 32

/* Descriptor types: none added, just labels, or a full default descriptor */
typedef enum { Desc_None, Desc_Labels, Desc_Default } desc_t;

/* Data types (match DBF_FIELD_WORD etc definitions) */
typedef enum { Type_Word, Type_Long, Type_Real, Type_Qstr } type_t;

/* Data type options */
typedef struct typeopt_s
{
	char *opt;
	type_t datatype;
} typeopt_t;

/* Global variables */
char *prog;
char *usage = "usage: %s [-c] [-l] [-r|-h] [-v] [-d]\nInput on stdin, output to stdout.\n-l\tFirst record of input is field labels\n-c\tcompatibility mode (for Data application) - writes 32 fields\n-r\traw, no field labels on input, no header on output\n-h\tadds defaults header (wrap, tab size etc)\n-d\tdebug\n-v\tverbose\n";
char *options="chlrdv";
char *fieldlabel[MAXFIELDS];
type_t fieldtype[MAXFIELDS];
desc_t desc = Desc_Labels;
int compatible = 0;
int numfields = 32;   /* 32 assumed if raw (no labels read) */
int verbose = 0;
int debug = 0;
typeopt_t typeopt[] =
{
	"(Int)",     Type_Word,
	"(Short)",   Type_Word,
	"(Word)",    Type_Word,
	"(Long)",    Type_Long,
	"(Integer)", Type_Long,
	"(Real)",    Type_Real,
	"(Float)",   Type_Real,
	"(String)",  Type_Qstr
};
int numtypeopt = sizeof(typeopt)/sizeof(typeopt_t);
char *typename[] = { "Word", "Long", "Real", "String" };


/*
 * Set default fields from options in case ReadHeader is not called.
 */
int
SetDefaultFields()
{
	int i;

	/* Blank out labels and set field types to string */
	/* Set everything up for compatible mode (may be overridden later) */
	for (i=0; i<MAXFIELDS; i++)
	{
		fieldlabel[i] = "";
		fieldtype[i] = Type_Qstr;
	}
	return(0);
}


/*
 * Read TSV header
 */
int
TSV_ReadHeader(FILE *fp)
{
	char line[MAXLINE];
	char *ptr;
	char sep = '\t';
	int i;
	int lenopt, lenlabel;
	int allstrings = 1;

	fgets(line, MAXLINE, fp);
	line[strlen(line)-1] = '\0';
	numfields=0;
	ptr = strctok(line, sep);
	while (ptr)
	{
		lenlabel = strlen(ptr);
		fieldlabel[numfields] = strcopy(ptr);
		fieldtype[numfields] = Type_Qstr;
		if (!compatible) for (i=0; i<numtypeopt; i++)
		{
			lenopt = strlen(typeopt[i].opt);
			/*fprintf(stderr,"Checking %s for %s at %s\n", ptr, typeopt[i].opt, lenopt<lenlabel?ptr+lenlabel-lenopt:ptr);*/
			if (strcmp(lenopt<lenlabel?ptr+lenlabel-lenopt:ptr, typeopt[i].opt)==0)
			{
				fieldtype[numfields] = typeopt[i].datatype;
				fieldlabel[numfields][lenopt<lenlabel?lenlabel-lenopt:0] = '\0';
				break;
			}
		}
		if (fieldtype[numfields] != Type_Qstr)
			allstrings = 0;
		numfields++;
		if (verbose) fprintf(stderr, "Field %2d: type %s, label %s\n", numfields, typename[fieldtype[numfields-1]], fieldlabel[numfields-1]);
		ptr = strctok(NULL, sep);
	}
#ifdef DETECT_COMPATIBLE
	/* You may not have said compatible explicitly but that's what you get */
	if (allstrings)
	{
		compatible = 1;
		if (debug) fprintf(stderr, "Detected compatible mode (all string fields).\n");
	}
#endif /* DETECT_COMPATIBLE */
	/* In compatible mode there are 32 fields, but labels are empty */
	while (compatible && numfields<32)
	{
		fieldlabel[numfields++] = "";
	}
	return(0);
}


/*
 * Write DBF file header
 */
int
DBF_WriteFileHeader(FILE *fp)
{
	writecstr(fp, DBF_IDSTRING);
	writeword(fp, DBF_VERCREAT);
	writeword(fp, 22);     /* Size of this header */
	writeword(fp, DBF_VERREAD);
	return(0);
}


/*
 * Write DBF file information record describing field types
 */
int
DBF_WriteFieldInfoRecord(FILE *fp)
{
	int i;
	/* describes type of each field */
	/* currently all fields are of type qstr */
	writerecordtype(fp, numfields, DBF_RECORD_FIELDINFO);
	for (i=0; i<numfields; i++)
		writebyte(fp, fieldtype[i]);
	return(0);
}


/*
 * Write DBF file descriptor record describing field names, preferences etc
 * Set desc to Desc_Default to use default descriptor (sets tab size, etc.)
 * or to Desc_Labels to just write labels.  Don't bother calling this func
 * if desc is Desc_None.
 */
int
DBF_WriteDescriptorRecord(FILE *fp, desc_t desc)
{
	int i, labelsize=0, size=0;

	if (desc == Desc_None) return(0);
	/* describes names (labels) of fields */
	/* find size of labels sub-record */
	for (i=0; i<numfields; i++)
		labelsize+=strlen(fieldlabel[i])+1;
	size = labelsize + 2; /* add size of sub-record header */
	/* Add size of default descriptor */
	if (desc == Desc_Default) size += DBF_SizeDefaultDescriptor;
	/* Write record header */
	writerecordtype(fp, size, DBF_RECORD_DESCRIPTOR);
	/* Write default sub-records */
	if (desc == Desc_Default) fwrite(DBF_DefaultDescriptor, DBF_SizeDefaultDescriptor, 1, fp);
	/* Write field label sub-record */
	writerecordtype(fp, labelsize, DBF_SUBRECORD_FIELDLABELS);
	/* We are allowed to skip last few labels if they are empty but we don't */
	for (i=0; i<numfields; i++)
		writeqstr(fp, fieldlabel[i]);
	return(0);
}


/*
 * Read TSV records and write to DBF file
 */
int
DBF_WriteRecords(FILE *fpin, FILE *fpout)
{
	char line[MAXLINE];
	char *ptr;
	char sep = '\t';
	char *copy;
	int size, len;
	int record=0;
	int fieldnum = 0;

	while (fgets(line, MAXLINE, fpin))
	{
		record++;
		line[strlen(line)-1] = '\0';
		size=0;
		/* take a copy incase strtok obliterates contents */
		copy = strcopy(line);
		/* find record size=sum of field lengths */
		ptr = strctok(line, sep);
		fieldnum = 0;
		while (ptr)
		{
			switch (fieldtype[fieldnum++])
			{
				case Type_Word: size += 2; break;
				case Type_Long: size += 4; break;
				case Type_Real: size += 8; break;
				default: /**/
				case Type_Qstr:
					len = strlen(ptr);
					if (len > DBF_MAXQSTRSIZE)
					{
						if (compatible)
							/* Add size of continuation chars and size bytes */
							len += 2 * ((len-1) / DBF_MAXQSTRSIZE);
						else
							/* String is truncated if not in compatible mode */
							len = DBF_MAXQSTRSIZE;
					}
					size += len+1;
					break;
			}
			ptr = strctok(NULL, sep);
		}
		if (size > DBF_MAXRECORDSIZE)
		{
			fprintf(stderr,"Warning: record %d is too large, not written\n",record);
			free(copy);
			continue;
		}
		writerecordtype(fpout, size, DBF_RECORD_DATA);
		/* write fields */
		ptr = strctok(copy, sep);
		fieldnum = 0;
		while (ptr)
		{
			switch (fieldtype[fieldnum++])
			{
				case Type_Word: writeword(fpout, atoi(ptr)); break;
				case Type_Long: writelong(fpout, atoi(ptr)); break;
				case Type_Real: writereal(fpout, atof(ptr)); break;
				default: /**/
				case Type_Qstr:
					if (compatible)
						writemultiqstr(fpout, ptr);
					else
						if (writeqstr(fpout, ptr))
							fprintf(stderr, "Warning: a field in record %d is incomplete.\n", record);
					break;
			}
			ptr = strctok(NULL, sep);
		}
		free(copy);
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

#ifdef PC /* Edwin.Klement@Dresdner-Bank.com */
	setmode(fileno(fpout), O_BINARY);
#endif
	prog = argv[0];
	GETOPT(c,options)
	{
		case 'c': compatible = 1; break;
		case 'h': desc = Desc_Default; break;
		case 'l': desc = Desc_Labels; break;
		case 'r': desc = Desc_None; break;
		case 'd': debug = 1; break;
		case 'v': verbose = 1; break;
		case '?': fprintf(stderr,usage,prog); exit(1);
	}
	SetDefaultFields();
	if (desc!=Desc_None)
		TSV_ReadHeader(fpin);
	DBF_WriteFileHeader(fpout);
	DBF_WriteFieldInfoRecord(fpout);
	if (desc!=Desc_None)
		DBF_WriteDescriptorRecord(fpout, desc);
	DBF_WriteRecords(fpin, fpout);
	fclose(fpin);
	fclose(fpout);
	return(0);
}

/* > psionio.c Copyright 1996 Andrew Brooks <arb@sat.dundee.ac.uk>
 * 1.14 arb Tue Mar 15 09:40:11 GMT 2005 - fixes to do_char_conversions.
 * 1.13 arb Tue Apr  2 15:53:20 BST 2002 - added do_char_conversions().
 * 1.12 arb Sun Jan 27 16:38:27 GMT 2002 - bug fix encoding chars in html.
 * 1.11 arb Sat Jun 10 16:32:46 BST 2000 - bug fix in iso2html, static enc.
 * 1.10 arb Mon Oct  4 10:40:47 BST 1999 - added ISO8859-1 to HTML encodings.
 * 1.09 arb Fri Aug  7 12:09:28 BST 1998 - added decrypt_block and cryptkey.
 * 1.08 arb Wed Jul 29 15:50:24 BST 1998 - added _block version of conv funcs.
 * 1.07 arb Mon Aug 25 14:17:15 BST 1997 - writeqstr only warns on truncate if
 *      not being called from writemultiqstr.
 * 1.06 arb Wed Jul 23 11:01:09 BST 1997 - added stradd function.
 * 1.05 arb Wed Jun 25 10:52:37 BST 1997 - fix max qstr length, remove
 *      restriction on cstr length, allow writing of longer qstr fields
 *      by writing multiple records, some tidying.
 * 1.04 arb Fri Mar 21 18:28:24 GMT 1997 - added daytodate and character set
 *      conversion functions (optional)
 * 1.03 arb Thu Mar 20 14:38:48 GMT 1997 - added byte ordering support
 * 1.02 arb Tue Mar 18 14:36:57 GMT 1997 - added readreal and writereal
 * 1.01 arb Sun Sep 15 16:25:09 BST 1996 - *unsigned* byte read in readqstr
 * 1.00 arb Fri Aug 23 10:21:52 BST 1996
 */

static char SCCSid[] = "@(#)psionio.c     1.14 (C) 1996 arb Psion I/O";

/*
 * Bugs:
 */

/*
 * To do:
 * Implement cryptkey (need GenMaskInit code)
 */

/*
 * Configuration:
 * 1. Define byte sex below or in Makefile: _LITTLE_ENDIAN or _BIG_ENDIAN
 * 2. Define whether you want characters converted between ISO-8859-1 and Code
 * Page 850 below or in Makefile: CONVERTCHARS
 * 3. Define TRUNCATE_CSTR if cstrings should be truncated to 254 characters
 * in the same way as qstrings. (Should not be necessary as length restriction
 * is imposed by single byte to store length before qstrs).
 */
/*#define TRUNCATE_CSTR*/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "psionio.h"


/*
 * Check byte ordering
 */
#if defined SUN || defined LINUX_SPARC
#ifndef _BIG_ENDIAN
#define _BIG_ENDIAN
#endif
#endif /* SUN */
#if defined PC || defined LINUX_PC
#ifndef _LITTLE_ENDIAN
#define _LITTLE_ENDIAN
#endif
#endif /* PC */
#ifdef ACORN
#ifndef _LITTLE_ENDIAN
#define _LITTLE_ENDIAN
#endif
#endif /* ACORN */
#ifndef _LITTLE_ENDIAN
#ifndef _BIG_ENDIAN
#error You must define either SUN, PC, LINUX_SPARC, LINUX_PC, ACORN or _LITTLE_ENDIAN / _BIG_ENDIAN in Makefile
#endif
#endif


/*
 * For displaying field types
 */
char *DBF_FieldTypeStr[] =
{
	"Word",
	"Long",
	"Real",
	"QStr"
};

/*
 * Default descriptor record:
 * Tabsize: 4
 * Wrap is on
 * Labels are visible
 * Status window: 2
 * Zoom level: 0
 * Printer setup: unknown
 * Printer driver: ROM::PSAPPLE.WDR model 1
 * Header text: ""
 * Footer text: "%P"
 * Diamond list: Find enabled, Change disabled, Add enabled
 * Search start field: 0
 * Search end field: -1 (ie. all)
 */

unsigned char DBF_DefaultDescriptor[] =
{
	0x02, 0x10, 0x04, 0x00, 0x02, 0x50, 0x16, 0x00, 0x3a, /*: otes:.0.....P..:*/
	0x60, 0x82, 0x2e, 0xc6, 0x41, 0x08, 0x07, 0x08, 0x07, 0x72, 0x20, 0xb6, 0x33, 0xd0, 0x02, 0xd0, /*: `...A....r .3...*/
	0x02, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x00, 0x00, /*: ................*/
	0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /*: ................*/
	0x00, 0x00, 0x00, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x12, 0x70, 0x01, 0x52, 0x4f, /*: ............p.RO*/
	0x4d, 0x3a, 0x3a, 0x50, 0x53, 0x41, 0x50, 0x50, 0x4c, 0x45, 0x2e, 0x57, 0x44, 0x52, 0x00, 0x01, /*: M::PSAPPLE.WDR..*/
	0x80, 0x00, 0x03, 0x90, 0x25, 0x50, 0x00, 0x03, 0xa0, 0x01, 0x00, 0x01, 0x04, 0xb0, 0x00, 0x00, /*: ....%P..........*/
	0xff, 0xff
	/*0x1a, 0x40, 0x06, 0x49, 0x73, 0x73, 0x75, 0x65, 0x3a, 0x05, 0x50, 0x61, 0x67, 0x65,*/ /*: ...@.Issue:.Page*/
	/*0x3a, 0x05, 0x43, 0x6f, 0x64, 0x65, 0x3a, 0x06, 0x54, 0x69, 0x74, 0x6c, 0x65, 0x3a*/  /*: :.Code:.Title:*/
};
int DBF_SizeDefaultDescriptor = sizeof(DBF_DefaultDescriptor)/sizeof(char);


/*
 * Two-way conversion between ISO8859-1 and cp850.
 * Only the upper 128 characters are affected.
 */
#ifdef CONVERTCHARS
static unsigned char tbl_iso2cp[128] =
{
        0x9f, 0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb9, 0xba,
        0xbb, 0xbc, 0xbf, 0xc0, 0xc1, 0xc2, 0xc3, 0xc4,
        0xc5, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce,
        0xd5, 0xd9, 0xda, 0xdb, 0xdc, 0xdf, 0xf2, 0xfe,
        0xff, 0xad, 0xbd, 0x9c, 0xcf, 0xbe, 0xdd, 0xf5,
        0xf9, 0xb8, 0xa6, 0xae, 0xaa, 0xf0, 0xa9, 0xee,
        0xf8, 0xf1, 0xfd, 0xfc, 0xef, 0xe6, 0xf4, 0xfa,
        0xf7, 0xfb, 0xa7, 0xaf, 0xac, 0xab, 0xf3, 0xa8,
        0xb7, 0xb5, 0xb6, 0xc7, 0x8e, 0x8f, 0x92, 0x80,
        0xd4, 0x90, 0xd2, 0xd3, 0xde, 0xd6, 0xd7, 0xd8,
        0xd1, 0xa5, 0xe3, 0xe0, 0xe2, 0xe5, 0x99, 0x9e,
        0x9d, 0xeb, 0xe9, 0xea, 0x9a, 0xed, 0xe7, 0xe1,
        0x85, 0xa0, 0x83, 0xc6, 0x84, 0x86, 0x91, 0x87,
        0x8a, 0x82, 0x88, 0x89, 0x8d, 0xa1, 0x8c, 0x8b,
        0xd0, 0xa4, 0x95, 0xa2, 0x93, 0xe4, 0x94, 0xf6,
        0x9b, 0x97, 0xa3, 0x96, 0x81, 0xec, 0xe8, 0x98
};

static unsigned char tbl_cp2iso[128] =
{
        0xc7, 0xfc, 0xe9, 0xe2, 0xe4, 0xe0, 0xe5, 0xe7,
        0xea, 0xeb, 0xe8, 0xef, 0xee, 0xec, 0xc4, 0xc5,
        0xc9, 0xe6, 0xc6, 0xf4, 0xf6, 0xf2, 0xfb, 0xf9,
        0xff, 0xd6, 0xdc, 0xf8, 0xa3, 0xd8, 0xd7, 0x80,
        0xe1, 0xed, 0xf3, 0xfa, 0xf1, 0xd1, 0xaa, 0xba,
        0xbf, 0xae, 0xac, 0xbd, 0xbc, 0xa1, 0xab, 0xbb,
        0x81, 0x82, 0x83, 0x84, 0x85, 0xc1, 0xc2, 0xc0,
        0xa9, 0x86, 0x87, 0x88, 0x89, 0xa2, 0xa5, 0x8a,
        0x8b, 0x8c, 0x8d, 0x8e, 0x8f, 0x90, 0xe3, 0xc3,
        0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0xa4,
        0xf0, 0xd0, 0xca, 0xcb, 0xc8, 0x98, 0xcd, 0xce,
        0xcf, 0x99, 0x9a, 0x9b, 0x9c, 0xa6, 0xcc, 0x9d,
        0xd3, 0xdf, 0xd4, 0xd2, 0xf5, 0xd5, 0xb5, 0xde,
        0xfe, 0xda, 0xdb, 0xd9, 0xfd, 0xdd, 0xaf, 0xb4,
        0xad, 0xb1, 0x9e, 0xbe, 0xb6, 0xa7, 0xf7, 0xb8,
        0xb0, 0xa8, 0xb7, 0xb9, 0xb3, 0xb2, 0x9f, 0xa0
};

static int env_convertchars = -1;    /* initially undefined */

int
do_char_conversions()
{
	char *env;

	/* If value known then return early */
	if (env_convertchars != -1)
		return(env_convertchars);

	/* Read environment variable */
	env = getenv("PSIONIO_CONVERTCHARS");
	/* If unset then convert characters */
	/* If set to non-zero then convert, if set to zero then don't */
	if (env == NULL)
		env_convertchars = 1;
	else
		env_convertchars = atoi(env);
	return(env_convertchars);
}

char *
cp2iso_block(char *str, int len)
{
	unsigned char *s = (unsigned char *) str;
	int i;

	if (!do_char_conversions())
		return(str);
	for (i=0; i<len; i++, s++)
		if (*s >= 128) *s = tbl_cp2iso[*s - 128];
	return(str);
}

char *
cp2iso(char *str)
{
	return cp2iso_block(str, strlen(str));
}

char *
iso2cp_block(char *str, int len)
{
	unsigned char *s = (unsigned char *) str;
	int i;

	if (!do_char_conversions())
		return(str);
	for (i=0; i<len; i++, s++)
		if (*s >= 128) *s = tbl_iso2cp[*s - 128];
	return(str);
}

char *
iso2cp(char *str)
{
	return iso2cp_block(str, strlen(str));
}
#endif /* CONVERTCHARS */


/*
 * Convert characters to their HTML encodings.
 */
static char * tbl_iso2html[96] =
{
	"nbsp", "iexcl", "cent", "pound", "curren", "yen", "brvbar", "sect", "uml",
	"copy", "ordf", "laquo", "not", "shy", "reg", "macr", "deg", "plusmn", "sup2",
	"sup3", "acute", "micro", "para", "middot", "cedil", "sup1", "ordm", "raquo",
	"frac14", "frac12", "frac34", "iquest", "Agrave", "Aacute", "Acirc", "Atilde",
	"Auml", "Aring", "AElig", "Ccedil", "Egrave", "Eacute", "Ecirc", "Euml",
	"Igrave", "Iacute", "Icirc", "Iuml", "ETH", "Ntilde", "Ograve", "Oacute",
	"Ocirc", "Otilde", "Ouml", "times", "Oslash", "Ugrave", "Uacute", "Ucirc",
	"Uuml", "Yacute", "THORN", "szlig", "agrave", "aacute", "acirc", "atilde",
	"auml", "aring", "aelig", "ccedil", "egrave", "eacute", "ecirc", "euml",
	"igrave", "iacute", "icirc", "iuml", "eth", "ntilde", "ograve", "oacute",
	"ocirc", "otilde", "ouml", "divide", "oslash", "ugrave", "uacute", "ucirc",
	"uuml", "yacute", "thorn", "yuml"
};

char *
iso2html(unsigned char c)
{
	static char enc[16];
	if (c >=160 && c <=255)
	{
		sprintf(enc, "&%s;", tbl_iso2html[c-160]);
		return(enc);
	}
	else if (c=='&') return("&amp;");
	else if (c=='<') return("&lt;");
	else if (c=='>') return("&gt;");
	else
	{
		enc[0] = c;
		enc[1] = '\0';
		return(enc);
	}
}


/*
 * Decryption functions
 */
int
decrypt_block(unsigned char *block, int len, unsigned char key[9])
{
	unsigned char fullkey[16];
	int i, c;

	for (i=0; i<16; i++)
		fullkey[i] = key[i % 9];
	for (i=0; i<len; i++)
	{
		c = block[i] + 0x100 - fullkey[i % 16];
		block[i] = c & 0xff; 
	}
	return(0);
}


#define HEX(c) (c<='9'?c-'0':c>='a'?c-'a'+10:c-'A'+10)

void
cryptkey(unsigned char *password, unsigned char key[])
{
	/* Should generate a key from a password but I don't know the algorithm */
	/* Instead just assumes password is a hex version of the key */
	/* 9 hex digits, ie. 18 characters */
	int i;
	if (strlen((char*)password) != 18)
	{
		fprintf(stderr, "error: password function not yet implemented.\n");
		fprintf(stderr, "argument should be a 9 hex digits with no spaces (ie. 18 characters)\n");
		exit(1);
	}
	for (i=0; i<9; i++)
		key[i] = HEX(password[i*2])*16 + HEX(password[i*2+1]);
}


/*
 * Output functions
 */
int writebyte(FILE *fp, char byte) { fputc(byte, fp); return(0); }
int writeword(FILE *fp, short word) { fputc(word & 0xff, fp); fputc((word >> 8) & 0xff, fp); return(0); }
int writelong(FILE *fp, long lng) { fputc(lng&0xff, fp); lng=(lng>>8)&0xffffff; fputc(lng&0xff, fp); lng=(lng>>8)&0xffff; fputc(lng&0xff, fp); lng=(lng>>8)&0xff; fputc(lng&0xff, fp); return(0); }
int writerecordtype(FILE *fp, int size, int type) { return writeword(fp, size | (type<<12)); }
#ifdef _BIG_ENDIAN
int writereal(FILE *fp, double real) { unsigned Byte *p = (unsigned Byte *)&real; int i; for (i=7; i>=0; i--) fputc(p[i], fp); return(0); }
#endif
#ifdef _LITTLE_ENDIAN
int writereal(FILE *fp, double real) { unsigned Byte *p = (unsigned Byte *)&real; int i; for (i=0; i<8; i++) fputc(p[i], fp); return(0); }
#endif

int
writecstr(FILE *fp, char *cstr)
{
	int trunc=0;
#ifdef TRUNCATE_CSTR
	int l=strlen(cstr);
	if (l > DBF_MAXQSTRSIZE)
	{
		/*char c;*/
		/*c = cstr[15];*/
		/*cstr[15] = '\0';*/
		/*fprintf(stderr, "Warning: truncated string (%s...)\n", cstr);*/
		/*cstr[15] = c;*/
		cstr[DBF_MAXQSTRSIZE] = '\0';
		l = DBF_MAXQSTRSIZE;
		trunc = 1;
	}
#endif /* TRUNCATE_CSTR */
#ifdef CONVERTCHARS
	fputs(iso2cp(cstr), fp);
#else
	fputs(cstr, fp);
#endif
	writebyte(fp, 0);
	return(trunc);
}

static int _writeqstr_warntrunc = 1;

int
writeqstr(FILE *fp, char *cstr)
{
	int len = strlen(cstr), trunc = 0;
	char c;
	if (len > DBF_MAXQSTRSIZE)
	{
		if (_writeqstr_warntrunc)
		{
			c = cstr[31];
			cstr[31] = '\0';
			fprintf(stderr, "Warning: truncated string (%s...)\n", cstr);
			cstr[31] = c;
		}
		c = cstr[DBF_MAXQSTRSIZE];
		cstr[DBF_MAXQSTRSIZE] = '\0';
		len = DBF_MAXQSTRSIZE;
		trunc = 1;
	}
	writebyte(fp, len);
#ifdef CONVERTCHARS
	fputs(iso2cp(cstr), fp);
#else
	fputs(cstr, fp);
#endif
	/*fprintf(stderr, "QSTR \"%s%s\"\n", cstr[0]==DBF_FIELD_QSTR_CONTINUATION?"[CONT]":"",cstr);*/
	if (trunc)
		cstr[DBF_MAXQSTRSIZE] = c;
	return(trunc);
}

int
writemultiqstr(FILE *fp, char *cstr)
{
	char *copy;
	char *ptr;

	/* Make a copy so we can insert continuation characters */
	copy = strcopy(cstr);
	ptr = copy;

	_writeqstr_warntrunc = 0;
	while (writeqstr(fp, ptr) != 0)
	{
		ptr += DBF_MAXQSTRSIZE -1;
		*ptr = DBF_FIELD_QSTR_CONTINUATION;
	}
	_writeqstr_warntrunc = 1;
	free(copy);
	return(0);
}


/*
 * Input functions
 */
Byte readbyte(FILE *fp) { return fgetc(fp); }
Word readword(FILE *fp) { unsigned Byte b1,b2; b1=fgetc(fp); b2=fgetc(fp); return(b1 | (b2<<8)); }
Long readlong(FILE *fp) { unsigned Byte b1,b2,b3,b4; b1=fgetc(fp); b2=fgetc(fp); b3=fgetc(fp); b4=fgetc(fp); return(b1 | (b2<<8) | (b3<<16) | (b4<<24)); }
int readrecordtype(FILE *fp, int *size, int *type) { Word w = readword(fp); *size = w & 0xfff; *type = (w >> 12) & 0xf; return(0); }
#ifdef _BIG_ENDIAN
Real readreal(FILE *fp) { double d; unsigned Byte *p = (unsigned Byte *)&d; int i; for (i=7; i>=0; i--) { p[i]=fgetc(fp); } return(d); }
#endif
#ifdef _LITTLE_ENDIAN
Real readreal(FILE *fp) { double d; unsigned Byte *p = (unsigned Byte *)&d; int i; for (i=0; i<8; i++) { p[i]=fgetc(fp); } return(d); }
#endif

char *
readcstr(FILE *fp)
{
	char buf[4096], *ptr=buf;
	int c;
	while ((c=fgetc(fp))!='\0' && c!=EOF) { *ptr++ = c; }
	*ptr = '\0';
#ifdef CONVERTCHARS
	return(strcopy(cp2iso(buf)));
#else
	return strcopy(buf);
#endif
}

char *
readqstr(FILE *fp)
{
	unsigned Byte b = readbyte(fp);
	char *str = (char*)malloc(b+1);
	if (str==NULL) return(NULL);
	fread(str,b,1,fp);
	str[b]='\0';
#ifdef CONVERTCHARS
	return(cp2iso(str));
#else
	return(str);
#endif
}


/*
 * Useful data
 */

int daysInMonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
char *dayName3[] = { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };
char *monthName3[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

/*
 * Useful functions
 */

/*
 * Check for a leap year.
 * In: year with century (although years < 1900 are assumed offset from 1900)
 * Returns: 1 for a leap year, else 0.
 */
int
leapyear(int y)
{
	if (y<1900)
		y += 1900;
	/* years which are a multiple of 4, except centuries, except 2000 */
	if ((y%4 == 0) && ((y%100 != 0) || (y==2000)))
		return(1);
	else
		return(0);
}

/*
 * Calculate the day of the week for a given day number.
 * In: Psion day number
 * Returns: day of the week (0=Monday, 6=Sunday).
 */
int
dayofweek(int daynumber)
{
	return ((daynumber+5) % 7);
}


/*
 * Convert a day number to a date.
 * In: Psion day number as used in Spreadsheet (offset 1900) not Agenda (1970)
 * Out: year (offset from 1900), month (1-12), day (1-31).
 * Returns: -1 for error.
 */
int
daytodate(int daynumber, int *year, int *month, int *day)
{
	int y=0, m=0, d, D=0, Dp;

	if (daynumber < 0 || daynumber > 93503)
		return(-1);
	/* How bizarre: daynumber 0 == daynumber 1 == daynumber 2! */
	if (daynumber>0) daynumber--;
	if (daynumber>0) daynumber--;
	while (1)
	{
		Dp = 365+leapyear(1900+y);
		if (D+Dp > daynumber) break;
		D+=Dp;
		y++;
	}
	daysInMonth[1] = 28;
	if (leapyear(1900+y)) daysInMonth[1]=29;
	while (1)
	{
		Dp = daysInMonth[m];
		if (D+Dp > daynumber) break;
		D+=Dp;
		m++;
	}
	daysInMonth[1] = 28;
	d = daynumber-D;
	*year=y; *month=m+1; *day=d+1;
	return(0);
}


/*
 * My version of strtok, but only takes a char as second arg, not a string.
 * Seems to work, unlike normal strtok which, given a separator of tab, will
 * skip over consecutive tabs rather than returning an empty string.
 */
char *
strctok(char *str0, int ch)
{
	static char *str = NULL;

	char *ptr, *tmp;

	if (str0 == NULL)
	{
		/* Not initialised? */
		if (str == NULL)
			return(NULL);
		/* Check if we reached end of string last time */
		if (*str == '\0')
			return(NULL);
		/* Find next separator */
		ptr = strchr(str, ch);
		if (ptr == NULL)
		{
			/* We are at the last, set end of string and return this */
			ptr = str;
			/* Next call will have str pointing at end null char */
			str += strlen(str);
			return(ptr);
		}
		else
		{
			/* Remember starting point */
			tmp = str;
			/* Set separator to null and remember position after */
			*ptr = '\0';
			str = ptr+1;
			/* Return starting point */
			return(tmp);
		}
	}
	else
	{
		/* Remember string start */
		str = str0;
		/* Find next separator */
		ptr = strchr(str, ch);
		if (ptr == NULL)
		{
			/* None found, set end of string and return original */
			str += strlen(str);
			return(str0);
		}
		else
		{
			/* Set separator to null and remember position after */
			*ptr = '\0';
			str = ptr+1;
			return(str0);
		}
	}
	/* Not reached */
}


/*
 * Frees first string, adds the two together and changes first pointer
 * to point to it.  Returns new pointer too, but you don't need to use it.
 * Second string is untouched.
 */
char *
stradd(char **strptr, char *appendstr)
{
	int len;
	char *newptr;

	len = strlen(*strptr) + strlen(appendstr);
	newptr = (char*)malloc(len+8);
	if (newptr == NULL)
		return(NULL);
	sprintf(newptr, "%s%s", *strptr, appendstr);
	free(*strptr);
	*strptr = newptr;
	return(newptr);
}

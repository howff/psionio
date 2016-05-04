/* > wrd2html.c Copyright 1997 Andrew Brooks <arb@sat.dundee.ac.uk>
 * 1.06 arb Mon Oct  4 10:40:47 BST 1999 - output HTML character encodings.
 * 1.05 arb Tue Aug 24 10:43:42 BST 1999 - allow lowercase heading names.
 *      Thanks to Mike Taylor <mike@tecc.co.uk> for the patch.
 * 1.04 arb Fri Aug  7 10:14:09 BST 1998 - added decryption hooks.
 * 1.03 arb Wed Jul 29 15:50:24 BST 1998 - convert to ISO 8859 if defined.
 * 1.02 arb Fri Jun 20 10:23:53 BST 1997 - turn styles off in opposite order,
 *      added underline, font name and size changes, and bulleted lists.
 *      Default style always displayed first before, for example, heading.
 * 1.01 arb Fri Jun 13 09:46:34 BST 1997 - added Heading styles.
 * 1.00 arb 09 June 1997
 * Extract WRD contents and write to stdout in text or HTML format.
 * Usage: wrd2html [-v] [-d] [-t | -h] file.wrd
 *  -t : text output }
 *  -h : HTML output } default depends on name of program (wrd2txt or wrd2html)
 *  -v : verbose       displays most useful records
 *  -d : debug         displays record types, sizes and offsets
 */

static char SCCSid[] = "@(#)wrd2html.c    1.06 (C) 1997 arb WRD to text or HTML";

/*
 * Bugs:
 * Don't know what to do with paragraph/line modes.
 * Do Headings/Bulleted lists etc. use the same names in international versions?
 * New paragraphs in "Bulleted list" style are always bulleted.
 * Non-breaking spaces are replaced by " "in HTML not "&nbsp;".
 */

/*
 * To do:
 * WRD_DisplayStyle and WRD_DisplayEmphasis not complete yet.
 * Inheritance.
 */

/*
 * Configuration:
 */
char *HTML_Header = "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\n<html><head>\n<title>%s</title>\n</head><body>\n\n";
char *HTML_Footer = "</body></html>\n";


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "psionio.h"

/* Types */
typedef struct emphasis_s
{
	char code[3];
	char *name;
	char defaul;
	Word font;
	Byte style;
	Word fontsize;
	Byte inherit;
} emphasis_t;
typedef struct style_s
{
	char code[3];
	char *name;
	char defaul;
	Word font;
	Byte style;
	Word fontsize;
	Byte inherit;
	Word align;
} style_t;
typedef struct stylelist_s
{
	int len;
	style_t *sty;
	emphasis_t *em;
} stylelist_t;

/* Global variables */
static char *prog;
static char *usage = "usage: %s [-v] [-d] [-t | -h] [-p password] file.wrd\n-v\tverbose\n-d\tdebug\n-t\tplain text output\n-h\tHTML output\n-p\tpassword (9 hex digits)\n";
char *options = "dvthp:";
unsigned char *password = NULL, passwordkey[16];
int htmlout = 1;
int encrypted = 0;
int paratype = 0;
char *headertext = NULL;
char *footertext = NULL;
char *bodytext = NULL;
int bodylen = 0;
style_t *styles = NULL;
emphasis_t *emphases = NULL;
stylelist_t *stylelist = NULL;
int numstyles = 0;
int numemphases = 0;
int numstylelist = 0;
int defaultstyledone = 0; /* should be static to DisplayStyle function */
int defaultemphasisdone = 0;
style_t *defaultstyle;
int inlist = 0; /* inside a bulleted list show <li> at each new paragraph */
static int verbose = 0;
static int debug = 0;

/* Font names */
char *WRD_FontNames[] =
{
	"Courier",
	"Pica",
	"Elite",
	"Prestige",
	"Letter Gothic",
	"Gothic",
	"Cubic",
	"Lineprinter",
	"Helvetica",
	"Avant Garde",
	"Spartan",
	"Metro",
	"Presentation",
	"APL",
	"OCR A",
	"OCR B",
	"Standard Roman",
	"Emperor",
	"Madeleine",
	"Zapf Humanist",
	"Classic",
	"Times Roman",
	"Century",
	"Palatino",
	"Souvenir",
	"Garamond",
	"Caledonia",
	"Bodoni",
	"University",
	"Script",
	"Script PS",
	"Commercial Script",
	"Park Avenue",
	"Coronet",
	"Greek",
	"Kana",
	"Hebrew",
	"Russian",
	"Narrator",
	"Emphasis",
	"Zapf Chancery",
	"Old English",
	"Cooper Black",
	"Symbol",
	"Line Draw",
	"Math 7",
	"Math 8",
	"Dingbats",
	"EAN",
	"PC Line"
};

#define SkipBytes(F,N) { int n=N; while(n--) getc(F); }
#define ON  1
#define OFF 0


/*
 * Read Database file header
 */
int
WRD_ReadFileHeader(FILE *fp)
{
	char *id;
	Word w;
	int i;

	id = readcstr(fp);
	if (strcmp(id, WRD_IDSTRING))
	{
		fprintf(stderr,"%s: not a wrd file\n",prog);
		exit(1);
	}
	free(id);
	w = readword(fp);
	if (w == WRD_VERENC)
		encrypted = 1;
	readword(fp); /* encryption algorithm version */
	for (i=20; i<=35; i++)
		readbyte(fp);
	readword(fp); /* 0xEAEA if unencrypted, 0 otherwise */
	readword(fp); /* unused */
	return(0);
}


/*
 * Display print setup record (inside wrd/dbf files)
 */
int
WRD_DisplayPrintSetupRecord(FILE *fpin, int recsize)
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
	recsize -= 58;
	SkipBytes(fpin, recsize);
	return(0);
}


/*
 * Read style and emphasis definitions
 */
int
WRD_ReadStyleDef(FILE *fp, int recsize)
{
	style_t *sty;
	Byte b;

	numstyles++;
	if (styles == NULL)
		sty = styles = (style_t*)malloc(sizeof(style_t));
	else
	{
		styles = (style_t*)realloc(styles, sizeof(style_t)*numstyles);
		sty = &styles[numstyles-1];
	}
	sty->code[0] = readbyte(fp);
	sty->code[1] = readbyte(fp);
	sty->code[2] = 0;
	sty->name = readcstr(fp);
	for (b=16-strlen(sty->name)-1; b; b--) readbyte(fp);
	b = readbyte(fp); /* bits set: 0=emphasis, 1=undeletable, 2=default style */
	sty->defaul = (b & (1<<2)) ? 1 : 0;
	if (debug) fprintf(stderr, "Parsing style \"%s\", code %s%s\n", sty->name, sty->code, sty->defaul?" (Default)":"");
	readbyte(fp); /* unused */
	sty->font = readword(fp);
	sty->style = readbyte(fp);
	readbyte(fp); /* unused */
	sty->fontsize = readword(fp);
	sty->inherit = readbyte(fp);
	readbyte(fp); /* unused */
    recsize -= 28;
    /* only for styles */
    readword(fp); readword(fp); readword(fp);
    sty->align = readword(fp);
    recsize -= 8;
	SkipBytes(fp,recsize);
	return(0);
}


int
WRD_ReadEmphasisDef(FILE *fp, int recsize)
{
	emphasis_t *em;
	Byte b;

	numemphases++;
	if (emphases == NULL)
		em = emphases = (emphasis_t*)malloc(sizeof(emphasis_t));
	else
	{
		emphases = (emphasis_t*)realloc(emphases, sizeof(emphasis_t)*numemphases);
		em = &emphases[numemphases-1];
	}
	em->code[0] = readbyte(fp);
	em->code[1] = readbyte(fp);
	em->code[2] = 0;
	em->name = readcstr(fp);
	for (b=16-strlen(em->name)-1; b; b--) readbyte(fp);
	b = readbyte(fp); /* bits set: 0=emphasis, 1=undeletable, 2=default style */
	em->defaul = (b & (1<<2)) ? 1 : 0;
	if (debug) fprintf(stderr, "Parsing emphasis \"%s\", code %s%s\n", em->name, em->code, em->defaul?" (Default)":"");
	readbyte(fp); /* unused */
	em->font = readword(fp);
	em->style = readbyte(fp);
	readbyte(fp); /* unused */
	em->fontsize = readword(fp);
	em->inherit = readbyte(fp);
	readbyte(fp); /* unused */
    recsize -= 28;
	SkipBytes(fp,recsize);
	return(0);
}


style_t *
WRD_LookupStyle(char *code)
{
	style_t *sty = &styles[0];
	int i;

	for (i=0; i<numstyles; i++)
	{
		if (strcmp(sty[i].code, code)==0) return(&sty[i]);
	}
	fprintf(stderr, "%s: style with code %s has not been defined\n", prog, code);
	exit(1);
	return(NULL); /* silly compiler */
}


emphasis_t *
WRD_LookupEmphasis(char *code)
{
	emphasis_t *em = emphases;
	int i;

	for (i=0; i<numemphases; i++)
	{
		if (strcmp(em[i].code, code)==0) return(&em[i]);
	}
	fprintf(stderr, "%s: emphasis with code %s has not been defined\n", prog, code);
	exit(1);
	return(NULL); /* silly compiler */
}


int
WRD_ReadStyleList(FILE *fp, int recsize)
{
	char stylename[3], emphasisname[3];
	stylelist_t *listitem;

	if (recsize < 1) return(-1);
	stylename[2] = emphasisname[2] = '\0';

	while (recsize)
	{
		numstylelist++;
		if (stylelist == NULL)
			listitem = stylelist = (stylelist_t*)malloc(sizeof(stylelist_t));
		else
		{
			stylelist = (stylelist_t*)realloc(stylelist, sizeof(stylelist_t)*numstylelist);
			listitem = &stylelist[numstylelist-1];
		}
		listitem->len = readword(fp);
		stylename[0] = readbyte(fp); stylename[1] = readbyte(fp);
		emphasisname[0] = readbyte(fp); emphasisname[1] = readbyte(fp);
		listitem->sty = WRD_LookupStyle(stylename);
		listitem->em  = WRD_LookupEmphasis(emphasisname);
		recsize -= 6;
		/*if (debug) fprintf(stderr, "Stylelist: %s/%s for %d bytes\n", stylename, emphasisname, listitem->len);*/
		if (debug) fprintf(stderr, "Stylelist: %s/%s for %d bytes\n", listitem->sty->name, listitem->em->name, listitem->len);
	}
	return(0);
}


/*
 * Display emphasis or style
 */
int
WRD_DisplayStyle(FILE *fp, style_t *sty, int on)
{
	char *o = on ? "" : "/";

	if (on && sty->defaul)
	{
		if (defaultstyledone) return(0);
		else defaultstyledone=1;
		defaultstyle = sty;
	}

	if (on)
	{
		if (strncmp(sty->name, "Heading ", 8)==0)
			fprintf(fp, "<%sh%d>", o, toupper(sty->name[8])-'A'+1);
		else
		{
			if (strcmp(sty->name, "Bulleted list")==0)
				{ fprintf(fp, "<%sul><li>", o); inlist = 1; }
			if (sty->font != -1 && sty->font != defaultstyle->font)
				fprintf(fp, "<%sfont name=\"%s\">", o, WRD_FontNames[sty->font]);
			if (sty->fontsize != 0 && sty->fontsize < defaultstyle->fontsize)
				fprintf(fp, "<%ssmall>", o);
			if (sty->fontsize != 0 && sty->fontsize > defaultstyle->fontsize)
				fprintf(fp, "<%sbig>", o);
			if (sty->style & (1<<0)) fprintf(fp, "<%su>", o);
			if (sty->style & (1<<1)) fprintf(fp, "<%sb>", o);
			if (sty->style & (1<<2)) fprintf(fp, "<%si>", o);
		}
	}
	else
	{
		if (strncmp(sty->name, "Heading ", 8)==0)
			fprintf(fp, "<%sh%d>", o, toupper(sty->name[8])-'A'+1);
		else
		{
			if (sty->style & (1<<2)) fprintf(fp, "<%si>", o);
			if (sty->style & (1<<1)) fprintf(fp, "<%sb>", o);
			if (sty->style & (1<<0)) fprintf(fp, "<%su>", o);
			if (sty->fontsize != 0 && sty->fontsize > defaultstyle->fontsize)
				fprintf(fp, "<%sbig>", o);
			if (sty->fontsize != 0 && sty->fontsize < defaultstyle->fontsize)
				fprintf(fp, "<%ssmall>", o);
			if (sty->font != -1 && sty->font != defaultstyle->font)
				fprintf(fp, "<%sfont name=\"%s\">", o, WRD_FontNames[sty->font]);
			if (strcmp(sty->name, "Bulleted list")==0)
				{ fprintf(fp, "<%sul>", o); inlist = 0; }
		}
	}

	/* align left,right,centre,justified */

	return(0);
}


int
WRD_DisplayEmphasis(FILE *fp, emphasis_t *em, int on)
{
	char *o = on ? "" : "/";

	if (on && em->defaul)
	{
		if (defaultemphasisdone) return(0);
		else defaultemphasisdone=1;
	}

	if (on)
	{
		if (em->font != -1 && em->font != defaultstyle->font)
			fprintf(fp, "<%sfont name=\"%s\">", o, WRD_FontNames[em->font]);
		if (em->fontsize != 0 && em->fontsize < defaultstyle->fontsize)
			fprintf(fp, "<%ssmall>", o);
		if (em->fontsize != 0 && em->fontsize > defaultstyle->fontsize)
			fprintf(fp, "<%sbig>", o);
		if (em->style & (1<<0)) fprintf(fp, "<%su>", o);
		if (em->style & (1<<1)) fprintf(fp, "<%sb>", o);
		if (em->style & (1<<2)) fprintf(fp, "<%si>", o);
		if (em->style & (1<<3)) fprintf(fp, "<%ssup>", o);
		if (em->style & (1<<4)) fprintf(fp, "<%ssub>", o);
	}
	else
	{
		if (em->style & (1<<4)) fprintf(fp, "<%ssub>", o);
		if (em->style & (1<<3)) fprintf(fp, "<%ssup>", o);
		if (em->style & (1<<2)) fprintf(fp, "<%si>", o);
		if (em->style & (1<<1)) fprintf(fp, "<%sb>", o);
		if (em->style & (1<<0)) fprintf(fp, "<%su>", o);
		if (em->fontsize != 0 && em->fontsize > defaultstyle->fontsize)
			fprintf(fp, "<%sbig>", o);
		if (em->fontsize != 0 && em->fontsize < defaultstyle->fontsize)
			fprintf(fp, "<%ssmall>", o);
		if (em->font != -1 && em->font != defaultstyle->font)
			fprintf(fp, "<%sfont name=\"%s\">", o, WRD_FontNames[em->font]);
	}
	return(0);
}


int
WRD_DisplayDefaultStyle(FILE *fp, int on)
{
	style_t *sty = &styles[0];
	int i;

	for (i=0; i<numstyles; i++)
	{
		if (sty[i].defaul) break;
	}
	if (i==numstyles) return(0);
	return WRD_DisplayStyle(fp, sty, on);
}

int
WRD_DisplayDefaultEmphasis(FILE *fp, int on)
{
	emphasis_t *em = &emphases[0];
	int i;

	for (i=0; i<numemphases; i++)
	{
		if (em[i].defaul) break;
	}
	if (i==numemphases) return(0);
	return WRD_DisplayEmphasis(fp, em, on);
}


/*
 * Read document
 */
int
WRD_ReadFile(FILE *fpin)
{
	int recsize, rectype;
	Byte b;
	char *str;

	WRD_ReadFileHeader(fpin);
	while (!feof(fpin))
	{
		rectype = readword(fpin); if (feof(fpin)) break;
		recsize = readword(fpin); if (feof(fpin)) break;
		switch (rectype)
		{
			case WRD_RECORD_FILEINFO:
				if (debug) fprintf(stderr,"Found file info record, type %d, length %d at %lx\n",rectype,recsize,ftell(fpin));
				readword(fpin); readbyte(fpin); readbyte(fpin); readbyte(fpin);
				paratype = readbyte(fpin);
				b = readbyte(fpin); readbyte(fpin); readword(fpin);
				if (verbose) fprintf(stderr,"Outlining level: %d\n", b);
				break;
			case WRD_RECORD_PRINTERSETUP:
				if (debug) fprintf(stderr,"Found print setup record, type %d, length %d at %lx\n",rectype,recsize,ftell(fpin));
				WRD_DisplayPrintSetupRecord(fpin, recsize);
				break;
			case WRD_RECORD_PRINTERDRIVER:
				if (debug) fprintf(stderr,"Found printer driver record, type %d, length %d at %lx\n",rectype,recsize,ftell(fpin));
				b = readbyte(fpin);
				str = readcstr(fpin);
				if (verbose) fprintf(stderr, "Printer driver: %s, model %d\n", str, b);
				free(str);
				break;
			case WRD_RECORD_HEADER:
				if (debug) fprintf(stderr,"Found header text record, type %d, length %d at %lx\n",rectype,recsize,ftell(fpin));
				headertext = readcstr(fpin);
				if (verbose) fprintf(stderr,"Header text: %s\n", headertext);
				break;
			case WRD_RECORD_FOOTER:
				if (debug) fprintf(stderr,"Found footer text record, type %d, length %d at %lx\n",rectype,recsize,ftell(fpin));
				footertext = readcstr(fpin);
				if (verbose) fprintf(stderr,"Footer text: %s\n", footertext);
				break;
			case WRD_RECORD_STYLE:
				if (debug) fprintf(stderr,"Found style definition record, type %d, length %d at %lx\n",rectype,recsize,ftell(fpin));
				WRD_ReadStyleDef(fpin, recsize);
				break;
			case WRD_RECORD_EMPHASIS:
				if (debug) fprintf(stderr,"Found emphasis definition record, type %d, length %d at %lx\n",rectype,recsize,ftell(fpin));
				WRD_ReadEmphasisDef(fpin, recsize);
				break;
			case WRD_RECORD_TEXT:
				bodytext = (char*) malloc(recsize);
				if (bodytext == NULL) { fprintf(stderr,"%s: out of memory\n", prog); return(-1); }
				if (fread(bodytext, recsize, 1, fpin) != 1) { fprintf(stderr,"%s: cannot read body text of document\n", prog); return(-1); }
				bodylen = recsize;
				break;
			case WRD_RECORD_STYLES:
				WRD_ReadStyleList(fpin, recsize);
				break;
			default:
				if (debug) fprintf(stderr,"Found unknown record, type %d, length %d at %lx\n",rectype,recsize,ftell(fpin));
				SkipBytes(fpin,recsize);
				break;
		}
	}
	return(0);
}


/*
 * Output document in text or html format.
 */
int
WRD_DisplayFile(FILE *fp, char *title)
{
	int i;

	if (bodytext == NULL || bodylen < 1)
	{
		fprintf(stderr, "%s: no body text in document\n", prog);
		return(-1);
	}
	if (htmlout && numstylelist < 1)
	{
		fprintf(stderr, "%s: warning: no styles to be applied to text\n", prog);
		htmlout = 0;
	}
	if (encrypted && !password)
	{
		fprintf(stderr, "%s: warning: body text is encrypted: no styles will be applied\n", prog);
		htmlout = 0;
	}

	if (encrypted)
		decrypt_block((unsigned char*)bodytext, bodylen, passwordkey);

#ifdef CONVERTCHARS
	cp2iso_block(bodytext, bodylen);
#endif

	/* First modify odd chars in body text */
	for (i=0; i<bodylen; i++)
	{
		if (bodytext[i] == WRD_CHAR_NOBRKHYPHEN) bodytext[i] = '-';
		else if (bodytext[i] == WRD_CHAR_NOBRKSPACE) bodytext[i] = ' ';
	}

	/* If text only then do it now */
	if (htmlout == 0)
	{
		for (i=0; i<bodylen; i++)
		{
			if (bodytext[i] == WRD_CHAR_SOFTHYPHEN) continue;
			else if (bodytext[i] == 0) fprintf(fp, "\n");
			else fprintf(fp, "%c", bodytext[i]);
		}
	}
	else
	{
		Long cnt=0;
		stylelist_t *style = stylelist;
		int parapending = 0;

		fprintf(fp, HTML_Header, title);
		if (headertext) fprintf(fp, "<!-- Header text \"%s\" -->\n", headertext);
		/* The only style attribute which cannot be set as default is centre */
		/* Display default style first */
		WRD_DisplayDefaultStyle(fp, ON);
		WRD_DisplayDefaultEmphasis(fp, ON);
		/* Display first style */
		WRD_DisplayStyle(fp, style->sty, ON);
		WRD_DisplayEmphasis(fp, style->em, ON);
		for (i=0; i<bodylen; i++)
		{
			cnt++;
			if (cnt > style->len)
			{
				cnt = 1;
				/* Don't bother changing to the same style! */
				if (style->sty != (style+1)->sty ||
					style->em != (style+1)->em)
				{
					WRD_DisplayStyle(fp, style->sty, OFF);
					WRD_DisplayEmphasis(fp, style->em, OFF);
					style++;
					if (parapending)
					{
						parapending=0;
						fprintf(fp, "\n\n<p>%s", inlist?"<li>":"");
					}
					WRD_DisplayStyle(fp, style->sty, ON);
					WRD_DisplayEmphasis(fp, style->em, ON);
				}
				else { /*fprintf(fp, "<!>");*/ style++; }
			}
			if (parapending)
			{
				parapending=0;
				fprintf(fp, "\n\n<p>%s", inlist?"<li>":"");
			}
			if (bodytext[i] == WRD_CHAR_SOFTHYPHEN) continue;
			else if (bodytext[i] == 0) parapending=1;
			/*fprintf(fp, "\n\n<p>%s", inlist?"<li>":"");*/
			else if (bodytext[i] == '&') fprintf(fp, "&amp;");
			else if (bodytext[i] == '<') fprintf(fp, "&lt;");
			else if (bodytext[i] == '>') fprintf(fp, "&gt;");
			else fprintf(fp, "%s", iso2html(bodytext[i]));
		}
		if (footertext) fprintf(fp, "<!-- Footer text \"%s\" -->\n", footertext);
		fprintf(fp, "%s", HTML_Footer);
	}
	return(0);
}


int
main(int argc, char *argv[])
{
	char *arg, *wrdfile = NULL;
	FILE *fpin, *fpout = stdout;
	int i;

	prog = argv[0];
	/* Default txt/html output depends on program name */
	/* Symbolic links can be used to give a different name */
	i = strlen(prog);
	if (prog[i-1] == 'l' || prog[i-1] == 'L') htmlout = 1;
	else if (prog[i-1] == 't' || prog[i-1] == 'T') htmlout = 0;
	GETOPT(i, options)
	{
		case 'd': debug = 1; break;
		case 'v': verbose = 1; break;
		case 'h': htmlout = 1; break;
		case 't': htmlout = 0; break;
		case 'p': password = (unsigned char *)optarg; cryptkey(password, passwordkey); break;
		case '?': fprintf(stderr, usage, prog); exit(1);
	}
	GETOPT_LOOP_REST(arg)
	{
		if (wrdfile == NULL)
			wrdfile = arg;
		else
			{ fprintf(stderr, usage, prog); exit(1); }
	}
	if (wrdfile == NULL) { fprintf(stderr, usage, prog); exit(1); }
	fpin = fopen(wrdfile, "rb");
	if (fpin == NULL) { fprintf(stderr,"%s: cannot open file %s\n",prog,wrdfile); exit(1); }
	if (WRD_ReadFile(fpin) == 0)
		WRD_DisplayFile(fpout, wrdfile);
	fclose(fpin);
	fclose(fpout);
	return(0);
}

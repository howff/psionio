/* > spr2tsv.c Copyright 1997 Andrew Brooks <arb@sat.dundee.ac.uk>
 * 1.04 arb Thu May 15 16:20:11 BST 1997 - some tidying of formula support.
 * 1.03 arb Fri May  9 11:25:16 BST 1997 - added rudimentary formula support:
 *      formulae are read from file, display values if requested else attempt
 *      to convert to string representation.
 * 1.02 arb Mon Mar 24 11:35:23 GMT 1997 - prettier currency display,
 *      scientific display supported
 * 1.01 arb Fri Mar 21 13:26:21 GMT 1997 - added -p, -h options for pretty
 *      text or HTML table.
 * 1.00 arb Mon Mar 17 16:33:10 GMT 1997
 * Extract SPR contents and write to stdout in TSV format.
 * Input cannot be stdin as we read file twice.
 * Usage: spr2tsv [-p] [-h] [-v] [-d] file.spr
 *  -p : pretty display (show percent/pound symbols, show dates and times,
 *       show numbers to specified number of decimal places etc.)
 *  -h : HTML table (align text and show in specified font. Can be combined
 *       with -p)
 *  -v : verbose (show cells in the order that they are read and show records
 *       that have been ignored)
 *  -d : debug (show conversion progress)
 */

static char SCCSid[] = "@(#)spr2tsv.c     1.04 (C) 1997 arb SPR to TSV";

/*
 * Bugs:
 * Use of ints will fail on 16-bit machines, should use long (esp. cellref).
 */

/*
 * Notes:
 * Should alignment be done in <p> or in <td>?
 */

/*
 * To do:
 * Read column widths (needed in General format).
 * Read styles for font information.
 * Display negative number is brackets or with leading - sign.
 * Decimal point character.
 * Thousands separator (triads).
 * Display formulas.
 * Display bargraphs.
 * General display (which requires column widths if pretty).
 * Pretty option for numbers (disp_comma) not yet implemented.
 * Instead of using a huge array we could store in a list, then sort.
 * Graphs are not converted.
 * Assumes styles haven't been changed, ie. that font1=bold etc.
 * Should set odd display formats to disp_fixed, as that is what they are based
 *  on, rather than just testing for number of digits (decimal places) to show.
 * Scientific display of large integers?
 */

/*
 * Configuration
 * Define SPR_DISP_DEFAULT to default display format (see list below)
 * Define FORMULA for rudimentary formula unmangling.
 */
#define SPR_DISP_DEFAULT disp_fixed
#define FORMULA


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
#include "psionio.h"
#ifndef SEEK_SET
#define SEEK_SET 0
#endif

/* Types */
typedef enum
{
	cell_empty=0, cell_float=1, cell_string=2, cell_int=3,
	cell_floatformula=5, cell_stringformula=6
} celltype_t;
typedef enum
{
	disp_fixed=0, disp_sci=1, disp_currency=2, disp_percent=3, disp_comma=4,
	disp_formula=5, disp_hidden=6, disp_date=9, disp_time=11,
	disp_default=15, disp_general=100, disp_bargraph=101
} displaytype_t;
typedef enum
{
	disp_repeat=0, disp_left=1, disp_right=2, disp_centre=3
} displayalign_t;
typedef struct cell_s
{
	celltype_t type;
	union
	{
		int i;
		double f;
		char *s;
	} data;
	displaytype_t display;
	displayalign_t align;
	int formula;
	int digits;
	int font;
} cell_t;
typedef struct formuladef_s
{
	char *str;
	char *arg; /* n=numeric, s=string, r=range, x=arg, ?=vararg */
} formuladef_t;
typedef struct formula_s
{
	int use;
	int len;
	char *def;
	/*char *text;*/
} formula_t;

/* Global variables */
static char *prog;
char *options = "vdph";
static char *usage = "usage: %s [-p] [-h] [-v] [-d] file.spr\n-p\tpretty display\n-h\tHTML table output\n-v\tverbose\n-d\tdebug\n";
static int pretty = 0;
static int html = 0;
static int verbose = 0;
static int debug = 0;
int numformulae = 0;
formula_t *formulae = NULL;

/* Formula definitions */
formuladef_t SPR_Op[] =
{
	{ "<",   "xx" },
	{ "<=",  "xx" },
	{ ">",   "xx" },
	{ ">=",  "xx" },
	{ "<>",  "xx" },
	{ "=",   "xx" },
	{ "+",   "xx" },
	{ "-",   "xx" },
	{ "*",   "xx" },
	{ "**",  "xx" },
	{ "+",   "x" },
	{ "-",   "x" },
	{ "NOT", "x" },
	{ "AND", "xx" },
	{ "OR",  "xx" },
	{ "&",   "xx" },
	{ "(",   "" },
	{ ")",   "" },
	{ ",",   "" },
	{ "",    "" }
};
formuladef_t SPR_Func[] =
{
	{ "ERR",         "" },
	{ "FALSE",       "" },
	{ "NA",          "" },
	{ "PI",          "" },
	{ "RAND",        "" },
	{ "NOW",         "" },
	{ "TRUE",        "" },
	{ "ABS",         "n" },
	{ "ACOS",        "n" },
	{ "ASIN",        "n" },
	{ "AT",          "s" },
	{ "ATAN",        "n" },
	{ "CELLPOINTER", "n" },
	{ "CHAR",        "n" },
	{ "CODE",        "s" },
	{ "COLS",        "r" },
	{ "COS",         "n" },
	{ "DATEVALUE",   "s" },
	{ "DAY",         "n" },
	{ "EXP",         "n" },
	{ "HOUR",        "n" },
	{ "INT",         "n" },
	{ "ISERR",       "r" },
	{ "ISNA",        "r" },
	{ "ISNUM",       "r" },
	{ "ISSTR",       "r" },
	{ "LEN",         "s" },
	{ "LN",          "n" },
	{ "LOG",         "n" },
	{ "LOWER",       "s" },
	{ "MINUTE",      "n" },
	{ "MONTH",       "n" },
	{ "N",           "r" },
	{ "PROPER",      "s" },
	{ "ROWS",        "r" },
	{ "S",           "r" },
	{ "SECOND",      "n" },
	{ "SIN",         "n" },
	{ "SQRT",        "n" },
	{ "TAN",         "n" },
	{ "TIMEVALUE",   "s" },
	{ "TRIM",        "s" },
	{ "UPPER",       "s" },
	{ "VALUE",       "s" },
	{ "YEAR",        "n" },
	{ "ATAN2",       "nn" },
	{ "CELL",        "nr" },
	{ "EXACT",       "ss" },
	{ "IRR",         "nn" },
	{ "LEFT",        "sn" },
	{ "MOD",         "nn" },
	{ "NPV",         "nn" },
	{ "--------",    "" },
	{ "REPEAT",      "sn" },
	{ "RIGHT",       "sn" },
	{ "ROUND",       "nn" },
	{ "STRING",      "nn" },
	{ "CTERM",       "nn" },
	{ "DATE",        "nn" },
	{ "DAVG",        "rnr" },
	{ "DCOUNT",      "rnr" },
	{ "DMAX",        "rnr" },
	{ "DMIN",        "rnr" },
	{ "DSTD",        "rnr" },
	{ "DSUM",        "rnr" },
	{ "DVAR",        "rnr" },
	{ "FIND",        "ssn" },
	{ "FV",          "nnn" },
	{ "HLOOKUP",     "nrn" },
	{ "IF",          "nnn" },
	{ "INDEX",       "rnn" },
	{ "MID",         "snn" },
	{ "PMT",         "nnn" },
	{ "PV",          "nnn" },
	{ "RATE",        "nnn" },
	{ "SIN",         "n" },
	{ "TERM",        "nnn" },
	{ "TIME",        "nnn" },
	{ "VLOOKUP",     "rnn" },
	{ "DDB",         "nnnn" },
	{ "REPLACE",     "snns" },
	{ "SYD",         "nnnn" }
};
formuladef_t SPR_VAFunc[] =
{
	{ "AVG",    "?" },
	{ "CHOOSE", "?" },
	{ "COUNT"   "?" },
	{ "MAX"     "?" },
	{ "MIN"     "?" },
	{ "STD"     "?" },
	{ "SUM"     "?" },
	{ "VAR"     "?" }
};

/*
 * Skip bytes on input
 */
#define SkipBytes(F,N) { int n=N; while(n--) getc(F); }
#define SPR_CellFont(F,C,N) { if (*(N)>0) { C->font=getc(F); *(N)=*(N)-1; } }

/* ----------------------------------------------------------------------------
 * Read spreadsheet file header
 */
int
SPR_ReadFileHeader(FILE *fp)
{
	char *id;

#define notspr { fprintf(stderr,"%s: not a spreadsheet file\n",prog); exit(1); }
	id = readcstr(fp);
	if (strcmp(id, SPR_IDSTRING)) notspr;
	free(id);
	if (readlong(fp)) notspr;
	if (readword(fp) != SPR_VERCREAT) notspr;
	if (readword(fp) != SPR_UKOFFSET) notspr;
	if (readword(fp) != SPR_VEROPL) notspr;
#undef notspr
	return(0);
}


/*
 * Read cell/range references.
 * Because they may be relative, the current cell position must be
 * a valid parameter.
 */
int
SPR_CellRef(unsigned Word cellrow, unsigned Word cellcol, Word *row, Word *col)
{
	if (cellcol>=0 && cellcol<=SPR_MAXCOL)
		*col = cellcol;
	else if (cellcol>SPR_CELL_PLUSOFFSET_CODE && cellcol<SPR_CELL_MINUSOFFSET_CODE)
		*col = cellcol - SPR_CELL_PLUSOFFSET;
	else if (cellcol>SPR_CELL_MINUSOFFSET_CODE)
		*col = SPR_CELL_MINUSOFFSET - cellcol;
	/* else leave unchanged (esp. if cellcol==SPR_CELL_CURRENT) */

	if (cellrow>=0 && cellrow<=SPR_MAXROW)
		*row = cellrow;
	else if (cellrow>SPR_CELL_PLUSOFFSET_CODE && cellrow<SPR_CELL_MINUSOFFSET_CODE)
		*row = cellrow - SPR_CELL_PLUSOFFSET;
	else if (cellrow>SPR_CELL_MINUSOFFSET_CODE)
		*row = SPR_CELL_MINUSOFFSET - cellrow;
	/* else leave unchanged (esp. if cellrow==SPR_CELL_CURRENT) */

	return(0);
}

int
SPR_ReadCellRef(FILE *fp, Word *row, Word *col)
{
	Word newrow, newcol;

	newcol = readword(fp);
	newrow = readword(fp);
	return SPR_CellRef(newrow, newcol, row, col);
}

int
SPR_RangeRef(Word l, Word t, Word r, Word b, Word *left, Word *top, Word *right, Word *bottom)
{
	int rc;
	rc = SPR_CellRef(l, t, left, top);
	if (rc == 0) SPR_CellRef(r, b, right, bottom);
	return(rc);
}

int
SPR_ReadRangeRef(FILE *fp, Word *left, Word *top, Word *right, Word *bottom)
{
	int rc;
	rc = SPR_ReadCellRef(fp, left, top);
	if (rc == 0) SPR_ReadCellRef(fp, right, bottom);
	return(rc);
}


/* ----------------------------------------------------------------------------
 * Formula handling
 */
int
SPR_AddFormula(int use, int len, char *def, char *text)
{
	numformulae++;
	if (formulae == NULL)
		formulae = (formula_t*)malloc(sizeof(formula_t));
	else
		formulae = (formula_t*)realloc(formulae, sizeof(formula_t)*numformulae);
	if (formulae == NULL)
		return(-1);
	formulae[numformulae-1].use = use;
	formulae[numformulae-1].len = len;
	formulae[numformulae-1].def = def;
	return(0);
}

char *
SPR_UnmangleFormula(formula_t *formula, Word currow, Word curcol)
{
	Word row, col;
	char *text;
	unsigned char *defptr;
	int i;

	text = (char*)malloc(strlen(formula->def) * 10);
	if (text == NULL)
		return(NULL);
	sprintf(text, "=");
#ifdef FORMULA
	defptr=(unsigned char*)formula->def;
	for (i=0; i<formula->len; i++, defptr++)
	{
		if (*defptr == 21) break;
		else if (*defptr < 21) strcat(text, SPR_Op[*defptr-1].str);
		else if (*defptr >=27 && *defptr <= 108) strcat(text, SPR_Func[*defptr-27].str);
		else if (*defptr >=22 && *defptr <=26) switch (*defptr)
		{
			case 22: defptr+=8; strcat(text,"real "); break; /* real */
			case 23: defptr+=2; strcat(text,"word "); break; /* word */
			case 24: defptr++; strcat(text, "text "); defptr += (*defptr)-1; break; /* qstr */
			case 25: defptr+=4; strcat(text, "cell "); break; /* SPR_CellRef */
			case 26: defptr+=8; strcat(text, "range "); break; /* range ref */
		}
		else if (*defptr >=120 && *defptr <=127)            /* START byte */
		{
			int func = *defptr;       /* variable arguments function code */
			strcat(text, SPR_VAFunc[func-120].str);
			while (1)
			{
				defptr++;
				if (*defptr == func+8) { defptr+=8; }      /* RANGE byte */
				else if (*defptr == func+16) { continue; } /* ARG byte */
				else if (*defptr == func-8) break;         /* END byte */
				else switch (*defptr)
				{
					case 22: defptr+=8; break;
					case 23: defptr+=2; break;
					case 24: defptr+=(*defptr)-1; break;
					case 25: defptr+=4; break;
					case 26: defptr+=8; break;
					default: strcat(text,"?"); break;
				}
			}
		}
	}
#endif /* FORMULA */
	text = (char*)realloc(text, strlen(text)+1);
	return(text);
}


/* ----------------------------------------------------------------------------
 * Extract cell format
 */
void
SPR_GetCellFormat(cell_t *cell, unsigned char contents, unsigned char format)
{
	switch (cell->type)
	{
		case cell_float: /* fallthrough */
		case cell_int:
			cell->align = (contents & 32) ? disp_left : disp_right;
			break;
		case cell_string:
			cell->align = (displayalign_t) ((contents >> 3) & 3);
			break;
	}
	cell->digits = -1;
	cell->display = (displaytype_t) ((format >> 4) & 7);
	if (cell->display == 7)
	{
		/* "special" format */
		cell->display = (displaytype_t) (format & 15);
		if (cell->display == 0)
			cell->display = disp_bargraph;
		if (cell->display == 1)
			cell->display = disp_general;
	}
	else
		cell->digits = format & 15;
	if (cell->display == disp_default)
		cell->display = SPR_DISP_DEFAULT;
}


/* ----------------------------------------------------------------------------
 * Insert commas into a numeric string to indicate thousands.
 */
char *
SPR_InsertTriads(char *num)
{
	static char str[64];
	char *start, *ptr;

	if (num==NULL || *num=='\0') return(NULL);
	ptr = num;
	while (*ptr && !isdigit(*ptr)) ptr++; /* look for digits */
	start = ptr;
	while (*ptr && isdigit(*ptr)) ptr++;  /* find end of digits */
	if (*ptr == '\0') return(NULL);
	/* now go backwards inserting triad character */
	/* we know length so move old string up by (ptr-start-1)/3 from here on */
	/* then go backwards and after each three insert a char */
	return(str);
}


/*
 * Display the contents of a cell.
 * The cell position is given so formulae with relative refs can be converted.
 */
void
SPR_PrintCell(cell_t *cell, Word row, Word col)
{
	double dataf = cell->data.f;
	int datai = cell->data.i;
	char *datas = cell->data.s;
	int display = cell->display;
	celltype_t celltype = cell->type;
	char *htmlalign[] = { "", "align=left", "align=right", "align=center" };
	char *htmlfont0[] = { "", "<b>", "<i>", "<b><i>" };
	char *htmlfont1[] = { "", "</b>", "</i>", "</i></b>" };
	char fmt[16];
	char *formula;
	
	/* Don't show hidden cells */
	if (display == disp_hidden)
		return;
	/* Cannot handle bargraphs */
	if (display == disp_bargraph)
		return;
	/* If cell is a formula but value is to be displayed change celltype */
	if (celltype == cell_floatformula && display != disp_formula)
		celltype = cell_float;
	if (celltype == cell_stringformula && display != disp_formula)
		celltype = cell_string;
	/* general? */

	if (html) printf("<p %s>", htmlalign[cell->align]);
	if (html && cell->font) printf("%s", htmlfont0[cell->font]);
	if (display == disp_percent && pretty)
		dataf *= 100;
	if (display == disp_currency && pretty)
	{
		/* minus sign has to come before currency symbol if negative */
		if (cell->type == cell_int)
		{
			if (datai < 0)
			{
				printf("-");
				datai = abs(datai);
			}
		}
		else if (cell->type == cell_float)
		{
			if (dataf < 0)
			{
				printf("-");
				dataf = fabs(dataf);
			}
		}
		if (html) printf(CURRENCY_HTML);
		else printf(CURRENCY_CHAR);
	}
	if (display == disp_date && pretty)
	{
		int y, m, d, daynumber=0;
		if (cell->type == cell_int) daynumber = datai;
		else if (cell->type == cell_float) daynumber = dataf;
		daytodate(daynumber, &y, &m, &d);
		printf("%02d/%02d/%04d", d, m, y+1900);
	}
	else if (display == disp_time && pretty)
	{
		int h, m;
		h = dataf;
		dataf = dataf - h; /* leave fractional part */
		h = 24 * dataf;
		m = ((24.0*dataf)-h)*60.0;
		if (cell->type == cell_int) h = m = 0; /* integer->midnight */
		printf("%02d:%02d", h, m);
	}
	else switch (celltype)
	{
		case cell_float:
			if (pretty && cell->digits!=-1)
			{
				if (display == disp_sci)
					sprintf(fmt, "%%.%dlg", cell->digits);
				else
					sprintf(fmt, "%%.%dlf", cell->digits);
			}
			else sprintf(fmt, "%%lg");
			printf(fmt, dataf);
			break;
		case cell_int:
			printf("%d", datai);
			break;
		case cell_string:
			printf("%s", datas);
			break;
		case cell_floatformula:
			/* we know display==disp_formula if we get here */
			formula = SPR_UnmangleFormula(&formulae[cell->formula], row, col);
			printf("%s", formula);
			free(formula);
			break;
		case cell_stringformula:
			formula = SPR_UnmangleFormula(&formulae[cell->formula], row, col);
			printf("%s", formula);
			free(formula);
			break;
		default:
			break;
	}
	if (display == disp_percent && pretty) printf("%%");
	if (html && cell->font) printf("%s", htmlfont1[cell->font]);
	if (html) printf("</p>");
}


/* ----------------------------------------------------------------------------
 * Read spreadsheet
 */
int
SPR_ReadSheet(FILE *fpin, FILE *fpout)
{
	int numrows = 0, numcols = 0;
	Word row, col;    /* current position */
	cell_t **cells;   /* big array of cells  */
	long datastart;   /* file pointer to start of data */
	cell_t *cellp;
	unsigned char cell_contents, cell_format;
	int type, size;   /* of record */
	int blank;

	/* File sheet header */
	SPR_ReadFileHeader(fpin);
	datastart = ftell(fpin);

	/* First go through whole sheet and find maximum cell number */
	/* Also read column widths, formulas, font styles etc. */
	if (debug) fprintf(stderr, "%s: determine sheet size\n", prog);
	row = col = 0;
	while (!feof(fpin))
	{
		type = readword(fpin);
		if (feof(fpin)) break;
		size = readword(fpin);
		switch (type)
		{
			case SPR_RECORD_CELL:
				SPR_ReadCellRef(fpin, &row, &col);
				cell_contents = readbyte(fpin);
				cell_format = readbyte(fpin);
				size -= 6;
				if (row < 0) { fprintf(stderr, "%s: bad row number %d\n", prog, row); SkipBytes(fpin, size); continue; }
				if (col < 0) { fprintf(stderr, "%s: bad col number %d\n", prog, col); SkipBytes(fpin, size); continue; }
				if (row >= numrows) numrows = row+1;
				if (col >= numcols) numcols = col+1;
				SkipBytes(fpin, size);
				break;
			case SPR_RECORD_FORMULA:
			{
				int use, length;
				char *def;
				use = readword(fpin); /* number of records using this formula */
				length = readbyte(fpin);
				def = (char*)malloc(length+1);
				if (def==NULL) return(-1);
				fread(def, length, 1, fpin);
				def[length] = '\0';
				SPR_AddFormula(use, length, def, NULL);
#ifdef DEBUG
				if (debug) memdmp(def, length);
#endif /* DEBUG */
				size -= length+3;
				SkipBytes(fpin, size);
				break;
			}
			default:
				SkipBytes(fpin, size);
				break;
		}
	}

	/* Create array large enough for whole sheet */
	/* Silly, but we can't easily print out in order otherwise */
	if (debug) fprintf(stderr, "%s: array creation %d by %d\n", prog, numcols, numrows);
	cells = (cell_t**)malloc(sizeof(cell_t*)*numrows);
	if (cells == NULL) return(-1);
	for (row=0; row<numrows; row++)
	{
		cells[row] = (cell_t*)calloc(1,sizeof(cell_t)*numcols);
		if (cells[row] == NULL) return(-1);
	}

	/* Now go through whole sheet again and extract cells */
	if (debug) fprintf(stderr, "%s: cell extraction\n", prog);
	fseek(fpin, datastart, SEEK_SET);
	row = col = 0;
	while (!feof(fpin))
	{
		type = readword(fpin);
		if (feof(fpin)) break;
		size = readword(fpin);
		if (debug) fprintf(stderr,"%s: record type %d size %d\n",prog,type,size);
		switch (type)
		{
			case SPR_RECORD_FORMULA:
				if (verbose) fprintf(stderr,"Skipping formula\n");
				SkipBytes(fpin, size);
				break;
			case SPR_RECORD_CELL:
				SPR_ReadCellRef(fpin, &row, &col);
				cell_contents = readbyte(fpin);
				cell_format = readbyte(fpin);
				size -= 6;
				if (row < 0) { fprintf(stderr, "%s: bad row number %d\n", prog, row); SkipBytes(fpin, size); continue; }
				if (col < 0) { fprintf(stderr, "%s: bad column number %d\n", prog, col); SkipBytes(fpin, size); continue; }
				cellp = &cells[row][col];
				cellp->type = (celltype_t) (cell_contents & 0x7);
				cellp->font = 0;
				SPR_GetCellFormat(cellp, cell_contents, cell_format);
				switch (cellp->type)
				{
					case cell_empty:
						SkipBytes(fpin, size);
						break;
					case cell_int:
						cellp->data.i = readword(fpin);
						if (verbose) fprintf(stderr,"%d,%d = %d\n", col, row, cellp->data.i);
						size -= DBF_SIZE_WORD;
						SPR_CellFont(fpin, cellp, &size);
						SkipBytes(fpin, size);
						break;
					case cell_float:
						cellp->data.f = readreal(fpin);
						if (verbose) fprintf(stderr,"%d,%d = %f\n", col, row, cellp->data.f);
						size -= DBF_SIZE_REAL;
						SPR_CellFont(fpin, cellp, &size);
						SkipBytes(fpin, size);
						break;
					case cell_string:
						cellp->data.s = readqstr(fpin);
						if (verbose) fprintf(stderr,"%d,%d = %s\n", col, row, cellp->data.s);
						size -= strlen(cellp->data.s)+1;
						SPR_CellFont(fpin, cellp, &size);
						SkipBytes(fpin, size);
						break;
					case cell_floatformula:
						cellp->formula = readword(fpin); /* index */
						if (verbose) fprintf(stderr,"%d,%d = number formula %d\n", col, row, cellp->data.i);
						cellp->data.f = readreal(fpin);
						size -= DBF_SIZE_WORD+DBF_SIZE_REAL;
						SPR_CellFont(fpin, cellp, &size);
						SkipBytes(fpin, size);
						break;
					case cell_stringformula:
						cellp->formula = readword(fpin); /* index */
						if (verbose) fprintf(stderr,"%d,%d = string formula %d\n", col, row, cellp->data.i);
						cellp->data.s = readqstr(fpin);
						size -= strlen(cellp->data.s)+1+DBF_SIZE_WORD;
						SPR_CellFont(fpin, cellp, &size);
						SkipBytes(fpin, size);
						break;
					default:
						fprintf(stderr,"%s: unknown cell type at (%d,%d)\n", prog, col, row);
						SkipBytes(fpin, size);
						break;
				}
				break;
			default:
				if (verbose) fprintf(stderr,"Skipping record type %d\n", type);
				SkipBytes(fpin, size);
				break;
		}
	}

	/* Go through sheet again and reduce size if trailing blank cells exist */
	if (debug) fprintf(stderr, "%s: sheet compression\n", prog);
	/* Remove blank rows from bottom */
	blank = 1;
	for (row=numrows-1; row>=0; row--)
	{
		for (col=0; col<numcols; col++)
			if (cells[row][col].type != cell_empty) { blank=0; break; }
		if (!blank)
			break;
	}
	numrows = row+1;
	/* Remove blank columns from right */
	blank = 1;
	for (col=numcols-1; col>=0; col--)
	{
		for (row=0; row<numrows; row++)
			if (cells[row][col].type != cell_empty) { blank = 0; break; }
		if (!blank)
			break;
	}
	numcols = col+1;

	/* Finally output sheet in TSV format */
	if (html) printf("<table border>\n");
	for (row=0; row<numrows; row++)
	{
		if (html) printf("<tr>");
		for (col=0; col<numcols; col++)
		{
			if (html) printf("<td>");
			SPR_PrintCell(&cells[row][col], row, col);
			if (html) printf("</td>");
			else printf("%c", col==numcols-1?'\n':'\t');
		}
		if (html) printf("</tr>\n");
	}
	if (html) printf("</table>\n");

	return(0);
}


/* ----------------------------------------------------------------------------
 * Main program
 */
int
main(int argc, char *argv[])
{
	char *sprfile = NULL;
	FILE *fpin = NULL;
	FILE *fpout = stdout;
	int i;

	prog = argv[0];
	GETOPT(i, options)
	{
		case 'p': pretty = 1; break;
		case 'h': html = 1; break;
		case 'v': verbose = 1; break;
		case 'd': debug = 1; break;
		case '?': fprintf(stderr,usage,prog); exit(1);
	}
	GETOPT_LOOP_REST(sprfile)
	{
		fpin = fopen(sprfile, "rb");
		if (fpin == NULL)
		{
			fprintf(stderr,"%s: cannot open %s\n",prog,sprfile);
			continue;
		}
		SPR_ReadSheet(fpin, fpout);
		fclose(fpin);
	}
	if (fpin == NULL)
		fprintf(stderr,usage,prog);
	fclose(fpout);
	return(0);
}

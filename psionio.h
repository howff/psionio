/* > psionio.h Copyright 1996 Andrew Brooks <arb@sat.dundee.ac.uk>
 * 1.03 arb
 */

#ifndef __PSIONIO_H
#define __PSIONIO_H

/* ASCII */
#ifndef A_CR
#define A_CR 13
#endif
#ifndef A_LF
#define A_LF 10
#endif

/* Defaults */
#define DECIMAL_POINT "."        /* decimal point for numbers        */
#define THOUSANDS_SEP ","        /* thousands separator for numbers  */
#define CURRENCY_CHAR "\243"     /* pound sign in iso-8859-1 (octal) */
#define CURRENCY_HTML "&#163;"   /* pound sign in iso-8859-1 (html)  */

/* File header info */
#define DBF_IDSTRING "OPLDatabaseFile"
#define DBF_VERCREAT 4111
#define DBF_VERREAD  4111
#define AGN_IDSTRING "AgendaFileType*"
#define AGN_VERCREAT 4111
#define AGN_VERREAD  0  /* unused */
#define SPR_IDSTRING "SPREADSHEET" /* needs zero padding */
#define SPR_VERCREAT 0
#define SPR_UKOFFSET 0
#define SPR_VEROPL   0
#define WRD_IDSTRING "PSIONWPDATAFILE"
#define WRD_VERENC   256
#define WRD_VERUNENC 1
/* DBF file limits */
#define DBF_MAXRECORDSIZE 4094
#define DBF_MAXQSTRSIZE 254
#define SPR_MAXRECORDSIZE 4094

/* DBF record types */
#define DBF_RECORD_DELETED      0
#define DBF_RECORD_DATA         1
#define DBF_RECORD_FIELDINFO    2
#define DBF_RECORD_DESCRIPTOR   3
#define DBF_RECORD_PRIVATESTART 4
#define DBF_RECORD_PRIVATEEND   7
#define DBF_RECORD_VOICEDATA   14
#define DBF_RECORD_RESERVED    15

/* DBF subrecord types */
#define DBF_SUBRECORD_TABSIZE       1
#define DBF_SUBRECORD_FIELDLABELS   4
#define DBF_SUBRECORD_DISPLAY       5
#define DBF_SUBRECORD_PRINTER       6
#define DBF_SUBRECORD_PRINTERDRIVER 7
#define DBF_SUBRECORD_HEADER        8
#define DBF_SUBRECORD_FOOTER        9
#define DBF_SUBRECORD_DIAMOND      10
#define DBF_SUBRECORD_SEARCH       11
#define DBF_SUBRECORD_AGENDA       15


/* DBF field types */
#define DBF_FIELD_WORD 0
#define DBF_FIELD_LONG 1
#define DBF_FIELD_REAL 2
#define DBF_FIELD_QSTR 3
#define DBF_FIELD_QSTR_CONTINUATION 20
#define DBF_FIELD_QSTR_LINEFEED     21

/* AGN record types */
#define AGN_RECORD_DELETED     0
#define AGN_RECORD_TIMEDDAY    1
#define AGN_RECORD_UNTIMEDDAY  2
#define AGN_RECORD_ANNIVERSARY 3
#define AGN_RECORD_TODO        4
#define AGN_RECORD_REPEAT      5
#define AGN_RECORD_ANON        6
#define AGN_RECORD_RESERVED7   7
#define AGN_RECORD_RESERVED8   8
#define AGN_RECORD_TODOLIST    9
#define AGN_RECORD_MEMOPREFS  10
#define AGN_RECORD_TODOPREFS  11
#define AGN_RECORD_VIEWPREFS  12
#define AGN_RECORD_PREFS      13
#define AGN_RECORD_PRINTPREFS 14
#define AGN_RECORD_INCOMPLETE 15

/* AGN repeat types */
#define AGN_REPEAT_DAILY         0
#define AGN_REPEAT_WEEKLY        1
#define AGN_REPEAT_MONTHLYBYDATE 2
#define AGN_REPEAT_MONTHLYBYDAYS 3
#define AGN_REPEAT_ANNUALLY      4

/* SPR record types */
#define SPR_RECORD_FORMULA        1
#define SPR_RECORD_CELL           2
#define SPR_RECORD_COLUMNS        3
#define SPR_RECORD_COLUMN         4
#define SPR_RECORD_STATUS         5
#define SPR_RECORD_DISPLAY        6
#define SPR_RECORD_RANGE          7
#define SPR_RECORD_PRINTRANGE     8
#define SPR_RECORD_DATABASE       9
#define SPR_RECORD_TABLE         10
#define SPR_RECORD_PRINTSETUP    11
#define SPR_RECORD_PRINTERFONT   12
#define SPR_RECORD_GRAPH         13
#define SPR_RECORD_CURRENTGRAPH  14
#define SPR_RECORD_FONTS         15
#define SPR_RECORD_PRINTER       16
#define SPR_RECORD_PRINTERDRIVER 17
#define SPR_RECORD_HEADER        18
#define SPR_RECORD_FOOTER        19
#define SPR_RECORD_SCREEN        20

/* SPR limits */
#define SPR_MAXROW 0x1FFF
#define SPR_MAXCOL 0x1FFF
#define SPR_MAXINT 0x7FFF

/* SPR cell types */
#define SPR_CELL_BLANK       0
#define SPR_CELL_REAL        1
#define SPR_CELL_TEXT        2
#define SPR_CELL_WORD        3
#define SPR_CELL_REALFORMULA 5
#define SPR_CELL_TEXTFORMULA 6

/* SPR cell formats */
#define SPR_TEXT_ALIGNLEFT    (1<<3)
#define SPR_NUMBER_ALIGNRIGHT (0<<5)
#define SPR_CELLFORMAT_FIXED      (0<<4)
#define SPR_CELLFORMAT_SCIENTIFIC (1<<4)
#define SPR_CELLFORMAT_GENERAL    (1)
#define SPR_CELLFORMAT_SPECIAL    (7<<4)
#define SPR_CELL_PROTECTED (1<<7)

/* SPR cell references in formulae */
#define SPR_CELL_PLUSOFFSET       0x8000
#define SPR_CELL_PLUSOFFSET_CODE  0x8000
#define SPR_CELL_MINUSOFFSET     0x10000
#define SPR_CELL_MINUSOFFSET_CODE 0xE000
#define SPR_CELL_CURRENT          0x8000

/* SPR functions (not all listed yet) */
#define SPR_FUNC_ERR    27
#define SPR_FUNC_FALSE  28
#define SPR_FUNC_NA     29
#define SPR_FUNC_PI     30
#define SPR_FUNC_RAND   31
#define SPR_FUNC_NOW    32
#define SPR_FUNC_TRUE   33
/* etc */

/* WRD record types, used singly in order */
#define WRD_RECORD_FILEINFO      1
#define WRD_RECORD_PRINTERSETUP  2
#define WRD_RECORD_PRINTERDRIVER 3
#define WRD_RECORD_HEADER        4
#define WRD_RECORD_FOOTER        5
#define WRD_RECORD_STYLE         6 /* repeated */
#define WRD_RECORD_EMPHASIS      7 /* repeated */
#define WRD_RECORD_TEXT          8
#define WRD_RECORD_STYLES        9

/* WRD paragraph types */
#define WRD_PARATYPE_PARA 0
#define WRD_PARATYPE_LINE 1

/* WRD characters */
#define WRD_CHAR_NOBRKHYPHEN  7
#define WRD_CHAR_SOFTHYPHEN  14
#define WRD_CHAR_NOBRKSPACE  15

/* Types */
#define Byte char
#define Word short
#define Long long
#define Real double

#define DBF_SIZE_BYTE sizeof(Byte)
#define DBF_SIZE_WORD sizeof(Word)
#define DBF_SIZE_LONG sizeof(Long)
#define DBF_SIZE_REAL sizeof(Real)


/*
 * Global data
 */
extern char *DBF_FieldTypeStr[];      /* string describing field type */
extern unsigned char DBF_DefaultDescriptor[];  /* default descriptor written to dbf */
extern int DBF_SizeDefaultDescriptor; /* size of above array */


#ifdef __cplusplus
extern "C" {
#endif


/*
 * Useful data and functions.
 * If you don't have getopt variables declared then uncomment them below.
 * If you don't have getopt at all then download the GNU version.
 * If you don't have strdup use the second definition instead.
 * strctok handles empty tokens properly but only allows char separators.
 * stradd adds two strings.
 */

extern int daysInMonth[];
extern char *dayName3[];
extern char *monthName3[];
extern char *optarg;
extern int   optind;
extern int   opterr;
/*extern char  getopt(int, const char**, const char*);*/
#define GETOPT(c, str)	while((c=getopt(argc, argv, str)) != -1) switch(c)
#define GETOPT_LOOP_REST(argp) \
	while((optind < argc)? (argp = argv[optind++]): (argp = NULL))
#define strcopy(S) strdup(S)
/*#define strcopy(S) strcpy((char*)malloc(strlen(S)+1),S)*/
extern char *strctok(char *str0, int ch);
extern char *stradd(char **str, char *append);
extern int leapyear(int year);
extern int dayofweek(int daynumber);
extern int daytodate(int daynumber, int *year, int *month, int *day);
extern char *cp2iso_block(char *str, int len);


/*
 * Decryption functions.
 */
int decrypt_block(unsigned char *block, int len, unsigned char *key);
void cryptkey(unsigned char *password, unsigned char key[]);


/*
 * Output functions.
 */
extern int writebyte(FILE *fp, Byte byte);
extern int writeword(FILE *fp, Word word);
extern int writelong(FILE *fp, Long lng);
extern int writereal(FILE *fp, Real real);
extern int writecstr(FILE *fp, char *cstr);
extern int writeqstr(FILE *fp, char *cstr);
extern int writemultiqstr(FILE *fp, char *cstr);
extern int writerecordtype(FILE *fp, int size, int type);
extern char *iso2html(unsigned char);

/*
 * Input functions.
 * Returned strings are allocated from the heap and must be freed manually.
 * Bugs: readcstr is limited to reading a fixed size as it doesn't know length
 * in advance (current limit is 4k, enough for a DBF record).
 */
extern Byte readbyte(FILE *fp);
extern Word readword(FILE *fp);
extern Long readlong(FILE *fp);
extern Real readreal(FILE *fp);
extern char *readcstr(FILE *fp);
extern char *readqstr(FILE *fp);
extern int readrecordtype(FILE *fp, int *size, int *type);


#ifdef __cplusplus
}
#endif

#endif /* !__PSIONIO_H */

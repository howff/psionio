/* > agn2tsv.h
 * 1.00 arb
 */

#ifndef __AGN2TSV_H
#define __AGN2TSV_H

/* Repeat list */
typedef struct Repeat_s
{
	struct Repeat_s *next;        /* next repeat entry in list */
	Byte repeattype;              /* repeat type: daily, weekly etc */
	Byte nextonly;                /* flag: only show next repeat */
	Byte interval;                /* interval between repeats */
	Word endday;                  /* last day of repeating entry (or -1) */
	Byte entrytype;               /* type of entry */
	Long entryposition;           /* position in file */
	Word *undays;                 /* days on which repeat is suppressed */
	/* Weekly */
	Byte days;                    /* days in the week (bitfield) */
	Byte firstday;                /* ? */
	/* Monthly by date */
	Long dates;                   /* dates to repeat (bitfield) */
	/* Monthly by day */
	Byte wdays[5];                /* days of week 1-4,last (bitfields) */
} Repeat_t;

/* Output array */
typedef struct Output_s
{
	Word day;
	Word time;
	char *text;
} Output_t;

#endif /* !__AGN2TSV_H */


/* > agn2tsv.c Copyright 1996 Andrew Brooks <arb@sat.dundee.ac.uk>
 * 1.15 arb Wed Feb 27 12:10:37 GMT 2013 - extract all memo data.
 * 1.14 arb Thu Nov 27 11:30:04 GMT 2003 - fix memo display memory allocation.
 * 1.13 arb Wed Nov  6 14:01:35 GMT 2002 - added -c to handle corrupt agenda.
 * 1.12 arb Tue Dec 18 11:14:24 GMT 2001 - show time length too, with mods from
 *      Guido Brugnara <gdo@leader.it>
 * 1.11 arb Thu Aug  9 12:16:39 BST 2001 - fix bug reading alarm information
 *      (would have caused mis-reading of all subsequent entries).
 * 1.10 arb Wed Aug  1 20:57:49 BST 2001 - added debug statement for unexpected
 *      repeat types and added undocumented environment variable to mask off
 *      the problem bits.  set AGN2TSV_MASKREPEATTYPE=1
 * 1.09 arb Mon Jun 22 12:58:54 BST 1998 - fix bug in suppressed repeats.
 * 1.08 arb Thu May 28 11:25:29 BST 1998 - remove unused seconds from TimeStr.
 * 1.07 arb Mon Nov 10 15:04:27 GMT 1997 - extract memos.
 * 1.06 arb Fri Jul 25 11:29:27 BST 1997 - kludge: repeating ToDo entries have
 *      their "due-by" and "show-from" dates stored differently!?
 * 1.05 arb Wed Jul 23 11:10:03 BST 1997 - handle ToDo dates better: show from
 *      warning date not just from to-do date.  Also show date to be done by.
 *      Show "today" for today's date.
 * 1.04 arb Thu May  8 15:06:32 BST 1997 - bug fix: show undated entries too.
 * 1.03 arb Wed May  7 10:01:28 BST 1997 - added monthly repeats, handle
 *      nextonly flag, bug fix: free(text) after strlen, bug fix: showing
 *      first occurrence of repeating entries.
 * 1.02 arb Mon May  5 14:39:14 BST 1997 - repeats implemented (except monthly),
 *      rewritten date and time routines, added options a,f,s,d,v.
 * 1.01 arb Tue Oct 29 10:41:41 GMT 1996 - remove hour subtract for GMT, but
 *      bug not fixed yet though.
 * 1.00 arb Fri Aug 23 10:17:22 BST 1996
 * Extract agenda details in TSV format to stdout.
 * Usage: agn2tsv [-a] [-c] [-d] [-f days] [-s] [-v] file.agn
 * -a    display all entries (default is to ignore those in the past)
 * -f n  repeated entries are repeated from present for n days (default n=14)
 * -m    show memos
 * -s    sort output by date
 * -d    debug
 * -v    verbose
 * -c    corruption check
 */

static char SCCSid[] = "@(#)agn2tsv.c     1.15 (C) 1997 arb AGN to TSV";

/*
 * Read through agenda looking for 'repeat' entries and note their details
 * and the position of the referred-to entries.
 * Read through again extracting entries and displaying any repeat details.
 */

/*
 * To do:
 * Speed up finding if entry is repeated by doing as first thing inside switch
 * so deleted entries etc do not search through repeat list.
 * FirstDay setting for weekly repeats (what does it mean?).
 * Display To-Do due dates with proper styles ("automatic", "days", "date").
 */

/*
 * Bugs:
 * Bug in documentation of due date for To-Do entries?  Kludge implemented.
 * ShowAll not fully checked for repeating entries.
 * Only first line of memo is displayed (memo format is not documented).
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include "psionio.h"
#include "agn2tsv.h"

/* Types */

/* Global variables */
char *prog;
char *usage = "usage: %s [-a] [-c] [-d] [-m] [-t] [-f days] [-s] [-v] file.agn\n"
	"-a\tshow all (past) entries\n"
	"-m\tshow memos\n"
	"-f n\trepeat entries for n days into the future\n"
	"-s\tsort output\n"
	"-t\tshow time length of each entry\n"
	"-d\toutput debugging information\n"
	"-v\toutput verbose information\n"
	"-c\tcorrupt agenda is searched for good entries\n";
char *options = "acdmtsvf:";
int futuredays = 14;
int showall = 0;
int sortoutput = 0;
int showmemo = 0;
int showtimelen = 0;
int corruptcheck = 0;
int verbose = 0;
int debug = 0;
Repeat_t *repeatlist = NULL;
int numrepeats = 0;
Output_t *outputarray = NULL;
int numoutput = 0;

Word day_today;
Word default_time = 6 * 60; /* 6am in minutes */

/* Defaults */
#define AGN_DAY1970 25569            /* SPR day number for 1/1/1970 */

#define SkipBytes(F,N) { int n=N; while(n>0 && n--) getc(F); }
/*void SkipBytes(FILE*fp,int n) { if (n<0) fprintf(stderr,"ERROR: SkipBytes(%d)\n",n); else while(n--) getc(fp); }*/

/*
 * Strings for debugging.
 */
char *rectypestr[] = { "deleted", "timedDay", "untimedDay", "anniv", "todo", "repeat", "anon", "reserved7", "reserved8", "todoList", "memoPrefs", "todoPrefs", "viewPrefs", "prefs", "printPrefs", "incomplete" };
char *reptypestr[] = { "daily", "weekly", "monthlyByDate", "monthlyByDays", "annually" };
char *Rectypestr(int n) { if (n>=0&&n<16) return rectypestr[n]; else return "BAD"; }
char *Reptypestr(int n) { if (n>=0&&n<5) return reptypestr[n]; else return "BAD"; }


/* ---------------------------------------------------------------------------
 * Date and time functions
 */
static void
AGN_GetCurrentTime()
{
	time_t now;

	time(&now);
	/* AGN day number origin is same as time_t origin: 1/1/1970 */
	day_today = now / 24 / 60 / 60;
}

int
AGN_DayOfWeek(int daynumber)
{
	/* 0=Monday */
	return((daynumber+3) % 7);
}

void
AGN_DayToDate(int daynumber, int *year, int *month, int *day)
{
	/* Add distance from 1900 to 1970 for SPR routine */
	daytodate(daynumber+AGN_DAY1970, year, month, day);
}

char *
AGN_TimeStr(Word day, Word mins)
{
	static char str[64];
	int D, M, Y;
	int h, m;

	if (day == -1)
		day = day_today;
	AGN_DayToDate(day, &Y, &M, &D);
	Y+=1900;
	h = mins / 60;
	m = mins - h*60;	
	if (mins==-1)
		sprintf(str, "%s %2d %s %04d %s",
			dayName3[AGN_DayOfWeek(day)], D, monthName3[M-1], Y,
			day==day_today?"today":"     ");
	else
		sprintf(str, "%s %2d %s %04d %02d:%02d",
			dayName3[AGN_DayOfWeek(day)], D, monthName3[M-1], Y, h, m);
	return str;
}


Word
AGN_ToDoDate(char **text, Word day_disp, Word day_todo, Word time, int flags)
{
	Word day;
	int datestyle;

	if (day_disp == -1) day_disp = day_today;
	if (day_todo == -1) day_todo = day_today;
	day = day_disp;
	datestyle = (flags & 0xf0) >> 4;
	/*fprintf(stderr, "%s: day_disp = %s\n", *text, AGN_TimeStr(day_disp,time));*/
	/*fprintf(stderr, "%s: day_todo = %s\n", *text, AGN_TimeStr(day_todo,time));*/
	/*fprintf(stderr, "%s: datestyle = %d\n", *text, datestyle);*/

	/* if "day_disp" has expired use todays date */
	/* unless "day_todo" has expired also */
	if (day_today >= day_disp && day_today <= day_todo)
		day = day_today;
	/* add date to be done by */
	if (day_today < day_todo && datestyle!=3)
	{
		stradd(text, "\t- do by ");
		stradd(text, AGN_TimeStr(day_todo, time));
	}
	return(day);
}


/* ---------------------------------------------------------------------------
 * Read Agenda file header
 */
int
AGN_ReadFileHeader(FILE *fp)
{
	char *id;
	Word hdrsize;

	id = readcstr(fp);
	if (strcmp(id, AGN_IDSTRING))
	{
		fprintf(stderr,"%s: not an agenda file\n",prog);
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


/* ---------------------------------------------------------------------------
 * Read repeating entry details and store for later use.
 */
int
AGN_AddRepeat(Repeat_t *repeat)
{
	Repeat_t *repeatcopy;

	repeatcopy = (Repeat_t*)malloc(sizeof(Repeat_t));
	if (repeatcopy == NULL)
		return(-1);
	(*repeatcopy) = (*repeat);
	/*memcpy(repeatcopy, repeat, sizeof(Repeat_t));*/
	repeatcopy->next = repeatlist;
	repeatlist = repeatcopy;
	numrepeats++;
	return(0);
}

int
AGN_ReadRepeats(FILE *fp)
{
	int recsize, rectype;
	Repeat_t repeat;
	long offset;

	if (debug) fprintf(stderr, "Reading Repeats...\n");
	AGN_ReadFileHeader(fp);
	while (!feof(fp))
	{
		offset = ftell(fp);
		readrecordtype(fp, &recsize, &rectype);
		if (feof(fp)) break;
		if (debug>1) fprintf(stderr, "[[%04lx %s size %d]]\n", offset, Rectypestr(rectype), recsize);
		if (rectype != AGN_RECORD_REPEAT)
		{
			SkipBytes(fp,recsize);
			continue;
		}
		repeat.repeattype = readbyte(fp);
		repeat.nextonly = (repeat.repeattype & 8) >> 3;
		repeat.repeattype = repeat.repeattype & (~8);
		/*if (getenv("AGN2TSV_MASKREPEATTYPE")) repeat.repeattype &= 7;*/
		repeat.interval = readbyte(fp);
		repeat.endday = readword(fp);
		repeat.entrytype = readbyte(fp);
		recsize -= 5;
		switch (repeat.repeattype)
		{
			case AGN_REPEAT_DAILY:
			case AGN_REPEAT_ANNUALLY:
				repeat.entryposition = readlong(fp);
				recsize -= 4;
				break;
			case AGN_REPEAT_WEEKLY:
				repeat.days = readbyte(fp);
				repeat.firstday = readbyte(fp);
				repeat.entryposition = readlong(fp);
				recsize -= 6;
				break;
			case AGN_REPEAT_MONTHLYBYDATE:
				repeat.dates = readlong(fp);
				repeat.entryposition = readlong(fp);
				recsize -= 8;
				break;
			case AGN_REPEAT_MONTHLYBYDAYS:
				repeat.wdays[0] = readbyte(fp);
				repeat.wdays[1] = readbyte(fp);
				repeat.wdays[2] = readbyte(fp);
				repeat.wdays[3] = readbyte(fp);
				repeat.wdays[4] = readbyte(fp);
				repeat.entryposition = readlong(fp);
				recsize -= 9;
				break;
			default:
				fprintf(stderr,"INTERNAL ERROR: Unknown repeat type %d\n",repeat.repeattype);
				break;
		}
		if (debug) fprintf(stderr,"entry at 0x%04lx repeats, type %s\n", repeat.entryposition, Reptypestr(repeat.repeattype));
		/* Suppressed days */
		repeat.undays = NULL;
		if (recsize>1)
		{
			int daynum=0;
			repeat.undays = (Word*)malloc(recsize+4);
			if (repeat.undays == NULL)
				return(-1);
			while (recsize>1)
			{
				repeat.undays[daynum++] = readword(fp);
				recsize -= 2;
			}
			/* Terminate day list with a zero day number */
			repeat.undays[daynum] = 0;
		}
		SkipBytes(fp, recsize);
		AGN_AddRepeat(&repeat);
	}
	return(0);
}


/*
 * Check if this entry should be repeated
 */
Repeat_t *
AGN_FindRepeat(long curpos)
{
	Repeat_t *repeatptr;

	repeatptr = repeatlist;
	while (repeatptr)
	{
		if (repeatptr->entryposition == curpos)
			break;
		repeatptr = repeatptr->next;
	}
	return(repeatptr);
}


/* ---------------------------------------------------------------------------
 * Entry output.
 */
int
AGN_Compare(const Output_t *o1, const Output_t *o2)
{
	Word d1 = o1->day, d2 = o2->day;
	Word t1 = o1->time, t2 = o2->time;
	if (d1 == -1) d1 = day_today;
	if (d2 == -1) d2 = day_today;
	if (o1->day < o2->day) return(-1);
	if (o1->day > o2->day) return(+1);
	/* same day so compare time */
	if (t1 == -1) t1 = default_time;
	if (t2 == -1) t2 = default_time;
	if (t1 < t2) return(-1);
	if (t1 > t2) return(+1);
	return(0);
}

int
AGN_Sort(FILE *fp)
{
	int i;

	qsort(outputarray, numoutput, sizeof(Output_t), (int(*)(const void*, const void*)) AGN_Compare);
	for (i=0; i<numoutput; i++)
	{
		fprintf(fp, "%s\n", outputarray[i].text);
	}
	return(0);
}

int
AGN_AddOutput(Word day, Word time, char *text)
{
	numoutput++;
	if (outputarray == NULL)
		outputarray = (Output_t*)malloc(sizeof(Output_t));
	else
		outputarray = (Output_t*)realloc(outputarray, sizeof(Output_t)*numoutput);
	if (outputarray == NULL)
		return(-1);
	outputarray[numoutput-1].day  = day; 
	outputarray[numoutput-1].time = time; 
	outputarray[numoutput-1].text = text; 
	return(0);
}

int
AGN_PrintEntry(FILE *fp, Word day, Word time, Word t_length, char *text, char *memo)
{
	char *str;
	int len;

	len = strlen(text) + 64;
	if (memo) len += strlen(memo+14);
	str = (char*)malloc(len);
	if (str == NULL)
		return(-1);
	if (showtimelen)
		sprintf(str, "%s\t%d\t%s", AGN_TimeStr(day, time), t_length, text);
	else
		sprintf(str, "%s\t%s", AGN_TimeStr(day, time), text);
	if (memo)
	{
		strcat(str, "\t");
		strcat(str, memo);
	}
	if (sortoutput)
	{
		AGN_AddOutput(day, time, str);
		/* don't free(str) as it's part of array now */
	}
	else
	{
		fprintf(fp, "%s\n", str);
		free(str);
	}
	return(0);
}


/*
 * Check day number is in range and not on suppressed list before printing.
 */
int
AGN_CheckPrintEntry(FILE *fp, Word day, Word time, Word t_length, Repeat_t *repeat, char *text, char *memo)
{
	int daynum;
	/* Check if day in range */
	if (day<day_today || day>day_today+futuredays)
		return(0);
	if (repeat->endday!=-1 && day>repeat->endday)
		return(0);
	/* Check suppressed days */
	if (repeat->undays)
	{
		for (daynum=0; repeat->undays[daynum]; daynum++)
		{
			if (day == repeat->undays[daynum])
				return(0);
		}
	}
	return AGN_PrintEntry(fp, day, time, t_length, text, memo);
}


/*
 * Display an entry taking note of repeats to show only current/future events.
 */
int
AGN_DisplayEntryWithRepeats(FILE *fp, Word day, Word time, Word t_length, Repeat_t *repeat, char *text, char *memo)
{
	int i;

	/* Undated entries (eg.ToDo) considered to be today */
	if (day == -1)
		day = day_today;
	if (debug) fprintf(stderr,"today=%d day=%d (%s) l=%dm endday=%d text=%s%s\n", day_today, day, AGN_TimeStr(day,time), t_length, repeat?repeat->endday:0, text, repeat?" (Repeats)":"");
	/* Ignore past entries which are not repeated */
	if (day<day_today && repeat==NULL && showall==0)
		return(0);
	/* Ignore past repeated entries which end before today */
	if (day<day_today && repeat && repeat->endday!=-1 && repeat->endday<day_today)
		return(0);
	/* Ignore entries which start way into the future */
	if (day>day_today+futuredays && showall==0)
		return(0);
	/* If not repeating then must be in range so display and return */
	if (repeat == NULL)
	{
		return AGN_PrintEntry(fp, day, time, t_length, text, memo);
	}
	if (debug) fprintf(stderr,"repeat type=%s nextonly=%d interval=%d endday=%d, text=%s\n", Reptypestr(repeat->repeattype), repeat->nextonly, repeat->interval, repeat->endday, text);
	/* Print first and search for subsequent occurrences within our window */
	while (1)
	{
		switch (repeat->repeattype)
		{
			case AGN_REPEAT_DAILY:
				/*fprintf(stderr,"Daily: interval=%d: %s\n",repeat->interval,text);*/
				AGN_CheckPrintEntry(fp, day, time, t_length, repeat, text, memo);
				day += repeat->interval+1;
				break;
			case AGN_REPEAT_ANNUALLY:
			{
				int year, month, mday;
				/*fprintf(stderr,"Annual: interval=%d: %s\n",repeat->interval,text);*/
				AGN_CheckPrintEntry(fp, day, time, t_length, repeat, text, memo);
				for (i=0; i<repeat->interval+1; i++)
				{
					AGN_DayToDate(day, &year, &month, &mday);
					if (month<3 && leapyear(year)) day+=366;
					else if (month>2 && leapyear(year+1)) day+=366;
					else day += 365;
				}
				break;
			}
			case AGN_REPEAT_WEEKLY:
			{
				int weekday;
				weekday = AGN_DayOfWeek(day);
				/*fprintf(stderr,"Weekly: starts on day %d, DayOfWeek %d, days bitfield=%d, interval=%d: %s\n", day, weekday, repeat->days, repeat->interval, text);*/
				for (i=0; i<7; i++)
				{
					/* Check each day of the week */
					if (repeat->days & (1 << ((weekday+i)%7)))
						AGN_CheckPrintEntry(fp, day, time, t_length, repeat, text, memo);
					day++;
					if (repeat->nextonly)
						break;
				}
				day += 7*(repeat->interval /* NB not +1 */);
				break;
			}
			case AGN_REPEAT_MONTHLYBYDATE:
			{
				int year, month, mday;
				AGN_DayToDate(day, &year, &month, &mday);
				/*fprintf(stderr,"MonthlyByDate: dates=%d: %s\n", repeat->dates, text);*/
				/*fprintf(stderr,"checking days from %d to %d in month %d\n",mday,daysInMonth[month-1],month);*/
				for (i=mday; i<=daysInMonth[month-1]; i++)
				{
					if (repeat->dates & (1 << (i-1)))
						AGN_CheckPrintEntry(fp, day, time, t_length, repeat, text, memo);
					day++;
					if (repeat->nextonly)
						break;
				}
				for (i=0; i<repeat->interval/*not+1*/; i++)
					day += daysInMonth[(month+i/*not-1*/) % 12];
				break;
			}
			case AGN_REPEAT_MONTHLYBYDAYS:
			{
				int year, month, mday;
				int monthday1; /* what day of the week is the 1st this month */
				int weeknum, daynum; /* loop counters */
				int daywanted[31]; /* flag: day of month matches repeat pattern */
				int date;
				/*fprintf(stderr,"MonthlyByDays: 1=%d, 2=%d, 3=%d, 4=%d, last=%d: %s\n", repeat->wdays[0], repeat->wdays[1], repeat->wdays[2], repeat->wdays[3], repeat->wdays[4], text);*/
				AGN_DayToDate(day, &year, &month, &mday);
				monthday1 = AGN_DayOfWeek(day-mday+1);
				for (daynum=0; daynum<31; daynum++)
					daywanted[daynum]=0;
				/* Inner loop converts day and week number into date */
				for (weeknum=0; weeknum<5; weeknum++)
				{
					for (daynum=0; daynum<7; daynum++)
					{
						date = weeknum*7 + ((7-monthday1)+daynum)%7 +1;
						if (date > daysInMonth[month-1])
							date -= 7;
						if (repeat->wdays[weeknum] & (1 << daynum))
							daywanted[date-1] = 1;
					}
				}
				/*for (daynum=0; daynum<31; daynum++) if (daywanted[daynum]) fprintf(stderr,"Month %d day %d should be checked\n",month,daynum+1);*/
				/*fprintf(stderr,"Checking days %d to %d\n",mday,daysInMonth[month-1]);*/
				for (daynum=mday; daynum<=daysInMonth[month-1]; daynum++)
				{
					if (daywanted[daynum-1])
						AGN_CheckPrintEntry(fp, day, time, t_length, repeat, text, memo);
					day++;
				}
				for (i=0; i<repeat->interval/*not+1*/; i++)
					day += daysInMonth[(month+i/*not-1*/) % 12];
				break;
			}
			default:
			{
				fprintf(stderr, "[UNKNOWN REPEAT %d]\n", repeat->repeattype);
				break;
			}
		}
		if (repeat->nextonly)
			break;
		if (day>day_today+futuredays)
			break;
		if (repeat->endday!=-1 && day>repeat->endday)
			break;
	}
	return(0);
}


/* ---------------------------------------------------------------------------
 * Convert the memo buffer in-place:
 * removes the 14-byte header,
 * changes all non-printing characters (including newlines) to full stops,
 * terminates at the first BTNN which always seems to occur after the text.
 * memo buffer is altered,
 * memolen is unchanged,
 * Returns zero at the moment.
 */
int
AGN_ConvertMemo(char *memo, int memolen)
{
	int i;
	if (memolen < 14 + 5)
		return(0);
	/* Skip the fixed size header, unknown contents */
	memmove(memo, memo+14, memolen-14);
	memolen -= 14;
	/* Seek the first occurrence of BTNN which always seems to occur */
	/* after the memo data and terminate the memo at this point */
	for (i=0; i<memolen-4; i++)
	{
		if (strncmp(memo+i, "BTNN", 4)==0)
		{
			memolen = i;
			memo[i] = '\0';
		}
	}
	/* Make into a single printable string. */
	for (i=0; i<memolen; i++)
	{
		if (!isprint(memo[i]))
				memo[i] = '.';
	}
	return(0);
}


/* ---------------------------------------------------------------------------
 * Check attributes: crossed out, alarm, memo.
 * Return memo if present otherwise NULL.
 * May modify recsize (and fpin) if alarm or memo parts present.
 */
int
AGN_CheckAttr(int attr, char **memo, FILE *fpin, int *recsize)
{
	char alarm[12];
	/* Crossed out */
	if ((attr & (1<<1)) == 0)
	{
		if (debug) fprintf(stderr, "entry is crossed out\n");
		return(0);
	}
	/* Alarm part, skip it to get at memo part */
	if ((attr & (1<<3)) == 0)
	{
		char *alarmname = alarm+3;
		fread(alarm, 11, 1, fpin);
		alarm[11] = '\0';
		*recsize -= 11;
		if (alarm[3] == 1) alarmname = "(rings)";
		if (alarm[3] == 2) alarmname = "(chimes)";
		if (alarm[3] == 16) alarmname = "(silent)";
		if (strcmp(alarmname, "SYS$AL01")==0) alarmname = "(fanfare)";
		if (strcmp(alarmname, "SYS$AL02")==0) alarmname = "(soft bells)";
		if (strcmp(alarmname, "SYS$AL03")==0) alarmname = "(church bell)";
		if (debug) fprintf(stderr, "skipping alarm %s\n", alarmname);
	}
	/* Memo part */
	if ((attr & (1<<4)) == 0 && showmemo)
	{
		Word memolen;

		if (debug) fprintf(stderr, "reading memo\n");
		memolen = readword(fpin);
		*recsize -= sizeof(Word);
		*memo = (char*)malloc(memolen+1);
		fread(*memo, memolen, 1, fpin);
		AGN_ConvertMemo(*memo, memolen);
		/* was just (*memo)[memolen] = '\0'; */
		*recsize -= memolen;
	}
	else
	{
		*memo = NULL;
	}
	return(0);
}


/* ---------------------------------------------------------------------------
 * Read agenda and output in TSV format
 */
int
AGN_ReadAgenda(FILE *fpin, FILE *fpout)
{
	int recsize, rectype;
	Repeat_t *repeat;
	Word day_todo, day, time, time_length;
	int attr, todo_flags;
	char *text, *memo;
	long offset;

	if (debug) fprintf(stderr, "Reading Agenda...\n");
	AGN_ReadFileHeader(fpin);
	while (!feof(fpin))
	{
		offset = ftell(fpin);
		if (debug) fprintf(stderr,"0x%04lx\t", offset);
		repeat = AGN_FindRepeat(offset);
		readrecordtype(fpin, &recsize, &rectype);
		if (feof(fpin)) break;
		if (debug) fprintf(stderr, "[%stype %s size %d]\t", repeat?"(repeats) ":"", Rectypestr(rectype), recsize);
		switch (rectype)
		{
			case AGN_RECORD_TIMEDDAY:
				day = readword(fpin);
				time = readword(fpin);
				attr = readbyte(fpin);
				readbyte(fpin); /* symbol in Year view */
				time_length = readword(fpin); /* length in minutes */
				readbyte(fpin); /* text style */
				text = readqstr(fpin);
				AGN_CheckAttr(attr, &memo, fpin, &recsize);
				AGN_DisplayEntryWithRepeats(fpout, day, time, time_length, repeat, text, memo);
				/*fprintf(fpout, "%s\t%s\n", AGN_TimeStr(day, time), text);*/
				recsize -= (strlen(text)+1 + 9);
				free(text);
				SkipBytes(fpin,recsize);
				break;
			case AGN_RECORD_ANNIVERSARY:
				day = readword(fpin);
				time = readword(fpin);
				attr = readbyte(fpin);
				readbyte(fpin); /* symbol in Year view */
				readword(fpin); /* start year */
				readbyte(fpin); /* year display */
				readbyte(fpin); /* text style */
				text = readqstr(fpin);
				AGN_CheckAttr(attr, &memo, fpin, &recsize);
				AGN_DisplayEntryWithRepeats(fpout, day, time, -1, repeat, text, memo);
				recsize -= (strlen(text)+1 + 10);
				free(text);
				SkipBytes(fpin,recsize);
				break;
			case AGN_RECORD_TODO:
				day = readword(fpin);       /* day number to display from */
				time = readword(fpin);
				attr = readbyte(fpin);
				readbyte(fpin);             /* symbol in Year view */
				day_todo = readword(fpin);  /* day number to be done by */
				readbyte(fpin);             /* to-do list code */
				todo_flags = readbyte(fpin);
				readlong(fpin);             /* position in manual order */
				readbyte(fpin);             /* text style */
				text = readqstr(fpin);
				recsize -= (strlen(text)+1 + 15);
				/* XXX Kludge: days are stored oddly for repeating ToDos */
				{
					Word diff = day_todo - day;
					Repeat_t *rep = AGN_FindRepeat(ftell(fpin)-(strlen(text)+16+2));
					/*if (rep) { fprintf(stderr,"Entry repeats: type=%d, interval=%d, nextonly=%d, endday=%s\n", rep->repeattype, rep->interval, rep->nextonly, AGN_TimeStr(rep->endday, -1)); }*/
					if (rep != NULL)
					{
						day_todo = day;   /* day_disp actually holds due date */
						day -= diff;      /* and day_todo hold date PLUS warning days! */
					}
				}
				day = AGN_ToDoDate(&text, day, day_todo, time, todo_flags);
				AGN_CheckAttr(attr, &memo, fpin, &recsize);
				AGN_DisplayEntryWithRepeats(fpout, day, time, -1, repeat, text, memo);
				free(text);
				SkipBytes(fpin,recsize);
				break;
			case AGN_RECORD_UNTIMEDDAY:
				day = readword(fpin);
				time = readword(fpin);
				attr = readbyte(fpin);
				readbyte(fpin); /* symbol in Year view */
				readbyte(fpin); /* text style */
				text = readqstr(fpin);
				AGN_CheckAttr(attr, &memo, fpin, &recsize);
				AGN_DisplayEntryWithRepeats(fpout, day, time, -1, repeat, text, memo);
				recsize -= (strlen(text)+1 + 7);
				free(text);
				SkipBytes(fpin,recsize);
				break;
			case AGN_RECORD_REPEAT:
				if (debug) fprintf(stderr,"Skipping Repeat entry (already read)\n");
				SkipBytes(fpin,recsize);
				continue;
			case AGN_RECORD_DELETED:
				if (debug) fprintf(stderr,"Skipping deleted entry\n");
				SkipBytes(fpin,recsize);
				continue;
			case AGN_RECORD_TODOLIST:
				if (debug) fprintf(stderr,"Skipping ToDoList entry\n");
				SkipBytes(fpin,recsize);
				continue;
			case AGN_RECORD_MEMOPREFS:
				if (debug) fprintf(stderr,"Skipping MemoPrefs entry\n");
				SkipBytes(fpin,recsize);
				continue;
			case AGN_RECORD_TODOPREFS:
				if (debug) fprintf(stderr,"Skipping ToDoPrefs entry\n");
				SkipBytes(fpin,recsize);
				continue;
			case AGN_RECORD_VIEWPREFS:
				if (debug) fprintf(stderr,"Skipping ViewPrefs entry\n");
				SkipBytes(fpin,recsize);
				continue;
			case AGN_RECORD_PREFS:
				if (debug) fprintf(stderr,"Skipping Preferences entry\n");
				SkipBytes(fpin,recsize);
				continue;
			case AGN_RECORD_PRINTPREFS:
				if (debug) fprintf(stderr,"Skipping PrintPrefs entry\n");
				SkipBytes(fpin,recsize);
				continue;
			default:
				if (debug) fprintf(stderr,"Skipping record type %d, length %d\n",rectype,recsize);
				if (corruptcheck)
				{
					fseek(fpin, -1, SEEK_CUR);
					if (verbose) fprintf(stderr, "Corrupt agenda detected\n");
					if (debug) fprintf(stderr, "Searching for good entry\n");
				}
				else
					SkipBytes(fpin,recsize);
				continue;
		}
	}
	return(0);
}


int
main(int argc, char *argv[])
{
	char c, *arg;
	char *agnfile = NULL;
	FILE *fpin;
	FILE *fpout = stdout;

	prog = argv[0];
	GETOPT(c,options)
	{
		case 'a': showall = 1; break;
		case 'c': corruptcheck = 1; break;
		case 'd': debug++; break;
		case 'm': showmemo = 1; break;
		case 'v': verbose++; break;
		case 's': sortoutput=1; break;
		case 't': showtimelen = 1; break;
		case 'f': futuredays=atoi(optarg); break;
		case '?': fprintf(stderr,usage,prog); exit(1);
	}
	GETOPT_LOOP_REST(arg)
	{
		if (agnfile == NULL)
			agnfile = arg;
		else
		{
			fprintf(stderr,usage,prog);
			exit(1);
		}
	}
	if (agnfile == NULL)
	{
		fprintf(stderr,usage,prog);
		exit(1);
	}
	fpin = fopen(agnfile, "rb");
	if (fpin == NULL)
	{
		fprintf(stderr,"%s: cannot open %s\n",prog,agnfile);
		exit(1);
	}

	AGN_GetCurrentTime();
	if (debug) fprintf(stderr, "Today is %s\n", AGN_TimeStr(day_today, -1));
	AGN_ReadRepeats(fpin);
	rewind(fpin);  /* fseek(fpin, 0L, 0); */
	AGN_ReadAgenda(fpin, fpout);
	if (sortoutput)
		AGN_Sort(fpout);
	fclose(fpin);
	fclose(fpout);

	return(0);
}

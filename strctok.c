/* > strctok.c
 * 1.00 arb Tue Aug  6 12:31:49 BST 1996
 */

static char SCCSid[] = "@(#)strctok.c     1.00 (C) 1996 arb arbLib: strctok";


/*
 * My version of strtok, but only takes a char as second arg, not a string.
 * Seems to work, unlike normal strtok which, given a separator of tab, will
 * skip over consecutive tabs rather than returning an empty string.
 */

/*
 * Bugs:
 */

/*
 * To do:
 */

#include <stdio.h>        /* for sprintf */
#include <stdlib.h>       /* for malloc */
#include <string.h>       /* for memcpy */
#include "arb/utils.h"



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
	return(NULL);
}

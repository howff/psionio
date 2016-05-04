/*
 * A naive attempt to read Psion5 DBF files.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char *r0="";   /* record start */
char *r1="\n";   /* record end */
char *f0="\""; /* field start */
char *f1="\""; /* field end */
char *fs=",";  /* field sep */

#define DB_SEP 0x55

char *
zstring(char *buf, int *offset)
{
	unsigned char length;
	char *str;
	buf += *offset;
	length = *(unsigned char*)buf++;
	str = (char*)malloc(length+1);
	strncpy(str, buf, length);
	*offset+=length+1;
	str[length] = '\0';
	return(str);
}

void
process(char *buf, long length)
{
	char *name, *phone, *addr, *keywords;
	unsigned char sep;
	int offset = 0xe4;

	while (offset < length && buf[offset++] )
	{
		name = zstring(buf, &offset);
		phone = zstring(buf, &offset);
		addr = zstring(buf, &offset);
		keywords = zstring(buf, &offset);
		printf("%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s", r0,
				f0, name, f1, fs,
				f0, phone, f1, fs,
				f0, addr, f1, fs,
				f0, keywords, f1,
				r1);
		free(name);
		free(phone);
		free(addr);
		free(keywords);
	}
}

int
main(int argc, char *argv[])
{
	char *filename = "addr";
	char *prog;
	char *usage = "usage: %s\n";
	FILE *fp;
	long size;
	char *buf;

	prog = argv[0];
	fp = fopen(filename, "r");
	if (fp == NULL) { fprintf(stderr, "cannot open %s\n", filename); exit(1); }
	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	rewind(fp);
	buf = (char*)malloc(size);
	/* use fread or mmap */
	if (fread(buf, size, 1, fp) != 1) fprintf(stderr, "read failed\n");
	fclose(fp);
	process(buf, size);
	return(0);
}

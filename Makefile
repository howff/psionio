#
# Configuration:
# You may need to change CC and CFLAGS
# Change your machine to one of Sun (-DSUN), PC (-DPC), Acorn (-DACORN),
# Linux x86 (-DLINUX_PC), Linux Sparc (-DLINUX_SPARC),
# or give the byte ordering explicitly: -D_LITTLE_ENDIAN or -D_BIG_ENDIAN
# If you do not know which is correct just try one and see whether the
# conversions work correctly!

# Sun cc
CC=cc -v

# gcc
CC=gcc -Wall

# Solaris SPARC
XARCH=-DSUN

# Linux x86
XARCH=-DLINUX_PC

CFLAGS=-O $(XARCH) -DCONVERTCHARS

#
# No more configuration necessary
#
SRCS=psionio.c
OBJS=${SRCS:.c=.o}
LIBS=
MATHLIB=-lm
PROGS=agn2tsv dbf2tsv spr2tsv tsv2dbf tsv2spr wrd2html csv2tsv tsv2csv
ZIPFILES=README INSTALL MANUAL CHANGES Makefile Makefile.pc README.LINUX README.SOLARIS
ZIPPROGS=psionio.c psionio.h agn2tsv.c agn2tsv.h dbf2tsv.c spr2tsv.c tsv2dbf.c tsv2spr.c wrd2html.c csv2tsv.c tsv2csv.c getopt.c
ZIPBINS=agn2tsv dbf2tsv spr2tsv tsv2dbf tsv2spr wrd2html csv2tsv tsv2csv
ZIPPCBINS=agn2tsv.exe dbf2tsv.exe spr2tsv.exe tsv2dbf.exe tsv2spr.exe wrd2html.exe csv2tsv.exe tsv2csv.exe

all: $(PROGS)

agn2tsv: agn2tsv.c $(OBJS)
	$(CC) $(CFLAGS) -o $@ agn2tsv.c $(OBJS) $(LIBS)

dbf2tsv: dbf2tsv.c $(OBJS)
	$(CC) $(CFLAGS) -o $@ dbf2tsv.c $(OBJS) $(LIBS)

pic2ps: pic2ps.c $(OBJS)
	$(CC) $(CFLAGS) -o $@ pic2ps.c $(OBJS) $(LIBS)

spr2tsv: spr2tsv.c $(OBJS)
	$(CC) $(CFLAGS) -o $@ spr2tsv.c $(OBJS) $(LIBS) $(MATHLIB)

tsv2dbf: tsv2dbf.c $(OBJS)
	$(CC) $(CFLAGS) -o $@ tsv2dbf.c $(OBJS) $(LIBS)

tsv2spr: tsv2spr.c $(OBJS)
	$(CC) $(CFLAGS) -o $@ tsv2spr.c $(OBJS) $(LIBS) $(MATHLIB)

wrd2html: wrd2html.c $(OBJS)
	$(CC) $(CFLAGS) -o $@ wrd2html.c $(OBJS) $(LIBS)

csv2tsv: csv2tsv.c $(OBJS)
	$(CC) $(CFLAGS) -o $@ csv2tsv.c $(OBJS) $(LIBS)

tsv2csv: tsv2csv.c $(OBJS)
	$(CC) $(CFLAGS) -o $@ tsv2csv.c $(OBJS) $(LIBS)

# wrd2txt is not arb's
wrd2txt: wrd2txt.c
	$(CC) $(CFLAGS) -o $@ wrd2txt.c $(OBJS) $(LIBS)

# dopic is not arb's
dopic: dopic.c
	$(CC) $(CFLAGS) -o $@ dopic.c

# common objs
psionio.o: psionio.c
	$(CC) $(CFLAGS) -c psionio.c

# dependencies
psionio.o: psionio.h

# tidy
clean:
	rm -f $(PROGS) *.o *.obj core

# archive  (-y stores links)
zip: $(PROGS)
	@rm -f psionio.zip psiolinux.zip psiosol.zip psiopc.zip
	zip -y psionio.zip $(ZIPFILES) $(ZIPPROGS)
	zip -y psiobin.zip $(ZIPFILES) $(ZIPPROGS) $(ZIPBINS)
	zip -y psiopc.zip $(ZIPFILES) $(ZIPPROGS) $(ZIPPCBINS)
	@chmod g+r psionio.zip psiobin.zip
	@echo Note: psiobin.zip has been created containing the current executables

# archive onto web site
INDIR=/users/local/arb/HTML/Public/psion
INZIP=$(INDIR)/psionio.zip
INDOC=$(INDIR)/index.html
INPCZIP=$(INDIR)/psiopc.zip

installzip: zip
	cp psiopc.zip $(INPCZIP)
	cp psionio.zip $(INZIP)
	cp CHANGES  $(INDIR)/changes.txt
	chmod g+r $(INZIP) $(INPCZIP) $(INDIR)/changes.txt
	unzip -l $(INZIP) | grep ' [0-9][0-9]:[0-9][0-9] ' | sed -f date.sed | sort -r +1 | replace -i $(INDOC) -o $(INDOC) -f ZIP_START -t ZIP_END -
	grep SCCS $(ZIPPROGS) | sed -f sccs.sed | replace -i $(INDOC) -o $(INDOC) -f SOURCE_START -t SOURCE_END -
	@echo ===== NOTE =====
	@echo psionio.zip and psiopc.zip have been installed
	@echo install psiobin.zip manually depending on architecture

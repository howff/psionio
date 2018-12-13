# psionio
Psion 3 file conversion

## Description
Programs for reading and writing Data (DBF) files and Sheet
(SPR) files, and reading Agenda (AGN) and Word (WRD) files.

These programs are simple command-line driven utilities for
converting Psion files.  They should compile and run fine
on Unix machines, PCs (DOS, Windows or NT) and other machines.

This is release 3.94: the differences from release 1 include
Sheet support, repeating Agenda entry support, plus some bug
fixes and minor modifications.  The differences from release
2 are the inclusion of WRD support and the ability to handle
databases containing field types other than just strings.

## Installation
Please read the INSTALL file.  You MUST make the necessary
changes to the Makefile regarding byte ordering.

## Licence
This software is PostCardWare; if you use this software you are
asked to send the author a postcard of where you live.

These file are all Copyright 1996-2001 Andrew Brooks.  They may
be distributed freely as long as the following conditions are
observed:
1) All files are distributed with no modifications,
2) Any bugs, comments, suggestions are sent to the author,
3) A postcard of where you live is sent to the author (see below).

## Programs
agn2tsv  - Convert Agenda file to TSV format  [TSV=Tab Separated Value]
dbf2tsv  - Convert Data file to TSV format
spr2tsv  - Convert Sheet file to TSV format or to an HTML table
tsv2dbf  - Convert TSV format file to Data file
tsv2spr  - Convert TSV format file to Sheet file
wrd2html - Convert WRD file to HTML or plain text.
tsv2csv  - Convert TSV format file to CSV
csv2tsv  - Convert CSV format file to TSV

## Limitations / bugs
spr2tsv  Does not display formulas.
tsv2spr  Guesses at distinction between integer and real numbers.
wrd2html Not all style variations are handled yet.

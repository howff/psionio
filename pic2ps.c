/* > pic2ps.c  (c) 1997 Andrew Brooks arb@sat.dundee.ac.uk
 * 1.00
 * Convert SpreadSheet graph (.PIC) files to PostScript.
 * NB. Not for bitmap .PIC format files.
 */

/*
Source: http://www.wotsit.demon.co.uk/formats/pic/picfile.txt

	  LOTUS 1-2-3 (tm) .PIC FILE FORMAT DETAILS
 
	This is a partial decoding of the sample pie-chart file
FACILITY.PIC which comes on the PrintGraph disk (or did,any- 
way, on mine). The HEX listing is the data you would see on a
DEBUG display of the file's contents; the rest of each line is 
my interpretation of the meaning, verified in most cases by 
patching other data into the file, making a plot, and seeing 
what effect the changes made.
 
HEX	MEANING OF DATA
------	-----------------------------------------------------
0100	Header Word 1, value 0001 (bytes reversed in file)
0000	Header Word 2
0100	Header Word 3
0800	Header Word 4
4400	Header Word 5
0000	Header Word 6
	significance of these 6 header words is not known, but
	changes to them often result in Divide Overflow errors
	when PrintGraph attempts to plot the changed file.
000C	X-coordinate scaling factor, decimal 3072
7F09	Y-coordinate scaling factor, decimal 2431
	changes to these scaling factors, within reason, will
	change the size of the plotted graph on the page
06	Start of Plot Data. From here on, 16-bit quantities
	are stored in the file high byte first, then low -- the
	exact reverse of 8086/8088 standard used in the first
	six words.
A7	Select Font...
  01	...number 2
B0	Select Color number 1 (B1=Color 2, etc.)
A700	Select Font 1
AC	Set Character Size to... 
  008C	...X=140 and...
  0078	...Y=120. Note hi-lo byte reversal.
A0	Move, with pen up, to...
  063F	...X=1599 and...
  0906	...Y=2310. Coordinate 0,0 is at lower left of screen.
A8	Print label...
  02	...centered horizontally below point 1599,2310...
    42	B	(ASCII characters follow, for label)
    55	U
    44	D
    47	G
    45	E
    54	T
    00	...end of label
AC	Set Character Size to...
  0046	...X=70 and...
  003C	...Y=60
A701	Select Font number 2
A0	Move, with pen up, to...
  063F	...X=1599...
  083E	...Y=2110
A8	Print label...
  04	...centered horizontally above point 1599,2110...
    46 55 4E 43 54 49 4F 4E 00	"FUNCTION"
A0	Move, with pen up, to...
  0907	...X=2311...
  040B	...Y=1035 (start to draw pie, at rightmost edge)
A2	Draw, with pen down, to...
  0906	...X=2310...
  03F0	...Y=1008
A2	Draw, with pen down, to...
  0904	...X=2308...
  03D4	...Y=980
	...this goes on for many pages of DEBUG listing, but
	it all follows the above pattern. At the end...
60	End of Plot Data
1A	DOS EOF marker byte
 
The byte which follows the A8 "LABEL" marker indicates label
placement with respect to the most recently established pen
location, and orientation of the label on the page. The low
four bits, taken MOD 9, indicate placement:
	X0	Label centered vertically and horizontally
			over the point
	X1	Label centered vertically, extending to the
			right of the point
	X2	Label centered horizontally below the point
	X3	Label centered vertically, extending to the
			left of the point
	X4	Label centered horizontally above the point
	X5	Lower right corner of label on the point
	X6	Lower left corner of label on the point
	X7	Upper right corner of label on the point
	X8	Upper left corner of label on the point
The next higher two bits indicate rotation of the entire label,
including placement, with respect to the rest of the graph:
	0X	Normal placement (0-degree rotation)
	1X	Rotated 90 degrees CCW
	2X	Rotated 180 degrees (upside down)
	3X	Rotated 270 degrees CCW (90 degrees CW)
No "opcode" bytes other than those listed above have been found
in any of the .PIC files I have examined; others may be used,
however. This information should be enough to let you change
fonts or label placement in .PIC files, using DEBUG to do the
hex patching. My software publishing enterprise, "the software
factory", is developing a user-friendly editor to permit you
to edit .PIC files without the need for hex patches; it will
allow additional text to be added to a graph, or any wording
to be changed. Leave a message to 73105,1650 here or on SASIG,
or use EMail, if you're interested in getting an announcement
when it is ready.		Jim Kyle, 12 March 1984
*/

#include <stdlib.h>
#include <stdio.h>
#include "psionio.h"

int
main(int argc, char *argv[])
{
	FILE *fpin = stdin;
	int c, ok=1;

	/* Header */
	readword(fpin);
	readword(fpin);
	readword(fpin);
	readword(fpin);
	readword(fpin);
	readword(fpin);
	/* X,Y scaling factors */
	readword(fpin);
	readword(fpin);
	/* Start */
	if (readbyte(fpin) != 0x06) fprintf(stderr, "No start byte\n");
	/* Body */
	while ((c = fgetc(fpin)) != EOF && ok)
	{
		switch (c)
		{
			case 0x60: ok=0; break; /* End of plot data */
			case 0xA0: fprintf(stderr, "Moveto\n"); readword(fpin);readword(fpin); break;
			case 0xA2: fprintf(stderr, "Lineto\n"); readword(fpin);readword(fpin); break;
			case 0xA7: fprintf(stderr, "Font %d\n", readbyte(fpin)); break;
			case 0xA8: readbyte(fpin); fprintf(stderr, "Label %s\n", readcstr(fpin)); break;
			case 0xAC: fprintf(stderr, "Char size\n"); readword(fpin); readword(fpin); break;
			case 0xB0: case 0xB1: case 0xB2: case 0xB3: case 0xB4: case 0xB5: case 0xB6: case 0xB7: case 0xB8: case 0xB9: case 0xBA: case 0xBB: case 0xBC: case 0xBD: case 0xBE: case 0xBF: fprintf(stderr, "Colour %d\n", c&0xF); break;
			default: fprintf(stderr, "Unknown: %02x\n", c);
		}
	}

	return(0);
}

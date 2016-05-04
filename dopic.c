/*
 * DOPIC.C V1-003
 * Convert a Lotus 123 PIC file (from the Psion Spreadsheet) into a PostScript
 * print file. This appears to work for pie charts, line charts and bar charts.
 * 3D bar charts come out as 2D, a limitation noted in the 3a User Guide.
 *
 * Author: flo (Paul Williams)
 * Contact: P.Williams@rrds.co.uk (preferred), or flo@easynet.co.uk
 * Creation Date: 11-Jan-1996
 * Environment: Any, though some MS-DOS/Windoze filename-handling routines will
 *              be used if appropriate.
 *
 * Modifications:
 *
 * 1-003 13-Jan-1996 flo
 *       Only output move and font change commands at the last possible moment,
 *       so that multiple redundant position or font changes are not output. This
 *       happens when, for example, the Psion Spreadsheet outputs empty strings for
 *       titles that the user didn't specify. Added final 'stroke' if necessary.
 *
 * 1-002 12 Jan 1996 flo
 *       Properly interpret all text rotation and placement options. Turned %i into %d
 *       throughout as, rather bizarrely, The VAX C RTL on VAX/VMS doesn't know about %i.
 *       Made source common for MS-DOS and other (ie. VMS) system. Use fputs instead of fprintf
 *       for constant strings. Added the input filename to the output in the Title line.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Return a big-endian 16-bit word from the input file */
int getint(FILE* f)
{
   int c1, c2, cint;
   c1 = fgetc(f);
   c2 = fgetc(f);
   cint = 256 * c1 + c2;
   return cint;
}

void drawpic(FILE* picf, FILE* psf, char* fname)
{
   int idx, xco, yco, byt, xcs, ycs, eod, tsx, tsy;
   char str[256];
   int stroke_started = 0;
   int pending_moveto = 0;
   int pending_fontchange = 0;

   /* The first 16 bytes of the file are a constant header. The 17th byte */
   /* is $06, which I assume is the `start graph' command, by analogy with */
   /* the $60 `end graph' command. */
   for (idx = 0; idx < 17; ++idx)
      fgetc(picf);

   fputs("%!PS-Adobe-3.0\n", psf);
   fprintf(psf, "%%%%Title: Converted from %s\n", fname);
   fputs("%%Creator: flo/dopic\n", psf);
   /* fprintf(psf, "%%%%BoundingBox: 0 0 3200 2311\n"); */
   fputs("%%Pages: 1\n", psf);
   fputs("%%Orientation: Landscape\n", psf);
   fputs("%%DocumentRequiredResources: font Helvetica\n", psf);
   fputs("%%EndComments\n", psf);
   fputs("%%BeginProlog\n", psf);
   fputs("/bd{bind def}bind def/ld{load def}bd/m/moveto ld/l/lineto ld/rm/rmoveto ld\n", psf);
   fputs("/gs/gsave ld/gr/grestore ld/sw/stringwidth ld/A0S{dup sw pop -2 div -0.365 tsy\n", psf);
   fputs("mul rm show}bd/A1S{0 -0.365 tsy mul rm show}bd/A2S{dup sw pop -2 div -0.73 tsy\n", psf);
   fputs("mul rm show}bd/A3S{dup sw pop neg -0.365 tsy mul rm show}bd/A4S{dup sw pop -2\n", psf);
   fputs("div 0 rm show}bd/A5S{dup sw pop neg 0 rm show}bd/A6S/show ld/A7S{dup sw pop neg\n", psf);
   fputs("-0.73 tsy mul rm show}bd/A8S{0 -0.73 tsy mul rm show}bd/R0P{gs}bd/R1P{gs\n", psf);
   fputs("currentpoint translate 90 rotate 0 0 m}bd/R2P{gs currentpoint translate 180\n", psf);
   fputs("rotate 0 0 m}bd/R3P{gs currentpoint translate -90 rotate 0 0 m}bd\n", psf);
   fputs("%%EndProlog\n", psf);
   fputs("%%Page: 1 1\n", psf);
   fputs("595 0 translate 90 rotate 50 30 translate 0.231 dup scale\n", psf);

   eod = 0;
   while (!eod && ((byt = fgetc(picf)) != EOF))
   {
      if (0xb0 == (byt & 0xf0))
      {
         /* colour change - unsupported */
      }
      else if (0xa0 == byt)
      {
         /* move */
         xcs = getint(picf);
         ycs = getint(picf);
         if (stroke_started)
         {
            fputs("stroke\n", psf);
            stroke_started = 0;
         }
         pending_moveto = 1;
      }
      else if (0xa2 == byt)
      {
         /* draw */
         if (pending_moveto)
         {
            fprintf(psf, "%d %d m\n", xcs, ycs);
            pending_moveto = 0;
         }
         xco = getint(picf);
         yco = getint(picf);
         fprintf(psf, "%d %d l\n", xco, yco);
         stroke_started = 1;
      }
      else if ((0x30 == byt) || (0xd0 == byt))
      {
         /* polygon */
         idx = fgetc(picf);
         xcs = getint(picf);
         ycs = getint(picf);
         fprintf(psf, "%d %d m\n", xcs, ycs);
         while (idx--)
         {
            xco = getint(picf);
            yco = getint(picf);
            fprintf(psf, "%d %d l\n", xco, yco);
         }
         fputs("closepath stroke\n", psf);
      }
      else if (0xac == byt)
      {
         /* font size change */
         tsx = getint(picf);
         tsy = getint(picf);
         pending_fontchange = 1;
      }
      else if (0xa7 == byt)
         /* font change - unsupported */
         byt = fgetc(picf);
      else if (0xa8 == byt)
      {
         /* string alignment bit assignment: 00rr0aaa */
         int align = fgetc(picf); /* alignment */
         int rotation = (align >> 4) & 3;
         align &= 7;
         /* collect the null-terminated string */
         idx = 0;
         while (byt = fgetc(picf))
            if (idx < 254) /* make sure we have room for next character AND any required escaping backslash */
            {
               /* Have to escape certain characters in PostScript strings */
               if (('(' == byt) || (')' == byt) || ('\\' == byt))
                  str[idx++] = '\\';
               str[idx++] = byt;
            }
         str[idx] = '\0';
         if (idx > 0)
         {
            if (stroke_started)
            {
               fputs("stroke\n", psf);
               stroke_started = 0;
            }
            if (pending_fontchange)
            {
               fprintf(psf, "/tsx %d def/tsy %d def\n", tsx, tsy);
               fputs("/Helvetica findfont [tsx 0 0 tsy 0 0] makefont setfont\n", psf);
               pending_fontchange = 0;
            }
            if (pending_moveto)
            {
               fprintf(psf, "%d %d m\n", xcs, ycs);
               pending_moveto = 0;
            }
            fprintf(psf, "R%dP (%s) A%dS gr\n", rotation, str, align);
         }
      }
      else if (0x60 == (byt & 0xf0))
         eod = 1;
      else
      {
         fprintf(stderr, "unrecognised opcode: 0x%x at 0x%lx\n", byt, ftell(picf));
         eod = 1;
      }
   }

   if (stroke_started)
      fputs("stroke\n", psf);
   fputs("showpage\n", psf);
   fputs("%%Trailer\n", psf);
   fputs("%%EOF\n", psf);
}

int main(int argc, char *argv[])
{
   FILE* picf;
   FILE* psf;

#ifdef _MSDOS
   char path_buffer[_MAX_PATH];
   char drive[_MAX_DRIVE];
   char dir[_MAX_DIR];
   char fname[_MAX_FNAME];
   char ext[_MAX_EXT];

   switch (argc)
   {
      case 2:
         /* make output name from input name */
         _splitpath(argv[1], drive, dir, fname, ext);
         _makepath(path_buffer, "", "", fname, ".PS");
         break;

      case 3:
         /* user has specified output name as well */
         strcpy(path_buffer, argv[2]);
         break;

      default:
         fprintf(stderr, "Usage: DOPIC PICfilename [PSfilename]\n");
         return 0;
   }

   if (NULL == (psf = fopen(path_buffer, "w")))
      return 1;
      
#else
   if (argc != 3)
   {
      fprintf(stderr, "Usage: DOPIC PICfilename PSfilename\n");
      return 0;
   }

   if (NULL == (psf = fopen(argv[2], "w")))
      return 1;
#endif

   if (NULL == (picf = fopen(argv[1], "rb")))
      return 2;

   drawpic(picf, psf, argv[1]);
   fclose(picf);
   fclose(psf);

   return 0;
}

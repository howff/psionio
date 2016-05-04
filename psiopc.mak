PROJ = psiopc
PROJFILE = psiopc.mak
DEBUG = 0

PWBRMAKE  = pwbrmake
NMAKEBSC1  = set
NMAKEBSC2  = nmake
CC	= cl
CFLAGS_G  = /AL /W4 /G2 /DPC /BATCH
CFLAGS_D  = /Gi$(PROJ).mdt /Zi /Od
CFLAGS_R  = /Ot /Ol /Og /Oi /Ow /FPi87 /Gs
ASM  = ml
AFLAGS_G  = /W3 /WX
AFLAGS_D  = /Zi
AFLAGS_R  = /nologo
FOR  = fl
FORFLAGS_G	= /W2 /G2
FORFLAGS_R	= /Ot
FORFLAGS_D	= /Zi /Od
MAPFILE_D  = NUL
MAPFILE_R  = NUL
LFLAGS_G  =  /NOI /NOE /STACK:4096	/BATCH
LFLAGS_D  = /CO /FAR /PACKC
LFLAGS_R  = /EXE /FAR /PACKC
LINKER	= link
ILINK  = ilink
LRF  = echo > NUL
LLIBS_G  = dundee7.lib graphics.lib

OBJS  = AGN2TSV.obj CSV2TSV.obj DBF2TSV.obj PSIONIO.obj SPR2TSV.obj\
		TSV2CSV.obj TSV2DBF.obj TSV2SPR.obj WRD2HTML.obj
SBRS  = AGN2TSV.sbr CSV2TSV.sbr DBF2TSV.sbr PSIONIO.sbr SPR2TSV.sbr\
		TSV2CSV.sbr TSV2DBF.sbr TSV2SPR.sbr WRD2HTML.sbr

all: $(PROJ).exe

.SUFFIXES:
.SUFFIXES: .sbr .obj .c

AGN2TSV.obj : AGN2TSV.C PSIONIO.H AGN2TSV.H

AGN2TSV.sbr : AGN2TSV.C PSIONIO.H AGN2TSV.H

CSV2TSV.obj : CSV2TSV.C PSIONIO.H

CSV2TSV.sbr : CSV2TSV.C PSIONIO.H

DBF2TSV.obj : DBF2TSV.C PSIONIO.H

DBF2TSV.sbr : DBF2TSV.C PSIONIO.H

PSIONIO.obj : PSIONIO.C PSIONIO.H

PSIONIO.sbr : PSIONIO.C PSIONIO.H

SPR2TSV.obj : SPR2TSV.C PSIONIO.H

SPR2TSV.sbr : SPR2TSV.C PSIONIO.H

TSV2CSV.obj : TSV2CSV.C PSIONIO.H

TSV2CSV.sbr : TSV2CSV.C PSIONIO.H

TSV2DBF.obj : TSV2DBF.C PSIONIO.H

TSV2DBF.sbr : TSV2DBF.C PSIONIO.H

TSV2SPR.obj : TSV2SPR.C PSIONIO.H

TSV2SPR.sbr : TSV2SPR.C PSIONIO.H

WRD2HTML.obj : WRD2HTML.C PSIONIO.H

WRD2HTML.sbr : WRD2HTML.C PSIONIO.H


$(PROJ).bsc : $(SBRS)
		$(PWBRMAKE) @<<
$(BRFLAGS) $(SBRS)
<<

$(PROJ).exe : $(OBJS)
!IF $(DEBUG)
		$(LRF) @<<$(PROJ).lrf
$(RT_OBJS: = +^
) $(OBJS: = +^
)
$@
$(MAPFILE_D)
$(LLIBS_G: = +^
) +
$(LLIBS_D: = +^
) +
$(LIBS: = +^
)
$(DEF_FILE) $(LFLAGS_G) $(LFLAGS_D);
<<
!ELSE
		$(LRF) @<<$(PROJ).lrf
$(RT_OBJS: = +^
) $(OBJS: = +^
)
$@
$(MAPFILE_R)
$(LLIBS_G: = +^
) +
$(LLIBS_R: = +^
) +
$(LIBS: = +^
)
$(DEF_FILE) $(LFLAGS_G) $(LFLAGS_R);
<<
!ENDIF
		$(LINKER) @$(PROJ).lrf


.c.sbr :
!IF $(DEBUG)
		$(CC) /Zs $(CFLAGS_G) $(CFLAGS_D) /FR$@ $<
!ELSE
		$(CC) /Zs $(CFLAGS_G) $(CFLAGS_R) /FR$@ $<
!ENDIF

.c.obj :
!IF $(DEBUG)
		$(CC) /c $(CFLAGS_G) $(CFLAGS_D) /Fo$@ $<
!ELSE
		$(CC) /c $(CFLAGS_G) $(CFLAGS_R) /Fo$@ $<
!ENDIF


run: $(PROJ).exe
		$(PROJ).exe $(RUNFLAGS)

debug: $(PROJ).exe
		CV $(CVFLAGS) $(PROJ).exe $(RUNFLAGS)

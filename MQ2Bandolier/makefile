!include "../global.mak"

ALL : "$(OUTDIR)\MQ2Bandolier.dll"

CLEAN :
	-@erase "$(INTDIR)\MQ2Bandolier.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\MQ2Bandolier.dll"
	-@erase "$(OUTDIR)\MQ2Bandolier.exp"
	-@erase "$(OUTDIR)\MQ2Bandolier.lib"
	-@erase "$(OUTDIR)\MQ2Bandolier.pdb"


LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib $(DETLIB) ..\Release\MQ2Main.lib /nologo /dll /incremental:no /pdb:"$(OUTDIR)\MQ2Bandolier.pdb" /debug /machine:I386 /out:"$(OUTDIR)\MQ2Bandolier.dll" /implib:"$(OUTDIR)\MQ2Bandolier.lib" /OPT:NOICF /OPT:NOREF 
LINK32_OBJS= \
	"$(INTDIR)\MQ2Bandolier.obj" \
	"$(OUTDIR)\MQ2Main.lib"

"$(OUTDIR)\MQ2Bandolier.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) $(LINK32_FLAGS) $(LINK32_OBJS)


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("MQ2Bandolier.dep")
!INCLUDE "MQ2Bandolier.dep"
!ELSE 
!MESSAGE Warning: cannot find "MQ2Bandolier.dep"
!ENDIF 
!ENDIF 


SOURCE=.\MQ2Bandolier.cpp

"$(INTDIR)\MQ2Bandolier.obj" : $(SOURCE) "$(INTDIR)"


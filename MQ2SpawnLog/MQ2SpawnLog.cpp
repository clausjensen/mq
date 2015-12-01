// This plugin is intended for VIP members of MacroQuest2.com only
// and may not be redistributed without permission of the author

#include "../MQ2Plugin.h"
#include <vector>

const char*  MODULE_NAME    = "MQ2SpawnLog";
const double MODULE_VERSION = 1.0;
PreSetup(MODULE_NAME);

const unsigned char ARG_SPAWN   = 1;
const unsigned char ARG_DESPAWN = 2;
const unsigned char ARG_MINLVL  = 3;
const unsigned char ARG_MAXLVL  = 4;
const unsigned char ARG_COLOR   = 5;
const unsigned char ARG_STOGGLE = 6;
const unsigned char ARG_DTOGGLE = 7;
const unsigned char ARG_EXCLUDE = 8;

time_t tSeconds;
__time64_t tCurrentTime;
bool bZoning = true;
bool bLoaded = false;
char szCharName[MAX_STRING] = {0};
vector<string> SpawnFilters;

// added logging features
char szDirPath[MAX_STRING]  = {0};
char szLogFile[MAX_STRING]  = {0};
char szLogPath[MAX_STRING]  = {0};

bool bLogActive             = false;
bool bLogReady              = false;
bool bLogCmdUsed            = false;

FILE* fOurLog;
time_t rawtime;
struct tm* timeinfo;

enum
{
    LOG_FAIL    = 0,
    LOG_SUCCESS = 1,
};

int StartLog()
{
    fOurLog = fopen(szLogPath, "a+");
    if (fOurLog == NULL)
    {
        WriteChatf("\ay%s\aw:: Unable to open log file ( \ag%s \ax)", MODULE_NAME, szLogPath);
        bLogActive = bLogReady = false;
        return LOG_FAIL;
    }
    else
    {
		WriteChatf("\ay%s\aw:: Logging spawns ( \ag%s \ax)", MODULE_NAME, szLogPath);
        fclose(fOurLog);
        bLogReady = true;
    }
    return LOG_SUCCESS;
}

int EndLog()
{
    fOurLog = fopen(szLogPath, "a+");
    if (fOurLog != NULL)
    {
        bLogActive = bLogReady = false;
        return LOG_SUCCESS;
    }
    return LOG_FAIL;
}

void WriteToLog(char* szWriteLine)
{
    fOurLog = fopen(szLogPath, "a+");
    if (fOurLog == NULL)
    {
        WriteChatf("\ay%s\aw:: Unable to open log file. Check your file path and permissions. ( \ag%s \ax)", MODULE_NAME, szLogPath);
        bLogActive = bLogReady = false;
        WriteChatf("\ay%s\aw:: Logging \arDISABLED\aw.", MODULE_NAME);
    }
    else
    {
        char szTemp[MAX_STRING] = {0};
        time(&rawtime);
        timeinfo = localtime(&rawtime);
        strftime(szTemp, MAX_STRING, "[%Y-%m-%d %H:%M:%S] ", timeinfo);
        strcat(szTemp, szWriteLine);
        strcat(szTemp, "\n");
        fprintf(fOurLog, "%s", szTemp);
        fclose(fOurLog);
    }
}

// end logging features

// watch type class: new instance created for each type
class CWatchType
{
public:
    CWatchType()
    {
        MinLVL = 1;
        MaxLVL = MAX_NPC_LEVEL;
        Spawn = Despawn = false;
        Color = 0xFFDEAD;
    }

    bool Spawn;
    bool Despawn;
    int MinLVL;
    int MaxLVL;
    unsigned long Color;
};
CWatchType* SpawnFormat = NULL;

// our configuration class: one instance
class CSpawnCFG
{
public:
    CSpawnCFG()
    {
        Delay = 10;
        ON         = Timestamp = true;
        PC.Spawn   = NPC.Spawn = true;
        AutoSave   = Location  = false;
        SaveByChar = Logging   = false;
    }

    CWatchType PC;
    CWatchType NPC;
    CWatchType UNKNOWN;

    CWatchType ALL;

    bool ON;
    bool AutoSave;
    bool Location;
    bool SaveByChar;
    bool SpawnID;
    bool Timestamp;
    bool Logging;
    unsigned long Delay;
};
CSpawnCFG CFG;

// UI window
class CSpawnWnd;
CSpawnWnd* OurWnd = NULL;
class CSpawnWnd : public CCustomWnd
{
public:
    CStmlWnd *StmlOut;
    CXWnd *OutWnd;
    struct _CSIDLWND *OutStruct;

    CSpawnWnd(CXStr *Template):CCustomWnd(Template)
    {
        SetWndNotification(CSpawnWnd);
        StmlOut = (CStmlWnd *)GetChildItem("CWChatOutput");
        OutWnd = (CXWnd*)StmlOut;
        OutWnd->Clickable = 1;
        OutStruct = (_CSIDLWND*)GetChildItem("CWChatOutput");
        OutStruct->Clickable = 0;
        BitOff(WindowStyle, CWS_CLOSE);
        CloseOnESC = 0;
        *(DWORD*)&(((PCHAR)StmlOut)[EQ_CHAT_HISTORY_OFFSET]) = 400;
    }

    int WndNotification(CXWnd *pWnd, unsigned int Message, void *data)
    {
        if (pWnd == NULL && Message == XWM_CLOSE)
        {
            dShow = 1;
            return 0;
        }
        return CSidlScreenWnd::WndNotification(pWnd,Message,data);
    };

    unsigned long FontSize;
};

void SaveOurWnd()
{
    PCSIDLWND UseWnd = (PCSIDLWND)OurWnd;
    if (!UseWnd || !OurWnd) return;

    char szTemp[20]               = {0};

    WritePrivateProfileString(CFG.SaveByChar ? szCharName : "Window", "ChatTop",      itoa(UseWnd->Location.top,    szTemp, 10), INIFileName);
    WritePrivateProfileString(CFG.SaveByChar ? szCharName : "Window", "ChatBottom",   itoa(UseWnd->Location.bottom, szTemp, 10), INIFileName);
    WritePrivateProfileString(CFG.SaveByChar ? szCharName : "Window", "ChatLeft",     itoa(UseWnd->Location.left,   szTemp, 10), INIFileName);
    WritePrivateProfileString(CFG.SaveByChar ? szCharName : "Window", "ChatRight",    itoa(UseWnd->Location.right,  szTemp, 10), INIFileName);
    WritePrivateProfileString(CFG.SaveByChar ? szCharName : "Window", "Fades",        itoa(UseWnd->Fades,           szTemp, 10), INIFileName);
    WritePrivateProfileString(CFG.SaveByChar ? szCharName : "Window", "Alpha",        itoa(UseWnd->Alpha,           szTemp, 10), INIFileName);
    WritePrivateProfileString(CFG.SaveByChar ? szCharName : "Window", "FadeToAlpha",  itoa(UseWnd->FadeToAlpha,     szTemp, 10), INIFileName);
    WritePrivateProfileString(CFG.SaveByChar ? szCharName : "Window", "Duration",     itoa(UseWnd->FadeDuration,    szTemp, 10), INIFileName);
    WritePrivateProfileString(CFG.SaveByChar ? szCharName : "Window", "Locked",       itoa(UseWnd->Locked,          szTemp, 10), INIFileName);
    WritePrivateProfileString(CFG.SaveByChar ? szCharName : "Window", "Delay",        itoa(UseWnd->TimeMouseOver,   szTemp, 10), INIFileName);
    WritePrivateProfileString(CFG.SaveByChar ? szCharName : "Window", "BGType",       itoa(UseWnd->BGType,          szTemp, 10), INIFileName);
    WritePrivateProfileString(CFG.SaveByChar ? szCharName : "Window", "BGTint.red",   itoa(UseWnd->BGColor.R,       szTemp, 10), INIFileName);
    WritePrivateProfileString(CFG.SaveByChar ? szCharName : "Window", "BGTint.green", itoa(UseWnd->BGColor.G,       szTemp, 10), INIFileName);
    WritePrivateProfileString(CFG.SaveByChar ? szCharName : "Window", "BGTint.blue",  itoa(UseWnd->BGColor.B,       szTemp, 10), INIFileName);
    WritePrivateProfileString(CFG.SaveByChar ? szCharName : "Window", "FontSize",     itoa(OurWnd->FontSize,   szTemp, 10), INIFileName);

    GetCXStr(UseWnd->WindowText, szTemp);
    WritePrivateProfileString(CFG.SaveByChar ? szCharName : "Window", "WindowTitle", szTemp, INIFileName);
}

void CreateOurWnd()
{
    if (OurWnd == NULL)
    {
        class CXStr ChatWnd("ChatWindow");
        OurWnd = new CSpawnWnd(&ChatWnd);

        OurWnd->Location.top    = GetPrivateProfileInt(CFG.SaveByChar ? szCharName : "Window", "ChatTop",      10,   INIFileName);
        OurWnd->Location.bottom = GetPrivateProfileInt(CFG.SaveByChar ? szCharName : "Window", "ChatBottom",   210,  INIFileName);
        OurWnd->Location.left   = GetPrivateProfileInt(CFG.SaveByChar ? szCharName : "Window", "ChatLeft",     10,   INIFileName);
        OurWnd->Location.right  = GetPrivateProfileInt(CFG.SaveByChar ? szCharName : "Window", "ChatRight",    410,  INIFileName);
        OurWnd->Fades           = GetPrivateProfileInt(CFG.SaveByChar ? szCharName : "Window", "Fades",        0,    INIFileName);
        OurWnd->Alpha           = GetPrivateProfileInt(CFG.SaveByChar ? szCharName : "Window", "Alpha",        255,  INIFileName);
        OurWnd->FadeToAlpha     = GetPrivateProfileInt(CFG.SaveByChar ? szCharName : "Window", "FadeToAlpha",  255,  INIFileName);
        OurWnd->FadeDuration    = GetPrivateProfileInt(CFG.SaveByChar ? szCharName : "Window", "Duration",     500,  INIFileName);
        OurWnd->Locked          = GetPrivateProfileInt(CFG.SaveByChar ? szCharName : "Window", "Locked",       0,    INIFileName);
        OurWnd->TimeMouseOver   = GetPrivateProfileInt(CFG.SaveByChar ? szCharName : "Window", "Delay",        2000, INIFileName);
        OurWnd->BGType          = GetPrivateProfileInt(CFG.SaveByChar ? szCharName : "Window", "BGType",       1,    INIFileName);
        OurWnd->BGColor.R       = GetPrivateProfileInt(CFG.SaveByChar ? szCharName : "Window", "BGTint.red",   255,  INIFileName);
        OurWnd->BGColor.G       = GetPrivateProfileInt(CFG.SaveByChar ? szCharName : "Window", "BGTint.green", 255,  INIFileName);
        OurWnd->BGColor.B       = GetPrivateProfileInt(CFG.SaveByChar ? szCharName : "Window", "BGTint.blue",  255,  INIFileName);

        char szWindowText[MAX_STRING] = {0};
        GetPrivateProfileString(CFG.SaveByChar ? szCharName : "Window", "WindowTitle", "Spawns", szWindowText, MAX_STRING, INIFileName);
        SetCXStr(&OurWnd->WindowText, szWindowText);
        ((CXWnd*)OurWnd)->Show(1, 1);
        BitOff(OurWnd->OutStruct->WindowStyle, CWS_CLOSE);
    }
}

void KillOurWnd(bool bSave)
{
    if (OurWnd)
    {
        if (bSave) SaveOurWnd();
        delete OurWnd;
        OurWnd = NULL;
    }
}

char* itohex(int iNum, char* szTemp)
{
    sprintf(szTemp, "%06X", iNum);
    return szTemp;
}

void HandleConfig(bool bSave)
{
    if (bSave)
    {
        char szSave[MAX_STRING] = {0};
        WritePrivateProfileString("Settings",  "AutoSave",   CFG.AutoSave             ? "on" : "off",    INIFileName);
        WritePrivateProfileString("Settings",  "Delay",      itoa((int)CFG.Delay,           szSave, 10), INIFileName);
        WritePrivateProfileString("Settings",  "Enabled",    CFG.ON                   ? "on" : "off",    INIFileName);
        WritePrivateProfileString("Settings",  "SaveByChar", CFG.SaveByChar           ? "on" : "off",    INIFileName);
        WritePrivateProfileString("Settings",  "ShowLoc",    CFG.Location             ? "on" : "off",    INIFileName);
        WritePrivateProfileString("Settings",  "SpawnID",    CFG.SpawnID              ? "on" : "off",    INIFileName);
        WritePrivateProfileString("Settings",  "Timestamp",  CFG.Timestamp            ? "on" : "off",    INIFileName);

        // logging uses savebychar at dewey's request
        WritePrivateProfileString(CFG.SaveByChar ? szCharName : "Settings",  "Logging",    CFG.Logging ? "on" : "off",    INIFileName);
        WritePrivateProfileString(CFG.SaveByChar ? szCharName : "Settings",  "LogPath",    szDirPath,                     INIFileName);
        // end of logging

        WritePrivateProfileString("ALL",          "Spawn",   CFG.ALL.Spawn            ? "on" : "off",    INIFileName);
        WritePrivateProfileString("ALL",          "Despawn", CFG.ALL.Despawn          ? "on" : "off",    INIFileName);
        WritePrivateProfileString("ALL",          "MinLVL",  itoa(CFG.ALL.MinLVL,           szSave, 10), INIFileName);
        WritePrivateProfileString("ALL",          "MaxLVL",  itoa(CFG.ALL.MaxLVL,           szSave, 10), INIFileName);
        WritePrivateProfileString("ALL",          "Color",   itohex(CFG.ALL.Color,          szSave),     INIFileName);
        WritePrivateProfileString("PC",           "Spawn",   CFG.PC.Spawn             ? "on" : "off",    INIFileName);
        WritePrivateProfileString("PC",           "Despawn", CFG.PC.Despawn           ? "on" : "off",    INIFileName);
        WritePrivateProfileString("PC",           "MinLVL",  itoa(CFG.PC.MinLVL,            szSave, 10), INIFileName);
        WritePrivateProfileString("PC",           "MaxLVL",  itoa(CFG.PC.MaxLVL,            szSave, 10), INIFileName);
        WritePrivateProfileString("PC",           "Color",   itohex(CFG.PC.Color,           szSave),     INIFileName);
        WritePrivateProfileString("NPC",          "Spawn",   CFG.NPC.Spawn            ? "on" : "off",    INIFileName);
        WritePrivateProfileString("NPC",          "Despawn", CFG.NPC.Despawn          ? "on" : "off",    INIFileName);
        WritePrivateProfileString("NPC",          "MinLVL",  itoa(CFG.NPC.MinLVL,          szSave, 10),  INIFileName);
        WritePrivateProfileString("NPC",          "MaxLVL",  itoa(CFG.NPC.MaxLVL,          szSave, 10),  INIFileName);
        WritePrivateProfileString("NPC",          "Color",   itohex(CFG.NPC.Color,         szSave),      INIFileName);
        WritePrivateProfileString("UNKNOWN",      "Spawn",   CFG.UNKNOWN.Spawn        ? "on" : "off",    INIFileName);
        WritePrivateProfileString("UNKNOWN",      "Despawn", CFG.UNKNOWN.Despawn      ? "on" : "off",    INIFileName);
        WritePrivateProfileString("UNKNOWN",      "MinLVL",  itoa(CFG.UNKNOWN.MinLVL,       szSave, 10), INIFileName);
        WritePrivateProfileString("UNKNOWN",      "MaxLVL",  itoa(CFG.UNKNOWN.MaxLVL,       szSave, 10), INIFileName);
        WritePrivateProfileString("UNKNOWN",      "Color",   itohex(CFG.UNKNOWN.Color,      szSave),     INIFileName);

        SaveOurWnd();
    }
    else
    {
        char szLoad[MAX_STRING] = {0};
        int iValid = GetPrivateProfileInt("Settings", "Delay", (int)CFG.Delay, INIFileName);
        if (iValid > 0) CFG.Delay = iValid;

        GetPrivateProfileString("Settings", "AutoSave",    CFG.AutoSave   ? "on" : "off", szLoad, MAX_STRING, INIFileName);
        CFG.AutoSave   = (!strnicmp(szLoad, "on", 3));
        GetPrivateProfileString("Settings", "Enabled",     CFG.ON         ? "on" : "off", szLoad, MAX_STRING, INIFileName);
        CFG.ON         = (!strnicmp(szLoad, "on", 3));
        GetPrivateProfileString("Settings", "SaveByChar",  CFG.SaveByChar ? "on" : "off", szLoad, MAX_STRING, INIFileName);
        CFG.SaveByChar = (!strnicmp(szLoad, "on", 3));
        GetPrivateProfileString("Settings", "ShowLoc",     CFG.Location   ? "on" : "off", szLoad, MAX_STRING, INIFileName);
        CFG.Location   = (!strnicmp(szLoad, "on", 3));
        GetPrivateProfileString("Settings", "SpawnID",     CFG.SpawnID    ? "on" : "off", szLoad, MAX_STRING, INIFileName);
        CFG.SpawnID    = (!strnicmp(szLoad, "on", 3));
        GetPrivateProfileString("Settings", "Timestamp",   CFG.Timestamp  ? "on" : "off", szLoad, MAX_STRING, INIFileName);
        CFG.Timestamp  = (!strnicmp(szLoad, "on", 3));

        //logging addition
        GetPrivateProfileString(CFG.SaveByChar ? szCharName : "Settings", "Logging", CFG.Logging ? "on" : "off", szLoad, MAX_STRING, INIFileName);
        CFG.Logging    = (!strnicmp(szLoad, "on", 3));
        GetPrivateProfileString(CFG.SaveByChar ? szCharName : "Settings", "LogPath", gszINIPath,              szDirPath, MAX_STRING, INIFileName);
        sprintf(szLogPath, "%s\\%s", szDirPath, szLogFile);
        if (CFG.Logging) bLogActive = true;
        // end of logging


        GetPrivateProfileString("ALL", "Spawn", CFG.ALL.Spawn ? "on" : "off", szLoad, MAX_STRING, INIFileName);
        CFG.ALL.Spawn = (!strnicmp(szLoad, "on", 3));
        GetPrivateProfileString("ALL", "Despawn", CFG.ALL.Despawn ? "on" : "off", szLoad, MAX_STRING, INIFileName);
        CFG.ALL.Despawn = (!strnicmp(szLoad, "on", 3));
        CFG.ALL.MinLVL = GetPrivateProfileInt("ALL", "MinLVL", CFG.ALL.MinLVL, INIFileName);
        CFG.ALL.MaxLVL = GetPrivateProfileInt("ALL", "MaxLVL", CFG.ALL.MaxLVL, INIFileName);
        GetPrivateProfileString("ALL", "Color", "FFFF00", szLoad, MAX_STRING, INIFileName);
        sscanf(szLoad, "%06X", &CFG.ALL.Color);

        GetPrivateProfileString("PC", "Spawn", CFG.PC.Spawn ? "on" : "off", szLoad, MAX_STRING, INIFileName);
        CFG.PC.Spawn = (!strnicmp(szLoad, "on", 3));
        GetPrivateProfileString("PC", "Despawn", CFG.PC.Despawn ? "on" : "off", szLoad, MAX_STRING, INIFileName);
        CFG.PC.Despawn = (!strnicmp(szLoad, "on", 3));
        CFG.PC.MinLVL = GetPrivateProfileInt("PC", "MinLVL", CFG.PC.MinLVL, INIFileName);
        CFG.PC.MaxLVL = GetPrivateProfileInt("PC", "MaxLVL", CFG.PC.MaxLVL, INIFileName);
        GetPrivateProfileString("PC", "Color", "00FF00", szLoad, MAX_STRING, INIFileName);
        sscanf(szLoad, "%06X", &CFG.PC.Color);

        GetPrivateProfileString("NPC", "Spawn", CFG.NPC.Spawn ? "on" : "off", szLoad, MAX_STRING, INIFileName);
        CFG.NPC.Spawn = (!strnicmp(szLoad, "on", 3));
        GetPrivateProfileString("NPC", "Despawn", CFG.NPC.Despawn ? "on" : "off", szLoad, MAX_STRING, INIFileName);
        CFG.NPC.Despawn = (!strnicmp(szLoad, "on", 3));
        CFG.NPC.MinLVL = GetPrivateProfileInt("NPC", "MinLVL", CFG.NPC.MinLVL, INIFileName);
        CFG.NPC.MaxLVL = GetPrivateProfileInt("NPC", "MaxLVL", CFG.NPC.MaxLVL, INIFileName);
        GetPrivateProfileString("NPC", "Color", "FF0000", szLoad, MAX_STRING, INIFileName);
        sscanf(szLoad, "%06X", &CFG.NPC.Color);

        GetPrivateProfileString("UNKNOWN", "Spawn", CFG.UNKNOWN.Spawn ? "on" : "off", szLoad, MAX_STRING, INIFileName);
        CFG.UNKNOWN.Spawn = (!strnicmp(szLoad, "on", 3));
        GetPrivateProfileString("UNKNOWN", "Despawn", CFG.UNKNOWN.Despawn ? "on" : "off", szLoad, MAX_STRING, INIFileName);
        CFG.UNKNOWN.Despawn = (!strnicmp(szLoad, "on", 3));
        CFG.UNKNOWN.MinLVL = GetPrivateProfileInt("UNKNOWN", "MinLVL", CFG.UNKNOWN.MinLVL, INIFileName);
        CFG.UNKNOWN.MaxLVL = GetPrivateProfileInt("UNKNOWN", "MaxLVL", CFG.UNKNOWN.MaxLVL, INIFileName);
        GetPrivateProfileString("UNKNOWN", "Color", "FF69B4", szLoad, MAX_STRING, INIFileName);
        sscanf(szLoad, "%06X", &CFG.UNKNOWN.Color);

        // add custom exclude list
        char szOurKey[MAX_STRING]    = {0};
        char szOurFilter[MAX_STRING] = {0};
        int  iFilt                   = 1;

        SpawnFilters.clear();
        GetPrivateProfileString("Exclude", itoa(iFilt, szOurKey, 10), NULL, szOurFilter, MAX_STRING, INIFileName);
        for (iFilt = 1; *szOurFilter; iFilt++)
        {
            if (*szOurFilter)
            {
                SpawnFilters.push_back(szOurFilter);
            }
            GetPrivateProfileString("Exclude", itoa(iFilt+1, szOurKey, 10), NULL, szOurFilter, MAX_STRING, INIFileName);
        }
        // end custom exclude list addition

        KillOurWnd(false);
    }
    if (bLoaded) WriteChatf("\ay%s\aw:: Configuration file %s.", MODULE_NAME, bSave ? "saved" : "loaded");
}

void WatchState(bool bSpawns)
{
    char szOn[10] = "\agON\ax";
    char szOff[10] = "\arOFF\ax";
    WriteChatf("\ay%s\aw:: %s - AutoSave(%s) Loc(%s) SpawnID(%s) Timestamp(%s) ZoneDelay(\ag%u\ax)", MODULE_NAME, CFG.ON ? szOn : szOff, CFG.AutoSave ? szOn : szOff, CFG.Location ? szOn : szOff, CFG.SpawnID ? szOn : szOff, CFG.Timestamp ? szOn : szOff, CFG.Delay);
    if (bSpawns)
    {
        WriteChatf("\aw - \ayPC\ax: %s ( \ay%d\ax to \ay%d\ax ) - %06X", (CFG.PC.Despawn ? (CFG.PC.Spawn ? "Spawn & Despawn" : "Despawn Only") : (CFG.PC.Spawn ? "Spawn Only" : "OFF")), CFG.PC.MinLVL, CFG.PC.MaxLVL, CFG.PC.Color);
        WriteChatf("\aw - \ayNPC\ax: %s ( \ay%d\ax to \ay%d\ax ) - %06X", (CFG.NPC.Despawn ? (CFG.NPC.Spawn ? "Spawn & Despawn" : "Despawn Only") : (CFG.NPC.Spawn ? "Spawn Only" : "OFF")), CFG.NPC.MinLVL, CFG.NPC.MaxLVL, CFG.NPC.Color);
        WriteChatf("\aw - \ayUNKNOWN\ax: %s ( \ay%d\ax to \ay%d\ax ) - %06X", (CFG.UNKNOWN.Despawn ? (CFG.UNKNOWN.Spawn ? "Spawn & Despawn" : "Despawn Only") : (CFG.UNKNOWN.Spawn ? "Spawn Only" : "OFF")), CFG.UNKNOWN.MinLVL, CFG.UNKNOWN.MaxLVL, CFG.UNKNOWN.Color);
        WriteChatf("\aw - \ayALL\ax: %s ( \ay%d\ax to \ay%d\ax ) - %06X", (CFG.ALL.Despawn ? (CFG.ALL.Spawn ? "Spawn & Despawn" : "Despawn Only") : (CFG.ALL.Spawn ? "Spawn Only" : "OFF")), CFG.ALL.MinLVL, CFG.ALL.MaxLVL, CFG.ALL.Color);
    }
    if (CFG.ALL.Spawn || CFG.ALL.Despawn) WriteChatf("\ay%s\aw:: \ayOVERRIDE\ax - Currently announcing \agALL\ax types: %s", MODULE_NAME, (CFG.ALL.Despawn ? (CFG.ALL.Spawn ? "Spawn & Despawn" : "Despawn Only") : "Spawn Only"));
}

void ToggleSetting(const char* pszToggleOutput, bool* pbSpawnTypeSet)
{
    char szTheMsg[MAX_STRING] = {0};
    *pbSpawnTypeSet = !*pbSpawnTypeSet;
    sprintf(szTheMsg, "\ay%s\aw:: %s %s", MODULE_NAME, *pbSpawnTypeSet ? "\agNow announcing\ax" : "\arNo longer announcing\ax", pszToggleOutput);
    WriteChatf(szTheMsg);
}

void SetSpawnColor(const char* pszSetOutput, unsigned long* pulSpawnTypeSet, unsigned long ulNewColor)
{
    char szTheMsg[MAX_STRING] = {0};
    unsigned long ulOldColor = *pulSpawnTypeSet;
    *pulSpawnTypeSet = ulNewColor;
    sprintf(szTheMsg, "\ay%s\aw:: %s color changed from \ay%06X\ax to \ag%06X\ax", MODULE_NAME, pszSetOutput, ulOldColor, ulNewColor);
    WriteChatf(szTheMsg);
}

void SetSpawnLevel(const char* pszSetOutput, int* piSpawnTypeSet, int iNewLevel)
{
    char szTheMsg[MAX_STRING] = {0};
    int iOldLevel = *piSpawnTypeSet;
    *piSpawnTypeSet = iNewLevel;
    sprintf(szTheMsg, "\ay%s\aw:: %s level changed from \ay%d\ax to \ag%d\ax", MODULE_NAME, pszSetOutput, iOldLevel, iNewLevel);
    WriteChatf(szTheMsg);
}

void WatchSpawns(PSPAWNINFO pLPlayer, char* szLine)
{
    int iNewLevel               = 0;
    unsigned long ulNewColor    = 0;
    unsigned char ucArgType     = 0;
    char szArg1[MAX_STRING]     = {0};
    char szArg2[MAX_STRING]     = {0};
    char szArg3[MAX_STRING]     = {0};
    GetArg(szArg1, szLine, 1);
    GetArg(szArg2, szLine, 2);
    GetArg(szArg3, szLine, 3);

    if (!*szArg1)
    {
        CFG.ON = !CFG.ON;
        WatchState(false);
        return;
    }

    if (*szArg2)
    {
        if (!strnicmp(szArg2,      "spawn",    6))  ucArgType = ARG_SPAWN;
        else if (!strnicmp(szArg2, "despawn",  8))  ucArgType = ARG_DESPAWN;
        else if (!strnicmp(szArg2, "minlevel", 9))  ucArgType = ARG_MINLVL;
        else if (!strnicmp(szArg2, "maxlevel", 9))  ucArgType = ARG_MAXLVL;
        else if (!strnicmp(szArg2, "color",    6))  ucArgType = ARG_COLOR;
        if (strnicmp(szArg1, "log", 3)) // weird log exclude, too lazy to clean all this shit up
        {
            if (*szArg3)
            {
                if (ucArgType == ARG_COLOR)
                {
                    sscanf(szArg3, "%06X", &ulNewColor);
                }
                else if (ucArgType == ARG_MINLVL || ucArgType == ARG_MAXLVL)
                {
                    char* pNotNum = NULL;
                    iNewLevel = strtoul(szArg3, &pNotNum, 10);
                    if (iNewLevel < 1 || iNewLevel > MAX_NPC_LEVEL || *pNotNum)
                    {
                        WriteChatf("\ay%s\aw:: \arLevel range must be between 1 and %u.", MODULE_NAME, MAX_NPC_LEVEL);
                        return;
                    }
                }
            }
            else
            {
                if (ucArgType == ARG_COLOR)
                {
                    WriteChatf("\ay%s\aw:: \arYou must provide a hexadecimal color code.", MODULE_NAME);
                    return;
                }
                else if (ucArgType == ARG_MINLVL || ucArgType == ARG_MAXLVL)
                {
                    WriteChatf("\ay%s\aw:: \arLevel range must be between 1 and %u.", MODULE_NAME, MAX_NPC_LEVEL);
                    return;
                }
            }
        }
    }
}

void WriteSpawn(PSPAWNINFO pFormatSpawn, char* szTypeString, char* szLocString, bool bSpawn)
{
    char szFinalOutput[MAX_STRING] = {0};
    char szColoredSpawn[MAX_STRING] = {0};
    char szStringStart[MAX_STRING] = {0};
    char szSpawnID[MAX_STRING] = {0};
    char szTime[30] = {0};
    char szProcessTemp[MAX_STRING] = {0};
    char szProcessTemp2[MAX_STRING] = {0};

    // added logging function 2010-09-04
    if (bLogReady)
    {
        char szLogOut[MAX_STRING] = {0};
		sprintf(szLogOut, "%s,[%s,%d,%d,%s,%s],[%s]", pFormatSpawn->DisplayedName, szTypeString, pFormatSpawn->SpawnID, pFormatSpawn->Level, pEverQuest->GetRaceDesc(pFormatSpawn->Race), GetClassDesc(pFormatSpawn->Class), szLocString);
        WriteToLog(szLogOut);
    }
    // end of logging function
}

CWatchType* IsTypeOn(CWatchType* pCheck, bool bSpawn)
{
    bool bAllCheck = (bSpawn ? CFG.ALL.Spawn : CFG.ALL.Despawn);
    if ((bSpawn && !pCheck->Spawn) || (!bSpawn && !pCheck->Despawn))
    {
        if (bAllCheck)
        {
            return &CFG.ALL;
        }
        return NULL;
    }
    return pCheck;
}

void CheckOurType(PSPAWNINFO pNewSpawn, bool bSpawn)
{
    char szType[MAX_STRING]     = {0};
    char szSpawnLoc[MAX_STRING] = {0};

    // add exclude list
    if (pNewSpawn->DisplayedName[0])
    {
        for (vector<string>::iterator it = SpawnFilters.begin() ; it != SpawnFilters.end(); it++)
        {
            string sFilter = *it;
            char szTempName[MAX_STRING] = {0};
            strcpy(szTempName, pNewSpawn->DisplayedName);
            strlwr(szTempName);
            if (strstr(szTempName, sFilter.c_str()))
            {
                return;
            }
        }
    }
    // end of exclude list addition

    switch(GetSpawnType(pNewSpawn))
    {
    case PC:
        SpawnFormat = IsTypeOn(&CFG.PC, bSpawn);
        if (!SpawnFormat) return;
        sprintf(szType, "PC");
        break;
    case NPC:
        SpawnFormat = IsTypeOn(&CFG.NPC, bSpawn);
        if (!SpawnFormat) return;
        sprintf(szType, "NPC");
        break;
    default:
        SpawnFormat = IsTypeOn(&CFG.UNKNOWN, bSpawn);
        if (!SpawnFormat) return;
        sprintf(szSpawnLoc, "UNKNOWN");
        sprintf(szType, "UNKNOWN");
        break;
    }

    // level-based filtering done here
    if (pNewSpawn->Level < SpawnFormat->MinLVL || pNewSpawn->Level > SpawnFormat->MaxLVL)
    {
        //DebugSpew("MQ2Spawns:: Filtered %s, %d", pNewSpawn->DisplayedName, pNewSpawn->Level);
        return;
    }
    // if we did not put INVALID or UNKNOWN above
    if (!*szSpawnLoc) sprintf(szSpawnLoc, "%.2f, %.2f, %.2f", pNewSpawn->Y, pNewSpawn->X, pNewSpawn->Z);

    WriteSpawn(pNewSpawn, szType, szSpawnLoc, bSpawn);
}

PLUGIN_API void OnAddSpawn(PSPAWNINFO pNewSpawn)
{
    if (GetGameState() == GAMESTATE_INGAME && !bZoning && CFG.ON && time(NULL) > tSeconds + CFG.Delay && pNewSpawn->SpawnID)
    {
        CheckOurType(pNewSpawn, true);
    }
}

PLUGIN_API void OnRemoveSpawn(PSPAWNINFO pOldSpawn)
{
    if (GetGameState() == GAMESTATE_INGAME && !bZoning && CFG.ON && time(NULL) > tSeconds + CFG.Delay && pOldSpawn->SpawnID)
    {
        CheckOurType(pOldSpawn, false);
    }
}

PLUGIN_API void OnBeginZone()
{
    bZoning = true;
}

PLUGIN_API void SetGameState(unsigned long ulGameState)
{
    ulGameState = GetGameState();
    if (ulGameState == GAMESTATE_INGAME)
    {
		sprintf(szLogFile, "MQ2SpawnLog-%s-%s-%i.log", ((PCHARINFO)pCharData)->Name, GetShortZone(((PSPAWNINFO)pLocalPlayer)->Zone), ((PSPAWNINFO)pLocalPlayer)->Instance);
        sprintf(szCharName, "%s.%s", EQADDR_SERVERNAME, ((PCHARINFO)pCharData)->Name);
        sprintf(szLogPath, "%s\\%s", szDirPath, szLogFile);
        tSeconds = time(NULL);
        bZoning = false;

        StartLog();
    }
    else
    {
        bZoning = true;
        EndLog();
    }
}

PLUGIN_API void OnReloadUI()
{
    if (GetGameState() == GAMESTATE_INGAME)
    {
        sprintf(szCharName, "%s.%s", EQADDR_SERVERNAME, ((PCHARINFO)pCharData)->Name);
    }
}

PLUGIN_API void OnCleanUI()
{
}

PLUGIN_API void OnPulse()
{
    if(InHoverState() && OurWnd)
    {
        ((CXWnd*)OurWnd)->DoAllDrawing();
    }
}

PLUGIN_API void InitializePlugin()
{
    tSeconds = time(NULL);
    HandleConfig(false);
    bLoaded = true;
}

PLUGIN_API void ShutdownPlugin()
{
    EndLog();
    HandleConfig(true);
}

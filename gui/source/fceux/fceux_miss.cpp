#include "driver.h"


// dummy

// Everyting was needed to compile (driver things)
// see doc/porting.txt to implement func ...

//Displays an error.  Can block or not.

void FCEUD_PrintError(const char *s) {
    printf("%s\r\n", s);
}

void FCEUD_Message(const char *s) {
    printf("%s\r\n", s);
}
// Need something to hold the PC palette

#undef DUMMY
#define DUMMY(f) void f(void) { };

DUMMY(FCEUD_HideMenuToggle)
DUMMY(FCEUD_TurboOn)
DUMMY(FCEUD_TurboOff)
DUMMY(FCEUD_TurboToggle)
DUMMY(FCEUD_SaveStateAs)
DUMMY(FCEUD_LoadStateFrom)
DUMMY(FCEUD_MovieRecordTo)
DUMMY(FCEUD_MovieReplayFrom)
DUMMY(FCEUD_ToggleStatusIcon)
DUMMY(FCEUD_DebugBreakpoint)
DUMMY(FCEUD_SoundToggle)
DUMMY(FCEUD_AviRecordTo)
DUMMY(FCEUD_AviStop)
void FCEUI_AviVideoUpdate(const unsigned char* buffer) {
}

int FCEUD_ShowStatusIcon(void) {
    return 0;
}

bool FCEUI_AviIsRecording(void) {
    return 0;
}

bool FCEUI_AviDisableMovieMessages() {
    return true;
}

const char *FCEUD_GetCompilerString() {
    return NULL;
}

void FCEUI_UseInputPreset(int preset) {
}

void FCEUD_SoundVolumeAdjust(int n) {
}

void FCEUD_SetEmulationSpeed(int cmd) {
}// Netplay

int FCEUD_SendData(void *data, unsigned long len) {
    return 1;
}

int FCEUD_RecvData(void *data, unsigned long len) {
    return 0;
}

void FCEUD_NetworkClose(void) {
}

void FCEUD_NetplayText(unsigned char *text) {
    printf("%s", text);
}

// File Control

FILE *FCEUD_UTF8fopen(const char *n, const char *m) {
    return (fopen(n, m));
}

EMUFILE_FILE* FCEUD_UTF8_fstream(const char *n, const char *m) {
    std::ios_base::openmode mode = std::ios_base::binary;
    if (!strcmp(m, "r") || !strcmp(m, "rb"))
        mode |= std::ios_base::in;
    else if (!strcmp(m, "w") || !strcmp(m, "wb"))
        mode |= std::ios_base::out | std::ios_base::trunc;
    else if (!strcmp(m, "a") || !strcmp(m, "ab"))
        mode |= std::ios_base::out | std::ios_base::app;
    else if (!strcmp(m, "r+") || !strcmp(m, "r+b"))
        mode |= std::ios_base::in | std::ios_base::out;
    else if (!strcmp(m, "w+") || !strcmp(m, "w+b"))
        mode |= std::ios_base::in | std::ios_base::out | std::ios_base::trunc;
    else if (!strcmp(m, "a+") || !strcmp(m, "a+b"))
        mode |= std::ios_base::in | std::ios_base::out | std::ios_base::app;
    return new EMUFILE_FILE(n, m);
}

bool FCEUD_ShouldDrawInputAids() {
    return false;
}

bool FCEUD_PauseAfterPlayback() {
    return false;
};

int link(const char *path1, const char *path2) {
    return -1;
}
bool turbo = false;
int closeFinishedMovie = 0;

void FCEUD_VideoChanged() {
}

FCEUFILE* FCEUD_OpenArchiveIndex(ArchiveScanRecord& asr, std::string &fname, int innerIndex) {
    return 0;
}

FCEUFILE* FCEUD_OpenArchive(ArchiveScanRecord& asr, std::string& fname, std::string* innerFilename) {
    return 0;
}

ArchiveScanRecord FCEUD_ScanArchive(std::string fname) {
    return ArchiveScanRecord();
}



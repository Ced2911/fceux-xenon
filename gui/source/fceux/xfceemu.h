#include "XEmu.h"

class xfceemu:public XEmu{
public:
    ~xfceemu();

    // initialise emulator - video - audio - input
    int Initialise();
    // shutdown
    int Shutdown();
    
    // save states
    int SaveState(char * filename);
    int SaveSram(char * filename);
    int LoadState(char * filename);
    int LoadSram(char * filename);
    
    // settings
    int SetDefaultOptions(OptionList * option);
    int GetOption(OptionList * option);
    int SetOptions(void * option);
    int LoadOptions();
    int SaveOptions();
    
    // emulation
    int Pause();
    int Resume();
    int OpenRom(char * root, char * dir, char * filename);
    int Reset();
    
    // string ...
    char * getRomName();
    char * getEmuName();
    char * getFolderName();
};
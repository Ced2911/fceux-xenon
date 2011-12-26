/* 
 * File:   XEmu.h
 * Author: cc
 *
 * Created on 25 décembre 2011, 20:39
 */

#ifndef XEMU_H
#define	XEMU_H

#include "../libwiigui/gui.h"

class XEmu {
public:
    XEmu();
    virtual ~XEmu() = 0;;

    // initialise emulator - video - audio - input
    virtual int Initialise()= 0;
    // shutdown
    virtual int Shutdown()= 0;
    
    // save states
    virtual int SaveState(char * filename)= 0;
    virtual int SaveSram(char * filename)= 0;
    virtual int LoadState(char * filename)= 0;
    virtual int LoadSram(char * filename)= 0;
    
    // settings
    virtual int GetOption(OptionList * option)= 0;
    virtual int SetOptions(void * option)= 0;
    
    // emulation
    virtual int Pause()= 0;
    virtual int Resume()= 0;
    virtual int OpenRom(char * root, char * dir, char * filename)= 0;
    virtual int Reset()= 0;
    
    // string ...
    virtual char * getRomName()= 0;
    virtual char * getEmuName()= 0;
    virtual char * getFolderName()= 0;
private:

};

#endif	/* XEMU_H */


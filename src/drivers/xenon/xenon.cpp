#include "xenon.h"
#include <xenon_sound/sound.h>
#include <input/input.h>
#include <byteswap.h>

#if 0

//xe_video.c
extern "C" {
    // direct access to xenos surface data
    extern unsigned int * nesBitmap;
    void SYSVideoInit();
    void SYSVideoUpdate();
}

uint32 powerpadbuf = 0;

struct pcpal {
    unsigned char r;
    unsigned char g;
    unsigned char b;
};
pcpal pcpalette[256];

static struct controller_data_s Gamepads[2];

#define JOY_A   1
#define JOY_B   2
#define JOY_SELECT      4
#define JOY_START       8
#define JOY_UP  0x10
#define JOY_DOWN        0x20
#define JOY_LEFT        0x40
#define JOY_RIGHT       0x80

uint16_t * pAudioBuffer = NULL;
uint16_t * pAudioStart = NULL;
int sound_pos = 0;
void update_sound(int32 * snd, int32 size){
    //return;
    //size = size * 4;
    uint16_t sound_s16;
    uint32_t * dst = (uint32_t *) pAudioBuffer;

    sound_pos = 0;
    
    for(int i = 0; i < size; i++ )
    {
            sound_s16 = snd[i] & 0xffff;
            //dst[sound_pos++] = sound_s16 | ( sound_s16 << 16);
            //dst[sound_pos++] = bswap_32(snd[i]);
        dst[sound_pos++] = bswap_32(sound_s16 | ( sound_s16 << 16));
            if (sound_pos == 4000)
                    sound_pos = 0;
    }

    while (xenon_sound_get_unplayed()>(size)) udelay(50);
    
    //udelay(16);

    xenon_sound_submit(pAudioBuffer, size*4);
}

void update_input() {
    usb_do_poll();
    
    unsigned char pad[4];
    memset(pad, 0, sizeof (char) * 4);

    for (int dwUser = 0; dwUser < 2; dwUser++) {
        
        get_controller_data(&Gamepads[dwUser], dwUser);
        
        if (!FCEUI_EmulationPaused()) {
            if (Gamepads[dwUser].s1_y > 13107)
                pad[dwUser] |= JOY_B;

            if (Gamepads[dwUser].s1_y < -13107)
                pad[dwUser] |= JOY_UP;

            if (Gamepads[dwUser].s1_x > 13107)
                pad[dwUser] |= JOY_RIGHT;

            if (Gamepads[dwUser].s1_x < -13107)
                pad[dwUser] |= JOY_LEFT;

            if (Gamepads[dwUser].up)
                pad[dwUser] |= JOY_UP;

            if (Gamepads[dwUser].down)
                pad[dwUser] |= JOY_DOWN;

            if (Gamepads[dwUser].left)
                pad[dwUser] |= JOY_LEFT;

            if (Gamepads[dwUser].right)
                pad[dwUser] |= JOY_RIGHT;

            if (Gamepads[dwUser].a)
                pad[dwUser] |= JOY_A;

            if (Gamepads[dwUser].b)
                pad[dwUser] |= JOY_B;

            if (Gamepads[dwUser].x)
                pad[dwUser] |= JOY_A;

            if (Gamepads[dwUser].y)
                pad[dwUser] |= JOY_B;

            if (Gamepads[dwUser].start)
                pad[dwUser] |= JOY_START;

            if (Gamepads[dwUser].select)
                pad[dwUser] |= JOY_SELECT;
        }
    }
    //-------------------------------------------------------------------------------------
    // Set input from all the gamepads
    //------------------------------------------------------------------------------------- 
    powerpadbuf = pad[0] | pad[1] << 8; //| pad[2] << 16 | pad[3] << 24;;
}


int main() {
    xenon_make_it_faster(XENON_SPEED_FULL);
    xenos_init(VIDEO_MODE_AUTO);

    console_init();
    xenon_sound_init();
    pAudioStart = pAudioBuffer = (uint16_t*) malloc(48000 * sizeof (uint16_t));
    memset(pAudioBuffer, 0, 48000 * sizeof (uint16_t));

    usb_init();
    usb_do_poll();
    SYSVideoInit();

    //    Allocates and initializes memory.  Should only be called once, before
    //    any calls to other FCEU functions.
    FCEUI_Initialize();

    //-------------------------------------------------------------------------------------
    // Set some setting
    //-------------------------------------------------------------------------------------
    //    Specifies the base FCE Ultra directory.  This should be called
    //    immediately after FCEUI_Initialize() and any time afterwards.
    std::string base = "uda:/";
    //FCEUI_SetBaseDirectory(base); // doesn't work ? newlib bug ?
    FCEUI_SetVidSystem(0);

    //Apply settings
    FCEUI_Sound(48000);
    FCEUI_SetSoundVolume(50);
    FCEUI_SetLowPass(0);


    if (FCEUI_LoadGame("uda:/Super Mario Bros. (Europe) (Rev 0A).zip", 0) != NULL) {
        FCEUI_SetInput(0, SI_GAMEPAD, (void*) &powerpadbuf, 0);
        FCEUI_SetInput(1, SI_GAMEPAD, (void*) &powerpadbuf, 0);

        //set to ntsc
        extern FCEUGI * GameInfo;
        GameInfo->vidsys = GIV_NTSC;
    }

    int32 * snd = NULL;
    int32 sndsize;

    //    Copy contents of XBuf over to video memory(or whatever needs to be 
    //    done to make the contents of XBuf visible on screen).
    //    Each line is 256 pixels(and bytes) in width, and there can be 240
    //    lines.  The pitch for each line is 272 bytes.
    //    XBuf will be 0 if the symbol FRAMESKIP is defined and this frame
    //    was skipped.

    uint8 * bitmap;
    while (1) {
        FCEUI_Emulate(&bitmap, &snd, &sndsize, 0);
        for (int i = 0; i < (256 * 240); i++) {
            //Make an ARGB bitmap
            nesBitmap[i] = ((pcpalette[bitmap[i]].r) << 16) | ((pcpalette[bitmap[i]].g) << 8) | (pcpalette[bitmap[i]].b) | (0xFF << 24);
        }
        SYSVideoUpdate();
        // Add Sound
        update_sound(snd,sndsize);
        // Add Input
        update_input();
    }

    return 0;
}

#endif
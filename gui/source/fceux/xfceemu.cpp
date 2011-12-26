#include "driver.h"
#include "xfceemu.h"
#include <xetypes.h>
#include <xenos/xe.h>
#include <xenon_sound/sound.h>
#include <input/input.h>
#include <byteswap.h>
#include <usb/usbmain.h>
#include <ppc/timebase.h>

typedef unsigned int DWORD;
#include "ps.h"
#include "vs.h"

int option_changed = 0;
OptionList * emu_options = NULL;

static char cart_name[256];
static char fceux_filename[256];
static char fceux_foldername[256];

static struct controller_data_s Gamepads[2];

static int running = 0;
static int fce_exit = 0;
static uint32 powerpadbuf = 0;

static uint8 * bitmap = NULL;
static uint16_t * pAudioBuffer = NULL;
static uint16_t * pAudioStart = NULL;

static int32 * snd = NULL;
static int32 sndsize;

static unsigned int * nesBitmap;


#define JOY_A           1
#define JOY_B           2
#define JOY_SELECT      4
#define JOY_START       8
#define JOY_UP          0x10
#define JOY_DOWN        0x20
#define JOY_LEFT        0x40
#define JOY_RIGHT       0x80


static u32 normaldiff;
static uint64_t prev;
static uint64_t now;

#define diff_usec(start,end) tb_diff_usec(end,start)

void setFrameTimer() {
    if (FCEUI_GetCurrentVidSystem(NULL, NULL) == 1) // PAL
        normaldiff = 20000; // 50hz
    else
        normaldiff = 16667; // 60hz
    prev = mftb();
}

void sync_speed() {
    now = mftb();
    u32 diff = diff_usec(prev, now);


    while (diff_usec(prev, now) < normaldiff) {
        now = mftb();
        usleep(50);
    }

    prev = now;
}

void update_video();
void init_video();

void update_sound(int32 * snd, int32 size) {
    //return;
    //size = size * 4;
    uint16_t sound_s16;
    uint32_t * dst = (uint32_t *) pAudioBuffer;

    int sound_pos = 0;

    for (int i = 0; i < size; i++) {
        sound_s16 = snd[i] & 0xffff;
        //dst[sound_pos++] = sound_s16 | ( sound_s16 << 16);
        //dst[sound_pos++] = bswap_32(snd[i]);
        dst[sound_pos++] = bswap_32(sound_s16 | (sound_s16 << 16));
    }

    while (xenon_sound_get_unplayed()>(size)) udelay(50);

    //udelay(16);
    sync_speed();
    //xenon_sound_submit(pAudioBuffer, size * 4);
}

void update_input() {
    usb_do_poll();

    unsigned char pad[4];
    memset(pad, 0, sizeof (char) * 4);

    for (int dwUser = 0; dwUser < 2; dwUser++) {

        get_controller_data(&Gamepads[dwUser], dwUser);

        if (!FCEUI_EmulationPaused()) {
            if (Gamepads[dwUser].logo)
                running = 0;

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

struct pcpal {
    unsigned char r;
    unsigned char g;
    unsigned char b;
};
pcpal pcpalette[256];

void FCEUD_SetPalette(unsigned char index, unsigned char r, unsigned char g, unsigned char b) {
    pcpalette[index].r = r;
    pcpalette[index].g = g;
    pcpalette[index].b = b;
}

void FCEUD_GetPalette(unsigned char i, unsigned char *r, unsigned char *g, unsigned char *b) {
    *r = pcpalette[i].r;
    *g = pcpalette[i].g;
    *b = pcpalette[i].b;
}

void fceux_loop() {
    while (running) {
        FCEUI_Emulate(&bitmap, &snd, &sndsize, 0);
        for (int i = 0; i < (256 * 240); i++) {
            //Make an ARGB bitmap
            nesBitmap[i] = ((pcpalette[bitmap[i]].r) << 16) | ((pcpalette[bitmap[i]].g) << 8) | (pcpalette[bitmap[i]].b) | (0xFF << 24);
        }
        update_video();
        // Add Sound
        update_sound(snd, sndsize);
        // Add Input
        update_input();
    }
}

// initialise emulator - video - audio - input

int xfceemu::Initialise() {
    // create audio buffer
    pAudioStart = pAudioBuffer = (uint16_t*) malloc(48000 * sizeof (uint16_t));
    memset(pAudioBuffer, 0, 48000 * sizeof (uint16_t));

    //    Allocates and initializes memory.  Should only be called once, before
    //    any calls to other FCEU functions.
    FCEUI_Initialize();

    FCEUI_SetVidSystem(0);
    //Apply settings
    FCEUI_Sound(48000);
    FCEUI_SetSoundVolume(50);
    FCEUI_SetLowPass(0);

    init_video();

    return 0;
};

// shutdown

int xfceemu::Shutdown() {
    return 0;
};

// save states

int xfceemu::SaveState(char * filename) {
    FCEUI_SaveState(filename);
    return 0;
};

int xfceemu::SaveSram(char * filename) {
    return 0;
};

int xfceemu::LoadState(char * filename) {
    FCEUI_LoadState(filename);
    return 0;
};

int xfceemu::LoadSram(char * filename) {
    return 0;
};

enum fceoptionorder {
    VIDEO_FILTER,
    VIDEO_STRETCH,
    OPTION_MAX
};

enum fce_video_filter {
    VF_NONE,
    VF_2XSAI,
    VF_BLINEAR,
    VF_MAX
};
enum fce_screet {
    VS_FULLSCREEN,
    VS_4_3,
    VS_MAX
};

int xfceemu::LoadOptions() {
    return 0;
};

int xfceemu::SaveOptions() {
    return 0;
};

int xfceemu::SetDefaultOptions(OptionList * options) {
    // set default settings
    for (int i = 0; i < 50; i++) {
        options->v[i].max = 0;
        options->v[i].curr = 0;
    }
    options->length = OPTION_MAX;

    strcpy(options->name[VIDEO_FILTER], "Video Filter");
    options->v[VIDEO_FILTER].curr = VF_NONE;
    options->v[VIDEO_FILTER].max = VF_MAX;

    strcpy(options->name[VIDEO_STRETCH], "Aspect Ratio");
    options->v[VIDEO_STRETCH].curr = VS_FULLSCREEN;
    options->v[VIDEO_STRETCH].max = VS_MAX;

    emu_options = options;

    return 0;
}

// settings

int xfceemu::GetOption(OptionList * options) {
    option_changed++;

    // clamp
//    for (int i = 0; i < 50; i++) {
//        if (options->v[i].curr >= options->v[i].max);
//        {
//            options->v[i].curr = 0;
//        }
//    }

    // vf
    switch (options->v[VIDEO_FILTER].curr) {
        case VF_NONE:
            strcpy(options->value[VIDEO_FILTER], "None");
            break;
        case VF_BLINEAR:
            strcpy(options->value[VIDEO_FILTER], "Blinear");
            break;
        case VF_2XSAI:
            strcpy(options->value[VIDEO_FILTER], "2xSai");
            break;
    }

    // vs
    if (options->v[VIDEO_STRETCH].curr==VS_FULLSCREEN) {
        strcpy(options->value[VIDEO_STRETCH], "FullScreen");
    } else {
        strcpy(options->value[VIDEO_STRETCH], "4/3");
    }

    return 0;
};

int xfceemu::SetOptions(void * option) {
    return 0;
};

// emulation

int xfceemu::Pause() {
    running = 0;

    return 0;
};

int xfceemu::Resume() {
    running = 1;
    fceux_loop();
    return 0;
};

int xfceemu::OpenRom(char * root, char * dir, char * filename) {
    strcpy(cart_name, filename);
    sprintf(fceux_filename, "%s/%s/%s", root, dir, filename);

    if (FCEUI_LoadGame(fceux_filename, 0) != NULL) {
        FCEUI_SetInput(0, SI_GAMEPAD, (void*) &powerpadbuf, 0);
        FCEUI_SetInput(1, SI_GAMEPAD, (void*) &powerpadbuf, 0);
    }

    setFrameTimer();

    return 0;
};

int xfceemu::Reset() {
    FCEUI_ResetNES();
    return 0;
};

// string ...

char * xfceemu::getRomName() {
    return cart_name;
};

char * xfceemu::getEmuName() {
    return "FCEUX Xenon";
};

char * xfceemu::getFolderName() {
    return fceux_foldername;
};
// video



#define XE_W 256
#define XE_H 240

static struct XenosVertexBuffer *vb = NULL;
//static struct XenosDevice * g_pVideoDevice = NULL;
static struct XenosShader * g_pVertexShader = NULL;
static struct XenosShader * g_pPixelTexturedShader = NULL;

struct XenosSurface * g_pTexture = NULL;

static uint32_t pitch = 0;

typedef struct DrawVerticeFormats {
    float x, y, z, w;
    unsigned int color;
    float u, v;
} DrawVerticeFormats;

enum {
    UvBottom = 0,
    UvTop,
    UvLeft,
    UvRight
};
float ScreenUv[4] = {0.f, 1.0f, 1.0f, 0.f};

void init_video() {
    //g_pVideoDevice = GetVideoDevice();

    Xe_SetRenderTarget(g_pVideoDevice, Xe_GetFramebufferSurface(g_pVideoDevice));

    static const struct XenosVBFFormat vbf = {
        3,
        {
            {XE_USAGE_POSITION, 0, XE_TYPE_FLOAT4},
            {XE_USAGE_COLOR, 0, XE_TYPE_UBYTE4},
            {XE_USAGE_TEXCOORD, 0, XE_TYPE_FLOAT2},
        }
    };

    g_pPixelTexturedShader = Xe_LoadShaderFromMemory(g_pVideoDevice, (void*) g_xps_PS);
    Xe_InstantiateShader(g_pVideoDevice, g_pPixelTexturedShader, 0);

    g_pVertexShader = Xe_LoadShaderFromMemory(g_pVideoDevice, (void*) g_xvs_VS);
    Xe_InstantiateShader(g_pVideoDevice, g_pVertexShader, 0);
    Xe_ShaderApplyVFetchPatches(g_pVideoDevice, g_pVertexShader, 0, &vbf);

    g_pTexture = Xe_CreateTexture(g_pVideoDevice, XE_W, XE_H, 1, XE_FMT_8888 | XE_FMT_ARGB, 0);
    //g_pTexture = Xe_CreateTexture(g_pVideoDevice, XE_W, XE_H, 1, XE_FMT_5551 | XE_FMT_ARGB, 0);
    nesBitmap = (unsigned int*) Xe_Surface_LockRect(g_pVideoDevice, g_pTexture, 0, 0, 0, 0, XE_LOCK_WRITE);

    pitch = g_pTexture->wpitch;
    Xe_Surface_Unlock(g_pVideoDevice, g_pTexture);

    // move it to ini file
    float x = -1.0f;
    float y = 1.0f;
    float w = 2.0f;
    float h = 2.0f;

    vb = Xe_CreateVertexBuffer(g_pVideoDevice, 3 * sizeof (DrawVerticeFormats));
    DrawVerticeFormats *Rect = (DrawVerticeFormats *) Xe_VB_Lock(g_pVideoDevice, vb, 0, 3 * sizeof (DrawVerticeFormats), XE_LOCK_WRITE);
    {
        // top left
        Rect[0].x = x;
        Rect[0].y = y;
        Rect[0].u = ScreenUv[UvBottom];
        Rect[0].v = ScreenUv[UvRight];
        Rect[0].color = 0;

        // bottom left
        Rect[1].x = x;
        Rect[1].y = y - h;
        Rect[1].u = ScreenUv[UvBottom];
        Rect[1].v = ScreenUv[UvLeft];
        Rect[1].color = 0;

        // top right
        Rect[2].x = x + w;
        Rect[2].y = y;
        Rect[2].u = ScreenUv[UvTop];
        Rect[2].v = ScreenUv[UvRight];
        Rect[2].color = 0;

        int i = 0;
        for (i = 0; i < 3; i++) {
            Rect[i].z = 0.0;
            Rect[i].w = 1.0;
        }
    }
    Xe_VB_Unlock(g_pVideoDevice, vb);

    Xe_SetClearColor(g_pVideoDevice, 0);
}

void update_video() {
    if (option_changed) {

        if (emu_options->v[VIDEO_FILTER].curr == VF_BLINEAR) {
            g_pTexture->use_filtering = 1;
        } else {
            g_pTexture->use_filtering = 0;
        }

        // apply filter
        DrawVerticeFormats *Rect = (DrawVerticeFormats *) Xe_VB_Lock(g_pVideoDevice, vb, 0, 3 * sizeof (DrawVerticeFormats), XE_LOCK_WRITE);
        {
            // center
            float x, y;
            x = 0;
            y = 0;

            float w = 0;
            float h = 0;
            if (emu_options->v[VIDEO_STRETCH].curr==VS_FULLSCREEN) {
                w = 1;
                h = 1;
            } else {
                // 4/3
                w = 3.f / 4.f;
                h = 1;
            }

            // bottom left
            Rect[0].x = x - w;
            Rect[0].y = y - h;
            Rect[0].u = 0;
            Rect[0].v = 1;

            // bottom right
            Rect[1].x = x + w;
            Rect[1].y = y - h;
            Rect[1].u = 1;
            Rect[1].v = 1;

            // top right
            Rect[2].x = x + w;
            Rect[2].y = y + h;
            Rect[2].u = 1;
            Rect[2].v = 0;
        }
        Xe_VB_Unlock(g_pVideoDevice, vb);
    }

    option_changed = 0;


    // Refresh texture cash
    Xe_Surface_LockRect(g_pVideoDevice, g_pTexture, 0, 0, 0, 0, XE_LOCK_WRITE);
    Xe_Surface_Unlock(g_pVideoDevice, g_pTexture);

    // Reset states
    Xe_InvalidateState(g_pVideoDevice);
    Xe_SetClearColor(g_pVideoDevice, 0);



    // Select stream and shaders
    Xe_SetTexture(g_pVideoDevice, 0, g_pTexture);
    Xe_SetCullMode(g_pVideoDevice, XE_CULL_NONE);
    Xe_SetStreamSource(g_pVideoDevice, 0, vb, 0, sizeof (DrawVerticeFormats));
    Xe_SetShader(g_pVideoDevice, SHADER_TYPE_PIXEL, g_pPixelTexturedShader, 0);
    Xe_SetShader(g_pVideoDevice, SHADER_TYPE_VERTEX, g_pVertexShader, 0);

    // Draw
    Xe_DrawPrimitive(g_pVideoDevice, XE_PRIMTYPE_RECTLIST, 0, 1);


    // Resolve
    Xe_Resolve(g_pVideoDevice);
    while (!Xe_IsVBlank(g_pVideoDevice));
    Xe_Sync(g_pVideoDevice);
}

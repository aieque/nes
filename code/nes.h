#include "common.h"

#define APP_EXPORT extern "C" __declspec(dllexport)

/// PLATFORM API ///
struct file_read_result {
    char *Contents;
    uint32 ContentsSize;
};

typedef file_read_result debug_platform_read_file(char *Filepath);
typedef void             debug_platform_free_file_data(file_read_result *Result);
typedef void             platform_swap_buffers();
typedef void *           platform_load_opengl_procedure(char *Name);
typedef void             platform_toggle_fullscreen();
typedef void *           platform_reserve(uint64 Size);
typedef void             platform_release(void *Memory);
typedef void             platform_commit(void *Memory, uint64 Size);
typedef void             platform_decommit(void *Memory, uint64 Size);


/// APPLICATION API ///
struct platform;
struct core;

typedef void app_update();
void AppUpdateStub() {}

typedef void app_hot_load(platform *_Platform);
void AppHotLoadStub(platform *_Platform) {}

typedef void app_quit();
void AppQuitStub() {}


enum key_code {
#define Key(Name, String) Key_##Name,
#include "platform_key_list.inc"
    Key_Max
};

#define IsKeyPressed(Key) (Platform->KeysDown[Key] && !Platform->PreviousKeysDown[Key])
#define IsKeyDown(Key) (Platform->KeysDown[Key])
#define IsKeyReleased(Key) (!Platform->KeysDown[Key] && Platform->PreviousKeysDown[Key])

#define IsMousePressed(Mouse) (Platform->MouseDown[Mouse] && !Platform->PreviousMouseDown[Mouse])
#define IsMouseDown(Mouse) (Platform->MouseDown[Mouse])
#define IsMouseReleased(Mouse) (!Platform->MouseDown[Mouse] && Platform->PreviousMouseDown[Mouse])

enum mouse_button {
    MouseButton_Left,
    MouseButton_Right,
    MouseButton_Middle,
    
    MouseButton_Max
};

struct platform {
    bool Quit;
    bool Fullscreen;
    
    bool IsInitialized;
    
    memory_arena PermanentArena;
    memory_arena ScratchArena;
    
    int16 *SampleOut;
    uint32 SampleCountToOutput;
    uint32 SamplesPerSecond;
    
    v2 WindowSize;
    
    v2 MousePosition;
    bool MouseDown[MouseButton_Max];
    bool PreviousMouseDown[MouseButton_Max];
    
    bool KeysDown[Key_Max];
    bool PreviousKeysDown[Key_Max];
    
    string_buffer *KeyboardRedirect;
    
    debug_platform_read_file *DEBUGReadFile;
    debug_platform_free_file_data *DEBUGFreeFileData;
    
    platform_toggle_fullscreen *ToggleFullscreen;
    platform_swap_buffers *SwapBuffers;
    platform_load_opengl_procedure *LoadOpenGLProcedure;
    
    platform_reserve *Reserve;
    platform_release *Release;
    platform_commit *Commit;
    platform_decommit *Decommit;
};

struct cpu {
    uint8 A;
    uint8 X, Y;
    
    uint8 Status;
    
    uint8 SP;
    uint16 PC;
    
    int Cycles;
};

struct cartridge_header {
    char NESText[4];
    
    uint8 PRGSize;
    uint8 CHRSize;
    
    union {
        struct {
            uint8 MirroringMode  : 1;
            uint8 HasBattery     : 1;
            uint8 HasTrainer     : 1;
            uint8 FourScreenVRAM : 1;
        };
        
        char Padding[10];
    };
};

struct cartridge {
    cartridge_header Header;
    uint8 *Trainer;
    uint8 *PRGROM;
    uint8 *CHRROM;
};

struct mmu {
    cartridge *Cartridge;
};

struct nes {
    cpu CPU;
    mmu MMU;
    cartridge Cartridge;
};

#include "renderer_opengl.h"
#include "imgui.h"

struct core {
    memory_arena *PermanentArena;
    memory_arena *ScratchArena;
    
    nes NES;
    
    //- Core Stuff 
    renderer Renderer;
    ui UI;
};
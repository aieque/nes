#include "nes.h"

global platform *Platform;
global core *Core;

#include "memory.cpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "opengl.h"

#include "renderer_opengl.cpp"
#include "imgui.cpp"

#include "cartridge.cpp"
#include "mmu.cpp"
#include "cpu.cpp"

APP_EXPORT void
AppHotLoad(platform *_Platform) {
    Platform = _Platform;
    Core = (core *)Platform->PermanentArena.Base;
    
    LoadOpenGLProcedures();
}

APP_EXPORT void
AppUpdate(platform *_Platform) {
    if (!Platform->IsInitialized) {
        Platform->PermanentArena = InitializeMemoryArena();
        Platform->ScratchArena = InitializeMemoryArena();
        
        Core = (core *)Platform->PermanentArena.Base;
        MemoryArenaPushZero(&Platform->PermanentArena, sizeof(core));
        
        Core->PermanentArena = &Platform->PermanentArena;
        Core->ScratchArena = &Platform->ScratchArena;
        
        Renderer_Init(&Core->Renderer, Core->PermanentArena, Megabytes(12));
        
        UI_DefaultStyle(&Core->UI.Style, LoadFont(Core->PermanentArena, "assets/fonts/ProggyClean.ttf", 16, 1, 1));
        
        Core->NES.MMU.Cartridge = &Core->NES.Cartridge;
        
        LoadCartridge("ice.nes", &Core->NES.Cartridge, Core->PermanentArena);
        ResetCPU(&Core->NES.CPU, &Core->NES.MMU);
        
        Platform->IsInitialized = true;
    }
    
    if (IsKeyPressed(Key_F3)) {
        Platform->ToggleFullscreen();
    }
    
    Core->UI.PreviousMousePosition = Core->UI.MousePosition;
    Core->UI.MousePosition = Platform->MousePosition;
    Core->UI.LeftMouseDown = IsMouseDown(MouseButton_Left);
    
    ClockCPU(&Core->NES.CPU, &Core->NES.MMU);
    
    UI_SetupFrame(&Core->UI);
    
    ShowCPURegisters(&Core->NES.CPU, &Core->UI);
    
    UI_BeginDropdown(&Core->UI, "NES");
    {
        UI_Button(&Core->UI, "Reset");
        
        UI_AddPadding(&Core->UI, 20);
        UI_Label(&Core->UI, "Load Rom:");
        UI_AddPadding(&Core->UI, 5);
        
        local_persist char ROMFileBuffer[60];
        UI_Textbox(&Core->UI, ROMFileBuffer, 60);
        
        UI_Button(&Core->UI, "Load");
    }
    UI_EndDropdown(&Core->UI);
    
    Renderer_BeginFrame(&Core->Renderer, Platform->WindowSize.Width, Platform->WindowSize.Height);
    
    UI_RenderUI(&Core->UI, &Core->Renderer);
    
    Renderer_EndFrame(&Core->Renderer);
    
    Platform->SwapBuffers();
}

APP_EXPORT void
AppQuit() {}
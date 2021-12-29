#include "nes.h"

#include <windows.h>
#include <GL/gl.h>
#include "wglext.h"
#include "glext.h"

#include <mmdeviceapi.h>
#include <audioclient.h>
#include <audiopolicy.h>

global platform GlobalPlatform;
global HDC GlobalDeviceContext;
global HWND GlobalWindow;

global char GlobalExecutablePath[128];
global char GlobalGameDLLPath[128];
global char GlobalTempGameDLLPath[128];

static const GUID IID_IAudioClient = {0x1CB9AD4C, 0xDBFA, 0x4c32, 0xB1, 0x78, 0xC2, 0xF5, 0x68, 0xA7, 0x03, 0xB2};
static const GUID IID_IAudioRenderClient = {0xF294ACFC, 0x3146, 0x4483, 0xA7, 0xBF, 0xAD, 0xDC, 0xA7, 0xC2, 0x60, 0xE2};
static const GUID CLSID_MMDeviceEnumerator = {0xBCDE0395, 0xE52F, 0x467C, 0x8E, 0x3D, 0xC4, 0x57, 0x92, 0x91, 0x69, 0x2E};
static const GUID IID_IMMDeviceEnumerator = {0xA95664D2, 0x9614, 0x4F35, 0xA7, 0x46, 0xDE, 0x8D, 0xB6, 0x36, 0x17, 0xE6};
static const GUID PcmSubformatGuid = {STATIC_KSDATAFORMAT_SUBTYPE_PCM};

#define SOUND_LATENCY_FPS 15
#define REFTIMES_PER_SEC 10000000

#define CO_CREATE_INSTANCE(name) HRESULT name(REFCLSID rclsid, LPUNKNOWN *pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID *ppv)
typedef CO_CREATE_INSTANCE(co_create_instance);
CO_CREATE_INSTANCE(CoCreateInstanceStub) { return 1; }
global co_create_instance *CoCreateInstanceProc = CoCreateInstanceStub;

#define CO_INITIALIZE_EX(name) HRESULT name(LPVOID pvReserved, DWORD dwCoInit)
typedef CO_INITIALIZE_EX(co_initialize_ex);
CO_INITIALIZE_EX(CoInitializeExStub) { return 1; }
global co_initialize_ex *CoInitializeExProc = CoInitializeExStub;

struct win32_sound_output {
    IMMDeviceEnumerator *DeviceEnumerator;
    IMMDevice *Device;
    IAudioClient *AudioClient;
    IAudioRenderClient *AudioRenderClient;
    REFERENCE_TIME SoundBufferDuration;
    uint32 BufferFrameCount;
    uint32 Channels;
    uint32 SamplesPerSecond;
    uint32 LatencyFrameCount;
};

internal void
Win32LoadWASAPI() {
    HMODULE Library = LoadLibraryA("ole32.dll");
    if (Library) {
        CoCreateInstanceProc = (co_create_instance *)GetProcAddress(Library, "CoCreateInstance");
        CoInitializeExProc = (co_initialize_ex *)GetProcAddress(Library, "CoInitializeEx");
    } else {
        CoCreateInstanceProc = CoCreateInstanceStub;
        CoInitializeExProc = CoInitializeExStub;
    }
}

internal void
Win32InitWASAPI(win32_sound_output *Output) {
    CoInitializeExProc(0, COINIT_SPEED_OVER_MEMORY);
    REFERENCE_TIME RequestedTimeDuration = REFTIMES_PER_SEC * 2;
    
    HRESULT Result = CoCreateInstanceProc(CLSID_MMDeviceEnumerator, 0, CLSCTX_ALL, IID_IMMDeviceEnumerator, (LPVOID *)(&Output->DeviceEnumerator));
    
    if (Result == S_OK) {
        // Since this uses COM, it's not really clear what's a pointer and what's not...
        
        Output->DeviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &Output->Device);
        Result = Output->Device->Activate(IID_IAudioClient, CLSCTX_ALL, 0, (void **)&Output->AudioClient);
        
        if (Result == S_OK) {
            WAVEFORMATEX *WaveFormat = 0;
            Output->AudioClient->GetMixFormat(&WaveFormat);
            
            Output->SamplesPerSecond = 44100;
            WORD BitsPerSample = sizeof(int16) * 8;
            WORD BlockAlign = (Output->Channels * BitsPerSample) / 8;
            DWORD AverageBytesPerSecond = BlockAlign * Output->SamplesPerSecond;
            WORD CBSize = 0;
            
            WAVEFORMATEX NewWaveFormat = {
                WAVE_FORMAT_PCM,
                (WORD)Output->Channels,
                Output->SamplesPerSecond,
                AverageBytesPerSecond,
                BlockAlign,
                BitsPerSample,
                CBSize
            };
            
            Result = Output->AudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED,
                                                     AUDCLNT_STREAMFLAGS_RATEADJUST |
                                                     AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM |
                                                     AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY,
                                                     RequestedTimeDuration, 0, &NewWaveFormat, 0);
            Output->LatencyFrameCount = Output->SamplesPerSecond / SOUND_LATENCY_FPS;
            
            if (Result == S_OK) {
                Result = Output->AudioClient->GetService(IID_IAudioRenderClient, (void **)&Output->AudioRenderClient);
                if (Result == S_OK) {
                    Output->AudioClient->GetBufferSize(&Output->BufferFrameCount);
                    Output->SoundBufferDuration =
                    (REFERENCE_TIME)((real64)REFTIMES_PER_SEC * Output->BufferFrameCount / Output->SamplesPerSecond);
                    Output->AudioClient->Start();
                }
            }
        }
    }
}

internal void
Win32CleanUpWASAPI(win32_sound_output *Output) {
    Output->AudioClient->Stop();
    Output->DeviceEnumerator->Release();
    Output->Device->Release();
    Output->AudioClient->Release();
    Output->AudioRenderClient->Release();
}

internal void
Win32FillSoundBuffer(uint32 SamplesToWrite, int16 *Samples, win32_sound_output *Output) {
    if (SamplesToWrite) {
        BYTE *Data = 0;
        DWORD Flags = 0;
        
        Output->AudioRenderClient->GetBuffer(SamplesToWrite, &Data);
        if (Data) {
            int16 *Destination = (int16 *)Data;
            int16 *Source = Samples;
            for (uint32 SampleIndex = 0; SampleIndex < SamplesToWrite; ++SampleIndex) {
                *Destination++ = *Source++;
                *Destination++ = *Source++;
            }
        }
        
        Output->AudioRenderClient->ReleaseBuffer(SamplesToWrite, Flags);
    }
}

struct win32_game_code {
    HMODULE GameDLL;
    FILETIME LastWriteTime;
    
    app_update *Update;
    app_hot_load *HotLoad;
    app_quit *Quit;
};

internal FILETIME
Win32GetLastWriteTime(char *FilePath) {
    FILETIME LastWriteTime = {};
    WIN32_FIND_DATA FindData;
    
    HANDLE FindHandle = FindFirstFileA(FilePath, &FindData);
    if (FindHandle != INVALID_HANDLE_VALUE) {
        FindClose(FindHandle);
        LastWriteTime = FindData.ftLastWriteTime;
    }
    
    return LastWriteTime;
}

internal win32_game_code
Win32LoadGameCode() {
    win32_game_code Result = {};
    
    CopyFileA(GlobalGameDLLPath, GlobalTempGameDLLPath, FALSE);
    
    Result.LastWriteTime = Win32GetLastWriteTime(GlobalGameDLLPath);
    Result.GameDLL = LoadLibraryA(GlobalTempGameDLLPath);
    
    if (Result.GameDLL) {
        Result.Update = (app_update *)GetProcAddress(Result.GameDLL, "AppUpdate");
        Result.HotLoad = (app_hot_load *)GetProcAddress(Result.GameDLL, "AppHotLoad");
        Result.Quit = (app_quit *)GetProcAddress(Result.GameDLL, "AppQuit");
        
        Result.HotLoad(&GlobalPlatform);
        
        if (!Result.Update) {
            Result.Update = AppUpdateStub;
        }
        
        if (!Result.HotLoad) {
            Result.HotLoad = AppHotLoadStub;
        }
        
        if (!Result.Quit) {
            Result.Quit = AppQuitStub;
        }
    } else {
        Result.Update = AppUpdateStub;
        Result.HotLoad = AppHotLoadStub;
        Result.Quit = AppQuitStub;
    }
    
    
    return Result;
}

internal void
Win32UnloadGameCode(win32_game_code *GameCode) {
    if (GameCode->GameDLL) {
        FreeLibrary(GameCode->GameDLL);
        GameCode->GameDLL = 0;
    }
    
    GameCode->Update = AppUpdateStub;
}

internal void
Win32UpdateGameCode(win32_game_code *GameCode) {
    FILETIME LastWriteTime = Win32GetLastWriteTime(GlobalGameDLLPath);
    
    if (CompareFileTime(&LastWriteTime, &GameCode->LastWriteTime)) {
        Win32UnloadGameCode(GameCode);
        *GameCode = Win32LoadGameCode();
    }
}

struct opengl_context {
    HGLRC GLRenderContext;
    
    PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB;
    PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB;
    PFNWGLMAKECONTEXTCURRENTARBPROC wglMakeContextCurrentARB;
    PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;
};

internal void *
Win32LoadOpenGLProcedure(char *ProcName) {
    void *Pointer = wglGetProcAddress(ProcName);
    return Pointer;
}

internal void
Win32InitOpenGL(opengl_context *RenderContext) {
    PIXELFORMATDESCRIPTOR PixelFormatDesc = {};
    PixelFormatDesc.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    PixelFormatDesc.iPixelType = PFD_TYPE_RGBA;
    PixelFormatDesc.cColorBits = 32;
    PixelFormatDesc.cDepthBits = 24;
    PixelFormatDesc.cStencilBits = 8;
    PixelFormatDesc.iLayerType = PFD_MAIN_PLANE;
    
    int PixelFormat = ChoosePixelFormat(GlobalDeviceContext, &PixelFormatDesc);
    if (PixelFormat) {
        SetPixelFormat(GlobalDeviceContext, PixelFormat, &PixelFormatDesc);
        
        HGLRC DummyContext = wglCreateContext(GlobalDeviceContext);
        wglMakeCurrent(GlobalDeviceContext, DummyContext);
        
        RenderContext->wglChoosePixelFormatARB =
        (PFNWGLCHOOSEPIXELFORMATARBPROC)Win32LoadOpenGLProcedure("wglChoosePixelFormatARB");
        RenderContext->wglCreateContextAttribsARB =
        (PFNWGLCREATECONTEXTATTRIBSARBPROC)Win32LoadOpenGLProcedure("wglCreateContextAttribsARB");
        RenderContext->wglMakeContextCurrentARB =
        (PFNWGLMAKECONTEXTCURRENTARBPROC)Win32LoadOpenGLProcedure("wglMakeContextCurrentARB");
        RenderContext->wglSwapIntervalEXT =
        (PFNWGLSWAPINTERVALEXTPROC)Win32LoadOpenGLProcedure("wglSwapIntervalEXT");
        
        int PixelFormatAttributes[] = {
            WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
            WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
            WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
            WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
            WGL_COLOR_BITS_ARB, 32,
            WGL_DEPTH_BITS_ARB, 24,
            WGL_STENCIL_BITS_ARB, 8,
            0
        };
        
        UINT FormatCount = 0;
        RenderContext->wglChoosePixelFormatARB(GlobalDeviceContext, PixelFormatAttributes,
                                               0, 1, &PixelFormat, &FormatCount);
        
        if (PixelFormat) {
            int ContextAttributes[] = {
                WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
                WGL_CONTEXT_MINOR_VERSION_ARB, 3,
                0
            };
            
            RenderContext->GLRenderContext = RenderContext->wglCreateContextAttribsARB(GlobalDeviceContext,
                                                                                       DummyContext,
                                                                                       ContextAttributes);
            
            if (RenderContext->GLRenderContext) {
                wglMakeCurrent(GlobalDeviceContext, RenderContext->GLRenderContext);
                wglDeleteContext(DummyContext);
                RenderContext->wglSwapIntervalEXT(1);
            }
        }
    }
}

internal void
Win32ToggleFullscreen() {
    local_persist WINDOWPLACEMENT LastPlacement = {sizeof(LastPlacement)};
    
    DWORD WindowStyle = GetWindowLong(GlobalWindow, GWL_STYLE);
    if (WindowStyle & WS_OVERLAPPEDWINDOW) {
        MONITORINFO MonitorInfo = {sizeof(MonitorInfo)};
        if (GetWindowPlacement(GlobalWindow, &LastPlacement) &&
            GetMonitorInfo(MonitorFromWindow(GlobalWindow, MONITOR_DEFAULTTOPRIMARY), &MonitorInfo)) {
            SetWindowLong(GlobalWindow, GWL_STYLE, WindowStyle & ~WS_OVERLAPPEDWINDOW);
            
            SetWindowPos(GlobalWindow, HWND_TOP,
                         MonitorInfo.rcMonitor.left,
                         MonitorInfo.rcMonitor.top,
                         MonitorInfo.rcMonitor.right - MonitorInfo.rcMonitor.left,
                         MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top, 
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    } else {
        SetWindowLong(GlobalWindow, GWL_STYLE, WindowStyle | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(GlobalWindow, &LastPlacement);
        SetWindowPos(GlobalWindow, 0, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}

internal file_read_result
Win32ReadFile(char *Filepath) {
    file_read_result Result = {};
    HANDLE FileHandle = CreateFileA(Filepath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    
    if (FileHandle) {
        uint32 FileSize = GetFileSize(FileHandle, 0);
        
        char *Contents = (char *)VirtualAlloc(0, FileSize + 1, MEM_COMMIT, PAGE_READWRITE);
        
        DWORD BytesRead;
        ReadFile(FileHandle, Contents, FileSize, &BytesRead, 0);
        
        Result.Contents = Contents;
        Result.ContentsSize = FileSize;
        
        CloseHandle(FileHandle);
    }
    
    return Result;
}

internal void
Win32FreeFileData(file_read_result *Result) {
    VirtualFree(Result->Contents, 0, MEM_RELEASE);
}

internal void *
Win32Reserve(uint64 Size) {
    return VirtualAlloc(0, Size, MEM_RESERVE, PAGE_NOACCESS);
}

internal void
Win32Release(void *Memory) {
    VirtualFree(Memory, 0, MEM_RELEASE);
}

internal void
Win32Commit(void *Memory, uint64 Size) {
    VirtualAlloc(Memory, Size, MEM_COMMIT, PAGE_READWRITE);
}

internal void
Win32Decommit(void *Memory, uint64 Size) {
    VirtualFree(Memory, Size, MEM_DECOMMIT);
}

internal void
Win32SwapBuffers() {
    wglSwapLayerBuffers(GlobalDeviceContext, WGL_SWAP_MAIN_PLANE);
}

internal LRESULT
Win32WindowCallback(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam) {
    LRESULT Result = 0;
    
    switch (Message) {
        case WM_QUIT:
        case WM_CLOSE:
        case WM_DESTROY: {
            GlobalPlatform.Quit = true;
        } break;
        
        case WM_CHAR: {
            string_buffer *Buffer = GlobalPlatform.KeyboardRedirect;
            if (Buffer) {
                if (Buffer->Used + 1 < Buffer->Size) {
                    Buffer->Base[Buffer->Used++] = WParam;
                }
            }
        } break;
        
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYUP:
        case WM_KEYDOWN: {
            int VirtualKey = WParam;
            bool IsDown = (LParam & (1 << 31)) == 0;
            
            string_buffer *Buffer = GlobalPlatform.KeyboardRedirect;
            if (Buffer) {
                if (VirtualKey == VK_BACK) {
                    if (Buffer->Used > 0) {
                        Buffer->Base[--Buffer->Used] = 0;
                    }
                }
            }
            
            if (VirtualKey >= 'A' && VirtualKey <= 'Z') {
                GlobalPlatform.KeysDown[VirtualKey - 'A' + Key_A] = IsDown;
            }
            
            if (VirtualKey >= '0' && VirtualKey <= '9') {
                GlobalPlatform.KeysDown[VirtualKey - '0' + Key_0] = IsDown;
            }
            
            if (VirtualKey >= VK_F1 && VirtualKey <= VK_F12) {
                GlobalPlatform.KeysDown[VirtualKey - VK_F1 + Key_F1] = IsDown;
            }
            
            if (VirtualKey == VK_RETURN) {
                GlobalPlatform.KeysDown[Key_Enter] = IsDown;
            }
            
            if (VirtualKey == VK_CONTROL) {
                GlobalPlatform.KeysDown[Key_Ctrl] = IsDown;
            }
            
            if (VirtualKey == VK_SHIFT) {
                GlobalPlatform.KeysDown[Key_Shift] = IsDown;
            }
            
            if (VirtualKey == VK_ESCAPE) {
                GlobalPlatform.KeysDown[Key_Escape] = IsDown;
            }
        } break;
        
        default: {
            Result = DefWindowProc(Window, Message, WParam, LParam);
        } break;
    }
    
    return Result;
}

int
WinMain(HINSTANCE Instance, HINSTANCE PreviousInstance, LPSTR CommandLine, int ShowCommand) {
    WNDCLASS WindowClass = {};
    WindowClass.lpszClassName = "NESWindowClass";
    WindowClass.hInstance = Instance;
    WindowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    WindowClass.lpfnWndProc = Win32WindowCallback;
    WindowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    
    if (RegisterClass(&WindowClass)) {
        HWND Window = CreateWindowEx(0,
                                     WindowClass.lpszClassName,
                                     "nes",
                                     WS_OVERLAPPEDWINDOW,
                                     CW_USEDEFAULT, CW_USEDEFAULT,
                                     CW_USEDEFAULT, CW_USEDEFAULT,
                                     0, 0, Instance, 0);
        
        GlobalWindow = Window;
        
        if (Window) {
            // Setup paths.
            uint32 ExecutablePathLength = GetModuleFileNameA(0, GlobalExecutablePath, sizeof(GlobalExecutablePath));
            int DirectoryPathLength = ExecutablePathLength - 13; // "win32_nes.exe"
            sprintf(GlobalGameDLLPath, "%.*snes.dll", DirectoryPathLength, GlobalExecutablePath);
            sprintf(GlobalTempGameDLLPath, "%.*stemp_nes.dll", DirectoryPathLength, GlobalExecutablePath);
            
            
            // Initialize OpenGL.
            GlobalDeviceContext = GetDC(Window);
            
            opengl_context OpenGLContext;
            Win32InitOpenGL(&OpenGLContext);
            
            
            // Initialize timer.
            LARGE_INTEGER CountsPerSecond;
            QueryPerformanceFrequency(&CountsPerSecond);
            
            bool SleepIsGranular = (timeBeginPeriod(1) == TIMERR_NOERROR);
            
            // Setup platform API.
            GlobalPlatform.DEBUGReadFile = Win32ReadFile;
            GlobalPlatform.DEBUGFreeFileData = Win32FreeFileData;
            
            GlobalPlatform.ToggleFullscreen = Win32ToggleFullscreen;
            GlobalPlatform.SwapBuffers = Win32SwapBuffers;
            GlobalPlatform.LoadOpenGLProcedure = Win32LoadOpenGLProcedure;
            
            GlobalPlatform.Reserve = Win32Reserve;
            GlobalPlatform.Release = Win32Release;
            GlobalPlatform.Commit = Win32Commit;
            GlobalPlatform.Decommit = Win32Decommit;
            
            
            win32_game_code GameCode = Win32LoadGameCode();
            
            
            win32_sound_output SoundOutput = {};
            SoundOutput.Channels = 2;
            SoundOutput.SamplesPerSecond = 48000;
            SoundOutput.LatencyFrameCount = 48000;
            
            Win32LoadWASAPI();
            Win32InitWASAPI(&SoundOutput);
            
            GlobalPlatform.SampleOut = (int16 *)HeapAlloc(GetProcessHeap(), 0, SoundOutput.SamplesPerSecond * sizeof(int16) * 2);
            GlobalPlatform.SamplesPerSecond = SoundOutput.SamplesPerSecond;
            
            
            ShowWindow(Window, SW_SHOW);
            
            
            GlobalPlatform.Quit = false;
            while (!GlobalPlatform.Quit) {
                LARGE_INTEGER StartTime;
                QueryPerformanceCounter(&StartTime);
                
                memcpy(GlobalPlatform.PreviousKeysDown, GlobalPlatform.KeysDown, sizeof(GlobalPlatform.KeysDown));
                
                MSG Message;
                while (PeekMessage(&Message, Window, 0, 0, PM_REMOVE)) {
                    TranslateMessage(&Message);
                    DispatchMessageA(&Message);
                }
                
                RECT ClientRect;
                GetClientRect(Window, &ClientRect);
                GlobalPlatform.WindowSize.Width = ClientRect.right - ClientRect.left;
                GlobalPlatform.WindowSize.Height = ClientRect.bottom - ClientRect.top;
                
                POINT MousePosition;
                GetCursorPos(&MousePosition);
                ScreenToClient(Window, &MousePosition);
                
                GlobalPlatform.MousePosition = V2(MousePosition.x, MousePosition.y);
                
                memcpy(GlobalPlatform.PreviousMouseDown, GlobalPlatform.MouseDown, sizeof(GlobalPlatform.MouseDown));
                GlobalPlatform.MouseDown[MouseButton_Left] = ((GetKeyState(VK_LBUTTON) & 0x8000) != 0);
                
                {
                    GlobalPlatform.SampleCountToOutput = 0;
                    uint32 SoundPaddingSize;
                    if (SUCCEEDED(SoundOutput.AudioClient->GetCurrentPadding(&SoundPaddingSize))) {
                        GlobalPlatform.SamplesPerSecond = SoundOutput.SamplesPerSecond;
                        GlobalPlatform.SampleCountToOutput = (uint32)(SoundOutput.LatencyFrameCount - SoundPaddingSize);
                        if (GlobalPlatform.SampleCountToOutput > SoundOutput.LatencyFrameCount) {
                            GlobalPlatform.SampleCountToOutput = SoundOutput.LatencyFrameCount;
                        }
                    }
                    
                    for (uint32 BufferIndex = 0; BufferIndex < SoundOutput.BufferFrameCount; ++BufferIndex) {
                        GlobalPlatform.SampleOut[BufferIndex] = 0;
                    }
                }
                
                bool OldFullscreen = GlobalPlatform.Fullscreen;
                
                GameCode.Update();
                
                Win32UpdateGameCode(&GameCode);
                
                if (OldFullscreen != GlobalPlatform.Fullscreen) {
                    Win32ToggleFullscreen();
                }
                
                Win32FillSoundBuffer(GlobalPlatform.SampleCountToOutput, GlobalPlatform.SampleOut, &SoundOutput);
                
                LARGE_INTEGER EndTime;
                QueryPerformanceCounter(&EndTime);
                
                real64 TargetSecondsPerFrame = (1 / 60.0f);
                int64 ElapsedTime = EndTime.QuadPart - StartTime.QuadPart;
                int64 TargetCounts = (int64)(TargetSecondsPerFrame * CountsPerSecond.QuadPart);
                int64 CountsToWait = TargetCounts - ElapsedTime;
                
                LARGE_INTEGER StartWait;
                LARGE_INTEGER EndWait;
                
                QueryPerformanceCounter(&StartWait);
                
                while (CountsToWait > 0) {
                    if (SleepIsGranular) {
                        DWORD MillisecondsToSleep =
                        (DWORD)(1000.0 * ((real64)(CountsToWait) / CountsPerSecond.QuadPart));
                        
                        if (MillisecondsToSleep > 0) {
                            Sleep(MillisecondsToSleep);
                        }
                    }
                    
                    QueryPerformanceCounter(&EndWait);
                    CountsToWait -= EndWait.QuadPart - StartWait.QuadPart;
                    StartWait = EndWait;
                }
            }
            
            GameCode.Quit();
        }
    }
    
    return 0;
}
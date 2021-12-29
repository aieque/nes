// The fonts should be moved out from the renderer but that will probably come later.

// @TODO Use the "Improved 3D API" in stb_truetype

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

struct font {
    uint32 TextureID;
    uint32 TextureWidth;
    uint32 TextureHeight;
    
    float Size;
    
    stbtt_packedchar CharacterData[255];
};

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// The texture loading should be moved elsewhere.
struct texture {
    int Width;
    int Height;
    int BitsPerPixel;
    
    uint32 ID;
};

#define RENDERER_OPENGL_FRAMEBUFFER_COLOR0 (1 << 0)
#define RENDERER_OPENGL_FRAMEBUFFER_COLOR1 (1 << 1)
#define RENDERER_OPENGL_FRAMEBUFFER_COLOR2 (1 << 2)
#define RENDERER_OPENGL_FRAMEBUFFER_COLOR3 (1 << 3)
#define RENDERER_OPENGL_FRAMEBUFFER_DEPTH  (1 << 4)

struct renderer_opengl_framebuffer {
    int32 Flags;
    
    uint32 ID;
    uint32 ColorTextures[4];
    uint32 DepthTexture;
    
    uint32 Width;
    uint32 Height;
};

enum renderer_request_type {
    RendererRequestType_null,
    RendererRequestType_rect,
    RendererRequestType_rounded_rect,
    RendererRequestType_line,
    RendererRequestType_text,
    RendererRequestType_texture,
    RendererRequestType_clip_rect,
    RendererRequestType_blur_rectangle,
    RendererRequestType_projection_matrix,
    RendererRequestType_MAX,
};

#define RENDERER_FLAG_ADDITIVE_BLEND (1 << 0)

struct renderer_request {
    renderer_request_type Type;
    int32 Flags;
    uint32 DataSize;
    
    uint32 TextureID;
    union {
        v4 BlurRect;
        v4 ClipRect;
        m4 Matrix;
    };
    // Current Offset + Size Of Header + DataSize = Header of next entry
};

struct renderer_request_line {
    v2 P1;
    v2 P2;
    v4 Color;
};

struct renderer_request_rect {
    v2 Position;
    v2 Size;
    v4 Color;
};

struct renderer_request_rounded_rect {
    v2 Position;
    v2 Size;
    v4 Color;
    real32 Radius;
    real32 Angle;
};

struct renderer_request_texture {
    v2 Position;
    v2 Size;
    v4 Tint;
    v4 TextureClip;
};

struct renderer_request_text {
    v2 Position;
    v2 Size;
    v4 Tint;
    v4 TextureClip;
};

enum renderer_shader_type {
    RendererShader_line,
    RendererShader_rect,
    RendererShader_rounded_rect,
    RendererShader_text,
    RendererShader_texture,
    RendererShader_gaussian_blur,
    RendererShader_Max
};

#define MAX_REQUEST_DATA_SIZE Megabytes(1)

struct renderer {
    real32 RenderWidth;
    real32 RenderHeight;
    
    renderer_request *ActiveRequest;
    
    struct {
        void *Base;
        uint64 Size;
        uint64 Used;
    } PushBuffer;
    
    uint32 GeneralVertexArray;
    
    uint32 LineVertexBuffer;
    uint32 LineVertexArray;
    
    uint32 RectVertexBuffer;
    uint32 RectVertexArray;
    
    uint32 RoundedRectVertexBuffer;
    uint32 RoundedRectVertexArray;
    
    uint32 TextureVertexBuffer;
    uint32 TextureVertexArray;
    
    renderer_opengl_framebuffer *ActiveFramebuffer;
    renderer_opengl_framebuffer MainFramebuffer;
    renderer_opengl_framebuffer ScratchFramebuffer;
    
    v4 ClipRectStack[32];
    int ClipRectStackCount;
    v4 ActiveClipRect;
    
    uint32 Shaders[RendererShader_Max];
};
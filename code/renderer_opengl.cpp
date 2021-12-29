internal font *
LoadFont(memory_arena *Arena, char *Filepath, real32 FontSize, int OversamplingX, int OversamplingY) {
    font *Result = (font *)MemoryArenaPushZero(Arena, sizeof(font));
    
    Result->Size = FontSize;
    
    Result->TextureWidth = 512;
    Result->TextureHeight = 512;
    uint32 TextureSize = Result->TextureWidth * Result->TextureHeight;
    uint8 *TextureData = (uint8 *)MemoryArenaPush(Arena, TextureSize);
    
    file_read_result FileResult = Platform->DEBUGReadFile(Filepath);
    
    stbtt_pack_context PackContext = {};
    stbtt_PackBegin(&PackContext, TextureData, Result->TextureWidth, Result->TextureHeight, 0, 1, 0);
    stbtt_PackSetOversampling(&PackContext, OversamplingX, OversamplingY);
    stbtt_PackFontRange(&PackContext, (uint8 *)FileResult.Contents, 0, STBTT_POINT_SIZE(FontSize), 0, 255, Result->CharacterData);
    stbtt_PackEnd(&PackContext);
    
    Platform->DEBUGFreeFileData(&FileResult);
    
    glGenTextures(1, &Result->TextureID);
    glBindTexture(GL_TEXTURE_2D, Result->TextureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, Result->TextureWidth, Result->TextureHeight, 0, GL_RED, GL_UNSIGNED_BYTE, TextureData);
    
    // Remove the TextureData from the arena.
    MemoryArenaPop(Arena, TextureSize);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    return Result;
}

internal real32
GetTextWidth(font *Font, char *Text) {
    real32 Width = 0.0f;
    
    char CurrentChar;
    while ((CurrentChar = *Text++)) {
        stbtt_packedchar *CharData = Font->CharacterData + (uint8)CurrentChar;
        Width += CharData->xadvance;
    }
    
    return Width;
}

internal real32
GetTextHeight(font *Font) {
    // @TODO Add newline support
    return Font->Size;
}

internal texture
LoadTexture(char *Filepath) {
    texture Result = {};
    
    file_read_result ReadResult = Platform->DEBUGReadFile(Filepath);
    if (ReadResult.Contents) {
        uint8 *Data = stbi_load_from_memory((uint8 *)ReadResult.Contents, ReadResult.ContentsSize,
                                            &Result.Width, &Result.Height, &Result.BitsPerPixel, 0);
        
        glGenTextures(1, &Result.ID);
        glBindTexture(GL_TEXTURE_2D, Result.ID);
        
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Result.Width, Result.Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, Data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    
    return Result;
}

internal renderer_opengl_framebuffer
Renderer_CreateOpenGLFramebuffer(uint32 Width, uint32 Height, int32 Flags) {
    renderer_opengl_framebuffer Result = {};
    Result.Flags = Flags;
    Result.Width = Width;
    Result.Height = Height;
    
    glGenFramebuffers(1, &Result.ID);
    glBindFramebuffer(GL_FRAMEBUFFER, Result.ID);
    
    uint32 Colors[4] = {};
    uint32 ColorCount = 0;
    
    for (int ColorIndex = 0; ColorIndex < 4; ++ColorIndex) {
        if (Flags & (1 << ColorIndex)) {
            glGenTextures(1, Result.ColorTextures + ColorIndex);
            glBindTexture(GL_TEXTURE_2D, Result.ColorTextures[ColorIndex]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Width, Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                   Result.ColorTextures[ColorIndex], 0);
            Colors[ColorCount++] = GL_COLOR_ATTACHMENT0 + ColorIndex;
        }
    }
    
    if (Flags & RENDERER_OPENGL_FRAMEBUFFER_DEPTH) {
        glGenTextures(1, &Result.DepthTexture);
        glBindTexture(GL_TEXTURE_2D, Result.DepthTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
                     Width, Height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                               Result.DepthTexture, 0);
    }
    
    glDrawBuffers(ColorCount, Colors);
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    return Result;
}

internal void
Renderer_DeleteOpenGLFramebuffer(renderer_opengl_framebuffer *Framebuffer) {
    glDeleteFramebuffers(1, &Framebuffer->ID);
    for (int TextureIndex = 0; TextureIndex < 4; ++TextureIndex) {
        if (Framebuffer->ColorTextures[TextureIndex]) {
            glDeleteTextures(1, &Framebuffer->ColorTextures[TextureIndex]);
        }
    }
    
    glDeleteTextures(1, &Framebuffer->DepthTexture);
    
    Framebuffer->ID = 0;
}

internal void
Renderer_ResizeOpenGLFramebuffer(renderer_opengl_framebuffer *Framebuffer,
                                 uint32 Width, uint32 Height, int32 Flags) {
    if (Width != Framebuffer->Width || Height != Framebuffer->Height) {
        Renderer_DeleteOpenGLFramebuffer(Framebuffer);
        *Framebuffer = Renderer_CreateOpenGLFramebuffer(Width, Height, Flags);
    }
}

internal void
Renderer_BindOpenGLFramebuffer(renderer *Renderer, renderer_opengl_framebuffer *Framebuffer) {
    if (Framebuffer) {
        glViewport(0, 0, Framebuffer->Width, Framebuffer->Height);
        glBindFramebuffer(GL_FRAMEBUFFER, Framebuffer->ID);
    } else {
        glViewport(0, 0, Renderer->RenderWidth, Renderer->RenderHeight);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    
    Renderer->ActiveFramebuffer = Framebuffer;
}

internal void
Renderer_ClearOpenGLFramebuffer(renderer *Renderer, renderer_opengl_framebuffer *Framebuffer) {
    renderer_opengl_framebuffer *PreviousFramebuffer = Renderer->ActiveFramebuffer;
    
    Renderer_BindOpenGLFramebuffer(Renderer, Framebuffer);
    
    if (Framebuffer->DepthTexture) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    } else {
        glClear(GL_COLOR_BUFFER_BIT);
    }
    
    Renderer_BindOpenGLFramebuffer(Renderer, PreviousFramebuffer);
}

internal void
Renderer_RenderBlurredOpenGLFramebuffer(renderer *Renderer, uint32 TextureID, real32 Width, real32 Height,
                                        bool IsVertical, real32 StandardDeviation, int Radius, v4 Clip) {
    uint32 Shader = Renderer->Shaders[RendererShader_gaussian_blur];
    
    glBindVertexArray(Renderer->GeneralVertexArray);
    
    glUseProgram(Shader);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, TextureID);
    
    glUniform2f(glGetUniformLocation(Shader, "TextureSize"), Width, Height);
    glUniform1i(glGetUniformLocation(Shader, "Radius"), Radius);
    glUniform1i(glGetUniformLocation(Shader, "IsVertical"), IsVertical);
    glUniform4f(glGetUniformLocation(Shader, "Clip"),
                Clip.X, Renderer->RenderHeight - Clip.Y - Clip.W, Clip.Z, Clip.W);
    
    int KernelSize = 128;
    int KernelMidpoint = KernelSize / 2;
    int KernelLowerBound = KernelMidpoint - Radius;
    int KernelUpperBound = KernelMidpoint + Radius;
    
    real32 GLSLBuffer[4] = {};
    int GLSLBufferCounter = 0;
    
    for (int KernelIndex = KernelLowerBound; KernelIndex <= KernelUpperBound; ++KernelIndex) {
        real32 X = KernelMidpoint - KernelIndex;
        
        GLSLBuffer[GLSLBufferCounter++] = (OneOverSquareRootOfTwoPiF / StandardDeviation) *
            powf(EulersNumberf, -(X * X) / (2 * StandardDeviation * StandardDeviation));
        
        int GLSLIndex = KernelIndex / 4;
        
        if (GLSLBufferCounter >= 4) {
            GLSLBufferCounter = 0;
            
            char UniformNameBuffer[40];
            sprintf(UniformNameBuffer, "Kernel[%i]", GLSLIndex);
            
            glUniform4fv(glGetUniformLocation(Shader, UniformNameBuffer), 1, GLSLBuffer);
        }
    }
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

internal void
Renderer_FinishActiveRequest(renderer *Renderer) {
    uint32 Pitch = 0;
    if (Renderer->ActiveRequest->Type != RendererRequestType_null) {
        Pitch = Renderer->ActiveRequest->DataSize + sizeof(renderer_request);
    }
    
    uint8 *NextRequest = (uint8 *)Renderer->ActiveRequest + Pitch;
    
    Renderer->ActiveRequest = (renderer_request *)NextRequest;
    Renderer->ActiveRequest->Type = RendererRequestType_null;
    Renderer->ActiveRequest->DataSize = 0;
}

internal uint32
Renderer_CreateOpenGLShader(char *Filepath, uint32 Type) {
    file_read_result Result = Platform->DEBUGReadFile(Filepath);
    
    uint32 Shader = glCreateShader(Type);
    
    glShaderSource(Shader, 1, &Result.Contents, 0);
    
    glCompileShader(Shader);
    
    int CompilationSuccess;
    glGetShaderiv(Shader, GL_COMPILE_STATUS, &CompilationSuccess);
    if (CompilationSuccess != GL_TRUE) {
        // Something went wrong.
        char Buffer[512];
        glGetShaderInfoLog(Shader, 512, NULL, Buffer);
        OutputDebugStringA(Buffer);
    }
    
    Platform->DEBUGFreeFileData(&Result);
    
    return Shader;
}

internal uint32
Renderer_CreateOpenGLShaderProgram(char *Name) {
    char NameBuffer[128];
    
    sprintf(NameBuffer, "assets/shaders/%s_vertex.glsl", Name);
    uint32 VertexShader = Renderer_CreateOpenGLShader(NameBuffer, GL_VERTEX_SHADER);
    
    sprintf(NameBuffer, "assets/shaders/%s_fragment.glsl", Name);
    uint32 FragmentShader = Renderer_CreateOpenGLShader(NameBuffer, GL_FRAGMENT_SHADER);
    
    uint32 Program = glCreateProgram();
    glAttachShader(Program, VertexShader);
    glAttachShader(Program, FragmentShader);
    
    glLinkProgram(Program);
    
    int LinkSuccess;
    glGetProgramiv(Program, GL_LINK_STATUS, &LinkSuccess);
    if (!LinkSuccess) {
        char Buffer[512];
        glGetProgramInfoLog(Program, 512, NULL, Buffer);
        OutputDebugStringA(Buffer);
    }
    // @TODO Check status
    
    return Program;
}

internal void
SetVertexAttribute(int Index, int ElementSize, uint32 Type, uint32 DataSize, uint64 Offset) {
    glVertexAttribPointer(Index, ElementSize, Type, GL_FALSE, DataSize, (void *)Offset);
    glVertexAttribDivisor(Index, 1);
    glEnableVertexAttribArray(Index);
}

internal void
Renderer_Init(renderer *Renderer, memory_arena *Arena, uint32 PushBufferSize) {
    LoadOpenGLProcedures();
    
    glGenVertexArrays(1, &Renderer->GeneralVertexArray);
    
    // Setup line vertex arrays & buffers
    glGenBuffers(1, &Renderer->LineVertexBuffer);
    glGenVertexArrays(1, &Renderer->LineVertexArray);
    
    glBindVertexArray(Renderer->LineVertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, Renderer->LineVertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, MAX_REQUEST_DATA_SIZE, 0, GL_DYNAMIC_DRAW);
    
    SetVertexAttribute(0, 2, GL_FLOAT, sizeof(renderer_request_line), offsetof(renderer_request_line, P1));
    SetVertexAttribute(1, 2, GL_FLOAT, sizeof(renderer_request_line), offsetof(renderer_request_line, P2));
    SetVertexAttribute(2, 4, GL_FLOAT, sizeof(renderer_request_line), offsetof(renderer_request_line, Color));
    
    // Setup rect vertex arrays & buffers
    glGenBuffers(1, &Renderer->RectVertexBuffer);
    glGenVertexArrays(1, &Renderer->RectVertexArray);
    
    glBindVertexArray(Renderer->RectVertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, Renderer->RectVertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, MAX_REQUEST_DATA_SIZE, 0, GL_DYNAMIC_DRAW);
    
    SetVertexAttribute(0, 2, GL_FLOAT, sizeof(renderer_request_rect), offsetof(renderer_request_rect, Position));
    SetVertexAttribute(1, 2, GL_FLOAT, sizeof(renderer_request_rect), offsetof(renderer_request_rect, Size));
    SetVertexAttribute(2, 4, GL_FLOAT, sizeof(renderer_request_rect), offsetof(renderer_request_rect, Color));
    
    // Setup rounded rect vertex arrays & buffers
    glGenBuffers(1, &Renderer->RoundedRectVertexBuffer);
    glGenVertexArrays(1, &Renderer->RoundedRectVertexArray);
    
    glBindVertexArray(Renderer->RoundedRectVertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, Renderer->RoundedRectVertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, MAX_REQUEST_DATA_SIZE, 0, GL_DYNAMIC_DRAW);
    
    SetVertexAttribute(0, 2, GL_FLOAT, sizeof(renderer_request_rounded_rect), offsetof(renderer_request_rounded_rect, Position));
    SetVertexAttribute(1, 2, GL_FLOAT, sizeof(renderer_request_rounded_rect), offsetof(renderer_request_rounded_rect, Size));
    SetVertexAttribute(2, 4, GL_FLOAT, sizeof(renderer_request_rounded_rect), offsetof(renderer_request_rounded_rect, Color));
    SetVertexAttribute(3, 1, GL_FLOAT, sizeof(renderer_request_rounded_rect), offsetof(renderer_request_rounded_rect, Radius));
    SetVertexAttribute(4, 1, GL_FLOAT, sizeof(renderer_request_rounded_rect), offsetof(renderer_request_rounded_rect, Angle));
    
    // Setup texture vertex arrays & buffers
    glGenBuffers(1, &Renderer->TextureVertexBuffer);
    glGenVertexArrays(1, &Renderer->TextureVertexArray);
    
    glBindVertexArray(Renderer->TextureVertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, Renderer->TextureVertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, MAX_REQUEST_DATA_SIZE, 0, GL_DYNAMIC_DRAW);
    
    SetVertexAttribute(0, 2, GL_FLOAT, sizeof(renderer_request_texture), offsetof(renderer_request_texture, Position));
    SetVertexAttribute(1, 2, GL_FLOAT, sizeof(renderer_request_texture), offsetof(renderer_request_texture, Size));
    SetVertexAttribute(2, 4, GL_FLOAT, sizeof(renderer_request_texture), offsetof(renderer_request_texture, Tint));
    SetVertexAttribute(3, 4, GL_FLOAT,sizeof(renderer_request_texture),offsetof(renderer_request_texture,TextureClip));
    
    Renderer->Shaders[RendererShader_line] = Renderer_CreateOpenGLShaderProgram("line");
    Renderer->Shaders[RendererShader_rect] = Renderer_CreateOpenGLShaderProgram("rect");
    Renderer->Shaders[RendererShader_rounded_rect] = Renderer_CreateOpenGLShaderProgram("rounded_rect");
    Renderer->Shaders[RendererShader_text] = Renderer_CreateOpenGLShaderProgram("text");
    Renderer->Shaders[RendererShader_texture] = Renderer_CreateOpenGLShaderProgram("texture");
    Renderer->Shaders[RendererShader_gaussian_blur] = Renderer_CreateOpenGLShaderProgram("gaussian_blur");
    
    Renderer->PushBuffer.Size = PushBufferSize;
    Renderer->PushBuffer.Base = MemoryArenaPushZero(Arena, PushBufferSize);
    Renderer->PushBuffer.Used = 0;
    
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    
    glEnable(GL_SCISSOR_TEST);
}

internal void
Renderer_BeginFrame(renderer *Renderer, real32 RenderWidth, real32 RenderHeight) {
    Renderer->RenderWidth = RenderWidth;
    Renderer->RenderHeight = RenderHeight;
    
    Renderer->PushBuffer.Used = 0;
    
    Renderer->ClipRectStackCount = 0;
    
    Renderer->ActiveRequest = (renderer_request *)Renderer->PushBuffer.Base;
    Renderer->ActiveRequest->Type = RendererRequestType_null;
    Renderer->ActiveRequest->DataSize = 0;
    
    Renderer->ActiveClipRect = V4(0, 0, Renderer->RenderWidth, Renderer->RenderHeight);
    
    Renderer_ResizeOpenGLFramebuffer(&Renderer->MainFramebuffer,
                                     Renderer->RenderWidth, Renderer->RenderHeight,
                                     RENDERER_OPENGL_FRAMEBUFFER_COLOR0 |
                                     RENDERER_OPENGL_FRAMEBUFFER_DEPTH);
    Renderer_ResizeOpenGLFramebuffer(&Renderer->ScratchFramebuffer,
                                     Renderer->RenderWidth, Renderer->RenderHeight,
                                     RENDERER_OPENGL_FRAMEBUFFER_COLOR0);
}

internal void
Renderer_EndFrame(renderer *Renderer) {
    Renderer_FinishActiveRequest(Renderer);
    
    glClearColor(0.1f, 0.1f, 0.13f, 1);
    glViewport(0, 0, (int)Renderer->RenderWidth, (int)Renderer->RenderHeight);
    glScissor(0, 0, (int)Renderer->RenderWidth, (int)Renderer->RenderHeight);
    
    Renderer_ClearOpenGLFramebuffer(Renderer, &Renderer->MainFramebuffer);
    Renderer_ClearOpenGLFramebuffer(Renderer, &Renderer->ScratchFramebuffer);
    
    Renderer_BindOpenGLFramebuffer(Renderer, &Renderer->MainFramebuffer);
    
    m4 ProjectionMatrix = M4Orthographic(0, Renderer->RenderWidth, 0, Renderer->RenderHeight, -100, 100);
    
    v4 ActiveClipRect = V4(0, 0, Renderer->RenderWidth, Renderer->RenderHeight);
    
    renderer_request *Request = (renderer_request *)Renderer->PushBuffer.Base;
    while (Request->Type != RendererRequestType_null) {
        if (Request->Flags & RENDERER_FLAG_ADDITIVE_BLEND) {
            glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ONE);
        }
        
        switch (Request->Type) {
            case RendererRequestType_line: {
                uint32 Shader = Renderer->Shaders[RendererShader_line];
                
                glUseProgram(Shader);
                
                glUniformMatrix4fv(glGetUniformLocation(Shader, "UniformProjectionMatrix"),
                                   1, GL_FALSE, &ProjectionMatrix.Elements[0][0]);
                
                glBindVertexArray(Renderer->LineVertexArray);
                glBindBuffer(GL_ARRAY_BUFFER, Renderer->LineVertexBuffer);
                glBufferSubData(GL_ARRAY_BUFFER, 0, Request->DataSize,
                                (uint8 *)Request + sizeof(renderer_request));
                
                glDrawArraysInstanced(GL_LINES, 0, 2, (Request->DataSize / sizeof(renderer_request_line)));
            } break;
            
            case RendererRequestType_rect: {
                uint32 Shader = Renderer->Shaders[RendererShader_rect];
                
                glUseProgram(Shader);
                
                glUniformMatrix4fv(glGetUniformLocation(Shader, "UniformProjectionMatrix"),
                                   1, GL_FALSE, &ProjectionMatrix.Elements[0][0]);
                
                glBindVertexArray(Renderer->RectVertexArray);
                glBindBuffer(GL_ARRAY_BUFFER, Renderer->RectVertexBuffer);
                glBufferSubData(GL_ARRAY_BUFFER, 0, Request->DataSize,
                                (uint8 *)Request + sizeof(renderer_request));
                
                glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, (Request->DataSize / sizeof(renderer_request_rect)));
            } break;
            
            case RendererRequestType_rounded_rect: {
                uint32 Shader = Renderer->Shaders[RendererShader_rounded_rect];
                
                glUseProgram(Shader);
                
                glUniformMatrix4fv(glGetUniformLocation(Shader, "UniformProjectionMatrix"),
                                   1, GL_FALSE, &ProjectionMatrix.Elements[0][0]);
                
                glBindVertexArray(Renderer->RoundedRectVertexArray);
                glBindBuffer(GL_ARRAY_BUFFER, Renderer->RoundedRectVertexBuffer);
                glBufferSubData(GL_ARRAY_BUFFER, 0, Request->DataSize,
                                (uint8 *)Request + sizeof(renderer_request));
                
                glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, (Request->DataSize / sizeof(renderer_request_rounded_rect)));
            } break;
            
            case RendererRequestType_text: {
                uint32 Shader = Renderer->Shaders[RendererShader_text];
                
                glUseProgram(Shader);
                
                glUniformMatrix4fv(glGetUniformLocation(Shader, "UniformProjectionMatrix"),
                                   1, GL_FALSE, &ProjectionMatrix.Elements[0][0]);
                
                glBindVertexArray(Renderer->TextureVertexArray);
                glBindBuffer(GL_ARRAY_BUFFER, Renderer->TextureVertexBuffer);
                glBufferSubData(GL_ARRAY_BUFFER, 0, Request->DataSize,
                                (uint8 *)Request + sizeof(renderer_request));
                
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, Request->TextureID);
                
                glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, (Request->DataSize / sizeof(renderer_request_text)));
            } break;
            
            case RendererRequestType_texture: {
                uint32 Shader = Renderer->Shaders[RendererShader_texture];
                
                glUseProgram(Shader);
                
                glUniformMatrix4fv(glGetUniformLocation(Shader, "UniformProjectionMatrix"),
                                   1, GL_FALSE, &ProjectionMatrix.Elements[0][0]);
                
                glBindVertexArray(Renderer->TextureVertexArray);
                glBindBuffer(GL_ARRAY_BUFFER, Renderer->TextureVertexBuffer);
                glBufferSubData(GL_ARRAY_BUFFER, 0, Request->DataSize,
                                (uint8 *)Request + sizeof(renderer_request));
                
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, Request->TextureID);
                
                glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, (Request->DataSize / sizeof(renderer_request_texture)));
            } break;
            
            case RendererRequestType_clip_rect: {
                ActiveClipRect = Request->ClipRect;
                glScissor(ActiveClipRect.X, Renderer->RenderHeight - ActiveClipRect.Y - ActiveClipRect.W,
                          ActiveClipRect.Z, ActiveClipRect.W);
            } break;
            
            case RendererRequestType_blur_rectangle: {
                Renderer_BindOpenGLFramebuffer(Renderer, &Renderer->ScratchFramebuffer);
                Renderer_RenderBlurredOpenGLFramebuffer(Renderer, Renderer->MainFramebuffer.ColorTextures[0],
                                                        Renderer->RenderWidth, Renderer->RenderHeight, true,
                                                        3.0f, 12, Request->BlurRect);
                Renderer_BindOpenGLFramebuffer(Renderer, &Renderer->MainFramebuffer);
                Renderer_RenderBlurredOpenGLFramebuffer(Renderer, Renderer->ScratchFramebuffer.ColorTextures[0],
                                                        Renderer->RenderWidth, Renderer->RenderHeight, false,
                                                        3.0f, 12, Request->BlurRect);
            } break;
            
            case RendererRequestType_projection_matrix: {
                ProjectionMatrix = Request->Matrix;
            } break;
        }
        
        if (Request->Flags & RENDERER_FLAG_ADDITIVE_BLEND) {
            glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        }
        
        Request = (renderer_request *)((uint8 *)Request + sizeof(renderer_request) + Request->DataSize);
    }
    
    Renderer_BindOpenGLFramebuffer(Renderer, 0);
    
    glBindFramebuffer(GL_READ_FRAMEBUFFER, Renderer->MainFramebuffer.ID);
    glBlitFramebuffer(0, 0,
                      Renderer->MainFramebuffer.Width, Renderer->MainFramebuffer.Height,
                      0, 0,
                      Renderer->RenderWidth, Renderer->RenderHeight,
                      GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

internal void
Renderer_PushLine(renderer *Renderer, v2 P1, v2 P2, v4 Color, int32 Flags) {
    if (Renderer->ActiveRequest->Type != RendererRequestType_line ||
        Renderer->ActiveRequest->Flags != Flags) {
        Renderer_FinishActiveRequest(Renderer);
        
        Renderer->ActiveRequest->Type = RendererRequestType_line;
        Renderer->ActiveRequest->Flags = Flags;
    }
    
    renderer_request_line *Request = (renderer_request_line *)((uint8 *)Renderer->ActiveRequest + sizeof(renderer_request) + Renderer->ActiveRequest->DataSize);
    Request->P1 = P1;
    Request->P2 = P2;
    Request->Color = Color;
    
    Renderer->ActiveRequest->DataSize += sizeof(renderer_request_line);
}

internal void
Renderer_PushHollowRect(renderer *Renderer, v2 Position, v2 Size, v4 Color, int32 Flags) {
    v2 P1 = Position;
    v2 P2 = Position + V2(Size.X, 0);
    v2 P3 = Position + Size;
    v2 P4 = Position + V2(0, Size.Y);
    
    Renderer_PushLine(Renderer, P1, P2, Color, Flags);
    Renderer_PushLine(Renderer, P2, P3, Color, Flags);
    Renderer_PushLine(Renderer, P4, P3, Color, Flags);
    Renderer_PushLine(Renderer, P1, P4, Color, Flags);
}

internal void
Renderer_PushRect(renderer *Renderer, v2 Position, v2 Size, v4 Color, int32 Flags) {
    if (Renderer->ActiveRequest->Type != RendererRequestType_rect ||
        Renderer->ActiveRequest->Flags != Flags) {
        Renderer_FinishActiveRequest(Renderer);
        
        Renderer->ActiveRequest->Type = RendererRequestType_rect;
        Renderer->ActiveRequest->Flags = Flags;
    }
    
    renderer_request_rect *Request = (renderer_request_rect *)((uint8 *)Renderer->ActiveRequest + sizeof(renderer_request) + Renderer->ActiveRequest->DataSize);
    Request->Position = Position;
    Request->Size = Size;
    Request->Color = Color;
    
    Renderer->ActiveRequest->DataSize += sizeof(renderer_request_rect);
}

internal void
Renderer_PushRoundedRect(renderer *Renderer, v2 Position, v2 Size, v4 Color, real32 Radius, real32 Angle = 0, int32 Flags = 0) {
    if (Renderer->ActiveRequest->Type != RendererRequestType_rounded_rect ||
        Renderer->ActiveRequest->Flags != Flags) {
        Renderer_FinishActiveRequest(Renderer);
        
        Renderer->ActiveRequest->Type = RendererRequestType_rounded_rect;
        Renderer->ActiveRequest->Flags = Flags;
    }
    
    renderer_request_rounded_rect *Request = (renderer_request_rounded_rect *)((uint8 *)Renderer->ActiveRequest + sizeof(renderer_request) + Renderer->ActiveRequest->DataSize);
    Request->Position = Position;
    Request->Size = Size;
    Request->Color = Color;
    Request->Radius = Radius;
    Request->Angle = Angle;
    
    Renderer->ActiveRequest->DataSize += sizeof(renderer_request_rounded_rect);
}

internal void
Renderer_PushRoundedLine(renderer *Renderer, v2 P1, v2 P2, real32 Radius, v4 Color, int32 Flags) {
    v2 Distance = P2 - P1;
    real32 Angle = atan2f(Distance.Y, Distance.X);
    
    v2 Position = P1 - V2(Radius * cosf(Angle) - Radius * sinf(Angle), Radius * cosf(Angle) + Radius * sinf(Angle));
    v2 Size = V2(V2Length(Distance) + Radius * 2, Radius * 2);
    
    Renderer_PushRoundedRect(Renderer, Position, Size, Color, Radius, Angle, 0);
}

internal void
Renderer_PushTexture(renderer *Renderer, texture *Texture, v2 Position, v2 Size, v4 Tint = V4(1, 1, 1, 1),
                     v4 TextureClip = V4(0, 0, 1, 1), int32 Flags = 0) {
    if (Renderer->ActiveRequest->Type != RendererRequestType_texture ||
        Renderer->ActiveRequest->Flags != Flags ||
        Renderer->ActiveRequest->TextureID != Texture->ID) {
        Renderer_FinishActiveRequest(Renderer);
        
        Renderer->ActiveRequest->Type = RendererRequestType_texture;
        Renderer->ActiveRequest->Flags = Flags;
        Renderer->ActiveRequest->TextureID = Texture->ID;
    }
    
    renderer_request_texture *Request = (renderer_request_texture *)((uint8 *)Renderer->ActiveRequest + sizeof(renderer_request) + Renderer->ActiveRequest->DataSize);
    
    Request->Position = Position;
    Request->Size = Size;
    Request->Tint = Tint;
    Request->TextureClip = TextureClip;
    
    Renderer->ActiveRequest->DataSize += sizeof(renderer_request_texture);
}

// @TODO Rewrite this function.
internal void
Renderer_PushTextChar(renderer *Renderer, v2 Position, v2 Size, v4 Tint,
                      v4 TextureClip, uint32 TextureID, int32 Flags) {
    if (Renderer->ActiveRequest->Type != RendererRequestType_text ||
        Renderer->ActiveRequest->Flags != Flags ||
        Renderer->ActiveRequest->TextureID != TextureID) {
        Renderer_FinishActiveRequest(Renderer);
        
        Renderer->ActiveRequest->Type = RendererRequestType_text;
        Renderer->ActiveRequest->Flags = Flags;
        Renderer->ActiveRequest->TextureID = TextureID;
    }
    
    renderer_request_text *Request = (renderer_request_text *)((uint8 *)Renderer->ActiveRequest + sizeof(renderer_request) + Renderer->ActiveRequest->DataSize);
    
    Request->Position = Position;
    Request->Size = Size;
    Request->Tint = Tint;
    Request->TextureClip = TextureClip;
    
    Renderer->ActiveRequest->DataSize += sizeof(renderer_request_text);
}

internal void
Renderer_PushText(renderer *Renderer, char *Text, font *Font, v2 Position, v4 Color, int32 Flags) {
    char CurrentChar;
    while ((CurrentChar = *Text++)) {
        stbtt_aligned_quad Quad;
        stbtt_GetPackedQuad(Font->CharacterData, Font->TextureWidth, Font->TextureHeight, (int)CurrentChar,
                            &Position.X, &Position.Y, &Quad, 0);
        
        v2 Size = V2(Quad.x1 - Quad.x0, Quad.y1 - Quad.y0);
        v2 RealPosition = V2(Quad.x0, Quad.y0 + Font->Size - 4);
        
        Renderer_PushTextChar(Renderer, RealPosition, Size, Color,
                              V4(Quad.s0, Quad.t0, Quad.s1, Quad.t1), Font->TextureID, Flags);
    }
}

internal void
Renderer_PushBlurRectangle(renderer *Renderer, v2 Position, v2 Size) {
    Renderer_FinishActiveRequest(Renderer);
    
    Renderer->ActiveRequest->Type = RendererRequestType_blur_rectangle;
    Renderer->ActiveRequest->BlurRect = V4(Position, Size);
}

internal void
Renderer_PushClipRect(renderer *Renderer, v4 Rect) {
    Renderer_FinishActiveRequest(Renderer);
    
    Renderer->ClipRectStack[Renderer->ClipRectStackCount++] = Renderer->ActiveClipRect;
    Renderer->ActiveClipRect = Rect;
    
    Renderer->ActiveRequest->Type = RendererRequestType_clip_rect;
    Renderer->ActiveRequest->ClipRect = Rect;
}

internal void
Renderer_PushConstrainedClipRect(renderer *Renderer, v4 Rect) {
    v4 CurrentClipRect = MakeRect(Renderer->ActiveClipRect.Position,
                                  Renderer->ActiveClipRect.Size);
    v4 ClipRect = MakeRect(Rect.Position, Rect.Size);
    
    v4 ConstrainedClipRect = IntersectV4(CurrentClipRect, ClipRect);
    v4 ConstrainedClip = V4(ConstrainedClipRect.P0, ConstrainedClipRect.P1 - ConstrainedClipRect.P0);
    
    Renderer_PushClipRect(Renderer, ConstrainedClip);
}

internal void
Renderer_PopClipRect(renderer *Renderer) {
    Renderer_FinishActiveRequest(Renderer);
    
    Renderer->ActiveClipRect = Renderer->ClipRectStack[--Renderer->ClipRectStackCount];
    
    Renderer->ActiveRequest->Type = RendererRequestType_clip_rect;
    Renderer->ActiveRequest->ClipRect = Renderer->ActiveClipRect;
}

internal void
Renderer_PushProjectionMatrix(renderer *Renderer, m4 Matrix) {
    Renderer_FinishActiveRequest(Renderer);
    
    Renderer->ActiveRequest->Type = RendererRequestType_projection_matrix;
    Renderer->ActiveRequest->Matrix = Matrix;
}
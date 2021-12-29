// Shamelessly stolen from:
// https://github.com/ryanfleury/dungeoneer/blob/master/game/source/opengl_procedure_list.h

GLProc(USEPROGRAM, UseProgram)

GLProc(ACTIVETEXTURE, ActiveTexture)
GLProc(GENERATEMIPMAP, GenerateMipmap)

GLProc(GENFRAMEBUFFERS, GenFramebuffers)
GLProc(DELETEFRAMEBUFFERS, DeleteFramebuffers)
GLProc(BINDFRAMEBUFFER, BindFramebuffer)
GLProc(FRAMEBUFFERTEXTURE, FramebufferTexture)
GLProc(FRAMEBUFFERTEXTURE2D, FramebufferTexture2D)
GLProc(BLITFRAMEBUFFER, BlitFramebuffer)

GLProc(GENBUFFERS, GenBuffers)
GLProc(DELETEBUFFERS, DeleteBuffers)
GLProc(BINDBUFFER, BindBuffer)
GLProc(DRAWBUFFERS, DrawBuffers)
GLProc(DRAWARRAYSINSTANCED, DrawArraysInstanced)

GLProc(GENVERTEXARRAYS, GenVertexArrays)
GLProc(DELETEVERTEXARRAYS, DeleteVertexArrays)
GLProc(BUFFERDATA, BufferData)
GLProc(BUFFERSUBDATA, BufferSubData)
GLProc(BINDVERTEXARRAY, BindVertexArray)
GLProc(ENABLEVERTEXATTRIBARRAY, EnableVertexAttribArray)
GLProc(DISABLEVERTEXATTRIBARRAY, DisableVertexAttribArray)
GLProc(VERTEXATTRIBPOINTER, VertexAttribPointer)
GLProc(VERTEXATTRIBIPOINTER, VertexAttribIPointer)
GLProc(VERTEXATTRIBDIVISOR, VertexAttribDivisor)

GLProc(GETUNIFORMLOCATION, GetUniformLocation)
GLProc(UNIFORMMATRIX4FV, UniformMatrix4fv)
GLProc(UNIFORM1I, Uniform1i)
GLProc(UNIFORM1F, Uniform1f)
GLProc(UNIFORM2F, Uniform2f)
GLProc(UNIFORM3F, Uniform3f)
GLProc(UNIFORM4F, Uniform4f)
GLProc(UNIFORM4FV, Uniform4fv)

GLProc(CREATESHADER, CreateShader)
GLProc(SHADERSOURCE, ShaderSource)
GLProc(COMPILESHADER, CompileShader)
GLProc(GETSHADERIV, GetShaderiv)
GLProc(ATTACHSHADER, AttachShader)
GLProc(GETSHADERINFOLOG, GetShaderInfoLog)
GLProc(CREATEPROGRAM, CreateProgram)
GLProc(BINDATTRIBLOCATION, BindAttribLocation)
GLProc(BINDFRAGDATALOCATION, BindFragDataLocation)
GLProc(DELETEPROGRAM, DeleteProgram)
GLProc(LINKPROGRAM, LinkProgram)
GLProc(DELETESHADER, DeleteShader)
GLProc(VALIDATEPROGRAM, ValidateProgram)
GLProc(GETPROGRAMIV, GetProgramiv)
GLProc(GETPROGRAMINFOLOG, GetProgramInfoLog)

GLProc(BLENDFUNCSEPARATE, BlendFuncSeparate)

#undef GLProc
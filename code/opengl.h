#include <GL/gl.h>
#include "wglext.h"
#include "glext.h"

#define GLProc(Type, Name) PFNGL##Type##PROC gl##Name = 0;
#include "opengl_procedures.h"

internal void
LoadOpenGLProcedures() {
#define GLProc(Type, Name) gl##Name = (PFNGL##Type##PROC)Platform->LoadOpenGLProcedure("gl" #Name);
#include "opengl_procedures.h"
}
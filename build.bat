@echo off

set MetadeskDir=%cd%\metadesk
set CodeDir=%cd%\code

set CompilerFlags=-nologo -FC -Gm- -GR- -EHa- -nologo -Zi -MT -I../ext -wd4477 -Od
set CommonLinkFlags=-opt:ref -incremental:no
set PlatformLinkFlags=user32.lib gdi32.lib winmm.lib opengl32.lib %CommonLinkFlags%

pushd build
echo --- Building DLL ---
cl %CompilerFlags% %CodeDir%\nes.cpp -LD -link %CommonLinkFlags% opengl32.lib

echo --- Compiling Platform Layer ---
cl %CompilerFlags% %CodeDir%\win32_nes.cpp -link %PlatformLinkFlags%

popd
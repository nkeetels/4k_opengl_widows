## Just a minimal OpenGL framework for Windows to mess around with.

### Build settings:

LIBS: _kernel32.lib opengl32.lib winmm.lib gdi32.lib user32.lib_

CRINKLER: _/COMPMODE:FAST /ORDERTRIES:5000 /TRUNCATEFLOATS:16 /HASHSIZE:200 /PRINT:LABELS /ENTRY:entrypoint /SUBSYSTEM:WINDOWS /RANGE:kernel32 /RANGE:opengl32 /RANGE:winmm /RANGE:gdi32 /RANGE:user32_

shader_minifier.exe _src\fsh.glsl src\csh.glsl -o src\shaders.h_


#include <windows.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <GL/gl.h>
#include <GL/glext.h>

#include "shaders.h"

#define XRES 1920/2
#define YRES 1200/2

#define WINDOW_STYLE WS_POPUP | WS_VISIBLE// | WS_MAXIMIZE

#define TEXT_TEXTURE_WIDTH 512 * 8
#define TEXT_TEXTURE_HEIGHT 128 * 8

#define SAMPLE_RATE 44100
#define NUM_CHANNELS 2
#define SONG_SECONDS 120 
#define TOTAL_SAMPLES SAMPLE_RATE * SONG_SECONDS
#define AUDIO_BUFFER_SIZE TOTAL_SAMPLES * NUM_CHANNELS * sizeof(float)

#define AUDIO_WORKGROUP_SIZE 64
#define NUM_WORKGROUPS_X 65535
#define NUM_WORKGROUPS_Y 2

#define glCreateShaderProgramv ((PFNGLCREATESHADERPROGRAMVPROC)wglGetProcAddress("glCreateShaderProgramv"))
#define glCreateProgram ((PFNGLCREATEPROGRAMPROC)wglGetProcAddress("glCreateProgram"))
#define glCreateShader ((FNGLCREATESHADERPROC)wglGetProcAddress("glCreateShader"))
#define glShaderSource ((PFNGLSHADERSOURCEPROC)wglGetProcAddress("glShaderSource"))
#define glCompileShader ((PFNGLCOMPILESHADERPROC)wglGetProcAddress("glCompileShader"))
#define glAttachShader ((PFNGLATTACHSHADERPROC)wglGetProcAddress("glAttachShader"))
#define glLinkProgram ((PFNGLLINKPROGRAMPROC)wglGetProcAddress("glLinkProgram"))
#define glUseProgram ((PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram"))
#define glGetUniformLocation ((PFNGLGETUNIFORMLOCATIONPROC)wglGetProcAddress("glGetUniformLocation"))
#define glUniform1i ((PFNGLUNIFORM1IPROC)wglGetProcAddress("glUniform1i"))
#define glUniform1f ((PFNGLUNIFORM1FPROC)wglGetProcAddress("glUniform1f"))
#define glDispatchCompute ((PFNGLDISPATCHCOMPUTEPROC)wglGetProcAddress("glDispatchCompute"))
#define glMemoryBarrier ((PFNGLMEMORYBARRIERPROC)wglGetProcAddress("glMemoryBarrier"))
#define glGenBuffers ((PFNGLGENBUFFERSPROC)wglGetProcAddress("glGenBuffers"))
#define glBindBuffer ((PFNGLBINDBUFFERPROC)wglGetProcAddress("glBindBuffer"))
#define glBufferData ((PFNGLBUFFERDATAPROC)wglGetProcAddress("glBufferData"))
#define glBindBufferBase ((PFNGLBINDBUFFERBASEPROC)wglGetProcAddress("glBindBufferBase"))
#define glMapBuffer ((PFNGLMAPBUFFERPROC)wglGetProcAddress("glMapBuffer"))
#define glUnmapBuffer ((PFNGLUNMAPBUFFERPROC)wglGetProcAddress("glUnmapBuffer"))
#define glActiveTexture ((PFNGLACTIVETEXTUREPROC)wglGetProcAddress("glActiveTexture"))

static float audio_buffer[AUDIO_BUFFER_SIZE];

HDC device_context;
GLuint text_texture, buffer;

WAVEFORMATEX wave_format = {WAVE_FORMAT_IEEE_FLOAT, NUM_CHANNELS, SAMPLE_RATE, SAMPLE_RATE * 8, 8, 32, 0};

static GLuint createTextTexture() {
    int bmi[11] = {40, 4096, 1024, 1 | (32 << 16)}; 
    HDC h = CreateCompatibleDC(0);
    void* bits;

    SelectObject(h, CreateDIBSection(h, (BITMAPINFO*)bmi, 0, &bits, 0, 0));
    SelectObject(h, CreateFontA(500, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, ANTIALIASED_QUALITY, DEFAULT_PITCH, "Georgia"));
    SetBkMode(h, 1);
    SetTextColor(h, 0xFFFFFF);
    TextOutA(h, 548, 262, "Le Solar Troupe", 15);

    glBindTexture(GL_TEXTURE_2D, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, 4, 4096, 1024, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, bits);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    return 1;
}

static void __forceinline create_window() {
    device_context = GetDC(CreateWindow((LPCSTR)0xC018, 0, WINDOW_STYLE, 0, 0, XRES, YRES, 0, 0, 0, 0));
    PIXELFORMATDESCRIPTOR pfd = {sizeof(pfd), 1, PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW/*, PFD_TYPE_RGBA, 32*/};
    SetPixelFormat(device_context, ChoosePixelFormat(device_context, &pfd), &pfd);
    wglMakeCurrent(device_context, wglCreateContext(device_context));
    ShowCursor(0);
}

static void __forceinline init_audio() {
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, AUDIO_BUFFER_SIZE, 0, GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffer);

    GLuint audio_shader = glCreateShaderProgramv(GL_COMPUTE_SHADER, 1, &csh_glsl);
    glUseProgram(audio_shader);
    glUniform1i(glGetUniformLocation(audio_shader, VAR_sampleRate), SAMPLE_RATE);
    glUniform1i(glGetUniformLocation(audio_shader, VAR_bufferSize), TOTAL_SAMPLES);

    glDispatchCompute(NUM_WORKGROUPS_X, NUM_WORKGROUPS_Y, 1);

    memcpy(audio_buffer, glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY), AUDIO_BUFFER_SIZE);

    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
}

void entrypoint() {
    create_window();
    init_audio();

    HWAVEOUT waveout;
    WAVEHDR wave_header = {(LPSTR)audio_buffer, AUDIO_BUFFER_SIZE};

    waveOutOpen(&waveout, WAVE_MAPPER, &wave_format, 0, 0, CALLBACK_NULL);
    waveOutPrepareHeader(waveout, &wave_header, sizeof(WAVEHDR));
    waveOutWrite(waveout, &wave_header, sizeof(WAVEHDR));

    GLuint fragment_shader = glCreateShaderProgramv(GL_FRAGMENT_SHADER, 1, &fsh_glsl);
    text_texture = createTextTexture();

    int start_time = (int)timeGetTime();

    do {
        PeekMessage(0, 0, 0, 0, PM_REMOVE);
        glUseProgram(fragment_shader);
        glActiveTexture(GL_TEXTURE0); 
        glBindTexture(GL_TEXTURE_2D, text_texture);
        glUniform1i(glGetUniformLocation(fragment_shader, VAR_fontTexture), 0); 
        glUniform1i(glGetUniformLocation(fragment_shader, VAR__t), timeGetTime() - start_time);
        glRecti(1, 1, -1, -1);
        SwapBuffers(device_context);
    } while (!GetAsyncKeyState(VK_ESCAPE));

    ExitProcess(0);
}
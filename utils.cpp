////////////////////////////////////////////////////////////////////////////////
//3456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789
//
// A test trying out OpenGL 3.x's transform-feedback feature with some SDL2.x
// glue code to make it work on multiple platforms
//
// Copyright 2015-2016 Mirco Müller
//
// Author(s):
//   Mirco "MacSlow" Müller <macslow@gmail.com>
//
// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License version 3, as published
// by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranties of
// MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
// PURPOSE.  See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program.  If not, see <http://www.gnu.org/licenses/>.
//
////////////////////////////////////////////////////////////////////////////////

#include "utils.h"

void frustum (float a,
              float b,
              float c,
              float d,
              float e,
              float g,
              float* out)
{
    assert (out);

    float h = b - a;
    float i = d - c;
    float j = g - e;

    out[0]  = e * 2.0f / h;
    out[1]  = 0.0f;
    out[2]  = 0.0f;
    out[3]  = 0.0f;
    out[4]  = 0.0f;
    out[5]  = e * 2.0f / i;
    out[6]  = 0.0f;
    out[7]  = 0.0f;
    out[8]  = (b + a) / h;
    out[9]  = (d + c) / i;
    out[10] = -(g + e) / j;
    out[11] = -1.0f;
    out[12] = 0.0f;
    out[13] = 0.0f;
    out[14] = -(g * e * 2.0f) / j;
    out[15] = 0.0f;
}

void perspective (float a,
                  float b,
                  float c,
                  float d,
                  float* out)
{
    assert (out);

    a = c * tan (a * M_PI / 360.0f);
    b = a * b;
    frustum (-b, b, -a, a, c, d, out);
}

void ortho (float left,
            float right,
            float bottom,
            float top,
            float nearVal,
            float farVal,
            float* out)
{
    assert (out);

    float a  = 2.0 / (right - left);
    float b  = 2.0 / (top - bottom);
    float c  = -2.0 / (farVal - nearVal);
    float tx = - (right + left) / (right - left);
    float ty = - (top + bottom) / (top - bottom);
    float tz = - (farVal + nearVal) / (farVal - nearVal);

    out[0]  = a;
    out[1]  = 0.0f;
    out[2]  = 0.0f;
    out[3]  = 0.0f;

    out[4]  = 0.0f;
    out[5]  = b;
    out[6]  = 0.0f;
    out[7]  = 0.0f;

    out[8]  = 0.0f;
    out[9]  = 0.0f;
    out[10] = c;
    out[11] = 0.0f;

    out[12] = tx;
    out[13] = ty;
    out[14] = tz;
    out[15] = 1.0f;
}

void checkGLError (const char* func)
{

#if defined RELEASE
    return;
#endif

    if (!func) {
        return;
    }

    std::cout << func <<"() - \033[31;1m";

    switch (glGetError ()) {
        case GL_INVALID_ENUM :
            std::cout << "invalid enum" << std::endl; 
        break;

        case GL_INVALID_VALUE :
            std::cout << "invalid value" << std::endl; 
        break;

        case GL_INVALID_OPERATION :
            std::cout << "invalid operation" << std::endl; 
        break;

        case GL_INVALID_FRAMEBUFFER_OPERATION :
            std::cout << "invalid framebuffer operation" << std::endl; 
        break;

        case GL_OUT_OF_MEMORY :
            std::cout << "out of memory" << std::endl; 
        break;

        case GL_STACK_UNDERFLOW :
            std::cout << "stack underflow" << std::endl; 
        break;

        case GL_STACK_OVERFLOW :
            std::cout << "stack overflow" << std::endl; 
        break;

        default :
            std::cout << "\033[32;1mok" << std::endl; 
        break;
    }

    std::cout << "\033[0m";
}

void dumpGLInfo ()
{
    std::cout << "OpenGL-vendor:\n\t"
              << glGetString (GL_VENDOR)
              << "\n\n";
    std::cout << "OpenGL-renderer:\n\t"
              << glGetString (GL_RENDERER)
              << "\n\n";
    std::cout << "OpenGL-version:\n\t"
              << glGetString (GL_VERSION)
              << "\n\n";
    std::cout << "OpenGL-shading-language:\n\t"
              << glGetString (GL_SHADING_LANGUAGE_VERSION)
              << "\n\n";
    std::cout << "OpenGL-extensions:\n";
    std::stringstream stream ((const char*) glGetString (GL_EXTENSIONS));
    std::istream_iterator<std::string> iter (stream);
    std::istream_iterator<std::string> end;
    std::list<std::string> extensions (iter, end);
    for (auto extension : extensions) {
        std::cout << "\t" << extension << std::endl;
    }
    std::cout << std::endl;

    std::cout << "created OpenGL-context:" << std::endl;
    int r, g, b, depth, dblbuf, samples;
    SDL_GL_GetAttribute (SDL_GL_RED_SIZE, &r);
    SDL_GL_GetAttribute (SDL_GL_GREEN_SIZE, &g);
    SDL_GL_GetAttribute (SDL_GL_BLUE_SIZE, &b);
    SDL_GL_GetAttribute (SDL_GL_DEPTH_SIZE, &depth);
    SDL_GL_GetAttribute (SDL_GL_DOUBLEBUFFER, &dblbuf);
    SDL_GL_GetAttribute (SDL_GL_MULTISAMPLESAMPLES, &samples);
    std::cout << "\t" << r << " bit red-channel" << std::endl;
    std::cout << "\t" << g << " bit green-channel" << std::endl;
    std::cout << "\t" << b << " bit blue-channel" << std::endl;
    std::cout << "\t" << depth << " bit z-buffer" << std::endl;
    std::cout << "\t" << samples << " msaa-samples" << std::endl;
    std::cout << "\t" << (dblbuf ? "is double-buffered" : "not double-buffered")
              << std::endl
              << std::endl;
}

GLuint createTexture (const char* filename)
{
    if (!filename) {
        return 0;
    }

    GLuint texture = 0;
    SDL_Surface* surface = NULL;
    surface = IMG_Load (filename);
    if (!surface) {
        std::cout << "Failed to create texture:" << IMG_GetError() << std::endl;
        return 0;
    }

    SDL_Surface* converted = NULL;
    converted = SDL_ConvertSurfaceFormat (surface, SDL_PIXELFORMAT_ABGR8888, 0);
    SDL_FreeSurface (surface);

    glGenTextures (1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D (GL_TEXTURE_2D,
                  0,
                  GL_RGBA,
                  converted->w,
                  converted->h,
                  0,
                  GL_RGBA,
                  GL_UNSIGNED_BYTE,
                  converted->pixels);
    SDL_FreeSurface (converted);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D,
                     GL_TEXTURE_MIN_FILTER,
                     GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glGenerateMipmap (GL_TEXTURE_2D);
    glBindTexture (GL_TEXTURE_2D, 0);

    return texture;
}

GLuint loadShader (const char *src, GLenum type)
{
    checkGLError (nullptr);
    GLuint shader = glCreateShader (type);
    checkGLError ("glCreateShader");
    if (shader)
    {
        GLint compiled;

        checkGLError (nullptr);
        glShaderSource (shader, 1, &src, NULL);
        checkGLError ("glShaderSource");

        checkGLError (nullptr);
        glCompileShader (shader);
        checkGLError ("glCompileShader");

        checkGLError (nullptr);
        glGetShaderiv (shader, GL_COMPILE_STATUS, &compiled);
        checkGLError ("glGetShaderiv");
        if (!compiled)
        {
            GLchar log[1024];

            checkGLError (nullptr);
            glGetShaderInfoLog (shader, sizeof log - 1, NULL, log);
            checkGLError ("glGetShaderInfoLog");

            log[sizeof log - 1] = '\0';
            std::cout << "loadShader compile failed: " << log << std::endl;

            checkGLError (nullptr);
            glDeleteShader (shader);
            checkGLError ("glDeleteShader");

            shader = 0;
        }
    }

    return shader;
}

GLuint createShaderProgram (const char* vertexShaderSrc,
                            const char* fragmentShaderSrc,
                            bool link)
{
    if (!vertexShaderSrc && !fragmentShaderSrc)
        return 0;

    GLuint vShaderId = 0;
    if (vertexShaderSrc) {
        vShaderId = loadShader (vertexShaderSrc, GL_VERTEX_SHADER);
        assert (vShaderId);
    }

    GLuint fShaderId = 0;
    if (fragmentShaderSrc) {
        fShaderId = loadShader (fragmentShaderSrc, GL_FRAGMENT_SHADER);
        assert (fShaderId);        
    }

    GLuint progId = 0;
    checkGLError (nullptr);
    progId = glCreateProgram ();
    checkGLError ("glCreateProgram");
    assert (progId);
    if (vertexShaderSrc) {
        checkGLError (nullptr);
        glAttachShader (progId, vShaderId);
        checkGLError ("glAttachShader");
    }

    if (fragmentShaderSrc) {
        checkGLError (nullptr);
        glAttachShader (progId, fShaderId);
        checkGLError ("glAttachShader");
    }

    if (!link) {
        return progId;
    }

    checkGLError (nullptr);
    glLinkProgram (progId);
    checkGLError ("glLinkProgram");

    GLint linked = 0;
    checkGLError (nullptr);
    glGetProgramiv (progId, GL_LINK_STATUS, &linked);
    checkGLError ("glGetProgramiv");
    if (!linked)
    {
        GLchar log[1024];
        checkGLError (nullptr);
        glGetProgramInfoLog (progId, sizeof log - 1, NULL, log);
        checkGLError ("glGetProgramInfoLog");
        log[sizeof log - 1] = '\0';
        std::cout << "Link failed: " << log << std::endl;
        glDeleteProgram (progId);
        return 0;
    }

    return progId;
}

void linkShaderProgram (GLuint progId)
{
    checkGLError (nullptr);
    glLinkProgram (progId);
    checkGLError ("glLinkProgram");

    GLint linked = 0;
    checkGLError (nullptr);
    glGetProgramiv (progId, GL_LINK_STATUS, &linked);
    checkGLError ("glGetProgramiv");
    if (!linked)
    {
        GLchar log[1024];
        checkGLError (nullptr);
        glGetProgramInfoLog (progId, sizeof log - 1, NULL, log);
        checkGLError ("glGetProgramInfoLog");
        log[sizeof log - 1] = '\0';
        std::cout << "Link failed: " << log << std::endl;
        glDeleteProgram (progId);
    }
}

GLuint createVBO (GLsizeiptr size, const GLvoid* data, GLenum usage)
{
    GLuint vbo = 0;

    checkGLError (nullptr);
    glGenBuffers (1, &vbo);
    checkGLError ("glGenBuffers");

    checkGLError (nullptr);
    glBindBuffer (GL_ARRAY_BUFFER, vbo);
    checkGLError ("glBindBuffer");

    checkGLError (nullptr);
    glBufferData (GL_ARRAY_BUFFER, size, data, usage);
    checkGLError ("glBufferData");

    checkGLError (nullptr);
    glBindBuffer (GL_ARRAY_BUFFER, 0);
    checkGLError ("glBindBuffer");

    return vbo;
}

void updateVBO (GLuint vbo, GLsizeiptr size, const GLvoid* data, GLenum usage)
{
    checkGLError (nullptr);
    glBindBuffer (GL_ARRAY_BUFFER, vbo);
    checkGLError ("glBindBuffer");

    checkGLError (nullptr);
    glBufferData (GL_ARRAY_BUFFER, size, data, usage);
    checkGLError ("glBufferData");

    checkGLError (nullptr);
    glBindBuffer (GL_ARRAY_BUFFER, 0);
    checkGLError ("glBindBuffer");
}

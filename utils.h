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

#ifndef _UTILS_H
#define _UTILS_H

#include <iostream>
#include <list>
#include <sstream>
#include <iterator>
#include <cassert>
#include <cmath>

#include <SDL.h>
#include <SDL_image.h>
#include <GL/glew.h>

#define GLSL(src) "#version 130\n" #src

void frustum (float a,
              float b,
              float c,
              float d,
              float e,
              float g,
              float* out);
void perspective (float a,
                  float b,
                  float c,
                  float d,
                  float* out);
void ortho (float left,
            float right,
            float bottom,
            float top,
            float nearVal,
            float farVal,
            float* out);
void checkGLError (const char* func);
void dumpGLInfo ();
GLuint createTexture (const char* filename);
GLuint loadShader (const char *src, GLenum type);
GLuint createShaderProgram (const char* vertexShaderSrc,
                            const char* fragmentShaderSrc,
                            bool link);
void linkShaderProgram (GLuint progId);
GLuint createVBO (GLsizeiptr size, const GLvoid* data, GLenum usage);
void updateVBO (GLuint vbo, GLsizeiptr size, const GLvoid* data, GLenum usage);

#endif // _UTILS_H

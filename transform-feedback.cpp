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

#include <iomanip>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <random>
#include <iostream>

#include "utils.h"

enum VertexAttribs {
    PositionAttr,
    VelocityAttr,
    DistanceAttr,
    TexCoordAttr
};

#define BG_COLOR .5, .5, .5
#define WIN_TITLE "Transform Feedback by MacSlow"
#define WIN_WIDTH 700
#define WIN_HEIGHT 700
#define CUBE_SIZE 100
#define NUM_PARTICLES (CUBE_SIZE * CUBE_SIZE * CUBE_SIZE)
#define NUM_FLOATS_PER_VERTEX 7
#define MAX_ELEMENTS (NUM_PARTICLES * NUM_FLOATS_PER_VERTEX)
#define Z_NEAR 0.1
#define Z_FAR 100.0
#define FOV 60.0
#define BLACK_HOLE_MASS 100000.0

GLuint vbo = 0;
GLuint tbo = 0;
GLint aVelocity = 0;
GLint aTexCoord = 0;
GLint uPersp = 0;
GLint uAngles = 0;
GLint uTranslate = 0;
GLint uUseOpacity = 0;
GLint uBlackHolePosition = 0;
GLint uBlackHoleMass = 0;
GLint uLimits = 0;
GLint uTimeStep = 0;
GLint uSampler = 0;
GLint uEye = 0;
GLint uAim = 0;
GLint uUp = 0;
GLint uPerspFeedback = 0;
GLint uAnglesFeedback = 0;
GLint uEyeFeedback = 0;
GLint uAimFeedback = 0;
GLint uUpFeedback = 0;
GLint uTranslateFeedback = 0;
GLint position = 0;
GLint velocity = 0;
GLfloat mouseX = WIN_WIDTH / 2;
GLfloat mouseY = WIN_HEIGHT / 2;
unsigned int lastFrameTick = 0;
GLfloat blackHoleMass = 0.0;
GLfloat eye[3] = {0.0, 0.0, 2.0};
GLfloat aim[3] = {0.0, 0.0, 0.0};
GLfloat up[3] = {0.0, 1.0, 0.0};
GLfloat translate[3] = {0.0, 0.0, -25.0};
GLfloat angles[3] = {0.0, 0.0, 0.0};
GLint useOpacity = 0;

// particle-drawing vertex- and fragment-shader
const GLchar* vShaderSrc = GLSL(
    in vec3 aPosition;
    in vec3 aVelocity;
    in float aDistance;

    uniform mat4 uPersp;
    uniform vec3 uEye;
    uniform vec3 uAim;
    uniform vec3 uUp;
    uniform vec3 uTranslate;
    uniform vec3 uAngles;

    out float vOpacity;

    mat4 rot (vec3 angles)
    {
        vec3 rad = radians (angles);
        vec3 c = cos (rad);
        vec3 s = sin (rad);

        mat4 matX = mat4 (vec4 (1.0, 0.0, 0.0, 0.0),
                          vec4 (0.0, c.x, s.x, 0.0),
                          vec4 (0.0,-s.x, c.x, 0.0),
                          vec4 (0.0, 0.0, 0.0, 1.0));

        mat4 matY = mat4 (vec4 (c.y, 0.0,-s.y, 0.0),
                          vec4 (0.0, 1.0, 0.0, 0.0),
                          vec4 (s.y, 0.0, c.y, 0.0),
                          vec4 (0.0, 0.0, 0.0, 1.0));

        mat4 matZ = mat4 (vec4 (c.z,  s.z, 0.0, 0.0),
                          vec4 (-s.z, c.z, 0.0, 0.0),
                          vec4 ( 0.0, 0.0, 1.0, 0.0),
                          vec4 ( 0.0, 0.0, 0.0, 1.0));

        return matZ * matY * matX;
    }

    mat4 trans (vec3 t)
    {
        mat4 mat = mat4 (vec4 (1.0, 0.0, 0.0, 0.0),
                         vec4 (0.0, 1.0, 0.0, 0.0),
                         vec4 (0.0, 0.0, 1.0, 0.0),
                         vec4 (t.x, t.y, t.z, 1.0));
        return mat;
    }

    mat4 lookAt (vec3 eye, vec3 aim, vec3 up)
    {
        vec3 f = normalize (aim - eye);
        vec3 s = normalize (cross (f, up));
        vec3 u = cross (s, f);
        mat4 view = mat4 (vec4 (s.x, u.x, -f.x, 0.0),
                          vec4 (s.y, u.y, -f.y, 0.0),
                          vec4 (s.z, u.z, -f.z, 0.0),
                          vec4 (0.0, 0.0, 0.0, 1.0));
        return view;
    }

    void main()
    {
        mat4 view = lookAt (uEye, uAim, uUp);
        mat4 model = trans (uTranslate) * rot (uAngles);
        gl_Position = uPersp * view * model * vec4 (aPosition, 1.0);
        gl_PointSize = 0.5;
        vOpacity = length (aVelocity);
    }
);

const GLchar* fShaderSrc = GLSL(
    uniform int uUseOpacity;
    in float vOpacity;
    void main()
    {
        if (uUseOpacity == 1) {
            gl_FragColor = vec4 (.85, .85, .85, clamp (vOpacity, .0, 1.));
        } else {
            gl_FragColor = vec4 (.85, .85, .85, 1.);
        }
    }
);

// particle-gravity vertex-shader
const GLchar* particleGravitySrc = GLSL(
    in vec3 aPosition;
    in vec3 aVelocity;
    in float aDistance;

    out vec3 vPosition;
    out vec3 vVelocity;
    out float vDistance;

    uniform mat4 uPersp;
    uniform vec3 uEye;
    uniform vec3 uAim;
    uniform vec3 uUp;
    uniform vec3 uTranslate;
    uniform vec3 uAngles;

    uniform vec3 uBlackHolePosition;
    uniform float uTimeStep;
    uniform float uBlackHoleMass;
    uniform vec3 uLimits;

    mat4 rot (vec3 angles)
    {
        vec3 rad = radians (angles);
        vec3 c = cos (rad);
        vec3 s = sin (rad);

        mat4 matX = mat4 (vec4 (1.0, 0.0, 0.0, 0.0),
                          vec4 (0.0, c.x, s.x, 0.0),
                          vec4 (0.0,-s.x, c.x, 0.0),
                          vec4 (0.0, 0.0, 0.0, 1.0));

        mat4 matY = mat4 (vec4 (c.y, 0.0,-s.y, 0.0),
                          vec4 (0.0, 1.0, 0.0, 0.0),
                          vec4 (s.y, 0.0, c.y, 0.0),
                          vec4 (0.0, 0.0, 0.0, 1.0));

        mat4 matZ = mat4 (vec4 (c.z,  s.z, 0.0, 0.0),
                          vec4 (-s.z, c.z, 0.0, 0.0),
                          vec4 ( 0.0, 0.0, 1.0, 0.0),
                          vec4 ( 0.0, 0.0, 0.0, 1.0));

        return matZ * matY * matX;
    }

    mat4 trans (vec3 t)
    {
        mat4 mat = mat4 (vec4 (1.0, 0.0, 0.0, 0.0),
                         vec4 (0.0, 1.0, 0.0, 0.0),
                         vec4 (0.0, 0.0, 1.0, 0.0),
                         vec4 (t.x, t.y, t.z, 1.0));
        return mat;
    }

    mat4 lookAt (vec3 eye, vec3 aim, vec3 up)
    {
        vec3 f = normalize (aim - eye);
        vec3 s = normalize (cross (f, up));
        vec3 u = cross (s, f);
        mat4 view = mat4 (vec4 (s.x, u.x, -f.x, 0.0),
                          vec4 (s.y, u.y, -f.y, 0.0),
                          vec4 (s.z, u.z, -f.z, 0.0),
                          vec4 (0.0, 0.0, 0.0, 1.0));
        return view;
    }

    void main() {
        vec3 blackHolePos = vec4 (rot (uAngles) * vec4 (uBlackHolePosition, 1.)).xyz;
        vec3 p = blackHolePos - aPosition;
        float g = 0.0000000000667384;
        float particleMass = 1000.0;
        float k = g * particleMass * uBlackHoleMass;
        float dist = length (p);
        float d = dist * dist;

        vDistance = dist;
        vec3 v = blackHolePos - aPosition;
        vec3 f = k * normalize (v) / d;

        vec3 a = particleMass * f;
        vec3 newVelocity = a + aVelocity;
        vec3 tmp = .475 * (aVelocity + newVelocity);
        vPosition = aPosition + tmp * uTimeStep;
        vVelocity = tmp;
        if (vPosition.x <= -uLimits.x ||
            vPosition.x >= uLimits.x ||
            vPosition.y <= -uLimits.y ||
            vPosition.y >= uLimits.y ||
            vPosition.z <= -uLimits.z ||
            vPosition.z >= uLimits.z) {
            vVelocity = 0.1 * tmp;
            if (vPosition.x <= -uLimits.x ) {
                vPosition.x = uLimits.x;
            } else if (vPosition.x >= uLimits.x) {
                vPosition.x = -uLimits.x;
            }
            if (vPosition.y <= -uLimits.y) {
                vPosition.y = uLimits.y;
            } else if (vPosition.y >= uLimits.y) {
                vPosition.y = -uLimits.y;
            }
            if (vPosition.z <= -uLimits.z) {
                vPosition.z = uLimits.z;
            } else if (vPosition.z >= uLimits.z) {
                vPosition.z = -uLimits.z;
            }
        }
        gl_Position = vec4 (0.0, 0.0, 0.0, 0.0);
    }
);

void updateFeedbackBuffer (GLuint program, int width, int height, float* persp)
{
    glUseProgram (program);
    glUniform1f (uTimeStep, (GLfloat) lastFrameTick / 100000.0f);
    glUniform3f (uBlackHolePosition,
                 30.0f * (mouseX / width) - 15.0f,
                 30.0f * (mouseY / height) - 15.0f,
                 .0f);
    glUniform3f (uLimits, 15.0f, 15.0f, 15.0f);
    glUniform1f (uBlackHoleMass, blackHoleMass);

    glUniformMatrix4fv (uPerspFeedback, 1, GL_FALSE, persp);
    glUniform3fv (uAnglesFeedback, 1, angles);
    glUniform3fv (uEyeFeedback, 1, eye);
    glUniform3fv (uAimFeedback, 1, aim);
    glUniform3fv (uUpFeedback, 1, up);
    glUniform3fv (uTranslateFeedback, 1, translate);

    glEnable (GL_RASTERIZER_DISCARD);
    glBindBuffer (GL_ARRAY_BUFFER, vbo);
    glEnableVertexAttribArray (PositionAttr);
    glEnableVertexAttribArray (VelocityAttr);

    GLchar* offset = 0;
    glVertexAttribPointer (PositionAttr,
                           3,
                           GL_FLOAT,
                           GL_FALSE,
                           NUM_FLOATS_PER_VERTEX * sizeof (GLfloat),
                           offset);
    glVertexAttribPointer (VelocityAttr,
                           3,
                           GL_FLOAT,
                           GL_FALSE,
                           NUM_FLOATS_PER_VERTEX * sizeof (GLfloat),
                           3 * sizeof (GLfloat) + offset);
    glVertexAttribPointer (DistanceAttr,
                           1,
                           GL_FLOAT,
                           GL_FALSE,
                           NUM_FLOATS_PER_VERTEX * sizeof (GLfloat),
                           6 * sizeof (GLfloat) + offset);

    glBindBufferBase (GL_TRANSFORM_FEEDBACK_BUFFER, 0, tbo);
    glBeginTransformFeedback (GL_POINTS);
    glDrawArrays (GL_POINTS, 0, NUM_PARTICLES);
    glEndTransformFeedback ();
    glBindBuffer (GL_ARRAY_BUFFER, 0);
    glDisableVertexAttribArray (PositionAttr);
    glDisableVertexAttribArray (VelocityAttr);
    glDisable (GL_RASTERIZER_DISCARD);

    std::swap (vbo, tbo);

    glFlush ();
}

void initGL (SDL_Window* window, int width, int height, float* persp)
{
    if (!window) {
        return;
    }

    dumpGLInfo (); // dumps to stdout when compiled without -mwindows under Win

    glClearColor (BG_COLOR, 1.0);
    //glViewport (0, 0, width, height);
    //ortho (0.0, width, height, 0.0, Z_NEAR, Z_FAR, persp);

    // setup proper GL-blending
    glEnable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation (GL_FUNC_ADD);
    glEnable (GL_PROGRAM_POINT_SIZE);

    perspective (FOV, (GLfloat) width / (GLfloat) height, Z_NEAR, Z_FAR, persp);
}

void resizeGL (SDL_Window* window, int width, int height, float* persp)
{
    if (!window) {
        return;
    }

    glViewport (0, 0, width, height);

    //ortho (0.0, width, height, 0.0, Z_NEAR, Z_FAR, persp);
    perspective (FOV, (GLfloat) width / (GLfloat) height, Z_NEAR, Z_FAR, persp);
}

void drawGL (SDL_Window* window, GLuint program, float* persp, GLuint bufferId)
{
    // vbo, uniform, attrib
    static unsigned int fps = 0;
    static unsigned int lastTick = 0;
    static unsigned int currentTick = 0;

    if (!window) {
        return;
    }

    glClear (GL_COLOR_BUFFER_BIT);
    glUseProgram (program);
    glBindBuffer (GL_ARRAY_BUFFER, bufferId);
    angles[0] += .3;
    angles[1] += .2;
    //angles[2] -= .35;
    glUniform1i (uUseOpacity, useOpacity);
    glUniform3fv (uAngles, 1, angles);
    glUniform3fv (uTranslate, 1, translate);
    glUniform3fv (uEye, 1, eye);
    glUniform3fv (uAim, 1, aim);
    glUniform3fv (uUp, 1, up);

    glUniformMatrix4fv (uPersp, 1, GL_FALSE, persp);
    glEnableVertexAttribArray (PositionAttr);
    glEnableVertexAttribArray (VelocityAttr);
    glEnableVertexAttribArray (DistanceAttr);

    GLchar* offset = 0;
    glVertexAttribPointer (PositionAttr,
                           3,
                           GL_FLOAT,
                           GL_FALSE,
                           NUM_FLOATS_PER_VERTEX * sizeof (GLfloat),
                           offset);
    glVertexAttribPointer (VelocityAttr,
                           3,
                           GL_FLOAT,
                           GL_FALSE,
                           NUM_FLOATS_PER_VERTEX * sizeof (GLfloat),
                           3 * sizeof (GLfloat) + offset);
    glVertexAttribPointer (DistanceAttr,
                           1,
                           GL_FLOAT,
                           GL_FALSE,
                           NUM_FLOATS_PER_VERTEX * sizeof (GLfloat),
                           5 * sizeof (GLfloat) + offset);

    glDrawArrays (GL_POINTS, 0, NUM_PARTICLES);
    glDisableVertexAttribArray (PositionAttr);
    glDisableVertexAttribArray (VelocityAttr);
    glDisableVertexAttribArray (DistanceAttr);
    glBindTexture (GL_TEXTURE_2D, 0);
    glBindBuffer (GL_ARRAY_BUFFER, 0);
    SDL_GL_SwapWindow (window);

    fps++;
    currentTick = SDL_GetTicks ();

    if (currentTick - lastTick > 1000)
    {
        std::stringstream title;
        title << WIN_TITLE << " - " << fps << " fps";
        std::string str (title.str ());
        SDL_SetWindowTitle (window, str.c_str ());
        fps = 0;
        lastTick = currentTick;
    }
    lastFrameTick = currentTick;
}

int main(int argc, char* argv[]) {
    // initialize SDL
    int result = 0;
    result = SDL_Init (SDL_INIT_VIDEO);
    if (result != 0) {
        std::cout << "SDL_Init() failed: " << SDL_GetError () << std::endl;
        return 1;
    }

    // initialize SDL_image
    int flags = IMG_INIT_PNG | IMG_INIT_JPG;
    result = IMG_Init (flags);
    if ((result & flags) != flags) {
        std::cout << "IMG_Init() failed: " << IMG_GetError () << std::endl;
        return 2;
    }

    SDL_GL_SetAttribute (SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute (SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute (SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute (SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute (SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute (SDL_GL_MULTISAMPLESAMPLES, 4);
    SDL_GL_SetAttribute (SDL_GL_MULTISAMPLEBUFFERS, 1);
    /*SDL_GL_SetAttribute (SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute (SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute (SDL_GL_CONTEXT_PROFILE_MASK,
                         SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);*/

    // setup window
    SDL_Window* window = NULL;
    SDL_ClearError ();
    window = SDL_CreateWindow (WIN_TITLE,
                               SDL_WINDOWPOS_UNDEFINED,
                               SDL_WINDOWPOS_UNDEFINED,
                               WIN_WIDTH,
                               WIN_HEIGHT,
                               SDL_WINDOW_OPENGL |
                               SDL_WINDOW_RESIZABLE);
    if (!window) {
        std::cout << "CreateWindow() failed: " << SDL_GetError () << std::endl;
        SDL_ShowSimpleMessageBox (SDL_MESSAGEBOX_ERROR,
                                  "Window Creation failed",
                                  SDL_GetError (),
                                  NULL);
        IMG_Quit ();
        SDL_Quit ();
        return 3;
    }

    // set window-icon
    SDL_Surface* icon = NULL;
    SDL_ClearError ();
    icon = IMG_Load ("./icon.png");
    if (icon) {
        SDL_SetWindowIcon (window, icon);
        SDL_FreeSurface (icon);
    } else {
        std::cout << "Failed to load icon: " << SDL_GetError () << std::endl;
    }

    // setup OpenGL-context
    SDL_GLContext context;
    SDL_ClearError ();
    context = SDL_GL_CreateContext (window);
    if (!context) {
        std::cout << "CreateContext() failed: " << SDL_GetError () << std::endl;
        SDL_ShowSimpleMessageBox (SDL_MESSAGEBOX_ERROR,
                                  "Context Creation failed",
                                  SDL_GetError (),
                                  NULL);
        SDL_DestroyWindow (window);
        IMG_Quit ();
        SDL_Quit ();
        return 4;
    }

    int success = glewInit();
    if (success != GLEW_OK) {
        std::cout << "OpenGL initialization failed" << std::endl;
        SDL_ShowSimpleMessageBox (SDL_MESSAGEBOX_ERROR,
                                  "Error",
                                  "OpenGL initialization failed",
                                  NULL);
    }

    // create vertex-only shader-program
    GLuint feedbackProg = createShaderProgram (particleGravitySrc, NULL, false);
    glBindAttribLocation (feedbackProg, PositionAttr, "aPosition");
    glBindAttribLocation (feedbackProg, VelocityAttr, "aVelocity");
    glBindAttribLocation (feedbackProg, DistanceAttr, "aDistance");

    const GLchar* feedbackVaryings[] = {"vPosition", "vVelocity", "vDistance"};
    glTransformFeedbackVaryings (feedbackProg,
                                 3,
                                 feedbackVaryings,
                                 GL_INTERLEAVED_ATTRIBS);

    linkShaderProgram (feedbackProg);
    glUseProgram (feedbackProg);

    // Create input VBO, vertex format and upload inital data
    GLfloat* data = (GLfloat*) std::calloc (MAX_ELEMENTS, sizeof (GLfloat));

    std::random_device rand;
    std::mt19937 generator (rand ());
    std::uniform_real_distribution<float> distributedX (-15, 15);
    std::uniform_real_distribution<float> distributedY (-15, 15);
    std::uniform_real_distribution<float> distributedZ (-15, 15);
    for (int i = 0; i < MAX_ELEMENTS; i += NUM_FLOATS_PER_VERTEX) {
        data[i]   = distributedX (generator);
        data[i+1] = distributedY (generator);
        data[i+2] = distributedZ (generator);
        data[i+3] = 0.0;
        data[i+4] = 0.0;
        data[i+5] = 0.0;
        data[i+6] = 0.0;
    }

	vbo = createVBO (MAX_ELEMENTS * sizeof (GLfloat), data, GL_DYNAMIC_COPY);
	tbo = createVBO (MAX_ELEMENTS * sizeof (GLfloat), nullptr, GL_DYNAMIC_COPY);

    glUseProgram (feedbackProg);
    glEnableVertexAttribArray (PositionAttr);
    glEnableVertexAttribArray (VelocityAttr);
    glEnableVertexAttribArray (DistanceAttr);

    // provide uniform values
    uBlackHolePosition = glGetUniformLocation (feedbackProg,
                                               "uBlackHolePosition");

    uTimeStep = glGetUniformLocation (feedbackProg, "uTimeStep");
    uLimits = glGetUniformLocation (feedbackProg, "uLimits");
    uBlackHoleMass = glGetUniformLocation (feedbackProg, "uBlackHoleMass");
    uPerspFeedback = glGetUniformLocation (feedbackProg, "uPersp");
    uAnglesFeedback = glGetUniformLocation (feedbackProg, "uAngles");
    uEyeFeedback = glGetUniformLocation (feedbackProg, "uEye");
    uAimFeedback = glGetUniformLocation (feedbackProg, "uAim");
    uUpFeedback = glGetUniformLocation (feedbackProg, "uUp");
    uTranslateFeedback = glGetUniformLocation (feedbackProg, "uTranslate");
    glUniform1f (uTimeStep, 0.0);
    glUniform2f (uBlackHolePosition, mouseX, mouseY);
    //glUniform2f (uLimits, (GLfloat) WIN_WIDTH, (GLfloat) WIN_HEIGHT);
    glUniform1f (uBlackHoleMass, blackHoleMass);
    GLuint particleProg = createShaderProgram (vShaderSrc, fShaderSrc, true);
    glBindAttribLocation (particleProg, PositionAttr, "aPosition");
    glBindAttribLocation (particleProg, VelocityAttr, "aVelocity");
    glBindAttribLocation (particleProg, DistanceAttr, "aDistance");
    uPersp = glGetUniformLocation (particleProg, "uPersp");
    uAngles = glGetUniformLocation (particleProg, "uAngles");
    uEye = glGetUniformLocation (particleProg, "uEye");
    uAim = glGetUniformLocation (particleProg, "uAim");
    uUp = glGetUniformLocation (particleProg, "uUp");
    uTranslate = glGetUniformLocation (particleProg, "uTranslate");
    uUseOpacity = glGetUniformLocation (particleProg, "uUseOpacity");

    if (argc == 2) {
        useOpacity = atoi (argv[1]);
    }

    float persp[16];
    initGL (window, WIN_WIDTH, WIN_HEIGHT, persp);

    // event-loop
    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent (&event)) {
            switch (event.type) {
                case SDL_KEYUP:
                    if (event.key.keysym.sym == SDLK_ESCAPE) {
                        running = false;
                    }
                    if (event.key.keysym.sym == SDLK_SPACE) {
                        updateVBO (vbo,
                                   MAX_ELEMENTS * sizeof (GLfloat),
                                   data,
                                   GL_DYNAMIC_COPY);
                        updateVBO (tbo,
                                   MAX_ELEMENTS * sizeof (GLfloat),
                                   nullptr,
                                   GL_DYNAMIC_COPY);
                        blackHoleMass = 0.0;
                    }
                break;

                case SDL_MOUSEMOTION:
                    if (SDL_GetMouseState (NULL, NULL) &
                        SDL_BUTTON (SDL_BUTTON_LEFT) || 
                        SDL_GetMouseState (NULL, NULL) &
                        SDL_BUTTON (SDL_BUTTON_RIGHT)) {
                        mouseX = (GLfloat) event.motion.x;
                        mouseY = (GLfloat) event.motion.y;
                    }
                break;

                case SDL_MOUSEBUTTONDOWN:
                    if (SDL_GetMouseState (NULL, NULL) &
                        SDL_BUTTON (SDL_BUTTON_LEFT)) {
                        mouseX = (GLfloat) event.button.x;
                        mouseY = (GLfloat) event.button.y;
                        blackHoleMass = BLACK_HOLE_MASS;
                    }

                    if (SDL_GetMouseState (NULL, NULL) &
                        SDL_BUTTON (SDL_BUTTON_RIGHT)) {
                        blackHoleMass = -0.25 * BLACK_HOLE_MASS;
                    }

                    if (SDL_GetMouseState (NULL, NULL) &
                        SDL_BUTTON (SDL_BUTTON_MIDDLE)) {
                        blackHoleMass = 0.0;
                    }
                break;

                case SDL_WINDOWEVENT:
                    if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
                        running = false;
                    } else if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                        resizeGL (window,
                                  event.window.data1,
                                  event.window.data2,
                                  persp);
                    }
                break;

                default:
                break;
            }
        }

        int width = 0;
        int height = 0;
        SDL_GetWindowSize (window, &width, &height);
        updateFeedbackBuffer (feedbackProg, width, height, persp);
        drawGL (window, particleProg, persp, vbo);
    }

    // clean up
    glDeleteBuffers (1, &vbo);
    glDeleteBuffers (1, &tbo);
    glDeleteProgram (feedbackProg);
    glDeleteProgram (particleProg);
    SDL_GL_DeleteContext (context);
    SDL_DestroyWindow (window);
    IMG_Quit ();
    SDL_Quit ();
    std::free (data);

    return 0;
}

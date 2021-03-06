/*
 * This proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2012 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

/**
 * \file AntiAlias.cpp
 * \brief A sample to show how to enable AntiAliasing
 *
 * \warning AntiAliasing is disabled on Windows and Desktop Linux platforms.
 * If your graphics card supports it, enable it in WindowsPlatform.cpp or DesktopLinux.cpp
 * depending on your platform.
 *
 * EGL_SAMPLES is used to specifiy the level of AntiAliasing to be used.
 * On Mali platforms, 4x AntiAliasing incurs almost no performance penalty.
 */

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <string>

#include "AntiAlias.h"
#include "Text.h"
#include "Shader.h"
#include "Matrix.h"
#include "Platform.h"
#include "EGLRuntime.h"
#include "Timer.h"

#define WINDOW_W 800
#define WINDOW_H 600

using std::string;
using namespace MaliSDK;

/* Asset directories and filenames. */
string resourceDirectory = "assets/";
string vertexShaderFilename = "AntiAlias_triangle.vert";
string fragmentShaderFilename = "AntiAlias_triangle.frag";

/* Shader variables. */
GLuint programID = 0;
GLint iLocPosition = -1;
GLint iLocFillColor = -1;
GLint iLocProjection = -1;

/* EGL variables. */
int numberOfSamples = 0;

/* A text object to draw text on the screen. */ 
Text* text;

bool setupGraphics(int width, int height)
{
    /* Full paths to the shader files */
    string vertexShaderPath = resourceDirectory + vertexShaderFilename; 
    string fragmentShaderPath = resourceDirectory + fragmentShaderFilename;
    
    /* Initialize OpenGL ES. */
    GL_CHECK(glEnable(GL_BLEND));
    /* Should do src * (src alpha) + dest * (1-src alpha). */
    GL_CHECK(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

    /* Initialize the Text object and add some text. */
    text = new Text(resourceDirectory.c_str(), width, height);
    text->addString(0, 0, "Anti-aliased triangle", 255, 255, 0, 255);

    /* Process shaders. */
    GLuint vertexShaderID = 0;
    GLuint fragmentShaderID = 0;
    LOGI("setupGraphics(%d, %d)", width, height);
    Shader::processShader(&vertexShaderID, vertexShaderPath.c_str(), GL_VERTEX_SHADER);
    LOGI("vertexShaderID = %d", vertexShaderID);
    Shader::processShader(&fragmentShaderID, fragmentShaderPath.c_str(), GL_FRAGMENT_SHADER);
    LOGI("fragmentShaderID = %d", fragmentShaderID);

    /* Set up shaders. */
    programID = GL_CHECK(glCreateProgram());
    if (!programID)
    {
        LOGE("Could not create program.");
        return false;
    }
    GL_CHECK(glAttachShader(programID, vertexShaderID));
    GL_CHECK(glAttachShader(programID, fragmentShaderID));
    GL_CHECK(glLinkProgram(programID));
    GL_CHECK(glUseProgram(programID));
    LOGI("Shaders in use...");

    /* Vertex positions. */
    iLocPosition = GL_CHECK(glGetAttribLocation(programID, "a_v4Position"));
    if(iLocPosition == -1)
    {
        LOGE("Attribute not found at %s:%i\n", __FILE__, __LINE__);
        return false;
    }
    GL_CHECK(glEnableVertexAttribArray(iLocPosition));

    /* Fill colors. */
    iLocFillColor = GL_CHECK(glGetAttribLocation(programID, "a_v4FillColor"));
    if(iLocFillColor == -1)
    {
        LOGD("Warning: Uniform not found at %s:%i\n", __FILE__, __LINE__);
    }
    else
    {
        GL_CHECK(glEnableVertexAttribArray(iLocFillColor));
    }

    /* Projection matrix. */
    iLocProjection = GL_CHECK(glGetUniformLocation(programID, "u_m4Projection"));
    if(iLocProjection == -1)
    {
        LOGD("Warning: Uniform not found at %s:%i\n", __FILE__, __LINE__);
    }
    else
    {
        GL_CHECK(glUniformMatrix4fv(iLocProjection, 1, GL_FALSE, Matrix::identityMatrix.getAsArray()));
    }

    /* Set clear screen color. RGBA format, so opaque blue. */
    GL_CHECK(glClearColor(0.0f, 0.0f, 1.0f, 1.0f));

    return true;
}

void renderFrame(void)
{
    /* Clear the screen on the EGL surface. */
    GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    GL_CHECK(glUseProgram(programID));

    /* Load EGL window-specific projection and modelview matrices. */
    if(iLocProjection != -1)
    {
        GL_CHECK(glUniformMatrix4fv(iLocProjection, 1, GL_FALSE, Matrix::identityMatrix.getAsArray()));
    }

    /* Set triangle vertex. */
    GL_CHECK(glVertexAttribPointer(iLocPosition, 3, GL_FLOAT, GL_FALSE, 0, triangleVertices));
    GL_CHECK(glEnableVertexAttribArray(iLocPosition));

    /* Set triangle colors. */
    if(iLocFillColor != -1)
    {
        GL_CHECK(glVertexAttribPointer(iLocFillColor, 4, GL_FLOAT, GL_FALSE, 0, triangleColors));
        GL_CHECK(glEnableVertexAttribArray(iLocFillColor));
    }

    /* Draw the triangle. */
    GL_CHECK(glDrawArrays(GL_TRIANGLES, 0, 3));

    /* Draw fonts. */
    text->draw();
}

int main(int argc, char **argv)
{
    /* Intialize the Platform object for platform specific functions. */
    Platform *platform = Platform::getInstance();

    /* Initialize windowing system. */
    platform->createWindow(WINDOW_W, WINDOW_H);

    /* Initialize EGL. */
    /* If we were passed an argument, request that as EGL_SAMPLES. */
    if(argc >= 2)
    {
        int samples = atoi(argv[1]);
        EGLRuntime::setEGLSamples(samples);
    }
    EGLRuntime::initializeEGL(EGLRuntime::OPENGLES2);
    EGL_CHECK(eglMakeCurrent(EGLRuntime::display, EGLRuntime::surface, EGLRuntime::surface, EGLRuntime::context));

    /* Recover the actual EGL_SAMPLE value. */
    int attributeValue = -1;
    eglGetConfigAttrib(EGLRuntime::display, EGLRuntime::config, EGL_SAMPLES, &attributeValue);
    LOGI("EGL_SAMPLES = %d", attributeValue);

    /* Initialize OpenGL ES graphics subsystem. */
    setupGraphics(WINDOW_W, WINDOW_H);

    /* Timer variable to calculate FPS. */
    Timer fpsTimer;
    fpsTimer.reset();

    bool end = false;
    /* The rendering loop to draw the scene. */
    while(!end)
    {
        /* If something has happened to the window, end the sample. */
        if(platform->checkWindow() != Platform::WINDOW_IDLE)
        {
            end = true;
        }

        /* Calculate FPS. */
        float fFPS = fpsTimer.getFPS();
        if(fpsTimer.isTimePassed(1.0f))
        {
            LOGI("FPS:\t%.1f\n", fFPS);
        }
        
        /* Render a single frame */
        renderFrame();

        /* 
         * Push the EGL surface color buffer to the native window.
         * Causes the rendered graphics to be displayed on screen.
         */
        eglSwapBuffers(EGLRuntime::display, EGLRuntime::surface);
    }

    /* Shut down OpenGL ES. */
    /* Shut down Text. */
    delete text;

    /* Shut down EGL. */
    EGLRuntime::terminateEGL();

    /* Shut down windowing system. */
    platform->destroyWindow();

    /* Shut down the Platform object*/
    delete platform;

    return 0;
}

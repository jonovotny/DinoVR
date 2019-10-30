#ifndef GLUTIL_H
#define GLUTIL_H

#include <iostream>


#if defined(WIN32)
#define NOMINMAX
#include <windows.h>
#include <GL/glew.h>
#include <GL/wglew.h>
#include <GL/gl.h>
#include <gl/GLU.h>

#elif defined(__APPLE__)

#include <GL/glew.h>
#include <OpenGL/OpenGL.h>
#include <OpenGL/glu.h>

#else
#define GL_GLEXT_PROTOTYPES
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#endif

//Copied from http://learnopengl.com/#!In-Practice/Debugging

void dummy();

GLenum glCheckError_(const char *file, int line);
/**
 * A utility function to generate useful error messsages easily.
 */
#define glCheckError() glCheckError_(__FILE__, __LINE__) 


#endif

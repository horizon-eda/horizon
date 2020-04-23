#pragma once

#ifndef OFFSCREEN
#include <epoxy/gl.h>
#else
#define GL_GLEXT_PROTOTYPES 1
#include <GL/gl.h>
#include <GL/glcorearb.h>
#endif

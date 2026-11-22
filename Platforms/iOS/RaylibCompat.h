#pragma once

// Import Apple types before raylib's private Point and Size declarations.
#include <UIKit/UIKit.h>

// The fork's rlgl.h includes the GLES1 extension header before GLES3 types.
#include <libGLESv2/GLES3/gl3.h>
typedef GLfixed GLclampx;
#define GL_API GL_APICALL

// These names are private to rcore.c and conflict with MacTypes.h.
#define Point LevyeRaylibPoint
#define Size LevyeRaylibSize

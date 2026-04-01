#pragma once
/// @file GLHeaders.h
/// @brief Platform-safe OpenGL include.
///
/// On Windows the Windows SDK's GL/gl.h depends on WINGDIAPI and APIENTRY
/// which are defined by <windows.h>.  Always include this file instead of
/// including <GL/gl.h> directly to guarantee the correct include order on
/// every platform.

#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#endif

#include <GL/gl.h>

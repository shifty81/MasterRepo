#pragma once
/// @file GLHeaders.h
/// @brief Platform-safe OpenGL include via GLAD.
///
/// GLAD provides function-pointer loading for all OpenGL 1.2+ entry points
/// (e.g. glActiveTexture, glGenBuffers, glBindVertexArray).  On Windows the
/// stock <GL/gl.h> only exposes OpenGL 1.1, so GLAD must be included instead.
/// Always include this file rather than <GL/gl.h> or <glad/gl.h> directly.

#include <glad/gl.h>

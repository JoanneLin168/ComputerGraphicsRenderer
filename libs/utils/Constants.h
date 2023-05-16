#pragma once
#define _USE_MATH_DEFINES
#include <math.h>

static const int WIDTH = 320;
static const int HEIGHT = 240;
static const int SCALE = 150; // used for scaling onto img canvas

// Values for translation and rotation
static const float DIST = 0.1f;
static const float ANGLE = float((1.0 / 360.0) * (2 * M_PI));

// Values for lighting
static const float LIGHT_STRENGTH = 10;
static const float SPECULAR_EXPONENT = 32; // Note: increase this to 256 for cornell box

enum ShadingType { SHADING_NONE, SHADING_FLAT, SHADING_GOURAUD, SHADING_PHONG };
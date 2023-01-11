#define _USE_MATH_DEFINES
#include <math.h>

const int WIDTH = 320;
const int HEIGHT = 240;
const int SCALE = 150; // used for scaling onto img canvas

// Values for translation and rotation
const float DIST = 0.1f;
const float ANGLE = float((1.0 / 360.0) * (2 * M_PI));

// Values for lighting
const float LIGHT_STRENGTH = 20;
const float SPECULAR_EXPONENT = 32; // Note: increase this to 256 for cornell box
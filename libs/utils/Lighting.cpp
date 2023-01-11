#define _USE_MATH_DEFINES
#include <math.h>
#include <glm/glm.hpp>
#include <vector>

float calculateProximityLighting(glm::vec3 lightPosition, glm::vec3 point, float s) { // s = source strength
	// equation: S/(4*pi*r^2)
	float r = glm::length(point - lightPosition); // distnce from source (think of it as radius)
	float brightness = s / (4 * M_PI * pow(r, 2));
	brightness = std::min(float(1.0), brightness);
	brightness = std::max(float(0.0), brightness);
	return brightness;
}

float calculateIncidenceLighting(glm::vec3 shadowRay, glm::vec3 normal) {
	// equation = (N.V)
	float dot = glm::dot(normal, normalize(shadowRay));
	dot = std::min(float(1.0), dot);
	dot = std::max(float(0.0), dot);
	return dot;
}

float calculateSpecularLighting(glm::vec3 view, glm::vec3 incidentRay, glm::vec3 normal, float exponent) {
	// equation = (V.R_r)^(n) where R_r = R_i - 2N(R_i.N)
	float dot = glm::dot(glm::normalize(incidentRay), normal);
	glm::vec3 reflectedRay = glm::normalize(incidentRay) - (2.0f * normal * dot);
	float result = std::max(0.0f, glm::dot(glm::normalize(view), glm::normalize(reflectedRay)));
	return pow(result, exponent);
}
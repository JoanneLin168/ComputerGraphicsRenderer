#pragma once

#include <glm/glm.hpp>

float calculateProximityLighting(glm::vec3 lightPosition, glm::vec3 point, float s);

float calculateIncidenceLighting(glm::vec3 shadowRay, glm::vec3 normal);

float calculateSpecularLighting(glm::vec3 view, glm::vec3 incidentRay, glm::vec3 normal, float exponent);
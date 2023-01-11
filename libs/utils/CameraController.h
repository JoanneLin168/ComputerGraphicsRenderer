#pragma once

#include <string>
#include <glm/glm.hpp>

void translateCamera(std::string axis, float x, glm::mat4& cameraPosition);

void rotateCameraPosition(glm::mat4& cameraPosition, glm::mat4 rotationMatrix);

void rotateCamera(std::string axis, float theta, glm::mat4& cameraPosition);

void lookAt(glm::mat4& cameraPosition);
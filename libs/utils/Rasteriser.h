#pragma once

#include <CanvasPoint.h>
#include <TexturePoint.h>
#include <glm/glm.hpp>
#include <vector>

std::vector<CanvasPoint> interpolateCanvasPoints(CanvasPoint from, CanvasPoint to, float n);

std::vector<TexturePoint> interpolateTexturePoints(TexturePoint from, TexturePoint to, float n);

CanvasPoint getCanvasIntersectionPoint(glm::mat4 cameraPosition, glm::vec3 vertexPosition, float focalLength);
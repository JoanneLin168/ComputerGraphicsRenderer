#pragma once

#include <CanvasTriangle.h>
#include <DrawingWindow.h>
#include <vector>

std::vector<CanvasPoint> interpolateCanvasPoints(CanvasPoint from, CanvasPoint to, float n);

std::vector<TexturePoint> interpolateTexturePoints(TexturePoint from, TexturePoint to, float n);

CanvasPoint getCanvasIntersectionPoint(glm::mat4 cameraPosition, glm::vec3 vertexPosition, float focalLength);
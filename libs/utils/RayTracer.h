// NOTE: for some reason, it is faster if the ray tracing code was in Renderer.cpp

//#pragma once
//
//#include <glm/glm.hpp>
//#include <ModelTriangle.h>
//#include <RayTriangleIntersection.h>
//#include <vector>
//#include <mutex>
//
////RayTriangleIntersection getClosestIntersection(bool getAbsolute, glm::vec3 sourceOfRay, ModelTriangle triangle, glm::vec3 rayDirection, int index);
//
//bool isShadowRayBlocked(int index, glm::vec3 point, glm::mat4 cameraPosition, glm::vec3 lightPosition, std::vector<ModelTriangle> triangles, float focalLength);
//
//glm::vec3 getVertexNormal(std::vector<ModelTriangle> modelTriangles, glm::vec3 vertex);
//
//void cameraFireRays(std::vector<std::vector<RayTriangleIntersection>>& closestTriangleBuffer,
//	glm::mat4 cameraPosition,
//	std::vector<ModelTriangle> triangles,
//	float focalLength,
//	size_t startY,
//	size_t endY
//);
//
//void drawRayTracingScene(DrawingWindow& window, glm::vec3& lightPosition, glm::mat4& cameraPosition, std::vector<ModelTriangle> modelTriangles, float focalLength, ShadingType shadingType);

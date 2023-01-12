#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <Colour.h>
#include <ModelTriangle.h>
#include <CanvasPoint.h>
#include <CanvasTriangle.h>
#include <TextureMap.h>
#include <unordered_map>

std::vector<ModelTriangle> generateModelTriangles(
	std::vector<glm::vec3> vertices,
	std::vector<glm::vec3> facets,
	std::vector<glm::vec3> vertices_tx,
	std::vector<glm::vec3> facets_tx,
	std::vector<std::string> colourNames,
	std::unordered_map<std::string, Colour> coloursMap,
	std::unordered_map<std::string, TextureMap> texturesMap);
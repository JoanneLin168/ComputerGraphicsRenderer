#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <Colour.h>
#include <TextureMap.h>

std::tuple<std::vector<glm::vec3>, std::vector<glm::vec3>, std::vector<glm::vec3>, std::vector<glm::vec3>, std::vector<std::string>> readOBJFile(std::string filename, float scaleFactor);

std::tuple<std::unordered_map<std::string, Colour>, std::unordered_map<std::string, TextureMap>> readMTLFile(std::string filename);
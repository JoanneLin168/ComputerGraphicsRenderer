#include "Files.h"
#include <Utils.h>
#include <fstream>
#include <vector>

#include <glm/glm.hpp>
#include <iostream>
#include <string>
#include <unordered_map>
#include <Colour.h>
#include <TextureMap.h>

// Week 4 - Task 2: Read from the file and return the vertices and the facets of the model
// Week 5 - Task 2: Get texture vertices as well
std::tuple<std::vector<glm::vec3>, std::vector<glm::vec3>, std::vector<glm::vec3>, std::vector<glm::vec3>, std::vector<std::string>> readOBJFile(std::string filename, float scaleFactor) {
	std::ifstream file(filename);
	std::string line;

	// Instantiate vectors
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec3> facets;
	std::vector<glm::vec3> vertices_tx;
	std::vector<glm::vec3> facets_tx;
	std::vector<std::string> colours;

	// Add temp values
	glm::vec3 temp(0.0, 0.0, 0.0);
	vertices.push_back(temp);
	vertices_tx.push_back(temp);

	// Add vertices and facets to vectors
	std::string colourName = "Temp";
	bool getTextureFacets = false;
	while (std::getline(file, line)) {
		if (line.size() > 0) {
			if (line[line.size() - 1] == '\r') line = line.substr(0, line.size() - 1); // apparently Linux doesn't remove \r from \r\n for every new line..
		}

		if (split(line, ' ')[0] == "usemtl") {
			colourName = split(line, ' ')[1];
		}
		else if (line[0] == 'v' && split(line, ' ')[0] != "vt") { // For triangle vertices
			std::vector<std::string> verticesStr = split(line, ' ');
			glm::vec3 vs(std::stof(verticesStr[1]) * scaleFactor, std::stof(verticesStr[2]) * scaleFactor, std::stof(verticesStr[3]) * scaleFactor);
			vertices.push_back(vs);
		}
		else if (split(line, ' ')[0] == "vt") { // For texture vertices
			std::vector<std::string> verticesStr_tx = split(line, ' ');
			float v0 = std::stof(verticesStr_tx[1]);
			float v1 = std::stof(verticesStr_tx[2]);
			float v2 = 0; // temporary, you don't need to use this
			glm::vec3 vs(fmod(v0, 1.0f), fmod(v1, 1.0f), fmod(v2, 1.0f)); // according to slides, sometimes points are > 1.0, so use % to wrap-around
			vertices_tx.push_back(vs);
			getTextureFacets = true;
		}
		else if (line[0] == 'f') {
			std::vector<std::string> facetsStr = split(line, ' ');
			std::string x = facetsStr[1].substr(0, facetsStr[1].size() - 1);
			std::string y = facetsStr[2].substr(0, facetsStr[2].size() - 1);
			std::string z = facetsStr[3].substr(0, facetsStr[3].size() - 1);
			glm::vec3 fs(std::stoi(x), std::stoi(y), std::stoi(z));
			facets.push_back(fs);
			colours.push_back(colourName); // push colour for every facet of the same colour, so facet[i] has colours[i]

			if (getTextureFacets) {
				std::string x_tx = facetsStr[1].substr(facetsStr[1].size() - 1, facetsStr[1].size());
				std::string y_tx = facetsStr[2].substr(facetsStr[2].size() - 1, facetsStr[2].size());
				std::string z_tx = facetsStr[3].substr(facetsStr[3].size() - 1, facetsStr[3].size());
				glm::vec3 fs_tx(std::stoi(x_tx), std::stoi(y_tx), std::stoi(z_tx));
				facets_tx.push_back(fs_tx);
				getTextureFacets = false;
			}
		}
	}

	vertices.shrink_to_fit();
	facets.shrink_to_fit();
	vertices_tx.shrink_to_fit();
	facets_tx.shrink_to_fit();
	colours.shrink_to_fit();

	file.close();

	// Only for objects with no colours
	if (colours[1] == "Temp") {
		for (int i = 1; i < colours.size(); i++) {
			colours[i] = "Red";
		}
	}

	return std::make_tuple(vertices, facets, vertices_tx, facets_tx, colours);
}

// Week 4 - Task 3: Read mtl
// Week 5 - Task 2: Get textures as well
std::tuple<std::unordered_map<std::string, Colour>, std::unordered_map<std::string, TextureMap>> readMTLFile(std::string filename) {
	std::ifstream file(filename);
	std::string line;

	// Instantiate vectors
	std::unordered_map<std::string, Colour> coloursMap;
	std::unordered_map<std::string, TextureMap> texturesMap;

	// Add colours to hashmap
	std::string colourName = ""; // will be updated every time "newmtl" is read
	while (std::getline(file, line)) {
		if (line.size() > 0) {
			if (line[line.size() - 1] == '\r') line = line.substr(0, line.size() - 1); // apparently Linux doesn't remove \r from \r\n for every new line..
		}

		if (split(line, ' ')[0] == "newmtl") {
			colourName = split(line, ' ')[1];
		}
		else if (split(line, ' ')[0] == "Kd") {
			std::vector<std::string> rgbStr = split(line, ' ');
			int r = (int)round(std::stof(rgbStr[1]) * 255);
			int g = (int)round(std::stof(rgbStr[2]) * 255);
			int b = (int)round(std::stof(rgbStr[3]) * 255);

			Colour colour = Colour(r, g, b);
			colour.name = colourName;
			coloursMap[colourName] = colour;
		}
		else if (split(line, ' ')[0] == "map_Kd") {
			std::string filename = split(line, ' ')[1];
			TextureMap texture = TextureMap(filename);
			texturesMap[colourName] = texture;
		}
	}
	file.close();

	return std::make_tuple(coloursMap, texturesMap);
}
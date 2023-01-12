#include <glm/glm.hpp>
#include <vector>
#include <Colour.h>
#include <ModelTriangle.h>
#include <CanvasPoint.h>
#include <CanvasTriangle.h>
#include <TextureMap.h>
#include <unordered_map>

// Week 4 - Task 7: Create vector of ModelTriangles
std::vector<ModelTriangle> generateModelTriangles(
	std::vector<glm::vec3> vertices,
	std::vector<glm::vec3> facets,
	std::vector<glm::vec3> vertices_tx,
	std::vector<glm::vec3> facets_tx,
	std::vector<std::string> colourNames,
	std::unordered_map<std::string, Colour> coloursMap,
	std::unordered_map<std::string, TextureMap> texturesMap) {

	std::vector<ModelTriangle> results;
	int index = 0; // used to update the texture map
	for (size_t i = 0; i < facets.size(); i++) {
		glm::vec3 facet = facets[i];
		Colour colour = coloursMap[colourNames[i]];
		ModelTriangle triangle_3d = ModelTriangle(vertices[facet.x], vertices[facet.y], vertices[facet.z], colour);

		// Check if there is a texture for that colour and use texture if it exists
		if (texturesMap.count(colourNames[i])) {
			TextureMap texture = texturesMap[colourNames[i]];
			glm::vec3 txPointVec3_0 = vertices_tx[facets_tx[index].x];
			glm::vec3 txPointVec3_1 = vertices_tx[facets_tx[index].y];
			glm::vec3 txPointVec3_2 = vertices_tx[facets_tx[index].z];
			TexturePoint txPoint_0 = TexturePoint(txPointVec3_0.x * texture.width, txPointVec3_0.y * texture.height);
			TexturePoint txPoint_1 = TexturePoint(txPointVec3_1.x * texture.width, txPointVec3_1.y * texture.height);
			TexturePoint txPoint_2 = TexturePoint(txPointVec3_2.x * texture.width, txPointVec3_2.y * texture.height);
			triangle_3d.texturePoints = { txPoint_0, txPoint_1, txPoint_2 };
		}

		// Week 8 - Task 3: Calculate normals of every model triangle
		glm::vec3 normal = glm::normalize(glm::cross((triangle_3d.vertices[1] - triangle_3d.vertices[0]), (triangle_3d.vertices[2] - triangle_3d.vertices[0])));
		triangle_3d.normal = normal;
		results.push_back(triangle_3d);
	}
	results.shrink_to_fit();
	return results;
}
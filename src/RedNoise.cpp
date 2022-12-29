#include <CanvasTriangle.h>
#include <DrawingWindow.h>
#include <Utils.h>
#include <fstream>
#include <vector>

#include <glm/glm.hpp> // Week 2 - Task 4
#include <CanvasPoint.h> // Week 3 - Task 2
#include <Colour.h> // Week 3 - Task 2
#include <CanvasTriangle.h> // Week 3 - Task 3
#include <TextureMap.h> // Week 3 - Task 5
#include <string>
#include <ModelTriangle.h> // Week 4 - Task 2
#include <iostream> // Week 4 - Task 2
#include <fstream> // Week 4 - Task 2
#include <unordered_map> // Week 4 - Task 3

#include <RayTriangleIntersection.h> // Week 7 - Task 2 
#include <thread> // For multithreading
#include <mutex>

#define WIDTH 320
#define HEIGHT 240
#define SCALE 150 // used for scaling onto img canvas

// Values for translation and rotation
const float DIST = float(0.1);
const float ANGLE = float((1.0 / 360.0) * (2 * M_PI));

std::mutex mtx; // Used for raytracing

enum ShadingType {SHADING_FLAT, SHADING_GOURAUD, SHADING_PHONG};

// Week 3 - Task 4
// Interpolate - for rasterising
std::vector<CanvasPoint> interpolateCanvasPoints(CanvasPoint from, CanvasPoint to, float n) {
	int numberOfValues = ceil(n) + 1; // to ensure interpolation includes last value, needs to be ceil() to ensure more every value is captured

	std::vector<CanvasPoint> result;
	float xDiff = to.x - from.x;
	float yDiff = to.y - from.y;
	float zDiff = to.depth - from.depth;

	float xIncrement = xDiff / (numberOfValues - 1);
	float yIncrement = yDiff / (numberOfValues - 1);
	float zIncrement = zDiff / (numberOfValues - 1);

	for (float i = 0; i < numberOfValues; i++) {
		float x = from.x + (xIncrement * i);
		float y = from.y + (yIncrement * i);
		float z = from.depth + (zIncrement * i);
		result.push_back(CanvasPoint(x, y, z));
	}
	result.shrink_to_fit();

	return result;
}

// Interpolate - for rasterising
std::vector<TexturePoint> interpolateTexturePoints(TexturePoint from, TexturePoint to, float n) {
	int numberOfValues = ceil(n) + 1; // to ensure interpolation includes last value, needs to be ceil() to ensure more every value is captured

	std::vector<TexturePoint> result;
	float xDiff = to.x - from.x;
	float yDiff = to.y - from.y;

	float xIncrement = xDiff / (numberOfValues - 1);
	float yIncrement = yDiff / (numberOfValues - 1);

	for (float i = 0; i < numberOfValues; i++) {
		float x = from.x + long(xIncrement * i); // NOTE: textures have to use long for some reason
		float y = from.y + long(yIncrement * i);
		result.push_back(TexturePoint(x, y));
	}
	result.shrink_to_fit();

	return result;
}

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

// =================================================== RASTERISE ======================================================== //

// Week 4 - Task 5
CanvasPoint getCanvasIntersectionPoint(glm::mat4 cameraPosition, glm::vec3 vertexPosition, float focalLength) {
	glm::vec3 cameraPosVec3 = glm::vec3(cameraPosition[3][0], cameraPosition[3][1], cameraPosition[3][2]);
	glm::mat3 cameraOrientMat3 = glm::mat3(
		cameraPosition[0][0], cameraPosition[0][1], cameraPosition[0][2],
		cameraPosition[1][0], cameraPosition[1][1], cameraPosition[1][2],
		cameraPosition[2][0], cameraPosition[2][1], cameraPosition[2][2]
	);
	glm::vec3 distanceFromCamera = vertexPosition - cameraPosVec3;
	distanceFromCamera = distanceFromCamera * cameraOrientMat3;

	// Equations on website - W/2 and H/2 are shifts to centre the projection to the centre of the screen
	float x_2d = (focalLength * SCALE * (distanceFromCamera.x / -distanceFromCamera.z)) + (WIDTH / 2);
	float y_2d = (focalLength * SCALE * (distanceFromCamera.y / distanceFromCamera.z)) + (HEIGHT / 2);
	float z_2d = -1/(distanceFromCamera.z);

	CanvasPoint intersectionPoint = CanvasPoint(x_2d, y_2d, z_2d);

	return intersectionPoint;
}

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

// Week 4 - Task 7: Convert ModelTriangles to CanvasTriangles
std::vector<CanvasTriangle> getCanvasTrianglesFromModelTriangles(std::vector<ModelTriangle> modelTriangles, glm::mat4 cameraPosition, float focalLength) {
	std::vector<CanvasTriangle> canvasTriangles;
	for (ModelTriangle mTriangle : modelTriangles) {
		CanvasPoint a = getCanvasIntersectionPoint(cameraPosition, mTriangle.vertices[0], focalLength);
		CanvasPoint b = getCanvasIntersectionPoint(cameraPosition, mTriangle.vertices[1], focalLength);
		CanvasPoint c = getCanvasIntersectionPoint(cameraPosition, mTriangle.vertices[2], focalLength);
		CanvasTriangle cTriangle = CanvasTriangle(a, b, c);
		canvasTriangles.push_back(cTriangle);
	}
	canvasTriangles.shrink_to_fit();
	return canvasTriangles;
}

// ==================================== DRAW ======================================= //
void drawLine(DrawingWindow& window, CanvasPoint from, CanvasPoint to, Colour colour, std::vector<std::vector<float>>& depthArray) {
	float xDiff = to.x - from.x;
	float yDiff = to.y - from.y;
	float zDiff = to.depth - from.depth;
	int numberOfValues = ceil(std::max(abs(xDiff), abs(yDiff))) + 1; // to ensure interpolation includes last value, needs to be ceil() to ensure more every value is captured

	if (numberOfValues == 1) return;

	float xIncrement = xDiff / (numberOfValues - 1);
	float yIncrement = yDiff / (numberOfValues - 1);
	float zIncrement = zDiff / (numberOfValues - 1);

	for (float i = 0; i < numberOfValues; i++) {
		float x = from.x + (xIncrement * i);
		float y = from.y + (yIncrement * i);
		float z = from.depth + (zIncrement * i);
		int red = colour.red;
		int green = colour.green;
		int blue = colour.blue;
		uint32_t colour = (255 << 24) + (red << 16) + (green << 8) + blue;

		// If the distance is closer to the screen (1/z bigger than value in depthArray[y][x]) then draw pixel
		float depthInverse = z; // 1/-depth was done in getCanvasIntersectionPoint()
		if (ceil(y) >=0 && round(y) < HEIGHT && round(x) >= 0 && round(x) < WIDTH) {
			if (depthInverse > depthArray[round(y)][round(x)]) {
				depthArray[round(y)][round(x)] = depthInverse;
				window.setPixelColour(round(x), round(y), colour);
			}
		}
	}
}

// Week 3 - Task 5
void drawLineUsingTexture(DrawingWindow& window, CanvasPoint from, CanvasPoint to, TextureMap texture, std::vector<std::vector<float>>& depthArray) {
	// For triangle
	float xDiff = to.x - from.x;
	float yDiff = to.y - from.y;
	float zDiff = to.depth - from.depth;

	int numberOfValues = ceil(std::max(abs(xDiff), abs(yDiff))) + 1; // to ensure interpolation includes last value, needs to be ceil() to ensure more every value is captured
	float xIncrement = xDiff / (numberOfValues - 1);
	float yIncrement = yDiff / (numberOfValues - 1);
	float zIncrement = zDiff / (numberOfValues - 1);

	// For texture
	float xDiff_tx = to.texturePoint.x - from.texturePoint.x;
	float yDiff_tx = to.texturePoint.y - from.texturePoint.y;

	float xIncrement_tx = xDiff_tx / (numberOfValues - 1);
	float yIncrement_tx = yDiff_tx / (numberOfValues - 1);

	for (float i = 0; i < numberOfValues; i++) {
		float x = from.x + (xIncrement * i); 
		float y = from.y + (yIncrement * i);
		float z = from.depth + (zIncrement * i);
		float x_tx = from.texturePoint.x + long(xIncrement_tx * i); // NOTE: textures have to use long for some reason
		float y_tx = from.texturePoint.y + long(yIncrement_tx * i);
		float index = (y_tx * texture.width) + x_tx; // number of rows -> y, so use width, x is for the remainder along the row
		uint32_t colour = texture.pixels[round(index)];
		
		// If the distance is closer to the screen (1/z bigger than value in depthArray[y][x]) then draw pixel
		float depthInverse = z; // 1/-depth was done in getCanvasIntersectionPoint()
		if (round(y) >= 0 && round(y) < HEIGHT && round(x) >= 0 && round(x) < WIDTH) {
			if (depthInverse > depthArray[round(y)][round(x)]) {
				depthArray[round(y)][round(x)] = depthInverse;
				window.setPixelColour(round(x), round(y), colour);
			}
		}
	}
}

// Week 3 - Task 3
void drawTriangle(DrawingWindow& window, CanvasPoint a, CanvasPoint b, CanvasPoint c, Colour colour, std::vector<std::vector<float>>& depthArray) {
	drawLine(window, a, b, colour, depthArray); // Draw line a to b
	drawLine(window, a, c, colour, depthArray); // Draw line a to c
	drawLine(window, b, c, colour, depthArray); // Draw line b to c
}

// For CanvasTriangles
std::vector<CanvasPoint> sortPointsOnTriangleByHeight(CanvasTriangle triangle) {
	std::vector<CanvasPoint> sortedTrianglePoints;

	sortedTrianglePoints.push_back(triangle.v0());
	sortedTrianglePoints.push_back(triangle.v1());
	sortedTrianglePoints.push_back(triangle.v2());

	if (sortedTrianglePoints[0].y > sortedTrianglePoints[2].y) std::swap(sortedTrianglePoints[0], sortedTrianglePoints[2]);
	if (sortedTrianglePoints[0].y > sortedTrianglePoints[1].y) std::swap(sortedTrianglePoints[0], sortedTrianglePoints[1]);
	if (sortedTrianglePoints[1].y > sortedTrianglePoints[2].y) std::swap(sortedTrianglePoints[1], sortedTrianglePoints[2]);

	sortedTrianglePoints.shrink_to_fit();
	return sortedTrianglePoints;
}

// For ModelTriangles
std::vector<glm::vec3> sortPointsOnTriangleByHeight(ModelTriangle triangle) {
	std::vector<glm::vec3> sortedTrianglePoints;

	sortedTrianglePoints.push_back(triangle.vertices[0]);
	sortedTrianglePoints.push_back(triangle.vertices[1]);
	sortedTrianglePoints.push_back(triangle.vertices[2]);

	if (sortedTrianglePoints[0].y > sortedTrianglePoints[2].y) std::swap(sortedTrianglePoints[0], sortedTrianglePoints[2]);
	if (sortedTrianglePoints[0].y > sortedTrianglePoints[1].y) std::swap(sortedTrianglePoints[0], sortedTrianglePoints[1]);
	if (sortedTrianglePoints[1].y > sortedTrianglePoints[2].y) std::swap(sortedTrianglePoints[1], sortedTrianglePoints[2]);

	sortedTrianglePoints.shrink_to_fit();
	return sortedTrianglePoints;
}

// Week 4 - Task 9
void drawFilledTriangle(DrawingWindow& window, CanvasTriangle triangle, Colour colour, std::vector<std::vector<float>>& depthArray) {
	// start off by cutting triangle horizontally so there are two flat bottom triangles
	// fill each triangle from the line

	// Split triangle into two flat-bottom triangles
	std::vector<CanvasPoint> sortedPoints = sortPointsOnTriangleByHeight(triangle);
	CanvasPoint top = sortedPoints[0];
	CanvasPoint mid = sortedPoints[1];
	CanvasPoint bot = sortedPoints[2];

	// Get barrier; barrier = line that splits 2 triangles
	float midLength = mid.y - top.y;
	float ratioX = (bot.x - top.x) / (bot.y - top.y); // if you cut big triangle, little triangle is SIMILAR to big triangle, therefore, need to use ratio
	float ratioZ = (bot.depth - top.depth) / (bot.y - top.y);
	float barrierEndX = top.x + (midLength * ratioX);
	float barrierEndZ = top.depth + (midLength * ratioZ);
	CanvasPoint barrierStart = CanvasPoint(mid.x, mid.y, mid.depth);
	CanvasPoint barrierEnd = CanvasPoint(barrierEndX, mid.y, barrierEndZ);

	//   a
	//  d    c <- barrier
	// b

	// Interpolate between the vertices
	// Top triangle
	float numberOfValuesA = std::max(abs(mid.x - top.x), abs(mid.y - top.y)); // Note: +1 here, when you interpolate, -1 for dividing to get increment, but keep +1 for for loop to get last row
	std::vector<CanvasPoint> pointsAToC = interpolateCanvasPoints(top, barrierStart, numberOfValuesA);
	std::vector<CanvasPoint> pointsAToD = interpolateCanvasPoints(top, barrierEnd, numberOfValuesA);

	// Bottom triangle
	float numberOfValuesB = std::max(abs(bot.x - mid.x), abs(bot.y - mid.y)); // Note: +1 here, when you interpolate, -1 for dividing to get increment, but keep +1 for for loop to get last row
	std::vector<CanvasPoint> pointsBToC = interpolateCanvasPoints(bot, barrierStart, numberOfValuesB);
	std::vector<CanvasPoint> pointsBToD = interpolateCanvasPoints(bot, barrierEnd, numberOfValuesB);

	// Rasterise - refer to depthArray
	for (int i = 0; i < pointsAToC.size(); i++) {
		drawLine(window, pointsAToC[i], pointsAToD[i], colour, depthArray);
	}
	for (int i = 0; i < pointsBToC.size(); i++) {
		drawLine(window, pointsBToC[i], pointsBToD[i], colour, depthArray);
	}
	drawLine(window, barrierStart, barrierEnd, colour, depthArray);
	drawTriangle(window, top, mid, bot, colour, depthArray);
}

// Week 3 - Task 5
void drawTexturedTriangle(DrawingWindow& window, CanvasTriangle triangle, TextureMap texture, std::vector<std::vector<float>>& depthArray) {
	// start off by cutting triangle horizontally so there are two flat bottom triangles
	// fill each triangle from the line

	// Split triangle into two flat-bottom triangles
	std::vector<CanvasPoint> sortedPoints = sortPointsOnTriangleByHeight(triangle);
	CanvasPoint top = sortedPoints[0];
	CanvasPoint mid = sortedPoints[1];
	CanvasPoint bot = sortedPoints[2];

	// Draw barrier; barrier = line that splits 2 triangles
	// Think of x/y = X/Y for right-angle triangles. So to get x, it is y * X/Y, where y = midLength, X/Y = ratio. And then you offset to correct pos by adding top.x
	float midLength = mid.y - top.y;
	float ratio = (bot.x - top.x) / (bot.y - top.y); // dx/dy
	float barrierEndX = top.x + (midLength * ratio);
	CanvasPoint barrierStart = CanvasPoint(mid.x, mid.y);
	CanvasPoint barrierEnd = CanvasPoint(barrierEndX, mid.y);

	// Interpolate between the vertices
	// Top triangle
	float numberOfValuesA = std::max(abs(mid.x - top.x), abs(mid.y - top.y));
	std::vector<CanvasPoint> pointsAToC = interpolateCanvasPoints(top, barrierStart, numberOfValuesA);
	std::vector<CanvasPoint> pointsAToD = interpolateCanvasPoints(top, barrierEnd, numberOfValuesA);

	// Bottom triangle
	float numberOfValuesB = std::max(abs(bot.x - mid.x), abs(bot.y - mid.y));
	std::vector<CanvasPoint> pointsBToC = interpolateCanvasPoints(bot, barrierStart, numberOfValuesB);
	std::vector<CanvasPoint> pointsBToD = interpolateCanvasPoints(bot, barrierEnd, numberOfValuesB);

	//================================================== CREATE TRIANGLES ON TEXTURE =========================================================//
	// Get texture points
	TexturePoint top_tx = top.texturePoint;
	TexturePoint mid_tx = mid.texturePoint;
	TexturePoint bot_tx = bot.texturePoint;

	// Get barrier for texture
	// Note: ensure ratio to the barrier for textures is the same as the ratio to the barrier for the triangle
	float ratioOfEdge = (barrierEnd.y - top.y) / (bot.y - top.y);
	glm::vec2 topVec2 = glm::vec2(top_tx.x, top_tx.y);
	glm::vec2 botVec2 = glm::vec2(bot_tx.x, bot_tx.y);
	glm::vec2 edgeToCut = botVec2 - topVec2;
	glm::vec2 barrierEndVec2 = topVec2 + (edgeToCut * ratioOfEdge);
	TexturePoint barrierStart_tx = TexturePoint(mid_tx.x, mid_tx.y);
	TexturePoint barrierEnd_tx = TexturePoint(barrierEndVec2.x, barrierEndVec2.y);
	barrierStart.texturePoint = barrierStart_tx;
	barrierEnd.texturePoint = barrierEnd_tx;

	// Interpolate between the vertices
	// Top triangle
	std::vector<TexturePoint> pointsAToC_tx = interpolateTexturePoints(top_tx, barrierStart_tx, numberOfValuesA);
	std::vector<TexturePoint> pointsAToD_tx = interpolateTexturePoints(top_tx, barrierEnd_tx, numberOfValuesA);

	// Bottom triangle
	std::vector<TexturePoint> pointsBToC_tx = interpolateTexturePoints(bot_tx, barrierStart_tx, numberOfValuesB);
	std::vector<TexturePoint> pointsBToD_tx = interpolateTexturePoints(bot_tx, barrierEnd_tx, numberOfValuesB);

	//================================================== MAP TEXTURE PIXELS TO CANVAS =========================================================//
	// Top triangle
	for (int i = 0; i < numberOfValuesA; i++) {
		pointsAToC[i].texturePoint = pointsAToC_tx[i];
		pointsAToD[i].texturePoint = pointsAToD_tx[i];
	}
	// Bot triangle
	for (int i = 0; i < numberOfValuesB; i++) {
		pointsBToC[i].texturePoint = pointsBToC_tx[i];
		pointsBToD[i].texturePoint = pointsBToD_tx[i];
	}

	//================================================== DRAW PIXELS =========================================================//
	for (int i = 0; i < numberOfValuesA; i++) {
		drawLineUsingTexture(window, pointsAToC[i], pointsAToD[i], texture, depthArray);
	}
	for (int i = 0; i < numberOfValuesB; i++) {
		drawLineUsingTexture(window, pointsBToC[i], pointsBToD[i], texture, depthArray);
	}
	// Draw middle line
	//drawLineUsingTexture(window, barrierStart, barrierEnd, texture, depthArray);
}

// ==================================== TRANSFORM CAMERA ======================================= //
void translateCamera(std::string axis, float x, glm::mat4& cameraPosition) {
	glm::mat4 translationMatrix;
	if (axis == "X") {
		translationMatrix = glm::mat4(
			1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			x, 0, 0, 1
		);
	}
	else if (axis == "Y") {
		translationMatrix = glm::mat4(
			1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			0, x, 0, 1
		);
	}
	else if (axis == "Z") {
		translationMatrix = glm::mat4(
			1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			0, 0, x, 1
		);
	}
	cameraPosition = cameraPosition * translationMatrix;
}

void rotateCameraPosition(glm::mat4& cameraPosition, glm::mat4 rotationMatrix) {
	cameraPosition = rotationMatrix * cameraPosition;
}

void rotateCamera(std::string axis, float theta, glm::mat4& cameraPosition) {
	glm::mat4 rotationMatrix;
	if (axis == "X") {
		rotationMatrix = glm::mat4(
			1, 0, 0, 0,
			0, cos(theta), sin(theta), 0,
			0, -sin(theta), cos(theta), 0,
			0, 0, 0, 1
		);
	}
	else if (axis == "Y") {
		rotationMatrix = glm::mat4(
			cos(theta), 0, sin(theta), 0,
			0, 1, 0, 0,
			-sin(theta), 0, cos(theta), 0,
			0, 0, 0, 1
		);
	}
	else if (axis == "Z") {
		rotationMatrix = glm::mat4(
			cos(theta), sin(theta), 0, 0,
			-sin(theta), cos(theta), 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1
		);
	}
	rotateCameraPosition(cameraPosition, rotationMatrix);
}

// LookAt function
void lookAt(glm::mat4& cameraPosition) {
	// Variables in world
	glm::vec3 origin = glm::vec3(0.0, 0.0, 0.0);
	glm::vec3 vertical = glm::vec3(0.0, 1.0, 0.0);

	// forward, up and right are relative to the camera
	// vertical is relative to the world (0,1,0)
	glm::vec3 cameraPosVec3 = glm::vec3(cameraPosition[3][0], cameraPosition[3][1], cameraPosition[3][2]);
	glm::vec3 forward = glm::normalize(cameraPosVec3 - origin);
	glm::vec3 right = glm::cross(vertical, forward);
	glm::vec3 up = glm::cross(forward, right);
	cameraPosition = glm::mat4(glm::vec4(right, 0), glm::vec4(up, 0), glm::vec4(forward, 0), cameraPosition[3]);

}

// ================================================== Wireframe ============================================================= //
void drawWireframe(DrawingWindow& window, glm::mat4& cameraPosition, float focalLength, std::vector<ModelTriangle> modelTriangles) {
	window.clearPixels();

	std::vector<CanvasTriangle> canvasTriangles = getCanvasTrianglesFromModelTriangles(modelTriangles, cameraPosition, focalLength);

	// Create a depth array
	std::vector<std::vector<float>> depthArray(HEIGHT, std::vector<float>(WIDTH, 0));

	for (size_t i = 0; i < canvasTriangles.size(); i++) {
		CanvasTriangle triangle = canvasTriangles[i];
		Colour colour = Colour(255, 255, 255);
		drawTriangle(window, triangle.v0(), triangle.v1(), triangle.v2(), colour, depthArray);
	}
}

// ================================================== RASTERISED ============================================================= //

// Week 4 - Task 9
void drawRasterisedScene(
	DrawingWindow& window,
	glm::mat4& cameraPosition,
	float focalLength,
	std::vector<ModelTriangle> modelTriangles,
	std::unordered_map<std::string, Colour> coloursMap,
	std::unordered_map<std::string, TextureMap> texturesMap,
	std::vector<std::string> colourNames) {

	window.clearPixels();

	std::vector<CanvasTriangle> canvasTriangles = getCanvasTrianglesFromModelTriangles(modelTriangles, cameraPosition, focalLength);

	// Create a depth array
	std::vector<std::vector<float>> depthArray(HEIGHT, std::vector<float>(WIDTH, 0));

	for (size_t i = 0; i < canvasTriangles.size(); i++) {
		CanvasTriangle triangle = canvasTriangles[i];
		Colour colour = coloursMap[colourNames[i]];

		if (texturesMap.count(colourNames[i])) {
			TextureMap texture = texturesMap[colourNames[i]];
			triangle.v0().texturePoint = modelTriangles[i].texturePoints[0];
			triangle.v1().texturePoint = modelTriangles[i].texturePoints[1];
			triangle.v2().texturePoint = modelTriangles[i].texturePoints[2];
			drawTexturedTriangle(window, triangle, texture, depthArray);
		}
		else {
			drawFilledTriangle(window, triangle, colour, depthArray);
		}
	}
}

// =================================================== RAY TRACING ======================================================== //
// Week 7 - Task 2: Detect when and where a projected ray intersects with a model triangle
// Barycentric coordinates
RayTriangleIntersection getClosestIntersection(bool getAbsolute, glm::vec3 point, ModelTriangle triangle, glm::vec3 rayDirection, int index) {

	// point refers to either camera or point on triangle, depending on which function is using this function

	glm::vec3 e0 = triangle.vertices[1] - triangle.vertices[0];
	glm::vec3 e1 = triangle.vertices[2] - triangle.vertices[0];
	glm::vec3 SPVector = point - triangle.vertices[0];
	glm::mat3 DEMatrix(-rayDirection, e0, e1);

	// possibleSolution = (t,u,v) where
	//    t = absolute distance from camera to intersection point
	//    u = proportional distance along first edge
	//    v = proportional distance along second edge
	glm::vec3 possibleSolution = glm::inverse(DEMatrix) * SPVector;

	float t = getAbsolute ? abs(possibleSolution[0]) : possibleSolution[0];
	float u = possibleSolution[1];
	float v = possibleSolution[2];
	if ((u >= 0.0) && (u <= 1.0) && (v >= 0.0) && (v <= 1.0) && (double(u) + double(v)) <= 1.0 && t >= 0) { // masking u and v to avoid overflow
		glm::vec3 r = triangle.vertices[0] + (u * (triangle.vertices[1] - triangle.vertices[0])) + (v * (triangle.vertices[2] - triangle.vertices[0]));
		RayTriangleIntersection rayTriangleIntersection = RayTriangleIntersection(r, t, triangle, index);
		rayTriangleIntersection.t = t;
		rayTriangleIntersection.u = u;
		rayTriangleIntersection.v = v;
		return rayTriangleIntersection;
	}
	else {
		return RayTriangleIntersection(glm::vec3(), INFINITY, ModelTriangle(), -1);
	}
}

// ==================================== DRAW ======================================= //

bool isShadowRayBlocked(int index, glm::vec3 point, glm::mat4 cameraPosition, glm::vec3 lightPosition, std::vector<ModelTriangle> triangles, float focalLength) {
	// If shadow ray hits a triangle, then you need to draw a shadow
	for (int j = 0; j < triangles.size(); j++) {
		ModelTriangle triangleCompare = triangles[j];
		glm::vec3 shadowRay = lightPosition - point; // to - from, to=light, from=surface of triangle

		RayTriangleIntersection rayTriangleIntersection = getClosestIntersection(false, point, triangleCompare, shadowRay, j);

		// float pointToLight = glm::length(lightPosition - point);
		// check if there is an intersection with a triangle - no idea why it works if distance < 1 and not with pointToLight
		if (rayTriangleIntersection.distanceFromCamera < 1 && rayTriangleIntersection.triangleIndex != index) {
			return true;
		}
	}
	return false;
}

glm::vec3 getVertexNormal(std::vector<ModelTriangle> modelTriangles, glm::vec3 vertex) {
	std::vector<glm::vec3> neighbouringNormals;
	glm::vec3 neighbouringNormalsSum;

	for (ModelTriangle triangle : modelTriangles) {
		for (int i = 0; i < triangle.vertices.size(); i++) {
			if (vertex == triangle.vertices[i]) {
				neighbouringNormals.push_back(triangle.normal);
				neighbouringNormalsSum += triangle.normal;
			}
		}
	}

	neighbouringNormals.shrink_to_fit();
	int n = neighbouringNormals.size();
	glm::vec3 vertexNormal = glm::vec3(neighbouringNormalsSum.x / n, neighbouringNormalsSum.y / n, neighbouringNormalsSum.z / n);
	
	return vertexNormal;
}

void cameraFireRays(std::vector<std::vector<RayTriangleIntersection>>& closestTriangleBuffer,
	glm::mat4 cameraPosition,
	std::vector<ModelTriangle> triangles,
	float focalLength,
	size_t startY,
	size_t endY) {

	for (size_t y = startY; y < endY; y++) {
		for (size_t x = 0; x < WIDTH; x++) {
			int index = 0;
			RayTriangleIntersection closestRayTriangleIntersection = RayTriangleIntersection(glm::vec3(), INFINITY, ModelTriangle(), -1);

			// For every pixel, check to see which triangles are intersected by the ray, and find the closest one
			for (ModelTriangle triangle : triangles) {
				glm::vec3 cameraPosVec3 = glm::vec3(cameraPosition[3][0], cameraPosition[3][1], cameraPosition[3][2]);
				glm::vec3 rayDirectionOriented = glm::vec3(x, y, 0) - cameraPosVec3; // ray is from the camera to the scene, so to - from = scene - camera

				float x_3d = ((rayDirectionOriented.x - (WIDTH / 2)) * -rayDirectionOriented.z) / (focalLength * SCALE);
				float y_3d = ((rayDirectionOriented.y - (HEIGHT / 2)) * rayDirectionOriented.z) / (focalLength * SCALE);
				float z_3d = rayDirectionOriented.z;

				glm::mat3 cameraOrientMat3 = glm::mat3(
					cameraPosition[0][0], cameraPosition[0][1], cameraPosition[0][2],
					cameraPosition[1][0], cameraPosition[1][1], cameraPosition[1][2],
					cameraPosition[2][0], cameraPosition[2][1], cameraPosition[2][2]
				);
				glm::vec3 rayDirection = cameraOrientMat3 * glm::vec3(x_3d, y_3d, z_3d);

				RayTriangleIntersection rayTriangleIntersection = getClosestIntersection(true, cameraPosVec3, triangle, rayDirection, index);

				// check if point on triangle is closer to camera than the previously stored one
				if (rayTriangleIntersection.distanceFromCamera < closestRayTriangleIntersection.distanceFromCamera) {
					closestRayTriangleIntersection = rayTriangleIntersection;
				}

				index++;
			}

			// Draw point if the ray hit something
			if (closestRayTriangleIntersection.triangleIndex >= 0) {
				mtx.lock();
				closestTriangleBuffer[y][x] = closestRayTriangleIntersection;
				mtx.unlock();
			}
		}
	}
}

float calculateProximityLighting(glm::vec3 lightPosition, glm::vec3 point, float s) { // s = source strength
	// equation: 1/4*r^2
	float r = glm::length(point - lightPosition); // distnce from source (think of it as radius)
	float brightness = s / (4 * M_PI * pow(r, 2));
	brightness = std::min(float(1.0), brightness);
	brightness = std::max(float(0.0), brightness);
	return brightness;
}

float calculateDPAngleOfIncidence(glm::vec3 shadowRay, glm::vec3 normal) {
	float dot = glm::dot(normal, normalize(shadowRay));
	dot = std::min(float(1.0), dot);
	dot = std::max(float(0.0), dot);
	return dot;
}

float calculateSpecularExponent(glm::vec3 view, glm::vec3 incidentRay, glm::vec3 normal, float power) {
	float dot = glm::dot(glm::normalize(incidentRay), normal);
	glm::vec3 reflectedRay = incidentRay - (normal*2.0f) * dot;
	float result = glm::dot(glm::normalize(view), glm::normalize(reflectedRay));
	return pow(result, power);
}

void drawRayTracingScene(DrawingWindow& window, glm::vec3& lightPosition, glm::mat4& cameraPosition, std::vector<ModelTriangle> modelTriangles, float focalLength, ShadingType shadingType) {
	window.clearPixels();

	std::vector<std::vector<RayTriangleIntersection>> closestTriangleBuffer(HEIGHT, std::vector<RayTriangleIntersection>(WIDTH, RayTriangleIntersection())); // similar to depthArray, but stores index of closest triangle

	// Multithreading
	const size_t num_threads = std::thread::hardware_concurrency();
	std::vector<std::thread> threads;
	threads.reserve(num_threads);
	int sectionSize = HEIGHT / num_threads;

	// Call threads to check if there is an intersection with the camera
	for (int i = 0; i < num_threads; i++) {
		int startY = i * sectionSize;
		int endY = (i + 1) * sectionSize;
		threads.push_back(std::thread(cameraFireRays, std::ref(closestTriangleBuffer), cameraPosition, modelTriangles, focalLength, startY, endY));
	}

	// Wait for threads to finish tasks
	for (std::thread& t : threads) {
		t.join();
	}

	// Lighting and Shading
	for (size_t y = 0; y < HEIGHT; y++) {
		for (size_t x = 0; x < WIDTH; x++) {
			// Set up
			RayTriangleIntersection rayTriangleIntersection = closestTriangleBuffer[y][x];
			int index = rayTriangleIntersection.triangleIndex;
			ModelTriangle triangle = rayTriangleIntersection.intersectedTriangle;
			glm::vec3 point = rayTriangleIntersection.intersectionPoint; // point that the cameraRay intersected with the triangle

			// Get u,v,w for Gouraud and Phong shading (used for interpolations)
			// REFERENCE: https://www.scratchapixel.com/lessons/3d-basic-rendering/introduction-to-shading/shading-normals
			float u = rayTriangleIntersection.u;
			float v = rayTriangleIntersection.v;
			float w = 1 - (u + v);

			glm::vec3 v0 = triangle.vertices[0];
			glm::vec3 v1 = triangle.vertices[1];
			glm::vec3 v2 = triangle.vertices[2];

			glm::vec3 n0 = getVertexNormal(modelTriangles, v0);
			glm::vec3 n1 = getVertexNormal(modelTriangles, v1);
			glm::vec3 n2 = getVertexNormal(modelTriangles, v2);

			glm::vec3 hitNormal = (w * n0) + (u * n1) + (v * n2); // for Phong shading

			// Add lighting and shadows
			if (index != -1) {
				// General
				bool shadowRayBlocked = isShadowRayBlocked(index, point, cameraPosition, lightPosition, modelTriangles, focalLength);
				glm::vec3 shadowRay = lightPosition - point;
				glm::vec3 cameraPosVec3 = glm::vec3(cameraPosition[3][0], cameraPosition[3][1], cameraPosition[3][2]);
				glm::vec3 view = cameraPosVec3 - point; // vector to the camera from the point
				glm::vec3 lightRay = -shadowRay;

				// Lighting setup
				float proximityLighting = 0;
				float dotIncidence = 0;
				float specularExponent = 0;
				float brightness = 0;
				float ambience = (!shadowRayBlocked) ? 20 : 0;

				if (shadingType == SHADING_FLAT) {
					// Lighting
					proximityLighting = calculateProximityLighting(lightPosition, point, 10);
					dotIncidence = calculateDPAngleOfIncidence(shadowRay, triangle.normal);
					specularExponent = calculateSpecularExponent(view, lightRay, triangle.normal, 256);
				}

				else if (shadingType == SHADING_GOURAUD) {
					// 1. Get vertex normals by getting average of neighbouring facet normals
					// 2. Get illumination for each vertex point
					// 3. Use linear interpolation to apply intensities across polygons
					float proximityLighting0 = calculateProximityLighting(lightPosition, n0, 10);
					float proximityLighting1 = calculateProximityLighting(lightPosition, n1, 10);
					float proximityLighting2 = calculateProximityLighting(lightPosition, n2, 10);
					float dotIncidence0 = calculateDPAngleOfIncidence(shadowRay, n0);
					float dotIncidence1 = calculateDPAngleOfIncidence(shadowRay, n1);
					float dotIncidence2 = calculateDPAngleOfIncidence(shadowRay, n2);
					float specularExponent0 = calculateSpecularExponent(view, lightRay, n0, 256);
					float specularExponent1 = calculateSpecularExponent(view, lightRay, n1, 256);
					float specularExponent2 = calculateSpecularExponent(view, lightRay, n2, 256);

					proximityLighting = (w * proximityLighting0) + (u * proximityLighting1) + (v * proximityLighting2);
					dotIncidence = (w * dotIncidence0) + (u * dotIncidence1) + (v * dotIncidence2);
					specularExponent = (w * specularExponent0) + (u * specularExponent1) + (v * specularExponent2);
				}

				else if (shadingType == SHADING_PHONG) {
					// 1. Get vertex normals by getting average of neighbouring facet normals
					// 2. Get illumination for each vertex point
					// 3. Use linear interpolation to get normals for each point
					proximityLighting = calculateProximityLighting(lightPosition, point, 10);
					dotIncidence = calculateDPAngleOfIncidence(shadowRay, hitNormal);
					specularExponent = calculateSpecularExponent(view, lightRay, hitNormal, 256);
				}

				// Colour in
				brightness = proximityLighting * dotIncidence;
				float r = 0;
				float g = 0;
				float b = 0;
				if (triangle.colour.red > 0)   r = ambience + triangle.colour.red * brightness;
				if (triangle.colour.green > 0) g = ambience + triangle.colour.green * brightness;
				if (triangle.colour.blue > 0)  b = ambience + triangle.colour.blue * brightness;

				if (dotIncidence > 0 && proximityLighting > 0) {
					r += 255 * specularExponent;
					g += 255 * specularExponent;
					b += 255 * specularExponent;
				}
				r = glm::clamp(r, 0.0f, 255.0f);
				g = glm::clamp(g, 0.0f, 255.0f);
				b = glm::clamp(b, 0.0f, 255.0f);

				uint32_t colour = (255 << 24) + (int(r) << 16) + (int(g) << 8) + int(b);
				window.setPixelColour(floor(x), ceil(y), colour);
			}
		}
	}
}

void changeMode(int& mode) {
	if (mode < 2) mode++;
	else mode = 0;
	std::cout << "Switched to mode: " << mode << std::endl;
}

void handleEvent(SDL_Event event, DrawingWindow &window, int& mode, glm::vec3& lightPosition, glm::mat4 &cameraPosition, bool& toOrbit, ShadingType& shadingType) {
	if (event.type == SDL_KEYDOWN) {
		// Translation
		if (event.key.keysym.sym == SDLK_d) translateCamera("X", DIST, cameraPosition);
		else if (event.key.keysym.sym == SDLK_a) translateCamera("X", -DIST, cameraPosition);
		else if (event.key.keysym.sym == SDLK_w) translateCamera("Y", DIST, cameraPosition);
		else if (event.key.keysym.sym == SDLK_s) translateCamera("Y", -DIST, cameraPosition);
		else if (event.key.keysym.sym == SDLK_q) translateCamera("Z", DIST, cameraPosition);
		else if (event.key.keysym.sym == SDLK_e) translateCamera("Z", -DIST, cameraPosition);

		// Rotation
		else if (event.key.keysym.sym == SDLK_l) rotateCamera("X", ANGLE, cameraPosition);
		else if (event.key.keysym.sym == SDLK_j) rotateCamera("X", -ANGLE, cameraPosition);
		else if (event.key.keysym.sym == SDLK_i) rotateCamera("Y", ANGLE, cameraPosition);
		else if (event.key.keysym.sym == SDLK_k) rotateCamera("Y", -ANGLE, cameraPosition);
		else if (event.key.keysym.sym == SDLK_u) rotateCamera("Z", ANGLE, cameraPosition);
		else if (event.key.keysym.sym == SDLK_o) rotateCamera("Z", -ANGLE, cameraPosition);

		// Move light
		else if (event.key.keysym.sym == SDLK_UP) lightPosition[1] += 0.01;
		else if (event.key.keysym.sym == SDLK_DOWN) lightPosition[1] -= 0.01;
		else if (event.key.keysym.sym == SDLK_RIGHT) lightPosition[2] += 0.01;
		else if (event.key.keysym.sym == SDLK_LEFT) lightPosition[2] -= 0.01;

		// Switch between modes
		else if (event.key.keysym.sym == SDLK_r) changeMode(mode);

		// Switch between shading types
		else if (event.key.keysym.sym == SDLK_1) { shadingType = SHADING_FLAT; std::cout << "Shading mode: Flat Shading" << std::endl; }
		else if (event.key.keysym.sym == SDLK_2) { shadingType = SHADING_GOURAUD; std::cout << "Shading mode: Gouraud Shading" << std::endl; }
		else if (event.key.keysym.sym == SDLK_3) { shadingType = SHADING_PHONG; std::cout << "Shading mode: Phong Shading" << std::endl; }

		// Toggle orbit
		else if (event.key.keysym.sym == SDLK_SPACE) toOrbit = !toOrbit;

		// Debug key
		else if (event.key.keysym.sym == SDLK_BACKSLASH) {
			glm::vec3 cameraPosVec3 = glm::vec3(cameraPosition[3][0], cameraPosition[3][1], cameraPosition[3][2]);
			std::cout << "Camera Position:" << cameraPosVec3.x << " " << cameraPosVec3.y << " " << cameraPosVec3.z << std::endl;
			std::cout << "Light Position:" << lightPosition.x << " " << lightPosition.y << " " << lightPosition.z << std::endl;
		}
	} else if (event.type == SDL_MOUSEBUTTONDOWN) {
		window.savePPM("output.ppm");
		window.saveBMP("output.bmp");
	}
}

int main(int argc, char *argv[]) {
	DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);
	SDL_Event event;

	// Week 4 - Task 2
	std::string objFile = "textured-cornell-box.obj";
	std::tuple<std::vector<glm::vec3>, std::vector<glm::vec3>, std::vector<glm::vec3>, std::vector<glm::vec3>, std::vector<std::string>> objResult = readOBJFile(objFile, 0.35);
	std::vector<glm::vec3> vertices = std::get<0>(objResult);
	std::vector<glm::vec3> facets = std::get<1>(objResult);
	std::vector<glm::vec3> vertices_tx = std::get<2>(objResult);
	std::vector<glm::vec3> facets_tx = std::get<3>(objResult);
	std::vector<std::string> colourNames = std::get<4>(objResult);

	// Week 4 - Task 3
	std::string mtlFile = "textured-cornell-box.mtl";
	std::tuple<std::unordered_map<std::string, Colour>, std::unordered_map<std::string, TextureMap>> mtlMaps = readMTLFile(mtlFile);
	std::unordered_map<std::string, Colour> coloursMap = std::get<0>(mtlMaps);
	std::unordered_map<std::string, TextureMap> texturesMap = std::get<1>(mtlMaps);

	std::vector<ModelTriangle> modelTriangles = generateModelTriangles(vertices, facets, vertices_tx, facets_tx, colourNames, coloursMap, texturesMap);

	// Variables for camera
	glm::mat4 cameraPosition = glm::mat4(
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 4, 1 // last column vector stores x, y, z
	);
	float focalLength = 2.0;
	bool toOrbit = false;
	int mode = 0;

	// Light needs to be slightly less than 1 or else it will always pass the triangle for the light
	glm::vec3 lightPosition = glm::vec3(0.0, 0.8, 1.0); // TODO: change this?
	ShadingType shadingType = SHADING_FLAT;

	while (true) {
		// We MUST poll for events - otherwise the window will freeze !
		if (window.pollForInputEvents(event)) handleEvent(event, window, mode, lightPosition, cameraPosition, toOrbit, shadingType);
		if (mode == 0) drawWireframe(window, cameraPosition, focalLength, modelTriangles);
		else if (mode == 1) drawRasterisedScene(window, cameraPosition, focalLength, modelTriangles, coloursMap, texturesMap, colourNames);
		else if (mode == 2) drawRayTracingScene(window, lightPosition, cameraPosition, modelTriangles, focalLength, shadingType);

		// Orbit and LookAt
		if (toOrbit) rotateCamera("Y", ANGLE, cameraPosition);
		//lookAt(cameraPosition);

		// Need to render the frame at the end, or nothing actually gets shown on the screen !
		window.renderFrame();
	}
}

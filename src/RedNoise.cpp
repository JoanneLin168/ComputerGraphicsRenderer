#include <CanvasTriangle.h>
#include <DrawingWindow.h>
#include <Utils.h>
#include <fstream>
#include <vector>

#include <glm/glm.hpp> // Week 2 - Task 4
#include <CanvasPoint.h> // Week 3 - Task 2
#include <Colour.h> // Week 3 - Task 2
#include <CanvasTriangle.h> // Week 3 - Task 3
#include <string>
#include <ModelTriangle.h> // Week 4 - Task 2
#include <iostream> // Week 4 - Task 2
#include <fstream> // Week 4 - Task 2
#include <unordered_map> // Week 4 - Task 3

#include <RayTriangleIntersection.h> // Week 7 - Task 2 
#include <thread> // For multithreading
#include <mutex>

//#define WIDTH 320
//#define HEIGHT 240
//#define SCALE 150 // used for scaling onto img canvas

#define WIDTH 320
#define HEIGHT 240
#define SCALE 150 // used for scaling onto img canvas


// Values for translation and rotation
const float DIST = float(0.1);
const float ANGLE = float((1.0 / 360.0) * (2 * M_PI));

std::mutex mtx; // Used for raytracing

enum ShadingType {SHADING_FLAT, SHADING_GOURAUD, SHADING_PHONG};

// Week 2 - Task 4 
std::vector<glm::vec3> interpolateThreeElementValues(glm::vec3 from, glm::vec3 to, int numberOfValues) {
	std::vector<glm::vec3> result;
	float xDiff = to.x - from.x;
	float yDiff = to.y - from.y;
	float zDiff = to.z - from.z;

	float xIncrement = xDiff / numberOfValues;
	float yIncrement = yDiff / numberOfValues;
	float zIncrement = zDiff / numberOfValues;

	for (float i = 0; i < numberOfValues; i++) {
		float x = from.x + float(xIncrement * i);
		float y = from.y + float(yIncrement * i);
		float z = from.z + float(zIncrement * i);
		result.push_back(glm::vec3(x, y, z));
	}
	result.shrink_to_fit();

	return result;
}

// Week 3 - Task 4
// Interpolate - for rasterising
std::vector<CanvasPoint> interpolateCanvasPoints(CanvasPoint from, CanvasPoint to, float numberOfValues) {
	std::vector<CanvasPoint> result;
	float xDiff = to.x - from.x;
	float yDiff = to.y - from.y;
	float zDiff = to.depth - from.depth;

	float xIncrement = xDiff / numberOfValues;
	float yIncrement = yDiff / numberOfValues;
	float zIncrement = zDiff / numberOfValues;

	for (float i = 0; i < numberOfValues; i++) {
		float x = from.x + float(xIncrement * i);
		float y = from.y + float(yIncrement * i);
		float z = from.depth + float(zIncrement * i);
		result.push_back(CanvasPoint(x, y, z));
	}
	result.shrink_to_fit();

	return result;
}

// Week 4 - Task 2: Read from the file and return the vertices and the facets of the model
std::tuple<std::vector<glm::vec3>, std::vector<glm::vec3>, std::vector<std::string>> readOBJFile(std::string filename, float scaleFactor) {
	std::ifstream file(filename);
	std::string line;

	// Instantiate vectors
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec3> facets;
	std::vector<std::string> colours;

	// Add temp values
	glm::vec3 temp(0.0, 0.0, 0.0);
	vertices.push_back(temp);

	// Add vertices and facets to vectors
	std::string colourName = "Temp";
	while (std::getline(file, line)) {
		// Output the text from the file
		if (split(line, ' ')[0] == "usemtl") {
			colourName = split(line, ' ')[1];
			colourName.pop_back(); // removes any weird remaining whitespace at the end
		}
		else if (line[0] == 'v') {
			std::vector<std::string> verticesStr = split(line, ' ');
			glm::vec3 vs(std::stof(verticesStr[1]) * scaleFactor, std::stof(verticesStr[2]) * scaleFactor, std::stof(verticesStr[3]) * scaleFactor);
			vertices.push_back(vs);
		}
		else if (line[0] == 'f') {
			std::vector<std::string> facetsStr = split(line, ' ');
			std::string x = facetsStr[1].substr(0, facetsStr[1].size() - 1);
			std::string y = facetsStr[2].substr(0, facetsStr[2].size() - 1);
			std::string z = facetsStr[3].substr(0, facetsStr[3].size() - 1);
			glm::vec3 fs(std::stoi(x), std::stoi(y), std::stoi(z));
			facets.push_back(fs);
			colours.push_back(colourName); // push colour for every facet of the same colour, so facet[i] has colours[i]
		}
	}

	vertices.shrink_to_fit();
	facets.shrink_to_fit();
	colours.shrink_to_fit();

	file.close();

	// Only for objects with no colours
	if (colours[1] == "Temp") {
		for (int i = 1; i < colours.size(); i++) {
			colours[i] = "Red";
		}
	}

	return std::make_tuple(vertices, facets, colours);
}

// Week 4 - Task 3: Read mtl
std::unordered_map<std::string, Colour> readMTLFile(std::string filename) {
	std::ifstream file(filename);
	std::string line;

	// Instantiate vectors
	std::unordered_map<std::string, Colour> coloursMap;

	// Add colours to hashmap
	std::string colourName = ""; // will be updated every time "newmtl" is read
	while (std::getline(file, line)) {
		// Output the text from the file
		if (split(line, ' ')[0] == "newmtl") {
			colourName = split(line, ' ')[1];
			colourName.pop_back(); // removes any weird remaining whitespace at the end
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
	}
	file.close();

	return coloursMap;
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
std::vector<ModelTriangle> generateModelTriangles(std::vector<glm::vec3> vertices,
	std::vector<glm::vec3> facets,
	std::vector<std::string> colourNames,
	std::unordered_map<std::string, Colour> coloursMap) {

	std::vector<ModelTriangle> results;
	for (size_t i = 0; i < facets.size(); i++) {
		glm::vec3 facet = facets[i];
		/*int xIndex = facet.x;
		int yIndex = facet.y;
		int zIndex = facet.z;*/
		Colour colour = coloursMap[colourNames[i]];
		ModelTriangle triangle_3d = ModelTriangle(vertices[facet.x], vertices[facet.y], vertices[facet.z], colour);

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
	float numberOfValues = std::max(abs(xDiff), abs(yDiff));

	float xIncrement = xDiff / numberOfValues;
	float yIncrement = yDiff / numberOfValues;
	float zIncrement = zDiff / numberOfValues;

	for (size_t i = 0; i < numberOfValues; i++) {
		float x = float(from.x + float(xIncrement * i));
		float y = float(from.y + float(yIncrement * i));
		float z = float(from.depth + float(zIncrement * i));
		float red = (float)colour.red;
		float green = (float)colour.green;
		float blue = (float)colour.blue;
		uint32_t colour = (255 << 24) + (int(red) << 16) + (int(green) << 8) + int(blue);

		// If the distance is closer to the screen (1/z bigger than value in depthArray[y][x]) then draw pixel
		float depthInverse = z; // 1/-depth was done in getCanvasIntersectionPoint()
		if (ceil(y) >=0 && ceil(y) < HEIGHT && floor(x) >= 0 && floor(x) < WIDTH) {
			if (depthInverse > depthArray[ceil(y)][floor(x)]) {
				depthArray[(size_t)ceil(y)][(size_t)floor(x)] = depthInverse;
				window.setPixelColour((size_t)floor(x), (size_t)ceil(y), colour);
			}
			else if (depthArray[(int)ceil(y)][(int)floor(x)] == 0) {
				depthArray[(size_t)ceil(y)][(size_t)floor(x)] = depthInverse;
				window.setPixelColour((size_t)floor(x), (size_t)ceil(y), colour);
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
	float numberOfValuesA = mid.y - top.y;
	std::vector<CanvasPoint> pointsAToC = interpolateCanvasPoints(top, barrierStart, numberOfValuesA);
	std::vector<CanvasPoint> pointsAToD = interpolateCanvasPoints(top, barrierEnd, numberOfValuesA);

	// Bottom triangle
	float numberOfValuesB = bot.y - mid.y;
	std::vector<CanvasPoint> pointsBToC = interpolateCanvasPoints(bot, barrierStart, numberOfValuesB);
	std::vector<CanvasPoint> pointsBToD = interpolateCanvasPoints(bot, barrierEnd, numberOfValuesB);

	// Rasterise - refer to depthArray
	for (size_t i = 0; i < pointsAToC.size(); i++) {
		drawLine(window, pointsAToC[i], pointsAToD[i], colour, depthArray);
	}
	for (size_t i = 0; i < pointsBToC.size(); i++) {
		drawLine(window, pointsBToC[i], pointsBToD[i], colour, depthArray);
	}
	drawLine(window, barrierStart, barrierEnd, colour, depthArray);
	drawTriangle(window, triangle.v0(), triangle.v1(), triangle.v2(), colour, depthArray);
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
void drawRasterisedScene(DrawingWindow& window, glm::mat4& cameraPosition, float focalLength, std::vector<ModelTriangle> modelTriangles, std::unordered_map<std::string, Colour> coloursMap, std::vector<std::string> colourNames) {
	window.clearPixels();

	std::vector<CanvasTriangle> canvasTriangles = getCanvasTrianglesFromModelTriangles(modelTriangles, cameraPosition, focalLength);

	// Create a depth array
	std::vector<std::vector<float>> depthArray(HEIGHT, std::vector<float>(WIDTH, 0));

	for (size_t i = 0; i < canvasTriangles.size(); i++) {
		CanvasTriangle triangle = canvasTriangles[i];
		Colour colour = coloursMap[colourNames[i]];
		drawFilledTriangle(window, triangle, colour, depthArray);
	}
}

// =================================================== RAY TRACING ======================================================== //
// Week 7 - Task 2: Detect when and where a projected ray intersects with a model triangle
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
	if ((u >= 0.0) && (u <= 1.0) && (v >= 0.0) && (v <= 1.0) && (double(u) + double(v)) <= 1.0 && t >= 0) {
		glm::vec3 r = triangle.vertices[0] + (u * (triangle.vertices[1] - triangle.vertices[0])) + (v * (triangle.vertices[2] - triangle.vertices[0]));
		return RayTriangleIntersection(r, t, triangle, index);
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

glm::vec3 getVertexNormal(glm::vec3 vertex, std::vector<ModelTriangle> modelTriangles) {
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
	glm::vec3 vertexNormal = float(1 / neighbouringNormals.size()) * neighbouringNormalsSum;

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

// Get ratios of two subtriangles from two edges of the triangle and the point
std::tuple<float, float, float> getUVWAreaRatios(glm::vec3 v0, glm::vec3 v1, glm::vec3 v2, glm::vec3 point) {
	glm::vec3 edgeA = v1 - v0;
	glm::vec3 edgeB = v2 - v0;
	float area = 0.5f * glm::length(glm::cross(edgeA, edgeB));

	// Get areas of subtriangles (triangles formed by points and edgeA and edgeB)
	// For subtriangle A
	glm::vec3 edgeA1 = v0 - point;
	glm::vec3 edgeA2 = v1 - point;
	float areaA = 0.5f * glm::length(glm::cross(edgeA1, edgeA2));

	// For subtriangle B
	glm::vec3 edgeB1 = v0 - point;
	glm::vec3 edgeB2 = v2 - point;
	float areaB = 0.5f * glm::length(glm::cross(edgeB1, edgeB2));

	// Get u, v and w
	float u = areaA / area;
	float v = areaB / area;
	float w = 1 - (u + v);

	return std::make_tuple(u, v, w);
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

			glm::vec3 v0 = triangle.vertices[0];
			glm::vec3 v1 = triangle.vertices[1];
			glm::vec3 v2 = triangle.vertices[2];

			glm::vec3 v0_normal = getVertexNormal(v0, modelTriangles);
			glm::vec3 v1_normal = getVertexNormal(v1, modelTriangles);
			glm::vec3 v2_normal = getVertexNormal(v2, modelTriangles);

			// Add lighting and shadows
			if (index != -1) {
				// General
				bool shadowRayBlocked = isShadowRayBlocked(index, point, cameraPosition, lightPosition, modelTriangles, focalLength);
				glm::vec3 shadowRay = lightPosition - point;
				glm::vec3 cameraPosVec3 = glm::vec3(cameraPosition[3][0], cameraPosition[3][1], cameraPosition[3][2]);
				glm::vec3 view = point - cameraPosVec3; // vector from the point to the camera
				glm::vec3 lightRay = -shadowRay;

				if (shadingType == SHADING_FLAT) {
					// Lighting
					float ambience = 50;
					float proximityLighting = calculateProximityLighting(lightPosition, point, 10);
					float dotIncidence = calculateDPAngleOfIncidence(shadowRay, triangle.normal);
					float specularExponent = calculateSpecularExponent(view, lightRay, triangle.normal, 128);
					float brightness = proximityLighting * dotIncidence + specularExponent;

					float r = float(triangle.colour.red);
					float g = float(triangle.colour.green);
					float b = float(triangle.colour.blue);

					if (specularExponent < proximityLighting * dotIncidence) {
						r = std::max(float(0), std::min(float(triangle.colour.red), ambience + triangle.colour.red * brightness));
						g = std::max(float(0), std::min(float(triangle.colour.green), ambience + triangle.colour.green * brightness));
						b = std::max(float(0), std::min(float(triangle.colour.blue), ambience + triangle.colour.blue * brightness));
					}
					else {
						r = 255 * specularExponent;
						g = 255 * specularExponent;
						b = 255 * specularExponent;
					}

					uint32_t colour = (255 << 24) + (int(r) << 16) + (int(g) << 8) + int(b);
					window.setPixelColour(floor(x), ceil(y), colour);
				}

				else if (shadingType == SHADING_GOURAUD) {
					// 1. Get vertex normals by getting average of neighbouring facet normals
					// 2. Get illumination for each vertex point
					// 3. Use linear interpolation to apply intensities across polygons
					std::tuple<float, float, float> uvw = getUVWAreaRatios(v0, v1, v2, point);
					float u = std::get<0>(uvw);
					float v = std::get<1>(uvw);
					float w = std::get<2>(uvw);

					bool shadowRayBlocked = isShadowRayBlocked(index, point, cameraPosition, lightPosition, modelTriangles, focalLength);
					glm::vec3 shadowRay = lightPosition - point;
					glm::vec3 cameraPosVec3 = glm::vec3(cameraPosition[3][0], cameraPosition[3][1], cameraPosition[3][2]);
					glm::vec3 view = point - cameraPosVec3; // vector from the point to the camera
					glm::vec3 lightRay = -shadowRay;

					// Lighting
					float proximityLighting0 = calculateProximityLighting(lightPosition, point, 5);
					float proximityLighting1 = calculateProximityLighting(lightPosition, point, 5);
					float proximityLighting2 = calculateProximityLighting(lightPosition, point, 5);
					float dotIncidence0      = calculateDPAngleOfIncidence(shadowRay, v0_normal);
					float dotIncidence1      = calculateDPAngleOfIncidence(shadowRay, v1_normal);
					float dotIncidence2      = calculateDPAngleOfIncidence(shadowRay, v2_normal);
					float specularExponent0  = calculateSpecularExponent(view, lightRay, v0_normal, 128);
					float specularExponent1  = calculateSpecularExponent(view, lightRay, v1_normal, 128);
					float specularExponent2  = calculateSpecularExponent(view, lightRay, v2_normal, 128);

					float ambience = 50;
					float proximityLighting = (u * proximityLighting0) + (v * proximityLighting1) + (w * proximityLighting2);
					float dotIncidence = (u * dotIncidence0) + (v * dotIncidence1) + (w * dotIncidence2);
					float specularExponent = (u * specularExponent0) + (v * specularExponent1) + (w * specularExponent2);
					float brightness = proximityLighting * dotIncidence + specularExponent;

					float r = float(triangle.colour.red);
					float g = float(triangle.colour.green);
					float b = float(triangle.colour.blue);

					/*if (specularExponent < proximityLighting * dotIncidence) {
						r = std::max(float(0), std::min(float(triangle.colour.red), ambience + triangle.colour.red * brightness));
						g = std::max(float(0), std::min(float(triangle.colour.green), ambience + triangle.colour.green * brightness));
						b = std::max(float(0), std::min(float(triangle.colour.blue), ambience + triangle.colour.blue * brightness));
					}
					else {
						r = 255 * specularExponent;
						g = 255 * specularExponent;
						b = 255 * specularExponent;
					}*/

					r = std::max(float(0), std::min(float(triangle.colour.red), ambience + triangle.colour.red * brightness));
					g = std::max(float(0), std::min(float(triangle.colour.green), ambience + triangle.colour.green * brightness));
					b = std::max(float(0), std::min(float(triangle.colour.blue), ambience + triangle.colour.blue * brightness));

					uint32_t colour = (255 << 24) + (int(r) << 16) + (int(g) << 8) + int(b);
					window.setPixelColour(floor(x), ceil(y), colour);
				}
			}
		}
	}
}

void changeMode(int& mode) {
	std::cout << "Switching mode: " << mode + 1 << std::endl;
	if (mode < 2) mode++;
	else mode = 0;
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
		else if (event.key.keysym.sym == SDLK_1) shadingType = SHADING_FLAT;
		else if (event.key.keysym.sym == SDLK_2) shadingType = SHADING_GOURAUD;
		else if (event.key.keysym.sym == SDLK_3) shadingType = SHADING_PHONG;

		// Toggle orbit
		else if (event.key.keysym.sym == SDLK_SPACE) toOrbit = !toOrbit;

		// Debug key
		else if (event.key.keysym.sym == SDLK_0) {
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
	std::string objFile = "cornell-box.obj";
	std::tuple<std::vector<glm::vec3>, std::vector<glm::vec3>, std::vector<std::string>> objResult = readOBJFile(objFile, 0.35);
	std::vector<glm::vec3> vertices = std::get<0>(objResult);
	std::vector<glm::vec3> facets = std::get<1>(objResult);
	std::vector<std::string> colourNames = std::get<2>(objResult);

	// Week 4 - Task 3
	std::string mtlFile = "cornell-box.mtl";
	std::unordered_map<std::string, Colour> coloursMap = readMTLFile(mtlFile);

	std::vector<ModelTriangle> modelTriangles = generateModelTriangles(vertices, facets, colourNames, coloursMap);

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
	glm::vec3 lightPosition = glm::vec3(0.0, 0.9, 0.0); // TODO: change this?
	ShadingType shadingType = SHADING_FLAT;

	while (true) {
		// We MUST poll for events - otherwise the window will freeze !
		if (window.pollForInputEvents(event)) handleEvent(event, window, mode, lightPosition, cameraPosition, toOrbit, shadingType);

		if (mode == 0) drawWireframe(window, cameraPosition, focalLength, modelTriangles);
		else if (mode == 1) drawRasterisedScene(window, cameraPosition, focalLength, modelTriangles, coloursMap, colourNames);
		else if (mode == 2) drawRayTracingScene(window, lightPosition, cameraPosition, modelTriangles, focalLength, shadingType);

		// Orbit and LookAt
		if (toOrbit) rotateCamera("Y", ANGLE, cameraPosition);
		//lookAt(cameraPosition);

		// Tmp: Light position
		uint32_t colour = (255 << 24) + (int(0) << 16) + (int(255) << 8) + int(0);
		CanvasPoint light = getCanvasIntersectionPoint(cameraPosition, lightPosition, focalLength);
		window.setPixelColour(floor(light.x), ceil(light.y), colour);

		// Need to render the frame at the end, or nothing actually gets shown on the screen !
		window.renderFrame();
	}
}

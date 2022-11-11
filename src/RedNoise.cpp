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

#define WIDTH 320
#define HEIGHT 240
#define SCALE 150 // used for scaling onto img canvas

// Values for translation and rotation
const float DIST = float(0.1);
const float ANGLE = float((1.0 / 360.0) * (2 * M_PI));

std::mutex mtx; // Used for raytracing

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
	std::string colourName = "Temp"; // tmp - to be updated whenever "usemtl" is shown
	while (std::getline(file, line)) {
		// Output the text from the file
		if (split(line, ' ')[0] == "usemtl") {
			colourName = split(line, ' ')[1];
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
			std::vector<std::string> colourNameStr = split(line, ' ');
			colourName = colourNameStr[1];
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
	float z_2d = -(distanceFromCamera.z);

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
		float depthInverse = 1 / z; // negative was changed in getCanvasIntersectionPoint()
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

// Week 4 - Task 9
void drawRasterisedScene(DrawingWindow& window, std::vector<CanvasTriangle> triangles, std::unordered_map<std::string, Colour> coloursMap, std::vector<std::string> colourNames) {
	window.clearPixels();

	// Create a depth array
	std::vector<std::vector<float>> depthArray(HEIGHT, std::vector<float>(WIDTH, 0));

	for (size_t i = 0; i < triangles.size(); i++) {
		CanvasTriangle triangle = triangles[i];
		Colour colour = coloursMap[colourNames[i]];
		drawFilledTriangle(window, triangle, colour, depthArray);
	}
}

// =================================================== RAY TRACING ======================================================== //
// Week 7 - Task 2: Detect when and where a projected ray intersects with a model triangle
RayTriangleIntersection getClosestIntersection(glm::vec3 point, ModelTriangle triangle, glm::vec3 rayDirection, int index) {

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

	float t = possibleSolution[0];
	float u = possibleSolution[1];
	float v = possibleSolution[2];
	if ((u >= 0.0) && (u <= 1.0) && (v >= 0.0) && (v <= 1.0) && (double(u) + double(v)) <= 1.0 && t >= 0) {
		glm::vec3 r = triangle.vertices[0] + (u * (triangle.vertices[1] - triangle.vertices[0])) + (v * (triangle.vertices[2] - triangle.vertices[0]));
		return RayTriangleIntersection(r, t, triangle, (size_t)index);
	}
	else {
		return RayTriangleIntersection(glm::vec3(), INFINITY, ModelTriangle(), -1);
	}

	
}

// ==================================== DRAW ======================================= //

std::vector<glm::vec3> getPointsOnModelTriangle(ModelTriangle triangle) {
	// Split triangle into two flat-bottom triangles
	if (triangle.vertices[0].y > triangle.vertices[2].y) std::swap(triangle.vertices[0], triangle.vertices[2]);
	if (triangle.vertices[0].y > triangle.vertices[1].y) std::swap(triangle.vertices[0], triangle.vertices[1]);
	if (triangle.vertices[1].y > triangle.vertices[2].y) std::swap(triangle.vertices[1], triangle.vertices[2]);

	glm::vec3 top = triangle.vertices[0];
	glm::vec3 mid = triangle.vertices[1];
	glm::vec3 bot = triangle.vertices[2];

	// Get barrier; barrier = line that splits 2 triangles
	float midLength = mid.y - top.y;
	float ratioX = (bot.x - top.x) / (bot.y - top.y);
	float ratioZ = (bot.z - top.z) / (bot.y - top.y);
	float barrierEndX = top.x + (midLength * ratioX);
	float barrierEndZ = top.z + (midLength * ratioZ);
	glm::vec3 barrierStart = glm::vec3(mid.x, mid.y, mid.z);
	glm::vec3 barrierEnd = glm::vec3(barrierEndX, mid.y, barrierEndZ);

	// Interpolate between the vertices
	// Top triangle
	float numberOfValuesA = (mid.y - top.y) * 100;
	std::vector<glm::vec3> pointsAToC = interpolateThreeElementValues(top, barrierStart, numberOfValuesA);
	std::vector<glm::vec3> pointsAToD = interpolateThreeElementValues(top, barrierEnd, numberOfValuesA);
	std::vector<glm::vec3> points;
	for (int i = 0; i < pointsAToC.size(); i++) {
		glm::vec3 to = pointsAToD[i];
		glm::vec3 from = pointsAToC[i];
		float xDiff = to.x - from.x;
		float yDiff = to.y - from.y;
		float zDiff = to.z - from.z;
		float numberOfValues = std::max(abs(xDiff), abs(yDiff));
		numberOfValues = std::max(numberOfValues, abs(zDiff)) * 100;
		std::vector<glm::vec3> line = interpolateThreeElementValues(from, to, numberOfValues);
		for (glm::vec3 point : line) points.push_back(point);
	}

	// Bottom triangle
	float numberOfValuesB = (bot.y - mid.y) * 100;
	std::vector<glm::vec3> pointsBToC = interpolateThreeElementValues(bot, barrierStart, numberOfValuesB);
	std::vector<glm::vec3> pointsBToD = interpolateThreeElementValues(bot, barrierEnd, numberOfValuesB);
	std::vector<glm::vec3> botHalf;
	for (int i = 0; i < pointsBToC.size(); i++) {
		glm::vec3 to = pointsBToD[i];
		glm::vec3 from = pointsBToC[i];
		float xDiff = to.x - from.x;
		float yDiff = to.y - from.y;
		float zDiff = to.z - from.z;
		float numberOfValues = std::max(abs(xDiff), abs(yDiff));
		numberOfValues = std::max(numberOfValues, abs(zDiff)) * 100;
		std::vector<glm::vec3> line = interpolateThreeElementValues(from, to, numberOfValues);
		for (glm::vec3 point : line) points.push_back(point);
	}

	points.shrink_to_fit();
	return points;
}

void lightFireRays(DrawingWindow& window,
	glm::mat4 cameraPosition,
	glm::mat4 lightPosition,
	std::vector<ModelTriangle> triangles,
	float focalLength) {

	// surface to the light, if blocked, it has shadow
	// to do this, do two for loops, so for each triangle, check with every other triangle to see if there is an intersection
	// if there is, then that point on the triangle has a shadow

	// For every pixel, check to see which triangles are intersected by the ray, and find the closest one
	for (int i = 0; i < triangles.size(); i++) {
		ModelTriangle triangle = triangles[i];

		std::vector<glm::vec3> points = getPointsOnModelTriangle(triangle);
		for (glm::vec3 point : points) {
			// Check if the shadow ray passes any triangle
			RayTriangleIntersection closestRayTriangleIntersection = RayTriangleIntersection(glm::vec3(), INFINITY, ModelTriangle(), -1);

			// For every pixel, check to see which triangles are intersected by the ray, and find the closest one
			for (int j = 0; j < triangles.size(); j++) {
				ModelTriangle triangleCompare = triangles[j];
				glm::vec3 lightPosVec3 = glm::vec3(lightPosition[3][0], lightPosition[3][1], lightPosition[3][2]);
				glm::mat3 lightOrientMat3 = glm::mat3(
					lightPosition[0][0], lightPosition[0][1], lightPosition[0][2],
					lightPosition[1][0], lightPosition[1][1], lightPosition[1][2],
					lightPosition[2][0], lightPosition[2][1], lightPosition[2][2]
				);
				glm::vec3 shadowRay = lightOrientMat3 * (lightPosVec3 - point); // to - from, to=light, from=surface of triangle

				// todo: change getClosestIntersection to account for things in between, i.e. triangleCompare. Not too sure it does that rn
				RayTriangleIntersection rayTriangleIntersection = getClosestIntersection(point, triangleCompare, shadowRay, j);

				// check if there is an intersection
				if (rayTriangleIntersection.triangleIndex >= 0 && rayTriangleIntersection.triangleIndex != i) {
					// TODO: dont rasterise?
					// calculate 2d pixels and set to black
					CanvasPoint canvasPoint = getCanvasIntersectionPoint(cameraPosition, point, focalLength);
					uint32_t colour = (255 << 24) + (int(0) << 16) + (int(0) << 8) + int(0);
					window.setPixelColour(floor(canvasPoint.x), ceil(canvasPoint.y), colour);
				}
			}
		}
	}
}

void cameraFireRays(std::vector<std::vector<int>>& closestTriangleBuffer,
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

				RayTriangleIntersection rayTriangleIntersection = getClosestIntersection(cameraPosVec3, triangle, rayDirection, index);

				// check if point on triangle is closer to camera than the previously stored one
				if (rayTriangleIntersection.distanceFromCamera < closestRayTriangleIntersection.distanceFromCamera) {
					closestRayTriangleIntersection = rayTriangleIntersection;
				}

				index++;
			}

			// Draw point if the ray hit something
			if (closestRayTriangleIntersection.triangleIndex >= 0) {
				mtx.lock();
				closestTriangleBuffer[y][x] = closestRayTriangleIntersection.triangleIndex;
				mtx.unlock();
			}
		}
	}
}

void drawRayTracingScene(DrawingWindow& window, glm::mat4& cameraPosition, std::vector<ModelTriangle> triangles, float focalLength) {
	window.clearPixels();

	std::vector<std::vector<int>> closestTriangleBuffer(HEIGHT, std::vector<int>(WIDTH, -1)); // similar to depthArray, but stores index of closest triangle

	// Multithreading
	const size_t num_threads = std::thread::hardware_concurrency();
	std::vector<std::thread> threads;
	threads.reserve(num_threads);
	int sectionSize = HEIGHT / num_threads;

	// Call threads
	for (int i = 0; i < num_threads; i++) {
		int startY = i * sectionSize;
		int endY = (i + 1) * sectionSize;
		threads.push_back(std::thread(cameraFireRays, std::ref(closestTriangleBuffer), cameraPosition, triangles, focalLength, startY, endY));
	}

	// Wait for threads to finish tasks
	for (std::thread& t : threads) {
		t.join();
	}

	// Single-threaded
	/*std::thread t1(cameraFireRays, std::ref(closestTriangleBuffer), cameraPosition, triangles, focalLength, 0, HEIGHT);
	t1.join();*/

	for (size_t y = 0; y < HEIGHT; y++) {
		for (size_t x = 0; x < WIDTH; x++) {
			int index = closestTriangleBuffer[y][x];
			if (index > -1) {
				ModelTriangle triangle = triangles[index];
				uint32_t colour = (255 << 24) + (int(triangle.colour.red) << 16) + (int(triangle.colour.green) << 8) + int(triangle.colour.blue);
				window.setPixelColour(x, y, colour);
			}
		}
	}

	// Add shadows
	glm::mat4 lightPosition = glm::mat4(
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0.9, 0, 1 // right-most column vector for x,y,z
	);
	lightFireRays(window, cameraPosition, lightPosition, triangles, focalLength);

	// Temp: Draw dot for light
	CanvasPoint canvasPoint = getCanvasIntersectionPoint(cameraPosition, glm::vec3(0,0.9,0), focalLength);
	uint32_t colour = (255 << 24) + (int(255) << 16) + (int(0) << 8) + int(0);
	window.setPixelColour(floor(canvasPoint.x), ceil(canvasPoint.y), colour);
}


// REFERENCE: http://www.cs.nott.ac.uk/~pszqiu/Teaching/Courses/G5BAGR/Slides/4-transform.pdf
void handleEvent(SDL_Event event, DrawingWindow &window, glm::mat4 &cameraPosition, bool& toOrbit) {
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

		else if (event.key.keysym.sym == SDLK_SPACE) toOrbit = !toOrbit;
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

	while (true) {
		// We MUST poll for events - otherwise the window will freeze !
		if (window.pollForInputEvents(event)) handleEvent(event, window, cameraPosition, toOrbit);

		// Get triangles
		//std::vector<CanvasTriangle> canvasTriangles = getCanvasTrianglesFromModelTriangles(modelTriangles, cameraPosition, focalLength);

		// Rasterise
		//drawRasterisedScene(window, canvasTriangles, coloursMap, colourNames);
		
		// Raytracing
		drawRayTracingScene(window, cameraPosition, modelTriangles, focalLength);

		// Orbit and LookAt
		if (toOrbit) {
			rotateCamera("Y", ANGLE, cameraPosition);
		}

		lookAt(cameraPosition);

		// Need to render the frame at the end, or nothing actually gets shown on the screen !
		window.renderFrame();
	}
}

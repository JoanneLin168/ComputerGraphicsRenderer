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

#define WIDTH 320
#define HEIGHT 240
#define SCALE 150 // used for scaling onto img canvas

// Values for translation and rotation
const float DIST = 0.1;
const float ANGLE = (1.0 / 360.0) * (2 * M_PI);

// Clear screen
void clearWindow(DrawingWindow& window) {
	uint32_t colour = (255 << 24) + (0 << 16) + (0 << 8) + 0;
	for (int y = 0; y < HEIGHT; y++) {
		for (int x = 0; x < WIDTH; x++) {
			window.setPixelColour(x, y, colour);
		}
	}
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
			std::vector<std::string> facetsStr = split(line, '/ ');
			glm::vec3 fs(std::stoi(facetsStr[1]), std::stoi(facetsStr[2]), std::stoi(facetsStr[3]));
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
			int r = round(std::stof(rgbStr[1]) * 255);
			int g = round(std::stof(rgbStr[2]) * 255);
			int b = round(std::stof(rgbStr[3]) * 255);

			Colour colour = Colour(r, g, b);
			colour.name = colourName;
			coloursMap[colourName] = colour;
		}
	}
	file.close();

	return coloursMap;
}

// Week 4 - Task 5
CanvasPoint getCanvasIntersectionPoint(glm::mat4 cameraPosition, glm::vec3 vertexPosition, float focalLength) {
	float x_3d = vertexPosition.x;
	float y_3d = vertexPosition.y;
	float z_3d = vertexPosition.z;

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
	float z_2d = -(z_3d + distanceFromCamera.z); // need to shift the z backwards so that the z values start from the plane

	CanvasPoint intersectionPoint = CanvasPoint(x_2d, y_2d, z_2d);

	return intersectionPoint;
}

// Week 7 - Task 2: Detect when and where a projected ray intersects with a model triangle
RayTriangleIntersection getClosestIntersection(glm::mat4 cameraPosition, ModelTriangle triangle, glm::vec3 rayDirection, int index) {
	glm::vec3 cameraPosVec3 = glm::vec3(cameraPosition[3][0], cameraPosition[3][1], cameraPosition[3][2]);

	glm::vec3 e0 = triangle.vertices[1] - triangle.vertices[0];
	glm::vec3 e1 = triangle.vertices[2] - triangle.vertices[0];
	glm::vec3 SPVector = cameraPosVec3 - triangle.vertices[0];
	glm::mat3 DEMatrix(-rayDirection, e0, e1);

	// possibleSolution = (t,u,v) where
	//    t = absolute distance from camera to intersection point
	//    u = proportional distance along first edge
	//    v = proportional distance along second edge
	glm::vec3 possibleSolution = glm::inverse(DEMatrix) * SPVector;

	float t = possibleSolution[1];
	float u = possibleSolution[1];
	float v = possibleSolution[2];
	glm::vec3 r;
	if ((u >= 0.0) && (u <= 1.0) && (v >= 0.0) && (v <= 1.0) && (u + v) <= 1.0 && t >= 0) {
		r = triangle.vertices[0] + (u * (triangle.vertices[1] - triangle.vertices[0])) + (v * (triangle.vertices[2] - triangle.vertices[0]));
	}

	return RayTriangleIntersection(r, t, triangle, (size_t)index);
}

// Week 4 - Task 7: Create vector of ModelTriangles
std::vector<ModelTriangle> generateModelTriangles(std::vector<glm::vec3> vertices,
	std::vector<glm::vec3> facets,
	std::vector<std::string> colourNames,
	std::unordered_map<std::string, Colour> coloursMap) {
	std::vector<ModelTriangle> results;
	for (int i = 0; i < facets.size(); i++) {
		glm::vec3 facet = facets[i];
		int xIndex = facet.x;
		int yIndex = facet.y;
		int zIndex = facet.z;
		Colour colour = coloursMap[colourNames[i]];
		ModelTriangle triangle_3d = ModelTriangle(vertices[xIndex], vertices[yIndex], vertices[zIndex], colour);
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
void draw(DrawingWindow& window, CanvasPoint from, CanvasPoint to, Colour colour, std::vector<std::vector<float>>& depthArray) {
	float xDiff = to.x - from.x;
	float yDiff = to.y - from.y;
	float zDiff = to.depth - from.depth;
	float numberOfValues = std::max(abs(xDiff), abs(yDiff));

	float xIncrement = xDiff / numberOfValues;
	float yIncrement = yDiff / numberOfValues;
	float zIncrement = zDiff / numberOfValues;

	for (float i = 0; i < numberOfValues; i++) {
		float x = float(from.x + float(xIncrement * i));
		float y = float(from.y + float(yIncrement * i));
		float z = float(from.depth + float(zIncrement * i));
		float red = colour.red;
		float green = colour.green;
		float blue = colour.blue;
		uint32_t colour = (255 << 24) + (int(red) << 16) + (int(green) << 8) + int(blue);

		// If the distance is closer to the screen (1/z bigger than value in depthArray[y][x]) then draw pixel
		float depthInverse = 1 / z; // negative was changed in getCanvasIntersectionPoint()
		if (ceil(y) >=0 && ceil(y) < HEIGHT && floor(x) >= 0 && floor(x) < WIDTH) {
			if (depthInverse > depthArray[ceil(y)][floor(x)]) {
				depthArray[ceil(y)][floor(x)] = depthInverse;
				window.setPixelColour(floor(x), ceil(y), colour);
			}
			else if (depthArray[ceil(y)][floor(x)] == 0) {
				depthArray[ceil(y)][floor(x)] = depthInverse;
				window.setPixelColour(floor(x), ceil(y), colour);
			}
		}
	}
}

// Week 3 - Task 3
void drawTriangle(DrawingWindow& window, CanvasPoint a, CanvasPoint b, CanvasPoint c, Colour colour, std::vector<std::vector<float>>& depthArray) {
	draw(window, a, b, colour, depthArray); // Draw line a to b
	draw(window, a, c, colour, depthArray); // Draw line a to c
	draw(window, b, c, colour, depthArray); // Draw line b to c
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
	for (int i = 0; i < pointsAToC.size(); i++) {
		draw(window, pointsAToC[i], pointsAToD[i], colour, depthArray);
	}
	for (int i = 0; i < pointsBToC.size(); i++) {
		draw(window, pointsBToC[i], pointsBToD[i], colour, depthArray);
	}
	draw(window, barrierStart, barrierEnd, colour, depthArray);
	drawTriangle(window, triangle.v0(), triangle.v1(), triangle.v2(), colour, depthArray);
}

// Week 4 - Task 9
void draw3D(DrawingWindow& window, std::vector<CanvasTriangle> triangles, std::unordered_map<std::string, Colour> coloursMap, std::vector<std::string> colourNames) {
	clearWindow(window);

	// Create a depth array
	std::vector<std::vector<float>> depthArray(HEIGHT, std::vector<float>(WIDTH, 0));

	for (int i = 0; i < triangles.size(); i++) {
		CanvasTriangle triangle = triangles[i];
		Colour colour = coloursMap[colourNames[i]];
		drawFilledTriangle(window, triangle, colour, depthArray);
	}
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

	// Variables in world
	glm::vec3 origin = glm::vec3(0.0, 0.0, 0.0);
	glm::vec3 vertical = glm::vec3(0.0, 1.0, 0.0);

	// Variables for camera
	glm::mat4 cameraPosition = glm::mat4(
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 4, 1 // last column vector stores x, y, z
	);
	float focalLength = 2.0;
	bool toOrbit = true;

	while (true) {
		// We MUST poll for events - otherwise the window will freeze !
		if (window.pollForInputEvents(event)) handleEvent(event, window, cameraPosition, toOrbit);

		// Get triangles
		std::vector<ModelTriangle> modelTriangles = generateModelTriangles(vertices, facets, colourNames, coloursMap);
		std::vector<CanvasTriangle> canvasTriangles = getCanvasTrianglesFromModelTriangles(modelTriangles, cameraPosition, focalLength);

		// Draw the triangles
		draw3D(window, canvasTriangles, coloursMap, colourNames);

		// Orbit
		glm::mat4 rotationMatrixY = glm::mat4(
			cos(ANGLE), 0, sin(ANGLE), 0,
			0, 1, 0, 0,
			-sin(ANGLE), 0, cos(ANGLE), 0,
			0, 0, 0, 1
		);
		if (toOrbit) {
			rotateCameraPosition(cameraPosition, rotationMatrixY);
		}

		// LookAt
		// forward, up and right are relative to the camera
		// vertical is relative to the world (0,1,0)
		glm::vec3 cameraPosVec3 = glm::vec3(cameraPosition[3][0], cameraPosition[3][1], cameraPosition[3][2]);
		glm::vec3 forward = glm::normalize(cameraPosVec3 - origin);
		glm::vec3 right = glm::cross(vertical, forward);
		glm::vec3 up = glm::cross(forward, right);
		cameraPosition = glm::mat4(glm::vec4(right, 0), glm::vec4(up, 0), glm::vec4(forward, 0), cameraPosition[3]);

		// Need to render the frame at the end, or nothing actually gets shown on the screen !
		window.renderFrame();
	}
}

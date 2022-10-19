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

#define WIDTH 320
#define HEIGHT 240
#define SCALE 150 // used for scaling onto img canvas

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

	float xIncrement = xDiff / numberOfValues;
	float yIncrement = yDiff / numberOfValues;

	for (float i = 0; i < numberOfValues; i++) {
		float x = from.x + float(xIncrement * i);
		float y = from.y + float(yIncrement * i);
		result.push_back(CanvasPoint(x, y));
	}
	result.shrink_to_fit();

	return result;
}

// Week 3 - Task 2
void drawLine(DrawingWindow& window, CanvasPoint from, CanvasPoint to, Colour colour) {
	float xDiff = to.x - from.x;
	float yDiff = to.y - from.y;
	float numberOfValues = std::max(abs(xDiff), abs(yDiff));

	float xIncrement = xDiff / numberOfValues;
	float yIncrement = yDiff / numberOfValues;

	for (float i = 0; i < numberOfValues; i++) {
		float x = float(from.x + float(xIncrement * i));
		float y = float(from.y + float(yIncrement * i));
		float red = colour.red;
		float green = colour.green;
		float blue = colour.blue;
		uint32_t colour = (255 << 24) + (int(red) << 16) + (int(green) << 8) + int(blue);
		window.setPixelColour(floor(x), ceil(y), colour);
	}
}

// Week 3 - Task 3
void drawTriangle(DrawingWindow& window, CanvasPoint a, CanvasPoint b, CanvasPoint c, Colour colour) {
	drawLine(window, a, b, colour); // Draw line a to b
	drawLine(window, a, c, colour); // Draw line a to c
	drawLine(window, b, c, colour); // Draw line b to c
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

// Week 3 - Task 4
void drawFilledTriangle(DrawingWindow& window, CanvasTriangle triangle, Colour colour) {
	// start off by cutting triangle horizontally so there are two flat bottom triangles
	// fill each triangle from the line

	// Split triangle into two flat-bottom triangles
	std::vector<CanvasPoint> sortedPoints = sortPointsOnTriangleByHeight(triangle);
	CanvasPoint top = sortedPoints[0];
	CanvasPoint mid = sortedPoints[1];
	CanvasPoint bot = sortedPoints[2];

	// Draw barrier; barrier = line that splits 2 triangles
	float midLength = mid.y - top.y;
	float ratio = (bot.x - top.x) / (bot.y - top.y); // if you cut big triangle, little triangle is SIMILAR to big triangle, therefore, need to use ratio
	float barrierEndX = top.x + (midLength * ratio);
	CanvasPoint barrierStart = CanvasPoint(mid.x, mid.y);
	CanvasPoint barrierEnd = CanvasPoint(barrierEndX, mid.y);

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

	// Rasterise
	for (int i = 0; i < pointsAToC.size(); i++) {
		drawLine(window, pointsAToC[i], pointsAToD[i], colour);
	}
	for (int i = 0; i < pointsBToC.size(); i++) {
		drawLine(window, pointsBToC[i], pointsBToD[i], colour);
	}
	drawLine(window, barrierStart, barrierEnd, colour);
	drawTriangle(window, triangle.v0(), triangle.v1(), triangle.v2(), colour);
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
CanvasPoint getCanvasIntersectionPoint(glm::vec3 cameraPosition, glm::vec3 vertexPosition, float focalLength) {
	float x_3d = vertexPosition.x;
	float y_3d = vertexPosition.y;
	float z_3d = vertexPosition.z;

	// Equations on website - W/2 and H/2 are shifts to centre the projection to the centre of the screen
	float x_2d = (focalLength * -SCALE * (x_3d / (z_3d - cameraPosition.z))) + (WIDTH / 2);
	float y_2d = (focalLength * SCALE * (y_3d / (z_3d - cameraPosition.z))) + (HEIGHT / 2);

	CanvasPoint intersectionPoint = CanvasPoint(x_2d, y_2d);

	return intersectionPoint;
}

// Week 4 - Task 6
void renderPointCloud(DrawingWindow& window, std::vector<glm::vec3> vertices, glm::vec3 cameraPosition, float focalLength) {
	std::vector<CanvasPoint> canvasPoints;
	for (int i = 1; i < vertices.size(); i++) {
		glm::vec3 vertex = vertices[i];
		CanvasPoint intersectionPoint = getCanvasIntersectionPoint(cameraPosition, vertex, focalLength);
		canvasPoints.push_back(intersectionPoint);
	}
	canvasPoints.shrink_to_fit();

	// Draw points
	for (float i = 0; i < canvasPoints.size(); i++) {
		float x = canvasPoints[i].x;
		float y = canvasPoints[i].y;

		int red = 255;
		int green = 255;
		int blue = 255;
		uint32_t colour = (255 << 24) + (int(red) << 16) + (int(green) << 8) + int(blue);
		window.setPixelColour(round(x), ceil(y), colour);
	}
}

// Week 4 - Task 7: Create vector of ModelTriangles
std::vector<ModelTriangle> generateModelTriangles(std::vector<glm::vec3> vertices,
												  std::vector<glm::vec3> facets,
												  std::vector<std::string> colours,
												  std::unordered_map<std::string, Colour> coloursMap) {
	std::vector<ModelTriangle> results;
	for (int i = 0; i < facets.size(); i++) {
		glm::vec3 facet = facets[i];
		int xIndex = facet.x;
		int yIndex = facet.y;
		int zIndex = facet.z;
		Colour colour = coloursMap[colours[i]];
		ModelTriangle triangle_3d = ModelTriangle(vertices[xIndex], vertices[yIndex], vertices[zIndex], colour);
		results.push_back(triangle_3d);
	}
	results.shrink_to_fit();
	return results;
}

// Week 4 - Task 7: Convert ModelTriangles to CanvasTriangles
std::vector<CanvasTriangle> getCanvasTrianglesFromModelTriangles(std::vector<ModelTriangle> modelTriangles, glm::vec3 cameraPosition, float focalLength) {
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

// ==================================== RANDOM ======================================= //
// Generate Random objects
CanvasPoint createRandomPoint() {
	float x = rand() % WIDTH;
	float y = rand() % HEIGHT;
	return CanvasPoint(x, y);
}

CanvasTriangle createRandomTriangle() {
	return CanvasTriangle(createRandomPoint(), createRandomPoint(), createRandomPoint());
}

Colour createRandomColour() {
	int r = rand() % 256;
	int g = rand() % 256;
	int b = rand() % 256;
	return Colour(r, g, b);
}

void handleEvent(SDL_Event event, DrawingWindow &window) {
	if (event.type == SDL_KEYDOWN) {
		if (event.key.keysym.sym == SDLK_LEFT) std::cout << "LEFT" << std::endl;
		else if (event.key.keysym.sym == SDLK_RIGHT) std::cout << "RIGHT" << std::endl;
		else if (event.key.keysym.sym == SDLK_UP) std::cout << "UP" << std::endl;
		else if (event.key.keysym.sym == SDLK_DOWN) std::cout << "DOWN" << std::endl;

		else if (event.key.keysym.sym == SDLK_u) // Week 3 - Task 3
			drawTriangle(window, createRandomPoint(), createRandomPoint(), createRandomPoint(), createRandomColour());
		else if (event.key.keysym.sym == SDLK_f) // Week 3 - Task 4
			drawFilledTriangle(window, createRandomTriangle(), createRandomColour());
	} else if (event.type == SDL_MOUSEBUTTONDOWN) {
		window.savePPM("output.ppm");
		window.saveBMP("output.bmp");
	}
}

bool printed = false;

int main(int argc, char *argv[]) {
	DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);
	SDL_Event event;

	while (true) {
		// We MUST poll for events - otherwise the window will freeze !
		if (window.pollForInputEvents(event)) handleEvent(event, window);

		// Week 4 - Task 2
		std::string objFile = "cornell-box.obj";
		std::tuple<std::vector<glm::vec3>, std::vector<glm::vec3>, std::vector<std::string>> objResult = readOBJFile(objFile, 0.35);
		std::vector<glm::vec3> vertices = std::get<0>(objResult);
		std::vector<glm::vec3> facets = std::get<1>(objResult);
		std::vector<std::string> colours = std::get<2>(objResult);

		// Week 4 - Task 3
		std::string mtlFile = "cornell-box.mtl";
		std::unordered_map<std::string, Colour> coloursMap = readMTLFile(mtlFile);

		// Week 4 - Task 5
		glm::vec3 cameraPosition = glm::vec3(0.0, 0.0, 4.0);
		float focalLength = 2.0;

		// Week 4 - Task 6
		//renderPointCloud(window, vertices, cameraPosition, focalLength);

		// Week 4 - Task 7
		std::vector<ModelTriangle> modelTriangles = generateModelTriangles(vertices, facets, colours, coloursMap);
		std::vector<CanvasTriangle> canvasTriangles = getCanvasTrianglesFromModelTriangles(modelTriangles, cameraPosition, focalLength);
		for (int i = 0; i < canvasTriangles.size(); i++) {
			CanvasTriangle cTriangle = canvasTriangles[i];
			Colour colour = coloursMap[colours[i]];
			drawTriangle(window, cTriangle.v0(), cTriangle.v1(), cTriangle.v2(), colour);
		}

		// Week 4 - Task 8
		for (int i = 0; i < canvasTriangles.size(); i++) {
			CanvasTriangle cTriangle = canvasTriangles[i];
			Colour colour = coloursMap[colours[i]];
			drawFilledTriangle(window, cTriangle, colour);
		}

		// Need to render the frame at the end, or nothing actually gets shown on the screen !
		window.renderFrame();
	}
}

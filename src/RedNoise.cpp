#include <CanvasTriangle.h>
#include <DrawingWindow.h>
#include <Utils.h>
#include <fstream>
#include <vector>

#include <glm/glm.hpp> // Week 2 - Task 4
#include <CanvasPoint.h> // Week 3 - Task 2
#include <Colour.h> // Week 3 - Task 2
#include <CanvasTriangle.h> // Week 3 - Task 3

#define WIDTH 320
#define HEIGHT 240

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

	for (float i = 0; i <= numberOfValues; i++) {
		size_t x = size_t(from.x) + size_t((xIncrement * i));
		size_t y = size_t(from.y) + size_t((yIncrement * i));
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

	for (float i = 0; i <= numberOfValues; i++) {
		size_t x = size_t(from.x) + size_t((xIncrement * i));
		size_t y = size_t(from.y) + size_t((yIncrement * i));
		int red = colour.red;
		int green = colour.green;
		int blue = colour.blue;
		uint32_t colour = (255 << 24) + (int(red) << 16) + (int(green) << 8) + int(blue);
		window.setPixelColour(x, y, colour);
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

// Week 4 - Task 4
void drawFilledTriangle(DrawingWindow& window, CanvasTriangle triangle, Colour colour) {
	// start off by cutting triangle horizontally so there are two flat bottom triangles
	// fill each triangle from the line

	Colour white = Colour(255, 255, 255);

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
	for (int i = 0; i < pointsBToC.size()-1; i++) {
		drawLine(window, pointsBToC[i], pointsBToD[i], colour);
	}
	drawTriangle(window, triangle.v0(), triangle.v1(), triangle.v2(), white);
}

CanvasPoint createRandomPoint() {
	int x = rand() % WIDTH;
	int y = rand() % HEIGHT;
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

int main(int argc, char *argv[]) {
	DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);
	SDL_Event event;

	while (true) {
		// We MUST poll for events - otherwise the window will freeze !
		if (window.pollForInputEvents(event)) handleEvent(event, window);

		// Week 3 - Task 2
		/*CanvasPoint from = CanvasPoint(0, 0);
		CanvasPoint to = CanvasPoint(WIDTH, HEIGHT);
		Colour colour = Colour(255, 0, 0);
		drawLine(window, from, to, colour);*/

		// Week 3 - Task 3 pt1 (pt2 is in handleEvent())
		/*int w = WIDTH - 1;
		int h = HEIGHT - 1;
		CanvasPoint a = CanvasPoint(0, 0);
		CanvasPoint b = CanvasPoint(w, h/2);
		CanvasPoint c = CanvasPoint(w/2, h);
		Colour colour = Colour(255, 0, 0);
		drawTriangle(window, a, b, c, colour);*/


		// Need to render the frame at the end, or nothing actually gets shown on the screen !
		window.renderFrame();
	}
}

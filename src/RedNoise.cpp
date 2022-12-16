#include <CanvasTriangle.h>
#include <DrawingWindow.h>
#include <Utils.h>
#include <fstream>
#include <vector>

#include <glm/glm.hpp> // Week 2 - Task 4
#include <CanvasPoint.h> // Week 3 - Task 2
#include <Colour.h> // Week 3 - Task 2
#include <CanvasTriangle.h> // Week 3 - Task 3
#include <TextureMap.h> // Weel 3 - Task 5
#include <string>

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

	for (float i = 0; i < numberOfValues; i++) {
		float x = from.x + long(xIncrement * i);
		float y = from.y + long(yIncrement * i);
		result.push_back(CanvasPoint(x, y));
	}
	result.shrink_to_fit();

	return result;
}

// Interpolate - for rasterising
std::vector<TexturePoint> interpolateTexturePoints(TexturePoint from, TexturePoint to, float numberOfValues) {
	std::vector<TexturePoint> result;
	float xDiff = to.x - from.x;
	float yDiff = to.y - from.y;

	float xIncrement = xDiff / numberOfValues;
	float yIncrement = yDiff / numberOfValues;

	for (float i = 0; i < numberOfValues; i++) {
		float x = from.x + long(xIncrement * i);
		float y = from.y + long(yIncrement * i);
		result.push_back(TexturePoint(x, y));
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
		float x = from.x + long(xIncrement * i);
		float y = from.y + long(yIncrement * i);
		int red = colour.red;
		int green = colour.green;
		int blue = colour.blue;
		uint32_t colour = (255 << 24) + (int(red) << 16) + (int(green) << 8) + int(blue);
		window.setPixelColour(round(x), ceil(y), colour);
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

	// Sorting the points
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
	for (int i = 0; i < pointsBToC.size(); i++) {
		drawLine(window, pointsBToC[i], pointsBToD[i], colour);
	}
	drawLine(window, barrierStart, barrierEnd, colour);
	drawTriangle(window, triangle.v0(), triangle.v1(), triangle.v2(), white);
}

void drawLineUsingTexture(DrawingWindow& window, CanvasPoint from, CanvasPoint to, TextureMap texture) {
	// For triangle
	float xDiff = to.x - from.x;
	float yDiff = to.y - from.y;
	float numberOfValues = std::max(abs(xDiff), abs(yDiff));
	float xIncrement = xDiff / numberOfValues;
	float yIncrement = yDiff / numberOfValues;
	
	// For texture
	float xDiff_tx = to.texturePoint.x - from.texturePoint.x;
	float yDiff_tx = to.texturePoint.y - from.texturePoint.y;

	float xIncrement_tx = xDiff_tx / numberOfValues;
	float yIncrement_tx = yDiff_tx / numberOfValues;

	for (float i = 0; i < numberOfValues; i++) {
		float x = from.x + long(xIncrement * i);
		float y = from.y + long(yIncrement * i);
		float x_tx = from.texturePoint.x + long(xIncrement_tx * i);
		float y_tx = from.texturePoint.y + long(yIncrement_tx * i);
		int index = round((y_tx * texture.width) + x_tx); // number of rows -> y, so use width, x is for the remainder along the row
		uint32_t colour = texture.pixels[index];
		window.setPixelColour(round(x), ceil(y), colour);
	}
}

// Week 3 - Task 5
void drawTexturedTriangle(DrawingWindow& window, CanvasTriangle triangle, TextureMap texture) {
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
	float ratio = (bot.x - top.x) / (bot.y - top.y); // if you cut big triangle, little triangle is SIMILAR to big triangle, therefore, need to use ratio
	float barrierEndX = top.x + (midLength * ratio);
	CanvasPoint barrierStart = CanvasPoint(mid.x, mid.y);
	CanvasPoint barrierEnd = CanvasPoint(barrierEndX, mid.y);

	// Interpolate between the vertices
	// Top triangle
	float numberOfValuesA = mid.y - top.y;
	std::vector<CanvasPoint> pointsAToC = interpolateCanvasPoints(top, barrierStart, numberOfValuesA);
	std::vector<CanvasPoint> pointsAToD = interpolateCanvasPoints(top, barrierEnd, numberOfValuesA);

	// Bottom triangle
	float numberOfValuesB = bot.y - mid.y;
	std::vector<CanvasPoint> pointsBToC = interpolateCanvasPoints(bot, barrierStart, numberOfValuesB);
	std::vector<CanvasPoint> pointsBToD = interpolateCanvasPoints(bot, barrierEnd, numberOfValuesB);

	//================================================== CREATE TRIANGLES ON TEXTURE =========================================================//
	// Get texture points
	TexturePoint top_tx = top.texturePoint;
	TexturePoint mid_tx = mid.texturePoint;
	TexturePoint bot_tx = bot.texturePoint;

	// Get barrier for texture
	// Note: ensure ratio to the barrier for textures is the same as the ratio to the barrier for the triangle
	glm::vec2 topVec2 = glm::vec2(top_tx.x, top_tx.y);
	glm::vec2 botVec2 = glm::vec2(bot_tx.x, bot_tx.y);
	glm::vec2 edgeToCut = botVec2 - topVec2;
	glm::vec2 barrierEndVec2 = topVec2 + (edgeToCut * ratio);
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
		drawLineUsingTexture(window, pointsAToC[i], pointsAToD[i], texture);
	}
	for (int i = 0; i < numberOfValuesB; i++) {
		drawLineUsingTexture(window, pointsBToC[i], pointsBToD[i], texture);
	}
	// Draw middle line
	drawLineUsingTexture(window, barrierStart, barrierEnd, texture);
}

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

		// Week 3 - Task 5
		CanvasPoint a = CanvasPoint(160, 10);
		CanvasPoint b = CanvasPoint(300, 230);
		CanvasPoint c = CanvasPoint(10, 150);
		TexturePoint a_t = TexturePoint(195, 5);
		TexturePoint b_t = TexturePoint(395, 380);
		TexturePoint c_t = TexturePoint(65, 330);
		a.texturePoint = a_t;
		b.texturePoint = b_t;
		c.texturePoint = c_t;
		CanvasTriangle triangle = CanvasTriangle(a, b, c);
		std::string filename = "texture.ppm";
		TextureMap texture = TextureMap(filename);
		drawTexturedTriangle(window, triangle, texture);

		// Need to render the frame at the end, or nothing actually gets shown on the screen !
		window.renderFrame();
	}
}

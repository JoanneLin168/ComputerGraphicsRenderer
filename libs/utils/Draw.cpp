#include <Constants.h>
#include <CanvasTriangle.h>
#include <DrawingWindow.h>
#include <vector>
#include <glm/glm.hpp>
#include <CanvasPoint.h>
#include <Colour.h>
#include <CanvasTriangle.h>
#include <TextureMap.h>

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
		if (ceil(y) >= 0 && round(y) < HEIGHT && round(x) >= 0 && round(x) < WIDTH) {
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
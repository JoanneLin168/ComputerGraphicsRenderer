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

// Template
void draw(DrawingWindow& window) {
	window.clearPixels();
	for (size_t y = 0; y < window.height; y++) {
		for (size_t x = 0; x < window.width; x++) {
			float red = rand() % 256;
			float green = 0.0;
			float blue = 0.0;
			uint32_t colour = (255 << 24) + (int(red) << 16) + (int(green) << 8) + int(blue);
			window.setPixelColour(x, y, colour);
		}
	}
}

// Week 3 - Task 2
void drawLine(DrawingWindow& window, CanvasPoint from, CanvasPoint to, Colour colour) {
	std::vector<Colour> result;
	float xDiff = to.x - from.x;
	float yDiff = to.y - from.y;
	float numberOfValues = std::max(abs(xDiff), abs(yDiff));

	float xIncrement = xDiff / numberOfValues;
	float yIncrement = yDiff / numberOfValues;

	for (float i = 0; i < numberOfValues; i++) {
		size_t x = size_t(from.x + (xIncrement * i));
		size_t y = size_t(from.y + (yIncrement * i));
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

CanvasPoint createRandomPoint() {
	int x = rand() % WIDTH;
	int y = rand() % HEIGHT;
	return CanvasPoint(x, y);
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

		else if (event.key.keysym.sym == SDLK_u)
			drawTriangle(window, createRandomPoint(), createRandomPoint(), createRandomPoint(), createRandomColour()); // Week 3 - Task 3
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
		int w = WIDTH - 1;
		int h = HEIGHT - 1;
		CanvasPoint a = CanvasPoint(0, 0);
		CanvasPoint b = CanvasPoint(w, h/2);
		CanvasPoint c = CanvasPoint(w/2, h);
		Colour colour = Colour(255, 0, 0);
		drawTriangle(window, a, b, c, colour);

		// Need to render the frame at the end, or nothing actually gets shown on the screen !
		window.renderFrame();
	}
}

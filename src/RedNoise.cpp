#include <CanvasTriangle.h>
#include <DrawingWindow.h>
#include <Utils.h>
#include <fstream>
#include <vector>

#include <glm/glm.hpp> // Week 2 - Task 4

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

// Week 2 - Task 3
void draw(DrawingWindow &window, std::vector<float> pixels) {
	window.clearPixels();
	for (size_t y = 0; y < window.height; y++) {
		for (size_t x = 0; x < window.width; x++) {
			float red = pixels[x];
			float green = pixels[x];
			float blue = pixels[x];
			uint32_t colour = (255 << 24) + (int(red) << 16) + (int(green) << 8) + int(blue);
			window.setPixelColour(x, y, colour);
		}
	}
}

// Week 2 - Task 5
void draw(DrawingWindow& window, std::vector<std::vector<glm::vec3>> pixels) {
	window.clearPixels();
	for (size_t y = 0; y < window.height; y++) {
		for (size_t x = 0; x < window.width; x++) {
			float red = pixels[y][x][0];
			float green = pixels[y][x][1];
			float blue = pixels[y][x][2];
			uint32_t colour = (255 << 24) + (int(red) << 16) + (int(green) << 8) + int(blue);
			window.setPixelColour(x, y, colour);
		}
	}
}

void handleEvent(SDL_Event event, DrawingWindow &window) {
	if (event.type == SDL_KEYDOWN) {
		if (event.key.keysym.sym == SDLK_LEFT) std::cout << "LEFT" << std::endl;
		else if (event.key.keysym.sym == SDLK_RIGHT) std::cout << "RIGHT" << std::endl;
		else if (event.key.keysym.sym == SDLK_UP) std::cout << "UP" << std::endl;
		else if (event.key.keysym.sym == SDLK_DOWN) std::cout << "DOWN" << std::endl;
	} else if (event.type == SDL_MOUSEBUTTONDOWN) {
		window.savePPM("output.ppm");
		window.saveBMP("output.bmp");
	}
}

// Week 2 - Task 2
std::vector<float> interpolateSingleFloats(float from, float to, int numberOfValues) {
	std::vector<float> result;
	float diff = to - from;
	float increments = diff / (numberOfValues-1);

	for (int i = 0; i < numberOfValues; i++) {
		result.push_back(from + (i * increments));
	}

	// Shrinks the vector
	result.shrink_to_fit();

	return result;
}

// Week 2 - Task 4
std::vector<glm::vec3> interpolateThreeElementValues(glm::vec3 from, glm::vec3 to, int numberOfValues) {
	std::vector<glm::vec3> result;
	glm::vec3 diff = to - from;
	float scale = (float) 1 / ((float)numberOfValues - 1);
	glm::vec3 increments = diff * scale;

	for (float i = 0; i < numberOfValues; i++) {
		glm::vec3 toAdd = increments * i;
		result.push_back(from + toAdd);
	}

	// Shrinks the vector
	result.shrink_to_fit();

	return result;
}

int main(int argc, char *argv[]) {
	DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);
	SDL_Event event;
	std::vector<float> result;

	// Week 2 - Task 3
	result = interpolateSingleFloats(255.0, 0.0, WIDTH);

	// Week 2 - Task 4
	glm::vec3 from(1.0, 4.0, 9.2);
	glm::vec3 to(4.0, 1.0, 9.8);
	std::vector<glm::vec3> result2 = interpolateThreeElementValues(from, to, 4);

	// Week 2 - Task 5
	glm::vec3 topLeft(255, 0, 0);        // red 
	glm::vec3 topRight(0, 0, 255);       // blue 
	glm::vec3 bottomRight(0, 255, 0);    // green 
	glm::vec3 bottomLeft(255, 255, 0);   // yellow

	// Get interpolation of left and right edges - to interpolate for every vector
	std::vector<glm::vec3> left = interpolateThreeElementValues(topLeft, bottomLeft, WIDTH);
	std::vector<glm::vec3> right = interpolateThreeElementValues(topRight, bottomRight, WIDTH);

	// Interpolate every row from left to right
	std::vector<std::vector<glm::vec3>> pixels;
	for (int i = 0; i < WIDTH; i++) {
		glm::vec3 firstPixel = left[i];
		glm::vec3 lastPixel = right[i];
		std::vector<glm::vec3> row = interpolateThreeElementValues(firstPixel,lastPixel, WIDTH);

		pixels.push_back(row);
	}
	pixels.shrink_to_fit();

	while (true) {
		// We MUST poll for events - otherwise the window will freeze !
		if (window.pollForInputEvents(event)) handleEvent(event, window);
		//draw(); // template
		//draw(window, result); // Task 3
		draw(window, pixels); // Task 5
		// Need to render the frame at the end, or nothing actually gets shown on the screen !
		window.renderFrame();
	}
}

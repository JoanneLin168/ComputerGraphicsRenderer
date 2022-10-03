#include <CanvasTriangle.h>
#include <DrawingWindow.h>
#include <Utils.h>
#include <fstream>
#include <vector>

#define WIDTH 320
#define HEIGHT 240

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

int main(int argc, char *argv[]) {
	DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);
	SDL_Event event;
	std::vector<float> result;

	result = interpolateSingleFloats(255.0, 0.0, WIDTH);
	// for (size_t i = 0; i < result.size(); i++) std::cout << result[i] << " ";
	// std::cout << std::endl;

	// Create array of 2d vectors (each vector will have 3 vectors for R, G, B)
	//std::vector<std::vector<float>> pixels;
	//for (int i = 0; i < WIDTH; i++) {
	//	float pixelValue = result[i];

	//	// Create an RGB pixel
	//	std::vector<float> pixel;
	//	for (int j = 0; j < 3; j++) {
	//		pixel.push_back(pixelValue);
	//	}
	//	pixel.shrink_to_fit(); // Shrinks the vector

	//	// Add RGB pixel to pixels
	//	pixels.push_back(pixel);
	//}

	while (true) {
		// We MUST poll for events - otherwise the window will freeze !
		if (window.pollForInputEvents(event)) handleEvent(event, window);
		draw(window, result);
		// Need to render the frame at the end, or nothing actually gets shown on the screen !
		window.renderFrame();
	}
}

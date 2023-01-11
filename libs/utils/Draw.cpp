#include <Constants.h>
#include <Rasteriser.h>
#include <DrawingWindow.h>
#include <vector>
#include <glm/glm.hpp>
#include <CanvasPoint.h>
#include <Colour.h>
#include <CanvasTriangle.h>
#include <ModelTriangle.h>
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

// For CanvasTriangles
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

// For ModelTriangles
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

	// Interpolate between the vertices
	// Top triangle
	float numberOfValuesA = std::max(abs(mid.x - top.x), abs(mid.y - top.y));
	std::vector<CanvasPoint> pointsAToC = interpolateCanvasPoints(top, barrierStart, numberOfValuesA);
	std::vector<CanvasPoint> pointsAToD = interpolateCanvasPoints(top, barrierEnd, numberOfValuesA);

	// Bottom triangle
	float numberOfValuesB = std::max(abs(bot.x - mid.x), abs(bot.y - mid.y));
	std::vector<CanvasPoint> pointsBToC = interpolateCanvasPoints(bot, barrierStart, numberOfValuesB);
	std::vector<CanvasPoint> pointsBToD = interpolateCanvasPoints(bot, barrierEnd, numberOfValuesB);

	// Rasterise - refer to depthArray
	for (int i = 0; i < pointsAToC.size(); i++) {
		drawLine(window, pointsAToC[i], pointsAToD[i], colour, depthArray);
	}
	for (int i = 0; i < pointsBToC.size(); i++) {
		drawLine(window, pointsBToC[i], pointsBToD[i], colour, depthArray);
	}
	drawLine(window, barrierStart, barrierEnd, colour, depthArray);
	drawTriangle(window, top, mid, bot, colour, depthArray);
}

// Week 3 - Task 5
// Note: If asked a question regarding how to get the value of a point in the texture triangle, using a point in the actual triangle
// use Barycentric coordinates to get ratios u and v (r = p0 + u(p1-p0) + v(p2-p0)) and use that equation but with the texture triangle vertices to get the value of the point on the texture triangle
void drawTexturedTriangle(DrawingWindow& window, CanvasTriangle triangle, TextureMap texture, std::vector<std::vector<float>>& depthArray) {
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
	float ratio = (bot.x - top.x) / (bot.y - top.y); // dx/dy
	float barrierEndX = top.x + (midLength * ratio);
	CanvasPoint barrierStart = CanvasPoint(mid.x, mid.y);
	CanvasPoint barrierEnd = CanvasPoint(barrierEndX, mid.y);

	// Interpolate between the vertices
	// Top triangle
	float numberOfValuesA = std::max(abs(mid.x - top.x), abs(mid.y - top.y));
	std::vector<CanvasPoint> pointsAToC = interpolateCanvasPoints(top, barrierStart, numberOfValuesA);
	std::vector<CanvasPoint> pointsAToD = interpolateCanvasPoints(top, barrierEnd, numberOfValuesA);

	// Bottom triangle
	float numberOfValuesB = std::max(abs(bot.x - mid.x), abs(bot.y - mid.y));
	std::vector<CanvasPoint> pointsBToC = interpolateCanvasPoints(bot, barrierStart, numberOfValuesB);
	std::vector<CanvasPoint> pointsBToD = interpolateCanvasPoints(bot, barrierEnd, numberOfValuesB);

	//================================================== CREATE TRIANGLES ON TEXTURE =========================================================//
	// Get texture points
	TexturePoint top_tx = top.texturePoint;
	TexturePoint mid_tx = mid.texturePoint;
	TexturePoint bot_tx = bot.texturePoint;

	// Get barrier for texture
	// Note: ensure ratio to the barrier for textures is the same as the ratio to the barrier for the triangle
	float ratioOfEdge = (barrierEnd.y - top.y) / (bot.y - top.y);
	glm::vec2 topVec2 = glm::vec2(top_tx.x, top_tx.y);
	glm::vec2 botVec2 = glm::vec2(bot_tx.x, bot_tx.y);
	glm::vec2 edgeToCut = botVec2 - topVec2;
	glm::vec2 barrierEndVec2 = topVec2 + (edgeToCut * ratioOfEdge);
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
		drawLineUsingTexture(window, pointsAToC[i], pointsAToD[i], texture, depthArray);
	}
	for (int i = 0; i < numberOfValuesB; i++) {
		drawLineUsingTexture(window, pointsBToC[i], pointsBToD[i], texture, depthArray);
	}
}
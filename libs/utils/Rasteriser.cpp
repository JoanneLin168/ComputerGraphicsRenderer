#include <Constants.h>
#include <glm/glm.hpp>
#include <CanvasTriangle.h>
#include <DrawingWindow.h>
#include <vector>
#include <CanvasPoint.h>
#include <Colour.h>
#include <CanvasTriangle.h>
#include <TextureMap.h>

// Interpolate canvas points
std::vector<CanvasPoint> interpolateCanvasPoints(CanvasPoint from, CanvasPoint to, float n) {
	int numberOfValues = ceil(n) + 1; // to ensure interpolation includes last value, needs to be ceil() to ensure more every value is captured

	std::vector<CanvasPoint> result;
	float xDiff = to.x - from.x;
	float yDiff = to.y - from.y;
	float zDiff = to.depth - from.depth;

	float xIncrement = xDiff / (numberOfValues - 1);
	float yIncrement = yDiff / (numberOfValues - 1);
	float zIncrement = zDiff / (numberOfValues - 1);

	for (float i = 0; i < numberOfValues; i++) {
		float x = from.x + (xIncrement * i);
		float y = from.y + (yIncrement * i);
		float z = from.depth + (zIncrement * i);
		result.push_back(CanvasPoint(x, y, z));
	}
	result.shrink_to_fit();

	return result;
}

// Interpolate texture points
std::vector<TexturePoint> interpolateTexturePoints(TexturePoint from, TexturePoint to, float n) {
	int numberOfValues = ceil(n) + 1; // to ensure interpolation includes last value, needs to be ceil() to ensure more every value is captured

	std::vector<TexturePoint> result;
	float xDiff = to.x - from.x;
	float yDiff = to.y - from.y;

	float xIncrement = xDiff / (numberOfValues - 1);
	float yIncrement = yDiff / (numberOfValues - 1);

	for (float i = 0; i < numberOfValues; i++) {
		float x = from.x + long(xIncrement * i); // NOTE: textures have to use long for some reason
		float y = from.y + long(yIncrement * i);
		result.push_back(TexturePoint(x, y));
	}
	result.shrink_to_fit();

	return result;
}

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
	float z_2d = -1 / (distanceFromCamera.z);

	CanvasPoint intersectionPoint = CanvasPoint(x_2d, y_2d, z_2d);

	return intersectionPoint;
}
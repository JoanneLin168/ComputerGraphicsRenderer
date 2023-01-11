#include <vector>
#include <string>
#include <glm/glm.hpp>

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
			cos(theta), 0, -sin(theta), 0,
			0, 1, 0, 0,
			sin(theta), 0, cos(theta), 0,
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

// LookAt function
void lookAt(glm::mat4& cameraPosition) {
	// Variables in world
	glm::vec3 origin = glm::vec3(0.0, 0.0, 0.0);
	glm::vec3 vertical = glm::vec3(0.0, 1.0, 0.0);

	// forward, up and right are relative to the camera
	// vertical is relative to the world (0,1,0)
	glm::vec3 cameraPosVec3 = glm::vec3(cameraPosition[3][0], cameraPosition[3][1], cameraPosition[3][2]);
	glm::vec3 forward = glm::normalize(cameraPosVec3 - origin);
	glm::vec3 right = glm::normalize(glm::cross(vertical, forward));
	glm::vec3 up = glm::normalize(glm::cross(forward, right));
	cameraPosition = glm::mat4(glm::vec4(right, 0), glm::vec4(up, 0), glm::vec4(forward, 0), cameraPosition[3]);
}
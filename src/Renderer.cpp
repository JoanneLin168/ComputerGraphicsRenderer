#include <CanvasTriangle.h>
#include <DrawingWindow.h>
#include <Utils.h>
#include <fstream>
#include <vector>

// Own libraries
#include <Files.h>
#include <Constants.h>
#include <Draw.h>
#include <CameraController.h>
#include <TriangleHelper.h>
#include <Rasteriser.h>
#include <RayTracer.h>
#include <Lighting.h>

#include <glm/glm.hpp> // Week 2 - Task 4
#include <CanvasPoint.h> // Week 3 - Task 2
#include <Colour.h> // Week 3 - Task 2
#include <CanvasTriangle.h> // Week 3 - Task 3
#include <TextureMap.h> // Week 3 - Task 5
#include <string>
#include <ModelTriangle.h> // Week 4 - Task 2

#include <RayTriangleIntersection.h> // Week 7 - Task 2 
#include <thread> // For raytracing
#include <mutex>


std::mutex mtx; // Used for raytracing

// ================================================== WIREFRAME ============================================================= //
void drawWireframe(DrawingWindow& window, glm::mat4& cameraPosition, float focalLength, std::vector<ModelTriangle> modelTriangles) {
	window.clearPixels();

	std::vector<CanvasTriangle> canvasTriangles = getCanvasTrianglesFromModelTriangles(modelTriangles, cameraPosition, focalLength);

	// Create a depth array
	std::vector<std::vector<float>> depthArray(HEIGHT, std::vector<float>(WIDTH, 0));

	for (size_t i = 0; i < canvasTriangles.size(); i++) {
		CanvasTriangle triangle = canvasTriangles[i];
		Colour colour = Colour(255, 255, 255);
		drawTriangle(window, triangle.v0(), triangle.v1(), triangle.v2(), colour, depthArray);
	}
}

// ================================================== RASTERISED ============================================================= //
void drawRasterisedScene(
	DrawingWindow& window,
	glm::mat4& cameraPosition,
	float focalLength,
	std::vector<ModelTriangle> modelTriangles,
	std::unordered_map<std::string, Colour> coloursMap,
	std::unordered_map<std::string, TextureMap> texturesMap,
	std::vector<std::string> colourNames) {

	window.clearPixels();

	std::vector<CanvasTriangle> canvasTriangles = getCanvasTrianglesFromModelTriangles(modelTriangles, cameraPosition, focalLength);

	// Create a depth array
	std::vector<std::vector<float>> depthArray(HEIGHT, std::vector<float>(WIDTH, 0));

	for (size_t i = 0; i < canvasTriangles.size(); i++) {
		CanvasTriangle triangle = canvasTriangles[i];
		Colour colour = coloursMap[colourNames[i]];

		if (texturesMap.count(colourNames[i])) {
			TextureMap texture = texturesMap[colourNames[i]];
			triangle.v0().texturePoint = modelTriangles[i].texturePoints[0];
			triangle.v1().texturePoint = modelTriangles[i].texturePoints[1];
			triangle.v2().texturePoint = modelTriangles[i].texturePoints[2];
			drawTexturedTriangle(window, triangle, texture, depthArray);
		}
		else {
			drawFilledTriangle(window, triangle, colour, depthArray);
		}
	}
}

// =================================================== RAY TRACING ======================================================== //
// NOTE: for some reason, it is faster if the ray tracing code was in Renderer.cpp, multithreading doesn't seem to work using code on RayTracer.cpp

// Detect when and where a projected ray intersects with a model triangle using Barycentric coordinates
RayTriangleIntersection getClosestIntersection(bool getAbsolute, glm::vec3 sourceOfRay, ModelTriangle triangle, glm::vec3 rayDirection, int index) {

	glm::vec3 e0 = triangle.vertices[1] - triangle.vertices[0];
	glm::vec3 e1 = triangle.vertices[2] - triangle.vertices[0];
	glm::vec3 SPVector = sourceOfRay - triangle.vertices[0];
	glm::mat3 DEMatrix(-rayDirection, e0, e1);

	// possibleSolution = (t,u,v) where
	//    t = absolute distance from camera to intersection point
	//    u = proportional distance along first edge
	//    v = proportional distance along second edge
	glm::vec3 possibleSolution = glm::inverse(DEMatrix) * SPVector;

	float t = getAbsolute ? abs(possibleSolution[0]) : possibleSolution[0];
	float u = possibleSolution[1];
	float v = possibleSolution[2];
	if ((u >= 0.0) && (u <= 1.0) && (v >= 0.0) && (v <= 1.0) && (double(u) + double(v)) <= 1.0 && t >= 0) { // masking u and v to avoid overflow
		glm::vec3 r = triangle.vertices[0] + (u * (triangle.vertices[1] - triangle.vertices[0])) + (v * (triangle.vertices[2] - triangle.vertices[0]));
		RayTriangleIntersection rayTriangleIntersection = RayTriangleIntersection(r, t, triangle, index);
		rayTriangleIntersection.t = t;
		rayTriangleIntersection.u = u;
		rayTriangleIntersection.v = v;
		return rayTriangleIntersection;
	}
	else {
		return RayTriangleIntersection(glm::vec3(), INFINITY, ModelTriangle(), -1);
	}
}

// If shadow ray hits a triangle, then you need to draw a shadow
bool isShadowRayBlocked(int index, glm::vec3 point, glm::mat4 cameraPosition, glm::vec3 lightPosition, std::vector<ModelTriangle> triangles, float focalLength) {
	for (int j = 0; j < triangles.size(); j++) {
		ModelTriangle triangleCompare = triangles[j];
		glm::vec3 shadowRay = glm::normalize(lightPosition - point); // to - from, to=light, from=surface of triangle

		RayTriangleIntersection rayTriangleIntersection = getClosestIntersection(false, point, triangleCompare, shadowRay, j);

		// float pointToLight = glm::length(lightPosition - point);
		if (rayTriangleIntersection.distanceFromCamera < glm::length(shadowRay) && rayTriangleIntersection.triangleIndex != index) {
			return true;
		}
	}
	return false;
}

glm::vec3 getVertexNormal(std::vector<ModelTriangle> modelTriangles, glm::vec3 vertex) {
	std::vector<glm::vec3> neighbouringNormals;
	glm::vec3 neighbouringNormalsSum;

	for (ModelTriangle triangle : modelTriangles) {
		for (int i = 0; i < triangle.vertices.size(); i++) {
			if (vertex == triangle.vertices[i]) {
				neighbouringNormals.push_back(triangle.normal);
				neighbouringNormalsSum += triangle.normal;
			}
		}
	}

	neighbouringNormals.shrink_to_fit();
	int n = neighbouringNormals.size();
	glm::vec3 vertexNormal = glm::vec3(neighbouringNormalsSum.x / n, neighbouringNormalsSum.y / n, neighbouringNormalsSum.z / n);

	return vertexNormal;
}

void cameraFireRays(std::vector<std::vector<RayTriangleIntersection>>& closestTriangleBuffer,
	glm::mat4 cameraPosition,
	std::vector<ModelTriangle> triangles,
	float focalLength,
	size_t startY,
	size_t endY,
	std::mutex& mtx) {

	for (size_t y = startY; y < endY; y++) {
		for (size_t x = 0; x < WIDTH; x++) {
			int index = 0;
			RayTriangleIntersection closestRayTriangleIntersection = RayTriangleIntersection(glm::vec3(), INFINITY, ModelTriangle(), -1);

			// For every pixel, check to see which triangles are intersected by the ray, and find the closest one
			for (ModelTriangle triangle : triangles) {
				glm::vec3 cameraPosVec3 = glm::vec3(cameraPosition[3][0], cameraPosition[3][1], cameraPosition[3][2]);
				glm::vec3 rayDirectionOriented = glm::vec3(x, y, 0) - cameraPosVec3; // ray is from the camera to the scene, so to - from = scene - camera

				float x_3d = ((rayDirectionOriented.x - (WIDTH / 2)) * -rayDirectionOriented.z) / (focalLength * SCALE);
				float y_3d = ((rayDirectionOriented.y - (HEIGHT / 2)) * rayDirectionOriented.z) / (focalLength * SCALE);
				float z_3d = rayDirectionOriented.z;

				glm::mat3 cameraOrientMat3 = glm::mat3(
					cameraPosition[0][0], cameraPosition[0][1], cameraPosition[0][2],
					cameraPosition[1][0], cameraPosition[1][1], cameraPosition[1][2],
					cameraPosition[2][0], cameraPosition[2][1], cameraPosition[2][2]
				);
				glm::vec3 rayDirection = glm::normalize(cameraOrientMat3 * glm::vec3(x_3d, y_3d, z_3d));

				RayTriangleIntersection rayTriangleIntersection = getClosestIntersection(true, cameraPosVec3, triangle, rayDirection, index);

				// check if point on triangle is closer to camera than the previously stored one
				if (rayTriangleIntersection.distanceFromCamera < closestRayTriangleIntersection.distanceFromCamera) {
					closestRayTriangleIntersection = rayTriangleIntersection;
				}

				index++;
			}

			// Draw point if the ray hit something
			if (closestRayTriangleIntersection.triangleIndex >= 0) {
				mtx.lock();
				closestTriangleBuffer[y][x] = closestRayTriangleIntersection;
				mtx.unlock();
			}
		}
	}
}

void drawRayTracingScene(DrawingWindow& window, glm::vec3& lightPosition, glm::mat4& cameraPosition, std::vector<ModelTriangle> modelTriangles, float focalLength, ShadingType shadingType) {
	window.clearPixels();

	std::vector<std::vector<RayTriangleIntersection>> closestTriangleBuffer(HEIGHT, std::vector<RayTriangleIntersection>(WIDTH, RayTriangleIntersection())); // similar to depthArray, but stores index of closest triangle

	// Multithreading
	const size_t num_threads = std::thread::hardware_concurrency();
	std::vector<std::thread> threads;
	threads.reserve(num_threads);
	int sectionSize = HEIGHT / num_threads;

	// Call threads to check if there is an intersection with the camera
	for (int i = 0; i < num_threads; i++) {
		int startY = i * sectionSize;
		int endY = (i + 1) * sectionSize;
		threads.push_back(std::thread(cameraFireRays, std::ref(closestTriangleBuffer), cameraPosition, modelTriangles, focalLength, startY, endY, std::ref(mtx)));
	}

	// Wait for threads to finish tasks
	for (std::thread& t : threads) {
		t.join();
	}

	// Lighting and Shading
	for (size_t y = 0; y < HEIGHT; y++) {
		for (size_t x = 0; x < WIDTH; x++) {
			// Set up
			RayTriangleIntersection rayTriangleIntersection = closestTriangleBuffer[y][x];
			int index = rayTriangleIntersection.triangleIndex;
			ModelTriangle triangle = rayTriangleIntersection.intersectedTriangle;
			glm::vec3 point = rayTriangleIntersection.intersectionPoint; // point that the cameraRay intersected with the triangle

			// Get u,v,w for Gouraud and Phong shading (used for interpolations)
			// REFERENCE: https://www.scratchapixel.com/lessons/3d-basic-rendering/introduction-to-shading/shading-normals
			float u = rayTriangleIntersection.u;
			float v = rayTriangleIntersection.v;
			float w = 1 - (u + v);

			glm::vec3 v0 = triangle.vertices[0];
			glm::vec3 v1 = triangle.vertices[1];
			glm::vec3 v2 = triangle.vertices[2];

			glm::vec3 n0 = getVertexNormal(modelTriangles, v0);
			glm::vec3 n1 = getVertexNormal(modelTriangles, v1);
			glm::vec3 n2 = getVertexNormal(modelTriangles, v2);

			// Add lighting and shadows
			if (index != -1) {
				// General
				bool shadowRayBlocked = isShadowRayBlocked(index, point, cameraPosition, lightPosition, modelTriangles, focalLength);
				glm::vec3 shadowRay = lightPosition - point;
				glm::vec3 cameraPosVec3 = glm::vec3(cameraPosition[3][0], cameraPosition[3][1], cameraPosition[3][2]);
				glm::vec3 view = cameraPosVec3 - point; // vector to the camera from the point
				glm::vec3 incidenceRay = point - lightPosition;

				// Lighting setup
				float proximityLighting = 0;
				float incidenceLighting = 0;
				float specularLighting = 0;
				float brightness = 0;
				float ambience = (shadowRayBlocked) ? 0 : 20;

				if (shadingType == SHADING_FLAT) {
					// Lighting
					proximityLighting = calculateProximityLighting(lightPosition, point, LIGHT_STRENGTH);
					incidenceLighting = calculateIncidenceLighting(shadowRay, triangle.normal);
					specularLighting = calculateSpecularLighting(view, incidenceRay, triangle.normal, SPECULAR_EXPONENT);
				}

				else if (shadingType == SHADING_GOURAUD) {
					// 1. Get vertex normals by getting average of neighbouring facet normals
					// 2. Get illumination for each vertex point
					// 3. Use Barycentric coordinates to interpolate and get intensities for each hit point
					glm::vec3 vertexToLight0 = lightPosition - v0;
					glm::vec3 vertexToLight1 = lightPosition - v1;
					glm::vec3 vertexToLight2 = lightPosition - v2;

					float proximityLighting0 = calculateProximityLighting(lightPosition, n0, LIGHT_STRENGTH);
					float proximityLighting1 = calculateProximityLighting(lightPosition, n1, LIGHT_STRENGTH);
					float proximityLighting2 = calculateProximityLighting(lightPosition, n2, LIGHT_STRENGTH);
					float incidenceLighting0 = calculateIncidenceLighting(vertexToLight0, n0);
					float incidenceLighting1 = calculateIncidenceLighting(vertexToLight1, n1);
					float incidenceLighting2 = calculateIncidenceLighting(vertexToLight2, n2);
					float specularExponent0 = calculateSpecularLighting(view, incidenceRay, n0, SPECULAR_EXPONENT);
					float specularExponent1 = calculateSpecularLighting(view, incidenceRay, n1, SPECULAR_EXPONENT);
					float specularExponent2 = calculateSpecularLighting(view, incidenceRay, n2, SPECULAR_EXPONENT);

					proximityLighting = (w * proximityLighting0) + (u * proximityLighting1) + (v * proximityLighting2);
					incidenceLighting = (w * incidenceLighting0) + (u * incidenceLighting1) + (v * incidenceLighting2);
					specularLighting = (w * specularExponent0) + (u * specularExponent1) + (v * specularExponent2);
				}

				else if (shadingType == SHADING_PHONG) {
					// 1. Get vertex normals by getting average of neighbouring facet normals
					// 2. Use Barycentric coordinates to interpolate and get normals for each hit point
					// 3. Use hit normals to get illumination for each hit point
					glm::vec3 hitNormal = (w * n0) + (u * n1) + (v * n2);
					proximityLighting = calculateProximityLighting(lightPosition, point, LIGHT_STRENGTH);
					incidenceLighting = calculateIncidenceLighting(shadowRay, hitNormal);
					specularLighting = calculateSpecularLighting(view, incidenceRay, hitNormal, SPECULAR_EXPONENT);
				}

				// Colour in hit point
				brightness = proximityLighting * incidenceLighting;
				float r = 0;
				float g = 0;
				float b = 0;
				if (triangle.colour.red > 0)   r = ambience + triangle.colour.red * brightness;
				if (triangle.colour.green > 0) g = ambience + triangle.colour.green * brightness;
				if (triangle.colour.blue > 0)  b = ambience + triangle.colour.blue * brightness;

				if (incidenceLighting > 0 && proximityLighting > 0 && specularLighting > 0) {
					r += 255 * specularLighting;
					g += 255 * specularLighting;
					b += 255 * specularLighting;
				}

				r = glm::clamp(r, 0.0f, 255.0f);
				g = glm::clamp(g, 0.0f, 255.0f);
				b = glm::clamp(b, 0.0f, 255.0f);

				uint32_t colour = (255 << 24) + (int(r) << 16) + (int(g) << 8) + int(b);
				window.setPixelColour(floor(x), ceil(y), colour);
			}
		}
	}
}

void changeMode(int& mode) {
	if (mode < 2) mode++;
	else mode = 0;
	std::cout << "Switched to mode: " << mode << std::endl;
}

void handleEvent(SDL_Event event, DrawingWindow &window, int& mode, glm::vec3& lightPosition, glm::mat4 &cameraPosition, bool& toOrbit, ShadingType& shadingType) {
	if (event.type == SDL_KEYDOWN) {
		// Translation
		if (event.key.keysym.sym == SDLK_d) translateCamera("X", DIST, cameraPosition);
		else if (event.key.keysym.sym == SDLK_a) translateCamera("X", -DIST, cameraPosition);
		else if (event.key.keysym.sym == SDLK_w) translateCamera("Y", DIST, cameraPosition);
		else if (event.key.keysym.sym == SDLK_s) translateCamera("Y", -DIST, cameraPosition);
		else if (event.key.keysym.sym == SDLK_q) translateCamera("Z", DIST, cameraPosition);
		else if (event.key.keysym.sym == SDLK_e) translateCamera("Z", -DIST, cameraPosition);

		// Rotation
		else if (event.key.keysym.sym == SDLK_l) rotateCamera("X", ANGLE, cameraPosition);
		else if (event.key.keysym.sym == SDLK_j) rotateCamera("X", -ANGLE, cameraPosition);
		else if (event.key.keysym.sym == SDLK_i) rotateCamera("Y", ANGLE, cameraPosition);
		else if (event.key.keysym.sym == SDLK_k) rotateCamera("Y", -ANGLE, cameraPosition);
		else if (event.key.keysym.sym == SDLK_u) rotateCamera("Z", ANGLE, cameraPosition);
		else if (event.key.keysym.sym == SDLK_o) rotateCamera("Z", -ANGLE, cameraPosition);

		// Move light
		else if (event.key.keysym.sym == SDLK_UP) lightPosition[1] += 0.01;
		else if (event.key.keysym.sym == SDLK_DOWN) lightPosition[1] -= 0.01;
		else if (event.key.keysym.sym == SDLK_RIGHT) lightPosition[2] += 0.01;
		else if (event.key.keysym.sym == SDLK_LEFT) lightPosition[2] -= 0.01;

		// Switch between modes
		else if (event.key.keysym.sym == SDLK_r) changeMode(mode);

		// Switch between shading types
		else if (event.key.keysym.sym == SDLK_1) { shadingType = SHADING_FLAT; std::cout << "Shading mode: Flat Shading" << std::endl; }
		else if (event.key.keysym.sym == SDLK_2) { shadingType = SHADING_GOURAUD; std::cout << "Shading mode: Gouraud Shading" << std::endl; }
		else if (event.key.keysym.sym == SDLK_3) { shadingType = SHADING_PHONG; std::cout << "Shading mode: Phong Shading" << std::endl; }

		// Toggle orbit
		else if (event.key.keysym.sym == SDLK_SPACE) toOrbit = !toOrbit;

		// Debug key
		else if (event.key.keysym.sym == SDLK_BACKSLASH) {
			glm::vec3 cameraPosVec3 = glm::vec3(cameraPosition[3][0], cameraPosition[3][1], cameraPosition[3][2]);
			std::cout << "Camera Position:" << cameraPosVec3.x << " " << cameraPosVec3.y << " " << cameraPosVec3.z << std::endl;
			std::cout << "Light Position:" << lightPosition.x << " " << lightPosition.y << " " << lightPosition.z << std::endl;
		}
	} else if (event.type == SDL_MOUSEBUTTONDOWN) {
		window.savePPM("output.ppm");
		window.saveBMP("output.bmp");
	}
}

int main(int argc, char *argv[]) {
	DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);
	SDL_Event event;

	// Week 4 - Task 2
	std::string objFile = "sphere.obj";
	std::tuple<std::vector<glm::vec3>, std::vector<glm::vec3>, std::vector<glm::vec3>, std::vector<glm::vec3>, std::vector<std::string>> objResult = readOBJFile(objFile, 0.35);
	std::vector<glm::vec3> vertices = std::get<0>(objResult);
	std::vector<glm::vec3> facets = std::get<1>(objResult);
	std::vector<glm::vec3> vertices_tx = std::get<2>(objResult);
	std::vector<glm::vec3> facets_tx = std::get<3>(objResult);
	std::vector<std::string> colourNames = std::get<4>(objResult);

	// Week 4 - Task 3
	std::string mtlFile = "textured-cornell-box.mtl";
	std::tuple<std::unordered_map<std::string, Colour>, std::unordered_map<std::string, TextureMap>> mtlMaps = readMTLFile(mtlFile);
	std::unordered_map<std::string, Colour> coloursMap = std::get<0>(mtlMaps);
	std::unordered_map<std::string, TextureMap> texturesMap = std::get<1>(mtlMaps);

	std::vector<ModelTriangle> modelTriangles = generateModelTriangles(vertices, facets, vertices_tx, facets_tx, colourNames, coloursMap, texturesMap);

	// Variables for camera
	glm::mat4 cameraPosition = glm::mat4(
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 4, 1 // last column vector stores x, y, z
	);
	float focalLength = 2.0;
	bool toOrbit = false;
	int mode = 0;

	// Lighting
	glm::vec3 lightPosition = (objFile == "sphere.obj") ? glm::vec3(-0.3, 0.8, 1.5) : glm::vec3(0.0, 0.8, 0.5);
	ShadingType shadingType = SHADING_FLAT;

	while (true) {
		// We MUST poll for events - otherwise the window will freeze !
		if (window.pollForInputEvents(event)) handleEvent(event, window, mode, lightPosition, cameraPosition, toOrbit, shadingType);
		if (mode == 0) drawWireframe(window, cameraPosition, focalLength, modelTriangles);
		else if (mode == 1) drawRasterisedScene(window, cameraPosition, focalLength, modelTriangles, coloursMap, texturesMap, colourNames);
		else if (mode == 2) drawRayTracingScene(window, lightPosition, cameraPosition, modelTriangles, focalLength, shadingType);

		// Orbit and LookAt
		if (toOrbit) rotateCamera("Y", -ANGLE, cameraPosition);
		//lookAt(cameraPosition);

		// For debugging
		CanvasPoint lightCanvasPoint = getCanvasIntersectionPoint(cameraPosition, lightPosition, focalLength);
		uint32_t lightColour = (255 << 24) + (255 << 16) + (255 << 8) + 255;
		if (round(lightCanvasPoint.x) < WIDTH && round(lightCanvasPoint.y) < HEIGHT) window.setPixelColour(round(lightCanvasPoint.x), round(lightCanvasPoint.y), lightColour);
		

		// Need to render the frame at the end, or nothing actually gets shown on the screen !
		window.renderFrame();
	}
}

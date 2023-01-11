// NOTE: for some reason, it is faster if the ray tracing code was in Renderer.cpp

//#include <Constants.h>
//#include <vector>
//#include <RayTriangleIntersection.h>
//#include <mutex>
//
//std::mutex mtx; // Used for raytracing
//
//// Detect when and where a projected ray intersects with a model triangle using Barycentric coordinates
//RayTriangleIntersection getClosestIntersection(bool getAbsolute, glm::vec3 sourceOfRay, ModelTriangle triangle, glm::vec3 rayDirection, int index) {
//
//	glm::vec3 e0 = triangle.vertices[1] - triangle.vertices[0];
//	glm::vec3 e1 = triangle.vertices[2] - triangle.vertices[0];
//	glm::vec3 SPVector = sourceOfRay - triangle.vertices[0];
//	glm::mat3 DEMatrix(-rayDirection, e0, e1);
//
//	// possibleSolution = (t,u,v) where
//	//    t = absolute distance from camera to intersection point
//	//    u = proportional distance along first edge
//	//    v = proportional distance along second edge
//	glm::vec3 possibleSolution = glm::inverse(DEMatrix) * SPVector;
//
//	float t = getAbsolute ? abs(possibleSolution[0]) : possibleSolution[0];
//	float u = possibleSolution[1];
//	float v = possibleSolution[2];
//	if ((u >= 0.0) && (u <= 1.0) && (v >= 0.0) && (v <= 1.0) && (double(u) + double(v)) <= 1.0 && t >= 0) { // masking u and v to avoid overflow
//		glm::vec3 r = triangle.vertices[0] + (u * (triangle.vertices[1] - triangle.vertices[0])) + (v * (triangle.vertices[2] - triangle.vertices[0]));
//		RayTriangleIntersection rayTriangleIntersection = RayTriangleIntersection(r, t, triangle, index);
//		rayTriangleIntersection.t = t;
//		rayTriangleIntersection.u = u;
//		rayTriangleIntersection.v = v;
//		return rayTriangleIntersection;
//	}
//	else {
//		return RayTriangleIntersection(glm::vec3(), INFINITY, ModelTriangle(), -1);
//	}
//}
//
//// If shadow ray hits a triangle, then you need to draw a shadow
//bool isShadowRayBlocked(int index, glm::vec3 point, glm::mat4 cameraPosition, glm::vec3 lightPosition, std::vector<ModelTriangle> triangles, float focalLength) {
//	for (int j = 0; j < triangles.size(); j++) {
//		ModelTriangle triangleCompare = triangles[j];
//		glm::vec3 shadowRay = glm::normalize(lightPosition - point); // to - from, to=light, from=surface of triangle
//
//		RayTriangleIntersection rayTriangleIntersection = getClosestIntersection(false, point, triangleCompare, shadowRay, j);
//
//		// float pointToLight = glm::length(lightPosition - point);
//		if (rayTriangleIntersection.distanceFromCamera < glm::length(shadowRay) && rayTriangleIntersection.triangleIndex != index) {
//			return true;
//		}
//	}
//	return false;
//}
//
//glm::vec3 getVertexNormal(std::vector<ModelTriangle> modelTriangles, glm::vec3 vertex) {
//	std::vector<glm::vec3> neighbouringNormals;
//	glm::vec3 neighbouringNormalsSum;
//
//	for (ModelTriangle triangle : modelTriangles) {
//		for (int i = 0; i < triangle.vertices.size(); i++) {
//			if (vertex == triangle.vertices[i]) {
//				neighbouringNormals.push_back(triangle.normal);
//				neighbouringNormalsSum += triangle.normal;
//			}
//		}
//	}
//
//	neighbouringNormals.shrink_to_fit();
//	int n = neighbouringNormals.size();
//	glm::vec3 vertexNormal = glm::vec3(neighbouringNormalsSum.x / n, neighbouringNormalsSum.y / n, neighbouringNormalsSum.z / n);
//
//	return vertexNormal;
//}
//
//void cameraFireRays(std::vector<std::vector<RayTriangleIntersection>>& closestTriangleBuffer,
//	glm::mat4 cameraPosition,
//	std::vector<ModelTriangle> triangles,
//	float focalLength,
//	size_t startY,
//	size_t endY) {
//
//	for (size_t y = startY; y < endY; y++) {
//		for (size_t x = 0; x < WIDTH; x++) {
//			int index = 0;
//			RayTriangleIntersection closestRayTriangleIntersection = RayTriangleIntersection(glm::vec3(), INFINITY, ModelTriangle(), -1);
//
//			// For every pixel, check to see which triangles are intersected by the ray, and find the closest one
//			for (ModelTriangle triangle : triangles) {
//				glm::vec3 cameraPosVec3 = glm::vec3(cameraPosition[3][0], cameraPosition[3][1], cameraPosition[3][2]);
//				glm::vec3 rayDirectionOriented = glm::vec3(x, y, 0) - cameraPosVec3; // ray is from the camera to the scene, so to - from = scene - camera
//
//				float x_3d = ((rayDirectionOriented.x - (WIDTH / 2)) * -rayDirectionOriented.z) / (focalLength * SCALE);
//				float y_3d = ((rayDirectionOriented.y - (HEIGHT / 2)) * rayDirectionOriented.z) / (focalLength * SCALE);
//				float z_3d = rayDirectionOriented.z;
//
//				glm::mat3 cameraOrientMat3 = glm::mat3(
//					cameraPosition[0][0], cameraPosition[0][1], cameraPosition[0][2],
//					cameraPosition[1][0], cameraPosition[1][1], cameraPosition[1][2],
//					cameraPosition[2][0], cameraPosition[2][1], cameraPosition[2][2]
//				);
//				glm::vec3 rayDirection = glm::normalize(cameraOrientMat3 * glm::vec3(x_3d, y_3d, z_3d));
//
//				RayTriangleIntersection rayTriangleIntersection = getClosestIntersection(true, cameraPosVec3, triangle, rayDirection, index);
//
//				// check if point on triangle is closer to camera than the previously stored one
//				if (rayTriangleIntersection.distanceFromCamera < closestRayTriangleIntersection.distanceFromCamera) {
//					closestRayTriangleIntersection = rayTriangleIntersection;
//				}
//
//				index++;
//			}
//
//			// Draw point if the ray hit something
//			if (closestRayTriangleIntersection.triangleIndex >= 0) {
//				mtx.lock();
//				closestTriangleBuffer[y][x] = closestRayTriangleIntersection;
//				mtx.unlock();
//			}
//		}
//	}
//}
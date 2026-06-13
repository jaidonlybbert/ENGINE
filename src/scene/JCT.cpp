/*
* Implementation of ray-polygon intersection based on Jordan Curve Theorem (JCT)
*/

#include<vector>
#include<optional>
#include<glm/glm.hpp>
#include "scene/JCT.hpp"


/**
 * Tests if ray intersects with a polygon using the Jordan Curve Theorem (JCT)
 */
bool ray_polygon_intersection(const glm::vec3 ray_origin, const glm::vec3 ray_direction, const std::vector<glm::vec3>& line_strip)
{
	// must have at least 3 points to define a closed polygon
	assert(line_strip.size() >= 3);

	// calculate plane equation for initial plane intersection test
	const auto& start = line_strip.at(0);
	const auto& end1 = line_strip.at(1);
	const auto& end2 = line_strip.at(2);

	// ccw winding order
	const auto& normal = glm::normalize(glm::cross((end1 - start), (end2 - start)));

	// if dot product is positive or zero, ray does not intersect plane of the polygon
	if (glm::dot(ray_direction, normal) >= 0) {
		return false;
	}

	// change-of-basis matrix to project lines onto 2D plane
	const auto& basisTransform = glm::mat2x4(glm::vec4(normal, glm::dot(-normal, start)), glm::vec4(0, 0, 0, 1));

	const auto& pointOfIntersection = ray_plane_intersection(ray_origin, ray_direction, start, normal);

	if (!pointOfIntersection.has_value()) [[unlikely]] {
		return false;
	}

	// Draw a line in an arbitrary direction on the plane from the point of intersection
	const auto& arbitraryPointOnPlane = start + 0.67f * (end1 - start);
	const auto& arbitraryLineDirection = glm::normalize(arbitraryPointOnPlane - pointOfIntersection.value());

	auto intersectionCount{0};
	for (int i = 0; i < line_strip.size() - 1; ++i) {
		const auto& segmentStart = line_strip.at(i);
		const auto& segmentEnd = line_strip.at(i + 1);
		const auto& segmentDirection = segmentEnd - segmentStart;
		const auto& segmentLength = glm::length(segmentDirection);
	}

	return false;
}

std::optional<glm::vec3> ray_line_intersection(const glm::vec3& ray_origin, const glm::vec3& ray_direction, const glm::vec3& segment_start, const glm::vec3& segment_end) {
	const auto& segmentVector = segment_end - segment_start;
	const auto& segmentLength = glm::length(segmentVector);
	const auto& segmentUnitVector = glm::normalize(segmentVector);


}


/**
* Ray-plane intersection algorithm
*/
std::optional<glm::vec3> ray_plane_intersection(const glm::vec3& ray_origin, const glm::vec3& ray_direction, const glm::vec3& plane_p0, const glm::vec3& plane_normal) {

	constexpr float EPSILON = 1E-8;

	auto denom = glm::dot(plane_normal, ray_direction);
	if (!(denom > EPSILON || denom < -EPSILON)) {
		return {};
	}

	auto hitDistance = glm::dot(plane_p0 - ray_origin, plane_normal) / denom;

	if (hitDistance >= 0) {
		return {ray_origin + hitDistance * ray_direction};
	}

	return {};
}

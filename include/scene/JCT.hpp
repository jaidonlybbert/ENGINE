#pragma once
#include <optional>
#include <vector>
#include "glm/glm.hpp"

bool ray_polygon_intersection(const glm::vec3 ray_origin, const glm::vec3 ray_direction, const std::vector<glm::vec3>& line_strip);
std::optional<glm::vec3> ray_plane_intersection(const glm::vec3& ray_origin, const glm::vec3& ray_direction, const glm::vec3& plane_p0, const glm::vec3& plane_normal);

#pragma once
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace ENG {
struct VertexPosNorTex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 texCoord;
};

struct VertexPosColTex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;
};

struct VertexPosNorCol {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec4 color;
};

struct VertexPos {
    glm::vec3 pos;
};

}  // namespace ENG

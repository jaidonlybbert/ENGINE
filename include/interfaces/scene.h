#ifndef ENG_SCENE
#define ENG_SCENE
#include "primitives/mesh.hpp"
#include "pipelines/shader_factory.hpp"
#include<optional>
#include "tiny_gltf.h"
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

using namespace tinygltf;

namespace ENG
{
class Node {
	/*
	 * Base class entities in a scene graph
	 */

public:
	std::string name{};
	glm::vec3 translation{ 0 };
	glm::vec3 rotation{ 0 };

	Node* parent{ nullptr };
	std::vector<Node*> children;
	std::vector<size_t> descriptorSetIds;
	std::optional<ENG_SHADER> shaderId;
	std::optional<Component*> mesh;
	std::optional<Component*> kinematic;
	std::optional<Component*> camera;
};

struct SceneGraph {
	Node* root{ nullptr };
	std::vector<Node> nodes;
};

struct SceneState {
	tinygltf::Model scene;
	SceneGraph graph;
	size_t activeCameraNodeIdx;
	glm::mat4 test_model = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

	double cursor_x;
	double cursor_y;
	std::vector<ENG::Mesh<VertexPosColTex>> posColTexMeshes;
	std::vector<ENG::Mesh<VertexPosNorTex>> posNorTexMeshes;
};

} // end namespace
#endif

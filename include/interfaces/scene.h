#ifndef ENG_SCENE
#define ENG_SCENE
#include "primitives/mesh.hpp"
#include<optional>
#include "tiny_gltf.h"
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace ENG
{
class Node {
	/*
	 * Base class entities in a scene graph
	 */

public:
	std::string name;
	glm::vec3 translation;
	glm::vec3 rotation;

	Node* parent;
	std::vector<Node*> children;
	std::optional<Component*> mesh;
	std::optional<Component*> kinematic;
	std::optional<Component*> camera;
};

template <typename T>
class Pool {
	std::vector<T> elements;

	Pool(const size_t& num_elements)
	{
		elements.reserve(num_elements);
	}

	T& allocate() {
		elements.push_back();
	};
};

struct SceneGraph {
	Node* root;
	std::vector<Node> nodes;
};

struct SceneState {
	tinygltf::Model scene;
	Node* graph;
	size_t activeCameraNodeIdx;
	glm::mat4 test_model = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

	double cursor_x;
	double cursor_y;
	std::vector<Mesh<VertexPosColTex>> posColTexMeshes;
	std::vector<Mesh<VertexPosNorTex>> posNorTexMeshes;
};

} // end namespace
#endif

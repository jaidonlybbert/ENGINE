#ifndef ENG_SCENE
#define ENG_SCENE
#include<optional>
#include<random>

#include "tiny_gltf.h"
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "gui/Gui.hpp"
#include "scene/Mesh.hpp"

using namespace tinygltf;

namespace ENG
{

class Camera : public Component {
public:
	Camera(const tinygltf::Camera& camera) {
		fovy = static_cast<float>(camera.perspective.yfov);
		aspect = static_cast<float>(camera.perspective.aspectRatio);
		znear = static_cast<float>(camera.perspective.znear);
		zfar = static_cast<float>(camera.perspective.zfar);
	}
	float fovy {0.f};
	float aspect {0.f}; 
	float znear {0.f}; 
	float zfar {0.f};
};

class Node {
	/*
	 * Base class entities in a scene graph
	 */

public:
	unsigned int nodeId;  // index into "nodes" vector
	std::string name{};
	glm::vec3 scale { 1.f };
	glm::vec3 translation{ 0.f };
	glm::quat rotation{ 1.f, 0.f, 0.f, 0.f };

	Node* parent{ nullptr };
	std::vector<Node*> children;
	std::vector<size_t> descriptorSetIds;
	std::optional<std::string> shaderId;
	Component* mesh { nullptr };
	Component* kinematic { nullptr };
	Component* camera { nullptr };
	bool visible{ true };
};

glm::mat4 transformation_matrix(const Node& node);

struct SceneGraph {
	Node* root{ nullptr };
	std::vector<Node> nodes;
	std::vector<Camera> cameras;
};

struct SceneState {
	GUISettings settings;
	SceneGraph graph;
	size_t activeCameraNodeIdx;
	int activeNodeIdx{ 0 };
	uint32_t selectedWorldFace{ 0 };

	double cursor_x;
	double cursor_y;
	std::vector<ENG::Mesh<VertexPosColTex>> posColTexMeshes;
	std::vector<ENG::Mesh<VertexPosNorTex>> posNorTexMeshes;
	std::vector<ENG::Mesh<VertexPos>> posMeshes;
	std::vector<ENG::Mesh<VertexPosNorCol>> posNorColMeshes;
	std::vector<glm::mat4> modelMatrices;
};

struct GameState {
	std::mt19937 randomizer;
};

Camera* get_active_camera(const SceneState& sceneState);
Node* find_node_by_name(const SceneGraph& graph, const std::string& name);

} // end namespace
#endif

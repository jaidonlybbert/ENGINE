#include <optional>
#include <tiny_gltf.h>
#include <glm/matrix.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/quaternion.hpp>

class StateObject
{
public:
private:
};

class Node : StateObject
{
public:
	Node(const tinygltf::Node &gltf_node) {
		if (gltf_node.camera != -1) {
			cameraIdx = gltf_node.camera;
		}

		if (gltf_node.children.size() > 0) {
			children = gltf_node.children;
		}

		if (gltf_node.skin != -1) {
			skin = gltf_node.skin;
		}

		if (gltf_node.matrix.size() == 16) {
			const auto &m = gltf_node.matrix;
			// Assumed row-major layout
			matrix = glm::mat4(
				m[0], m[1], m[2], m[3],
				m[4], m[5], m[6], m[7],
				m[8], m[9], m[10], m[11],
				m[12], m[13], m[14], m[15]);
		}

		if (gltf_node.mesh != -1) {
			mesh = gltf_node.mesh;
		}
		
		if (gltf_node.rotation.size() == 4) {
			const auto &r = gltf_node.rotation;
			rotation = glm::quat(r[0], r[1], r[2], r[3]);
		}

		if (gltf_node.scale.size() == 3) {
			const auto &s = gltf_node.scale;
			scale = glm::vec3(s[0], s[1], s[2]);
		}

		if (gltf_node.translation.size() == 3) {
			const auto &t = gltf_node.translation;
			translation = glm::vec3(t[0], t[1], t[2]);
		}

		if (gltf_node.weights.size() > 0) {
			weights = gltf_node.weights;
		}

		if (gltf_node.name.size() > 0) {
			name = gltf_node.name;
		}
	}

private:
	std::optional<int> cameraIdx;
	std::optional<std::vector<int>> children;
	std::optional<int> skin;
	glm::mat4 matrix = glm::mat4(1.0f);
	std::optional<int> mesh;
	glm::quat rotation = glm::quat{0., 0., 0., 1.};
	glm::vec3 scale = glm::vec3{1., 1., 1.};
	glm::vec3 translation = glm::vec3{0., 0., 0.};
	std::optional<std::vector<double>> weights;
	std::optional<std::string> name;

	Node()=delete;
	Node(const Node &other)=delete;
	Node operator=(const Node &other)=delete;
};

class Camera : StateObject
{
public:
	Camera(const tinygltf::Camera &gltf_camera) {

	}
};

class Scene : StateObject
{
};



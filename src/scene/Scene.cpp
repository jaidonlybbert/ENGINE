#include "scene/Scene.hpp"
#include "logger/Logging.hpp"

namespace ENG
{

Node& get_node_by_id(SceneGraph& sceneGraph, const size_t nodeId)
{
	assert(nodeId < sceneGraph.nodes.size());
	return sceneGraph.nodes.at(nodeId);
}

glm::mat4 transformation_matrix(const Node& node)
{
	glm::mat4 model_matrix = glm::mat4(1.0f);
	model_matrix = glm::scale(model_matrix, node.scale);
	model_matrix = model_matrix * glm::mat4_cast(node.rotation);
	model_matrix = glm::translate(model_matrix, node.translation);

	return model_matrix;
}

Camera* get_active_camera(const SceneState& sceneState)
{
	const auto& cameraNode = sceneState.graph.nodes.at(sceneState.activeCameraNodeIdx);

	ENG_LOG_TRACE("Updating UniformBuffer from camera" << std::endl);
	ENG_LOG_TRACE("Camera node name: " << cameraNode.name << "at idx: " << sceneState.activeCameraNodeIdx << std::endl); 
	ENG_LOG_TRACE("Cameras vec address: " << &sceneState.graph.cameras << std::endl);
	ENG_LOG_TRACE("Camera address: " << cameraNode.camera << std::endl);
	ENG_LOG_TRACE("Camera ptr retrieved" << std::endl);

	auto* cameraPtr = dynamic_cast<ENG::Camera*>(cameraNode.camera);

	assert(cameraPtr != nullptr);

	ENG_LOG_TRACE("Camera node cast" << std::endl);
	ENG_LOG_TRACE("Camera address: " << cameraPtr << std::endl);

	return cameraPtr;
}


Node* find_node_by_name(const SceneGraph& graph, const std::string& name)
{
	for (const auto& node : graph.nodes) {
		if (node.name == name) return const_cast<Node*>(&node);
	}

	return nullptr;
}

} // end namespace

#include "interfaces/scene.h"
#include "interfaces/logging.h"

namespace ENG
{

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

	auto* cameraPtr = CAST_OR_DIE(dynamic_cast<ENG::Camera*>(cameraNode.camera));

	ENG_LOG_TRACE("Camera node cast" << std::endl);
	ENG_LOG_TRACE("Camera address: " << cameraPtr << std::endl);

	return cameraPtr;
}

} // end namespace

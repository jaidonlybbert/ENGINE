#include "interfaces/scene.h"

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

} // end namespace

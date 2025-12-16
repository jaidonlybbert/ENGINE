#include "glm/glm.hpp"

namespace ENG
{
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
		glm::vec3 color;
	};

	struct VertexPos {
		glm::vec3 pos;
	};

} // end namespace

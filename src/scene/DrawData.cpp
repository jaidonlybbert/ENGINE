// Stripped down renderer-independent data buffers needed for rendering
#include<variant>
#include<vector>

#include "glm/glm.hpp"
#include "scene/Primitives.hpp"

namespace ENG {

	using vbuffer = std::variant<
		std::vector<VertexPosNorTex>,
		std::vector<VertexPosColTex>,
		std::vector<VertexPosNorCol>,
		std::vector<VertexPos>
	>;	

	using ibuffer = std::vector<size_t>;

struct DrawData
{
	std::vector<vbuffer> vertexBuffers;
	std::vector<ibuffer> indexBuffers;
};

} // end namespace

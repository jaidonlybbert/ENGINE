#include "vulkan/vulkan.h"
#include "scene/Gltf.hpp"
#include "logger/Logging.hpp"
#include "scene/Mesh.hpp"
#include "renderer/vk_adapter/VkAdapter.hpp"

namespace ENG
{

static VkFormat get_vk_format_from_tinygltf_accessor(const tinygltf::Accessor& acc, size_t& size_bytes)
{
	VkFormat retval{ VkFormat::VK_FORMAT_UNDEFINED };
	const auto& ctype = acc.componentType;
	const auto& type = acc.type;
	assert(ctype != -1);
	assert(type != -1);

	if (ctype == TINYGLTF_COMPONENT_TYPE_FLOAT)
	{
		retval = 
			type == TINYGLTF_TYPE_VEC2 ? VkFormat::VK_FORMAT_R32G32_SFLOAT :
			type == TINYGLTF_TYPE_VEC3 ? VkFormat::VK_FORMAT_R32G32B32_SFLOAT :
			type == TINYGLTF_TYPE_VEC4 ? VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT : VkFormat::VK_FORMAT_UNDEFINED;
	}

	size_bytes = tinygltf::GetNumComponentsInType(type) * tinygltf::GetComponentSizeInBytes(ctype);

	assert(retval != VK_FORMAT_UNDEFINED);
	return retval;
}

bool load_gltf_model(const std::filesystem::path gltf_path, tinygltf::Model& model) {
	auto loader = tinygltf::TinyGLTF();
	std::string err;
	std::string warn;

	bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, gltf_path.string());

	if (!warn.empty()) {
		ENG_LOG_ERROR("Warn: " << warn.c_str() << std::endl);
	}

	if (!err.empty()) {
		ENG_LOG_ERROR("Err: " << err.c_str() << std::endl);
	}

	if (!ret) {
		ENG_LOG_ERROR("Failed to parse glTF" << std::endl);
	} else {
		ENG_LOG_DEBUG("Load glTF successful" << std::endl);
	}

	return ret;
}

void get_vertex_and_index_buffer(
	const tinygltf::Primitive& primitive,
	const tinygltf::Model& model,
	std::vector<VertexPosNorTex>& vertices,
	std::vector<uint32_t>& indices)
{
	const auto& pos_acc = model.accessors[primitive.attributes.at("POSITION")];
	const auto& nor_acc = model.accessors[primitive.attributes.at("NORMAL")];
	const auto& tex_acc = model.accessors[primitive.attributes.at("TEXCOORD_0")];
	assert(primitive.indices >= 0);
	const auto& ind_acc = model.accessors[primitive.indices];

	const auto& pos_bv = model.bufferViews[pos_acc.bufferView];
	const auto& nor_bv = model.bufferViews[nor_acc.bufferView];
	const auto& tex_bv = model.bufferViews[tex_acc.bufferView];
	const auto& ind_bv = model.bufferViews[ind_acc.bufferView];

	const auto& pos_buff = model.buffers[pos_bv.buffer];
	const auto& nor_buff = model.buffers[nor_bv.buffer];
	const auto& tex_buff = model.buffers[tex_bv.buffer];
	const auto& ind_buff = model.buffers[ind_bv.buffer];

	size_t pos_size{ get_size_bytes_from_tinygltf_accessor(pos_acc) };
	size_t nor_size{ get_size_bytes_from_tinygltf_accessor(nor_acc) };
	size_t tex_size{ get_size_bytes_from_tinygltf_accessor(tex_acc) };
	size_t ind_size{ get_size_bytes_from_tinygltf_accessor(ind_acc) };
	assert(pos_size == 12);
	assert(nor_size == 12);
	assert(tex_size == 8);
	assert(ind_size == 2);

	const size_t num_elements = pos_bv.byteLength / pos_size;
	const size_t num_indices = ind_bv.byteLength / ind_size;
	assert(num_elements == nor_bv.byteLength / nor_size);
	assert(num_elements == tex_bv.byteLength / tex_size);

	vertices.resize(num_elements);
	indices.resize(num_indices);

	for (size_t i = 0; i < num_elements; ++i)
	{
		// Assumes data is not interleaved
		assert(pos_bv.byteStride == 0);
		assert(nor_bv.byteStride == 0);
		assert(tex_bv.byteStride == 0);

		std::memcpy(&vertices.at(i).pos, &pos_buff.data[pos_bv.byteOffset + i * pos_size], sizeof(vertices[0].pos));
		std::memcpy(&vertices.at(i).normal, &nor_buff.data[nor_bv.byteOffset + i * nor_size], sizeof(vertices[0].normal));
		std::memcpy(&vertices.at(i).texCoord, &tex_buff.data[tex_bv.byteOffset + i * tex_size], sizeof(vertices[0].texCoord));
	}
	for (size_t i = 0; i < num_indices; ++i)
	{
		assert(i < indices.size());
		unsigned short tmp;
		std::memcpy(&tmp, &ind_buff.data[ind_bv.byteOffset + i * ind_size], sizeof(unsigned short));
		indices.at(i) = static_cast<uint32_t>(tmp);
	}
}

void get_vertex_and_index_buffer(
	const tinygltf::Primitive& primitive,
	const tinygltf::Model& model,
	std::vector<VertexPosColTex>& vertices,
	std::vector<uint32_t>& indices)
{
	ENG_LOG_DEBUG("debug PosColTex mesh entry" << std::endl);

	const auto& pos_acc = model.accessors[primitive.attributes.at("POSITION")];
	const auto& col_acc = model.accessors[primitive.attributes.at("COLOR0")];
	const auto& tex_acc = model.accessors[primitive.attributes.at("TEXCOORD_0")];
	assert(primitive.indices >= 0);
	const auto& ind_acc = model.accessors[primitive.indices];

	const auto& pos_bv = model.bufferViews[pos_acc.bufferView];
	const auto& col_bv = model.bufferViews[col_acc.bufferView];
	const auto& tex_bv = model.bufferViews[tex_acc.bufferView];
	const auto& ind_bv = model.bufferViews[ind_acc.bufferView];

	const auto& pos_buff = model.buffers[pos_bv.buffer];
	const auto& col_buff = model.buffers[col_bv.buffer];
	const auto& tex_buff = model.buffers[tex_bv.buffer];
	const auto& ind_buff = model.buffers[ind_bv.buffer];

	size_t pos_size{ get_size_bytes_from_tinygltf_accessor(pos_acc) };
	size_t col_size{ get_size_bytes_from_tinygltf_accessor(col_acc) };
	size_t tex_size{ get_size_bytes_from_tinygltf_accessor(tex_acc) };
	size_t ind_size{ get_size_bytes_from_tinygltf_accessor(ind_acc) };
	assert(pos_size == 12);
	assert(col_size == 12);
	assert(tex_size == 8);
	assert(ind_size == 2);

	const size_t num_elements = pos_bv.byteLength / pos_size;
	const size_t num_indices = ind_bv.byteLength / ind_size;
	assert(num_elements == col_bv.byteLength / col_size);
	assert(num_elements == tex_bv.byteLength / tex_size);
	assert(num_elements == ind_bv.byteLength / ind_size);

	ENG_LOG_DEBUG("Debug posCoTex pos1 " << std::endl);
	vertices.resize(num_elements);
	indices.resize(num_indices);
	for (size_t i = 0; i < num_elements; ++i)
	{
		VertexPosColTex vert;
		// Assumes data is not interleaved
		assert(pos_bv.byteStride == 0);
		assert(col_bv.byteStride == 0);
		assert(tex_bv.byteStride == 0);
		vert.pos = static_cast<glm::vec3>(pos_buff.data[pos_bv.byteOffset + i * pos_size]);
		vert.color = static_cast<glm::vec3>(col_buff.data[col_bv.byteOffset + i * col_size]);
		vert.texCoord = static_cast<glm::vec2>(tex_buff.data[tex_bv.byteOffset + i * tex_size]);

		vertices[i] = vert;
	}
	for (size_t i = 0; i < num_indices; ++i)
	{
		indices[i] = static_cast<uint32_t>(ind_buff.data[ind_bv.byteOffset + i * ind_size]);
	}

	ENG_LOG_DEBUG("Debug posCoTex pos2" << std::endl);
}

void load_gltf_mesh_attributes(
	VkAdapter& adapter,
	const tinygltf::Model& model,
	const tinygltf::Primitive& primitive,
	const uint32_t nodeId)
{
	// Assumes vertex data is NOT interleaved in gltf buffer
	// Each attribute exists in a contiguous section of the gltf buffer, and gets it's own
	// binding & attribute description
	// TODO: fix this so it doesn't hard-code shaders
	if (primitive.attributes.contains("POSITION") && primitive.attributes.contains("COLOR0")
		&& primitive.attributes.contains("TEXCOORD_0"))
	{
		std::vector<uint32_t> indices;
		std::vector<VertexPosColTex> vertices;

		get_vertex_and_index_buffer(primitive, model, vertices, indices);

		adapter.graphicsEventQueue.push(
			BindHostMeshDataEvent{
				HostMeshData{
					std::move(vertices),
					std::move(indices),
					"VertexPosColTex",
					"PosColTex"
				},
				nodeId
			}
		);
		//auto &mesh = sceneState.posColTexMeshes.emplace_back(device, physicalDevice, commands, mesh_name, model, primitive, graphicsQueue);
		//eng_node.mesh_idx = sceneState.posColTexMeshes.size() - 1;
		//eng_node.mesh_type = "VertexPosColTex";
		//eng_node.shaderId = "PosColTex";
	}
	else if (primitive.attributes.contains("POSITION") && primitive.attributes.contains("NORMAL")
		&& primitive.attributes.contains("TEXCOORD_0"))
	{
		std::vector<uint32_t> indices;
		std::vector<VertexPosNorTex> vertices;

		get_vertex_and_index_buffer(primitive, model, vertices, indices);

		adapter.graphicsEventQueue.push(
			BindHostMeshDataEvent{
				HostMeshData{
					std::move(vertices),
					std::move(indices),
					"VertexPosNorTex",
					"PosNorTex"
				},
				nodeId
			}
		);

		//auto& mesh = sceneState.posNorTexMeshes.emplace_back(device, physicalDevice, commands, mesh_name, model, primitive, graphicsQueue);
		//eng_node.mesh_idx = sceneState.posNorTexMeshes.size() - 1;
		//eng_node.mesh_type = "VertexPosNorTex";
		//eng_node.shaderId = "PosNorTex";
	}
}

void load_gltf_node(
	VkAdapter& adapter,
	const tinygltf::Node& node,
	SceneState& sceneState,
	ENG::Node& eng_node,
	const tinygltf::Model& model)
{
	if (node.camera != -1)
	{
		auto* new_cam = &sceneState.graph.cameras.emplace_back(model.cameras.at(node.camera));
		eng_node.camera = new_cam;
		ENG_LOG_TRACE("Set camera on node with name: " << eng_node.name << std::endl);
		ENG_LOG_TRACE("Camera address: " << eng_node.camera << std::endl);
		ENG_LOG_TRACE("Cameras vec address: " << &sceneState.graph.cameras << std::endl);
	}

	if (node.mesh < 0) return;
	const auto& gltf_mesh = model.meshes[node.mesh];
	for (const auto& primitive : gltf_mesh.primitives)
	{
		const auto& mesh_name = gltf_mesh.name;
		load_gltf_mesh_attributes(adapter, model, primitive, eng_node.nodeId);
	}
}

bool load_gltf(
	VkAdapter& adapter,
	const std::filesystem::path gltf_path,
	SceneState& sceneState,
	Node& attachmentPoint)
{
	tinygltf::Model model;
	if (!load_gltf_model(gltf_path, model)) {
		return false;
	}

	// Since we are attaching this gltf scene to ours, the node index is offset by
	// the number of nodes loaded before this function is called
	const auto& nodeOffset = sceneState.graph.nodes.size();

	ENG_LOG_DEBUG("Nodes found:" << std::endl);
	for (const auto& node : model.nodes) {
		auto& newNode = sceneState.graph.create_node();
		// Default to adding node as child of root
		attachmentPoint.children.push_back(&newNode);
		newNode.name = node.name;
		newNode.parent = &attachmentPoint;	

		ENG_LOG_DEBUG("\t" << node.name << "\t" << newNode.nodeId << std::endl);
		if (node.name == "main_camera") {
			sceneState.activeCameraNodeIdx = newNode.nodeId;
			ENG_LOG_DEBUG("Camera node set with idx: " << newNode.nodeId << std::endl);
		}

		load_gltf_node(adapter, node, sceneState, newNode, model);
	}

	// Iterate again now that all nodes are loaded, and update parent-child relationships
	size_t nodeCounter = 0;
	for (const auto& node : model.nodes) {
		if (node.children.size() > 0)
		{
			assert(nodeCounter + nodeOffset < sceneState.graph.nodes.size());
			auto& engParentNode = sceneState.graph.nodes.at(nodeCounter + nodeOffset);
			for (const auto& childIdx : node.children)
			{
				const auto& engNodeChildIdx = childIdx + nodeOffset;
				assert(engNodeChildIdx < sceneState.graph.nodes.size());
				auto& engChild = sceneState.graph.nodes.at(engNodeChildIdx);
				// Clear child from old parent
				if (engChild.parent)
				{
					std::erase_if(engChild.parent->children, [engNodeChildIdx](const ENG::Node* oldParentChild) {
							return (oldParentChild->nodeId == engNodeChildIdx);
						});
				}

				engParentNode.children.push_back(&engChild);
				engChild.parent = &engParentNode;
			}
		}
		nodeCounter++;
	}

	return true;
}

} // end namespace

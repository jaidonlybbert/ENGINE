#include <optional>
#include "scenes/ProceduralGeometry.hpp"
#include "scene/Mesh.hpp"
#include "renderer/vk_adapter/VkAdapter.hpp"
#include "application/ConcurrentQueue.hpp"

void create_world_polyhedra(VkRenderer& app, SceneState& sceneState)
{
	// Seed randomizer
	// first: 20398475
	sceneState.randomizer.seed(20398475);

	// Create triangulated goldberg polyhedra including faceIds
	{
		auto mesh = pmp::icosphere(5);
		dual(mesh);

		// apply a face index for each face (preserved after triangulation)
		auto faceId = mesh.add_face_property<uint32_t>("f:faceId");

		auto facecount = static_cast<uint32_t>(0);
		for (auto f : mesh.faces())
		{
			faceId[f] = facecount++;
			ENG_LOG_TRACE("PRE TRIANGULARIZATION FACEID: " << faceId[f] << std::endl);
		}

		const auto& UNVISITED = 0;
		const auto& OCEAN = 1;
		const auto& LAND = 2;

		const auto& oceanblue = glm::vec4(0.f, 0.369f, 0.722f, 1.0f);
		const auto& forestgreen = glm::vec4(0.133f, 0.545f, 0.133f, 1.f);
		const auto& sunset = glm::vec4(0.98f, 0.77f, 0.4f, 1.f);

		std::vector<glm::vec4> colorMap{
			glm::vec4(0.4f, 0.4f, 0.7f, 1.0f),
			oceanblue,
			forestgreen
		};

		// are faces land or ocean? indexed by faceId
		std::vector<int> climate(facecount, 0);

		/// PARAMETERS AFFECTING GENERATION OUTPUT
		const auto& continentGrowthFactor = 0.45f;
		
		// Vector of face colors indexed by faceId
		std::vector<glm::vec4> faceColors;
		faceColors.resize(facecount);

		// First 12 faces are pentagons
		// Choose a random 5 pentagons to be continents
		std::vector<size_t> samples;
		std::vector<size_t> choices{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
		std::sample(choices.begin(), choices.end(), std::back_inserter(samples), 5, sceneState.randomizer);

		std::uniform_real_distribution<float> randFloatDistribution(0.f, 1.f);
		// Grow continents (mark faces as land) according to parameters
		for (const auto& sample : samples)
		{
			// land is traversed "depth-first" / longest-shore first
			// once land perimeter is complete (the landmass is sourounded by water), start popping off the stack
			// until a land tile with an unvisited shore is found, then follow that inland path until it is surrounded
			// by water or land, and repeat
			auto i = 0;
			auto currentF = pmp::Face(sample);
			climate[sample] = LAND;

			struct landStackContext {
				pmp::Halfedge start;
				pmp::Halfedge current;
			};

			std::stack<landStackContext> landStack; 
			auto start_h = *mesh.halfedges(currentF).begin();
			auto h = start_h;
			landStack.push({start_h, h});
			while (!landStack.empty())
			{
				auto f = mesh.face(mesh.opposite_halfedge(h));  // opposite face from current h
				ENG_LOG_DEBUG("At iteration " << i++ << " face is " << climate[f.idx()] << std::endl);

				// If previously visited, simply iterate the halfedge, and re-evaluate next iteration
				if (climate[f.idx()] == OCEAN || climate[f.idx()] == LAND)
				{
					h = mesh.next_halfedge(h);
				}

				// If unvisited, assign climate randomly and if land, push halfedge to stack, move to face, continue
				else if (climate[f.idx()] == UNVISITED)
				{
					if (randFloatDistribution(sceneState.randomizer) < continentGrowthFactor)
					{
						climate[f.idx()] = LAND;
						landStack.push({ start_h, h });
						ENG_LOG_DEBUG("PUSH: " << h.idx() << std::endl);
						start_h = mesh.opposite_halfedge(h);
						h = start_h;
						continue;
					}
					else
					{
						climate[f.idx()] = OCEAN;
						continue;
					}
				}

				if (h == start_h)
				{
					assert(!landStack.empty());
					auto context = landStack.top();
					landStack.pop();
					start_h = context.start;
					h = context.current;
					ENG_LOG_DEBUG("POP: " << h.idx() << std::endl);
				}

			}
		}

		// "hole-filling" algorithm - if any ocean tiles are surrounded by land tiles, change them to land tiles
		for (auto f : mesh.faces())
		{
			if (climate[f.idx()] != OCEAN)
			{
				continue;
			}

			auto count{ 0U };
			for (auto h : mesh.halfedges(f))
			{
				if (climate[mesh.face(mesh.opposite_halfedge(h)).idx()] == LAND)
				{
					++count;
				}
			}

			if (count >= 4)
			{
				climate[f.idx()] = LAND;
			}
		}


		// "hole-filling" algorithm - if any ocean tiles are surrounded by land tiles, change them to land tiles
		for (auto f : mesh.faces())
		{
			if (climate[f.idx()] != OCEAN)
			{
				continue;
			}

			auto count{ 0U };
			for (auto h : mesh.halfedges(f))
			{
				if (climate[mesh.face(mesh.opposite_halfedge(h)).idx()] == LAND)
				{
					++count;
				}
			}

			if (count >= 6)
			{
				climate[f.idx()] = LAND;
			}
		}

		// Fill unvisited with ocean tiles
		for (auto f : mesh.faces())
		{
			if (climate[f.idx()] == UNVISITED)
			{
				climate[f.idx()] = OCEAN;
			}
		}

		// paint faces according to climate
		for (size_t i = 0; i < faceColors.size(); ++i)
		{
			faceColors[i] = colorMap[climate[i]];
		}

		// renderAdapter->allocateMemory(facecount * sizeof(glm::vec4), )
		// renderAdapter->writeMappedMemory(faceColors.data(), facecount * sizeof(glm::vec4));

		app.createFaceColorBuffers(facecount);
		const auto& colorBufferSize = faceColors.size() * sizeof(glm::vec4);
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			memcpy(app.faceColorBuffersMapped[i], faceColors.data(), colorBufferSize);
		}

		triangulate_as_triangle_fan_preserving_face_ids(mesh);

		// Vector of faceIds indexed by primitiveId (gl_PrimitiveID) to lookup faceID in shader
		std::vector<uint32_t> primitiveToFaceIdMap;
		primitiveToFaceIdMap.reserve(mesh.faces_size());
		faceId = mesh.get_face_property<uint32_t>("f:faceId");
		for (auto f : mesh.faces())
		{
			primitiveToFaceIdMap.push_back(faceId[f]);
			ENG_LOG_TRACE("POST TRIANGULURIZATION FACEID: " << faceId[f] << std::endl);
		}
		load_pmp_mesh(mesh, "GoldbergMesh", "GoldbergPolyhedra", app, sceneState);

		app.createFaceIdBuffers(mesh.faces_size());
		const auto& bufferSize = primitiveToFaceIdMap.size() * sizeof(uint32_t);
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			memcpy(app.faceIdMapBuffersMapped[i], primitiveToFaceIdMap.data(), bufferSize);
		}
	}
}

void addBoundingBoxChild(ENG::Node* node, VkRenderer& app, const std::string &bbName, SceneState& sceneState)
{
	if (node == nullptr)
	{
		return;
	}
		
	if (!node->mesh_idx.has_value())
	{
		ENG_LOG_ERROR("Attempted to create bounding box on node " << node->name << " which has no mesh index");
		return;
	}
	const auto& mesh = sceneState.posNorTexMeshes.at(node->mesh_idx.value());
	const auto& minXIt = std::min_element(mesh.vertices.begin(), mesh.vertices.end(), [](const VertexPosNorTex& rhs, const VertexPosNorTex& lhs) {
		return (rhs.pos.x < lhs.pos.x);
	});
	const auto& minYIt = std::min_element(mesh.vertices.begin(), mesh.vertices.end(), [](const VertexPosNorTex& rhs, const VertexPosNorTex& lhs) {
		return (rhs.pos.y < lhs.pos.y);
	});
	const auto& minZIt = std::min_element(mesh.vertices.begin(), mesh.vertices.end(), [](const VertexPosNorTex& rhs, const VertexPosNorTex& lhs) {
		return (rhs.pos.z < lhs.pos.z);
	});
	const auto& maxXIt = std::max_element(mesh.vertices.begin(), mesh.vertices.end(), [](const VertexPosNorTex& rhs, const VertexPosNorTex& lhs) {
		return (rhs.pos.x < lhs.pos.x);
	});
	const auto& maxYIt = std::max_element(mesh.vertices.begin(), mesh.vertices.end(), [](const VertexPosNorTex& rhs, const VertexPosNorTex& lhs) {
		return (rhs.pos.y < lhs.pos.y);
	});
	const auto& maxZIt = std::max_element(mesh.vertices.begin(), mesh.vertices.end(), [](const VertexPosNorTex& rhs, const VertexPosNorTex& lhs) {
		return (rhs.pos.z < lhs.pos.z);
	});

	const auto& maxX = maxXIt->pos.x;
	const auto& maxY = maxYIt->pos.y;
	const auto& maxZ = maxZIt->pos.z;
	const auto& minX = minXIt->pos.x;
	const auto& minY = minYIt->pos.y;
	const auto& minZ = minZIt->pos.z;

	std::vector<VertexPos> bbVertices {
		{ { minX, minY, maxZ } },
		{ { maxX, minY, maxZ } },
		{ { maxX, maxY, maxZ } },
		{ { minX, maxY, maxZ } },
		{ { minX, minY, minZ } },
		{ { maxX, minY, minZ } },
		{ { maxX, maxY, minZ } },
		{ { minX, maxY, minZ } },
	};

	std::vector<uint32_t> bbIndices {
		0, 1, 1, 2, 2, 3, 3, 0,
		4, 5, 5, 6, 6, 7, 7, 4,
		0, 4, 1, 5, 2, 6, 3, 7
	};

	auto& bbMesh = sceneState.posMeshes.emplace_back(app.device, app.physicalDevice, app.commands.get(), bbName, bbVertices, bbIndices, app.graphicsQueue);
	auto& bbNode = sceneState.graph.nodes.emplace_back();
	bbNode.name = bbName;
	bbNode.nodeId = sceneState.graph.nodes.size() - 1;
	bbNode.parent = node;
	bbNode.mesh_type = "VertexPos";
	bbNode.mesh_idx = sceneState.posMeshes.size() - 1;
	bbNode.shaderId = "PosBB";
	node->children.push_back(&bbNode);
}


void create_tetrahedron_no_pmp(SceneState& sceneState, ConcurrentQueue<BindHostMeshDataEvent>& meshBindQueue)
{
	std::vector<VertexPosNorCol> tetraVertices {
		{ {1.,  1.,  1.} },
		{ {1., -1., -1.} },
		{ {-1., 1., -1.} },
		{ {-1., -1., 1.} }
	};

	std::vector<uint32_t> tetraIndices {
		0, 1, 2,
		0, 3, 1,
		0, 2, 3,
		3, 2, 1
	};

	std::vector<glm::vec3> colors {
		{1.0, 0.5, 0.5},
		{0.5, 1.0, 0.5},
		{0.5, 0.5, 1.0},
		{0.5, 0.5, 0.5},
	};

	// vertices are duplicated for face-specific color
	std::vector<VertexPosNorCol> tetraVerticesDuplicated {
		tetraVertices.at(0), tetraVertices.at(1), tetraVertices.at(2),
		tetraVertices.at(0), tetraVertices.at(3), tetraVertices.at(1),
		tetraVertices.at(0), tetraVertices.at(2), tetraVertices.at(3),
		tetraVertices.at(3), tetraVertices.at(2), tetraVertices.at(1)
	};

	for (size_t i = 0; i < 4; ++i)
	{
		auto normal = glm::normalize(
			glm::cross(
				tetraVerticesDuplicated.at(i * 3 + 1).pos - tetraVerticesDuplicated.at(i * 3).pos,
				tetraVerticesDuplicated.at(i * 3 + 2).pos - tetraVerticesDuplicated.at(i * 3).pos
			)
		);
		for (size_t j = 0; j < 3; ++j)
		{
			tetraVerticesDuplicated.at(i * 3 + j).normal = normal;
			tetraVerticesDuplicated.at(i * 3 + j).color = colors.at(i);
		}
	}

	auto& tetraNode = sceneState.graph.nodes.emplace_back();
	tetraNode.name = "Tetrahedron";
	tetraNode.nodeId = sceneState.graph.nodes.size() - 1;
	tetraNode.parent = sceneState.graph.root;
	sceneState.graph.root->children.push_back(&tetraNode);

	meshBindQueue.push(
		BindHostMeshDataEvent{
			HostMeshData{
				std::move(tetraVerticesDuplicated),
				std::move(tetraIndices),
				"VertexPosNorCol",
				"PosNorCol"
			},
			tetraNode.nodeId
		}
	);
}


void init_for_vulkan_tetrahedron(
	VkAdapter& adapter,
	SceneState& sceneState
)
{
	while (!adapter.meshBindEventQueue.empty())
	{
		auto bindEvent = adapter.meshBindEventQueue.pop();

		auto& hostMesh = bindEvent.meshData;
		auto& node = get_node_by_id(sceneState.graph, bindEvent.nodeId);

		if (std::holds_alternative<std::vector<VertexPosNorCol>>(hostMesh.vertexBuffer))
		{

			const auto drawIdx = adapter.emplaceDrawData(
				{
					DrawDataProperties::CLEAR,
					{bindEvent.nodeId},
					{std::nullopt},  // no descriptor sets
					adapter.create_draw_data(
						std::move(std::get<std::vector<VertexPosNorCol>>(hostMesh.vertexBuffer)),
						std::move(hostMesh.indexBuffer)
					),
				}
			);

			auto* drawDataPtr = adapter.getDrawDataFromIdx(drawIdx);
			assert(drawDataPtr);

			adapter.commandCompletionHandlerQueue.push([drawDataPtr, &node] {
				set_property(*drawDataPtr, DrawDataProperties::INDEX_BUFFERS_INITIALIZED);
				set_property(*drawDataPtr, DrawDataProperties::VERTEX_BUFFERS_INITIALIZED); 
				ENG_LOG_INFO("Created draw data for " << node.name << std::endl);
			});

			node.mesh_type = bindEvent.meshData.meshType;
			node.shaderId = bindEvent.meshData.shaderId;
			node.draw_data_idx = drawIdx;
		}
	}
}

void initializeWorldScene(VkRenderer& renderer, VkAdapter& adapter, SceneState& sceneState) {
	// TODO: implement pools to avoid reference invalidation on reallocation problem
	sceneState.posColTexMeshes.reserve(100);
	sceneState.posNorTexMeshes.reserve(100);
	sceneState.posMeshes.reserve(100);
	sceneState.posNorColMeshes.reserve(100);
	sceneState.graph.nodes.reserve(100);
	sceneState.graph.cameras.reserve(10);

	auto& attachmentPoint = sceneState.graph.nodes.emplace_back();
	sceneState.graph.root = &attachmentPoint;
	sceneState.graph.root->name = "Root";

	load_gltf(renderer.device, renderer.physicalDevice, renderer.graphicsQueue, 
		renderer.commands.get(), get_gltf_dir(), sceneState, attachmentPoint);
	auto& cameraNode = sceneState.graph.nodes.at(sceneState.activeCameraNodeIdx);

	// const auto& meshName = std::string("Room");
	// ENG::loadModel(renderer.device, renderer.physicalDevice, renderer.commands.get(), meshName, 
	// 	renderer.graphicsQueue, get_model_dir(), sceneState, attachmentPoint);

	// Create bounding box around Suzanne
	//auto* suzanneNode = find_node_by_name(sceneState.graph, "Suzanne");
	//addBoundingBoxChild(suzanneNode, renderer, "SuzanneBoundingBox", sceneState);

	// Create Tetrahedron
	// create_tetrahedron_no_pmp(renderer, sceneState);

	ENG_LOG_INFO("Creating tetrahedron2" << std::endl);
	create_tetrahedron_no_pmp(sceneState, adapter.meshBindEventQueue);

	ENG_LOG_INFO("Init for vulkan tetrahedron2" << std::endl);
	init_for_vulkan_tetrahedron(adapter, sceneState);
	ENG_LOG_INFO("complete tetrahedron2" << std::endl);

	// Create world mesh
	//create_world_polyhedra(renderer, sceneState);

	// Create modelMatrices mapped to SceneGraph node idx (for now, 1-1 with scenegraph.nodes)
	sceneState.modelMatrices.resize(sceneState.graph.nodes.size());

	renderer.createModelMatrices(sizeof(glm::mat4) * sceneState.graph.nodes.size());

	for (auto& node : sceneState.graph.nodes)
	{
		if (node.draw_data_idx.has_value())
		{
			auto* drawDataPtr = adapter.getDrawDataFromIdx(node.draw_data_idx.value());
			if (drawDataPtr)
			{
				adapter.createDescriptorSets(*drawDataPtr, sceneState.graph);
				set_property(*drawDataPtr, DrawDataProperties::DESCRIPTOR_SETS_INITIALIZED);
				ENG_LOG_INFO("Created descriptor sets for " << node.name << std::endl);
			}
		}
		else
		{
			renderer.createDescriptorSets(node);
		}
	}

	ENG_LOG_DEBUG("PosColTex Meshes loaded:" << std::endl);
	for (const auto& mesh : sceneState.posColTexMeshes)
	{
		ENG_LOG_DEBUG("\t" << mesh.name << std::endl);
	}

	ENG_LOG_DEBUG("PosNorTex Meshes loaded:" << std::endl);
	for (const auto& mesh : sceneState.posNorTexMeshes)
	{
		ENG_LOG_DEBUG("\t" << mesh.name << std::endl);
	}

	ENG_LOG_DEBUG("PosBB Meshes loaded:" << std::endl);
	for (const auto& mesh : sceneState.posMeshes)
	{
		ENG_LOG_DEBUG("\t" << mesh.name << std::endl);
	}

	ENG_LOG_INFO("Finished loading data" << std::endl);

	ENG_LOG_DEBUG("Size of NODE (bytes): " << sizeof(ENG::Node) << std::endl);

	// custom settings overrides
	//if (suzanneNode != nullptr)
	//{
	//	suzanneNode->visible = false;
	//}
	//auto* suzanneBoundingBoxNode = find_node_by_name(sceneState.graph, "SuzanneBoundingBox");
	//if (suzanneBoundingBoxNode != nullptr)
	//{
	//	suzanneBoundingBoxNode->visible = false;
	//}
	//auto* roomNode = find_node_by_name(sceneState.graph, "Room");
	//if (roomNode != nullptr)
	//{
	//	roomNode->visible = false;
	//}
	//auto* tetrahedronNode = find_node_by_name(sceneState.graph, "Tetrahedron");
	//if (tetrahedronNode != nullptr)
	//{
	//	tetrahedronNode->visible = false;
	//}
	auto* camera = cameraNode.camera;
	camera->fovy = 0.7;

	cameraNode.translation = glm::vec3(0., 0., 2.);
	sceneState.activeNodeIdx = 3;
}

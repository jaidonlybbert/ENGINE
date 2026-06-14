#include <optional>
#include "scenes/ProceduralGeometry.hpp"
#include "scene/Mesh.hpp"
#include "renderer/vk_adapter/VkAdapter.hpp"
#include "application/ConcurrentQueue.hpp"
#include "scenes/SceneWorld.hpp"
#include "scenes/SceneWorldInput.hpp"

static constexpr size_t SCENE_WORLD_MAX_NODES = 10000;

void create_sun_polyhedra(VkRenderer& renderer, VkAdapter& adapter, SceneState& sceneState)
{
	auto mesh = pmp::icosphere(5);
	dual(mesh);

}

void create_world_polyhedra(VkRenderer& renderer, VkAdapter& adapter, SceneState& sceneState)
{
	// Seed randomizer
	// first: 20398475
	sceneState.randomizer.seed(20398475);

	// Create triangulated goldberg polyhedra including faceIds
	{
		// zzzz... race condition? crashes >= 4
		auto mesh = pmp::icosphere(3);
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

		std::vector<glm::vec4> tileTypeToColorMap{
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
				ENG_LOG_TRACE("At iteration " << i++ << " face is " << climate[f.idx()] << std::endl);

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
						ENG_LOG_TRACE("PUSH: " << h.idx() << std::endl);
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
					ENG_LOG_TRACE("POP: " << h.idx() << std::endl);
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
			faceColors[i] = tileTypeToColorMap[climate[i]];
		}


		triangulate_as_triangle_fan_preserving_face_ids(mesh, faceColors, adapter, sceneState);

	}
}

void create_tetrahedron_no_pmp(SceneState& sceneState, ConcurrentQueue<GraphicsEvent>& graphicsEventQueue, const std::string& nodeName)
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

	std::vector<glm::vec4> colors {
		{1.0, 0.5, 0.5, 1.0},
		{0.5, 1.0, 0.5, 1.0},
		{0.5, 0.5, 1.0, 1.0},
		{0.5, 0.5, 0.5, 1.0},
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

	auto& tetraNode = sceneState.graph.create_node();
	tetraNode.name = nodeName;
	tetraNode.parent = sceneState.graph.root;
	sceneState.graph.root->children.push_back(&tetraNode);

	graphicsEventQueue.push(
		BindHostMeshDataEvent{
			HostMeshData{
				std::move(tetraVerticesDuplicated),
				std::move(tetraIndices),
				"PosNorCol"
			},
			tetraNode.nodeId
		}
	);
}

void unloadWorldScene(SceneState& sceneState)
{

}

void initializeWorldScene(VkRenderer& renderer, VkAdapter& adapter, SceneState& sceneState) {
	// Set callback handlers for inputs
	SceneWorldInput::set_callbacks();

	// TODO: implement pools to avoid reference invalidation on reallocation problem
	sceneState.graph.nodes.reserve(1000);
	sceneState.graph.cameras.reserve(100);

	// Create modelMatrices mapped to SceneGraph node idx (index is 1-1 with scenegraph.nodes)
	// set to max node size
	sceneState.modelMatrices.resize(SCENE_WORLD_MAX_NODES);
	renderer.createModelMatrices(sizeof(glm::mat4) * SCENE_WORLD_MAX_NODES);

	// These are AABBs for nodes, with 1-1 indexing with scenegraph.nodes
	sceneState.aabbs.resize(SCENE_WORLD_MAX_NODES);

	auto& attachmentPoint = sceneState.graph.nodes.emplace_back();
	sceneState.graph.root = &attachmentPoint;
	sceneState.graph.root->name = "Root";

	load_gltf(adapter, get_gltf_dir(), sceneState, attachmentPoint);
	auto& cameraNode = sceneState.graph.nodes.at(sceneState.activeCameraNodeIdx);

	const auto& meshName = std::string("Room");
	ENG::loadModel(adapter, meshName, get_room_obj(), get_room_tex(), sceneState, attachmentPoint);

	// load space floor
	ENG::loadModel(adapter, "Spacefloor3", get_spacefloor_obj2(), get_spacefloor_tex(), sceneState, attachmentPoint);


	// Create bounding box around Suzanne
	//const auto suzanneNodeIdx = find_node_by_name(sceneState.graph, "Suzanne")->nodeId;
	//addBoundingBoxChild(suzanneNodeIdx, renderer, "SuzanneBoundingBox", sceneState);

	ENG_LOG_INFO("Creating tetrahedron2" << std::endl);
	create_tetrahedron_no_pmp(sceneState, adapter.graphicsEventQueue, "Tetrahedron");

	// Create world mesh
	create_world_polyhedra(renderer, adapter, sceneState);

	ENG_LOG_INFO("Finished loading data" << std::endl);

	ENG_LOG_DEBUG("Size of NODE (bytes): " << sizeof(ENG::Node) << std::endl);

	// custom settings overrides
	auto* roomNode = find_node_by_name(sceneState.graph, "Room-0");
	if (roomNode != nullptr)
	{
		roomNode->visible = true;
	}

	auto* suzanneNode = find_node_by_name(sceneState.graph, "Suzanne");
	if (suzanneNode != nullptr)
	{
		suzanneNode->visible = true;
	}

	auto* tetrahedronNode = find_node_by_name(sceneState.graph, "Tetrahedron");
	if (tetrahedronNode != nullptr)
	{
		tetrahedronNode->visible = true;
		tetrahedronNode->translation = glm::vec3(0., 1., 0.);
	}

	auto* camera = cameraNode.camera;
	camera->fovy = 1.;

	cameraNode.translation = glm::vec3(0., 0., 3.);

	sceneState.activeNodeIdx = 3;
}

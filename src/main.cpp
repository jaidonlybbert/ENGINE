#include <stdio.h>
#include <application.hpp>

// Necessary definition for PMP header compilation
#ifndef M_PI
#define M_PI 3.1415926
#endif

#include "pmp/surface_mesh.h"
#include "pmp/algorithms/triangulation.h"
#include "pmp/algorithms/shapes.h"
#include "pmp/algorithms/utilities.h"

#include "lua.h"

#include "interfaces/Logging.hpp"
#include "Utils.hpp"
#include "interfaces/Gltf.hpp"
#include "interfaces/FilesystemInterface.hpp"
#include "interfaces/Obj.hpp"
#include "ProceduralGeometry.h"

void addBoundingBoxChild(ENG::Node* node, VulkanTemplateApp& app, const std::string &bbName)
{
	if (node == nullptr)
	{
		return;
	}
		
	const auto* mesh = checked_cast<ENG::Component, ENG::Mesh<VertexPosNorTex>>(node->mesh);
	const auto& minXIt = std::min_element(mesh->vertices.begin(), mesh->vertices.end(), [](const VertexPosNorTex& rhs, const VertexPosNorTex& lhs) {
		return (rhs.pos.x < lhs.pos.x);
	});
	const auto& minYIt = std::min_element(mesh->vertices.begin(), mesh->vertices.end(), [](const VertexPosNorTex& rhs, const VertexPosNorTex& lhs) {
		return (rhs.pos.y < lhs.pos.y);
	});
	const auto& minZIt = std::min_element(mesh->vertices.begin(), mesh->vertices.end(), [](const VertexPosNorTex& rhs, const VertexPosNorTex& lhs) {
		return (rhs.pos.z < lhs.pos.z);
	});
	const auto& maxXIt = std::max_element(mesh->vertices.begin(), mesh->vertices.end(), [](const VertexPosNorTex& rhs, const VertexPosNorTex& lhs) {
		return (rhs.pos.x < lhs.pos.x);
	});
	const auto& maxYIt = std::max_element(mesh->vertices.begin(), mesh->vertices.end(), [](const VertexPosNorTex& rhs, const VertexPosNorTex& lhs) {
		return (rhs.pos.y < lhs.pos.y);
	});
	const auto& maxZIt = std::max_element(mesh->vertices.begin(), mesh->vertices.end(), [](const VertexPosNorTex& rhs, const VertexPosNorTex& lhs) {
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

	auto& bbMesh = app.sceneState.posMeshes.emplace_back(app.device, app.physicalDevice, app.commands.get(), bbName, bbVertices, bbIndices, app.graphicsQueue);
	auto& bbNode = app.sceneState.graph.nodes.emplace_back();
	bbNode.name = bbName;
	bbNode.nodeId = app.sceneState.graph.nodes.size() - 1;
	bbNode.parent = node;
	bbNode.mesh = &bbMesh;
	bbNode.shaderId = ENG_SHADER::PosBB;
	node->children.push_back(&bbNode);
}

int main() {
	
	try {
		printf("Starting app\n");
		// ENG_LOG_TRACE("Application path: " << install_dir.native().c_str() << std::endl);

		VulkanTemplateApp app;
		ENG_LOG_INFO(app);

		// TODO: implement pools to avoid reference invalidation on reallocation problem
		app.sceneState.posColTexMeshes.reserve(100);
		app.sceneState.posNorTexMeshes.reserve(100);
		app.sceneState.posMeshes.reserve(100);
		app.sceneState.posNorColMeshes.reserve(100);
		app.sceneState.graph.nodes.reserve(100);
		app.sceneState.graph.cameras.reserve(10);

		auto& attachmentPoint = app.sceneState.graph.nodes.emplace_back();
		app.sceneState.graph.root = &attachmentPoint;
		app.sceneState.graph.root->name = "Root";

		// ENG_LOG_INFO("GLTF path: " << gltf_dir.native().c_str() << std::endl);
		load_gltf(app.device, app.physicalDevice, app.graphicsQueue, app.commands.get(), get_gltf_dir(), app.sceneState, attachmentPoint);
		auto& cameraNode = app.sceneState.graph.nodes.at(app.sceneState.activeCameraNodeIdx);

		const auto& meshName = std::string("Room");
		ENG::loadModel(app.device, app.physicalDevice, app.commands.get(), meshName, app.graphicsQueue, get_model_dir(), app.sceneState, attachmentPoint);

		// Create bounding box around Suzanne
		auto* suzanneNode = find_node_by_name(app.sceneState.graph, "Suzanne");
		addBoundingBoxChild(suzanneNode, app, "SuzanneBoundingBox");

		// Create Tetrahedron
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


			auto& tetraMesh = app.sceneState.posNorColMeshes.emplace_back(app.device, app.physicalDevice, app.commands.get(), "TetrahedronMesh", tetraVerticesDuplicated, tetraIndices, app.graphicsQueue);
			auto& tetraNode = app.sceneState.graph.nodes.emplace_back();
			tetraNode.name = "Tetrahedron";
			tetraNode.nodeId = app.sceneState.graph.nodes.size() - 1;
			tetraNode.parent = app.sceneState.graph.root;
			tetraNode.mesh = &tetraMesh;
			tetraNode.shaderId = ENG_SHADER::PosNorCol;
			app.sceneState.graph.root->children.push_back(&tetraNode);
		}

		// Seed randomizer
		// first: 20398475
		app.gameState.randomizer.seed(20398475);

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
			std::sample(choices.begin(), choices.end(), std::back_inserter(samples), 5, app.gameState.randomizer);

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
						if (randFloatDistribution(app.gameState.randomizer) < continentGrowthFactor)
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
			load_pmp_mesh(mesh, "GoldbergMesh", "GoldbergPolyhedra", app);

			app.createFaceIdBuffers(mesh.faces_size());
			const auto& bufferSize = primitiveToFaceIdMap.size() * sizeof(uint32_t);
			for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
			{
				memcpy(app.faceIdMapBuffersMapped[i], primitiveToFaceIdMap.data(), bufferSize);
			}
		}

		// Create modelMatrices mapped to SceneGraph node idx (for now, 1-1 with scenegraph.nodes)
		app.sceneState.modelMatrices.resize(app.sceneState.graph.nodes.size());

		app.createModelMatrices();

		for (auto& node : app.sceneState.graph.nodes)
		{
			app.createDescriptorSets(node);
		}

		ENG_LOG_INFO("PosColTex Meshes loaded:" << std::endl);
		for (const auto& mesh : app.sceneState.posColTexMeshes)
		{
			ENG_LOG_INFO("\t" << mesh.name << std::endl);
		}

		ENG_LOG_INFO("PosNorTex Meshes loaded:" << std::endl);
		for (const auto& mesh : app.sceneState.posNorTexMeshes)
		{
			ENG_LOG_INFO("\t" << mesh.name << std::endl);
		}

		ENG_LOG_INFO("PosBB Meshes loaded:" << std::endl);
		for (const auto& mesh : app.sceneState.posMeshes)
		{
			ENG_LOG_INFO("\t" << mesh.name << std::endl);
		}

		ENG_LOG_INFO("Finished loading data" << std::endl);

		ENG_LOG_DEBUG("Size of NODE (bytes): " << sizeof(ENG::Node) << std::endl);

		// custom settings overrides
		if (suzanneNode != nullptr)
		{
			suzanneNode->visible = false;
		}
		auto* suzanneBoundingBoxNode = find_node_by_name(app.sceneState.graph, "SuzanneBoundingBox");
		if (suzanneBoundingBoxNode != nullptr)
		{
			suzanneBoundingBoxNode->visible = false;
		}
		auto* roomNode = find_node_by_name(app.sceneState.graph, "Room");
		if (roomNode != nullptr)
		{
			roomNode->visible = false;
		}
		auto* tetrahedronNode = find_node_by_name(app.sceneState.graph, "Tetrahedron");
		if (tetrahedronNode != nullptr)
		{
			tetrahedronNode->visible = false;
		}
		auto* camera = checked_cast<ENG::Component, ENG::Camera>(cameraNode.camera);
		camera->fovy = 0.7;

		cameraNode.translation = glm::vec3(0., 0., 2.);
		app.sceneState.activeNodeIdx = 3;

		app.run();

	} catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
	    	return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

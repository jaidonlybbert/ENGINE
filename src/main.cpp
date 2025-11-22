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
#include "primitives/Mesh.hpp"

pmp::Point centroid(const pmp::SurfaceMesh& mesh, pmp::Face f)
{
	pmp::Point c(0, 0, 0);
	pmp::Scalar n(0);
	for (auto v : mesh.vertices(f)) {
		c += mesh.position(v);
		++n;
	}
	c /= n;
	return c;
}

void dual(pmp::SurfaceMesh& mesh)
{
	// the new dual mesh
	pmp::SurfaceMesh tmp;

	// a property to remember new vertices per face
	auto fvertex = mesh.add_face_property<pmp::Vertex>("f:vertex");

	// for each face add the centroid to the dual mesh
	for (auto f : mesh.faces())
	{
		fvertex[f] = tmp.add_vertex(centroid(mesh, f));
	}

	// add new face for each vertex
	for (auto v : mesh.vertices())
	{
		std::vector<pmp::Vertex> vertices;
		for (auto f : mesh.faces(v))
			vertices.push_back(fvertex[f]);

		tmp.add_face(vertices);
	}

	// swap old and new meshes, don't copy properties
	mesh.assign(tmp);
}

void project_to_unit_sphere(pmp::SurfaceMesh& mesh)
{
	for (auto v : mesh.vertices()) {
		auto p = mesh.position(v);
		auto n = norm(p);
		mesh.position(v) = (1.0 / n) * p;
	}
}

pmp::SurfaceMesh create_tetrahedron()
{
	pmp::SurfaceMesh mesh;

	// choose coordinates on the unit sphere
	float a = 1.0f / 3.0f;
	float b = sqrt(8.0f / 9.0f);
	float c = sqrt(2.0f / 9.0f);
	float d = sqrt(2.0f / 3.0f);

	// add the 4 vertices
	auto v0 = mesh.add_vertex(pmp::Point(0, 0, 1));
	auto v1 = mesh.add_vertex(pmp::Point(-c, d, -a));
	auto v2 = mesh.add_vertex(pmp::Point(-c, -d, -a));
	auto v3 = mesh.add_vertex(pmp::Point(b, 0, -a));

	// add the 4 faces
	mesh.add_triangle(v0, v1, v2);
	mesh.add_triangle(v0, v2, v3);
	mesh.add_triangle(v0, v3, v1);
	mesh.add_triangle(v3, v2, v1);

	return mesh;
}

pmp::SurfaceMesh create_hexahedron()
{
	pmp::SurfaceMesh mesh;

	// choose coordinates on the unit sphere
	float a = 1.0f / sqrt(3.0f);

	// add the 8 vertices
	auto v0 = mesh.add_vertex(pmp::Point(-a, -a, -a));
	auto v1 = mesh.add_vertex(pmp::Point(a, -a, -a));
	auto v2 = mesh.add_vertex(pmp::Point(a, a, -a));
	auto v3 = mesh.add_vertex(pmp::Point(-a, a, -a));
	auto v4 = mesh.add_vertex(pmp::Point(-a, -a, a));
	auto v5 = mesh.add_vertex(pmp::Point(a, -a, a));
	auto v6 = mesh.add_vertex(pmp::Point(a, a, a));
	auto v7 = mesh.add_vertex(pmp::Point(-a, a, a));

	// add the 6 faces
	mesh.add_quad(v3, v2, v1, v0);
	mesh.add_quad(v2, v6, v5, v1);
	mesh.add_quad(v5, v6, v7, v4);
	mesh.add_quad(v0, v4, v7, v3);
	mesh.add_quad(v3, v7, v6, v2);
	mesh.add_quad(v1, v5, v4, v0);

	return mesh;
}

pmp::SurfaceMesh create_icosahedron()
{
	pmp::SurfaceMesh mesh;

	float phi = (1.0f + sqrt(5.0f)) * 0.5f; // golden ratio
	float a = 1.0f;
	float b = 1.0f / phi;

	// add vertices
	auto v1 = mesh.add_vertex(pmp::Point(0, b, -a));
	auto v2 = mesh.add_vertex(pmp::Point(b, a, 0));
	auto v3 = mesh.add_vertex(pmp::Point(-b, a, 0));
	auto v4 = mesh.add_vertex(pmp::Point(0, b, a));
	auto v5 = mesh.add_vertex(pmp::Point(0, -b, a));
	auto v6 = mesh.add_vertex(pmp::Point(-a, 0, b));
	auto v7 = mesh.add_vertex(pmp::Point(0, -b, -a));
	auto v8 = mesh.add_vertex(pmp::Point(a, 0, -b));
	auto v9 = mesh.add_vertex(pmp::Point(a, 0, b));
	auto v10 = mesh.add_vertex(pmp::Point(-a, 0, -b));
	auto v11 = mesh.add_vertex(pmp::Point(b, -a, 0));
	auto v12 = mesh.add_vertex(pmp::Point(-b, -a, 0));

	project_to_unit_sphere(mesh);

	// add triangles
	mesh.add_triangle(v3, v2, v1);
	mesh.add_triangle(v2, v3, v4);
	mesh.add_triangle(v6, v5, v4);
	mesh.add_triangle(v5, v9, v4);
	mesh.add_triangle(v8, v7, v1);
	mesh.add_triangle(v7, v10, v1);
	mesh.add_triangle(v12, v11, v5);
	mesh.add_triangle(v11, v12, v7);
	mesh.add_triangle(v10, v6, v3);
	mesh.add_triangle(v6, v10, v12);
	mesh.add_triangle(v9, v8, v2);
	mesh.add_triangle(v8, v9, v11);
	mesh.add_triangle(v3, v6, v4);
	mesh.add_triangle(v9, v2, v4);
	mesh.add_triangle(v10, v3, v1);
	mesh.add_triangle(v2, v8, v1);
	mesh.add_triangle(v12, v10, v7);
	mesh.add_triangle(v8, v11, v7);
	mesh.add_triangle(v6, v12, v5);
	mesh.add_triangle(v11, v9, v5);

	return mesh;
}

pmp::SurfaceMesh create_dodecahedron()
{
	auto mesh = create_icosahedron();
	dual(mesh);
	project_to_unit_sphere(mesh);
	return mesh;
}

ENG::Mesh<VertexPosNorCol>* load_pmp_mesh(const pmp::SurfaceMesh& mesh, const std::string& mesh_name, const std::string& node_name,
	VulkanTemplateApp& app)
{
		std::vector<VertexPosNorCol> vertices;
		std::vector<uint32_t> indices;
		const auto& color = glm::vec3( 0.5f, 0.6f, 0.6f);

		vertices.resize(mesh.vertices_size() * 3);
		indices.resize(12); // unused

		const auto& points = mesh.get_vertex_property<pmp::Point>("v:point");

		size_t idx{0};
		for (const auto& face : mesh.faces())
		{
			// extract vertex positions into glm::vec3 positions (counter-clockwise order)
			assert(idx + 2 < vertices.size());
			auto circulator = pmp::SurfaceMesh::VertexAroundFaceCirculator(&mesh, face);
			auto& it = circulator.begin();
			const auto& p0 = points[*it];
			const auto& p1 = points[*(++it)];
			const auto& p2 = points[*(++it)];
			const auto& v0 = glm::vec3(p0[0], p0[1], p0[2]);
			const auto& v1 = glm::vec3(p1[0], p1[1], p1[2]);
			const auto& v2 = glm::vec3(p2[0], p2[1], p2[2]);

			// compute normal vector
			const auto& normal = glm::normalize(glm::cross(v1-v0, v2-v0));

			vertices.at(idx).pos = v0;
			vertices.at(idx).normal = normal;
			vertices.at(idx).color = color;
			vertices.at(idx+1).pos = v1;
			vertices.at(idx+1).normal = normal;
			vertices.at(idx+1).color = color;
			vertices.at(idx+2).pos = v2;
			vertices.at(idx+2).normal = normal;
			vertices.at(idx+2).color = color;

			idx += 3;
		}

		auto& pmpMesh = app.sceneState.posNorColMeshes.emplace_back(app.device, app.physicalDevice, app.commands.get(), mesh_name, vertices, indices, app.graphicsQueue);
		auto& pmpNode = app.sceneState.graph.nodes.emplace_back();
		pmpNode.name = node_name;
		pmpNode.nodeId = app.sceneState.graph.nodes.size() - 1;
		pmpNode.parent = app.sceneState.graph.root;
		pmpNode.mesh = &pmpMesh;
		pmpNode.shaderId = ENG_SHADER::Goldberg;
		app.sceneState.graph.root->children.push_back(&pmpNode);

		return &pmpMesh;
}

void triangulate_as_triangle_fan_preserving_face_ids(pmp::SurfaceMesh& mesh)
{
	// the new dual mesh
	pmp::SurfaceMesh tmp;

	tmp.reserve(mesh.vertices_size() * 2, mesh.edges_size() * 2, mesh.faces_size() * 2);

	// apply a face index for each face (preserved after triangulation)
	auto oldFaceId = mesh.get_face_property<uint32_t>("f:faceId");
	auto newFaceId = tmp.add_face_property<uint32_t>("f:faceId");

	auto facecount{ 0 };
	for (auto f : mesh.faces())
	{
		// calculate center of triangle fan for new mesh face
		const auto& centerVert = tmp.add_vertex(centroid(mesh, f));

		// iterate over all vertices in original face, creating triangles
		auto vertRange = mesh.vertices(f);
		auto it = vertRange.begin();
		const auto& end = vertRange.end();
		ENG_LOG_TRACE("Verts in face: " << std::distance(it, end) << std::endl);
		assert(std::distance(it, end) > 2);
		auto v0 = tmp.add_vertex(mesh.position(*(it++)));
		auto firstVert = v0;
		auto v1 = tmp.add_vertex(mesh.position(*(it++)));
		auto tri = tmp.add_triangle(centerVert, v0, v1);
		facecount++;
		newFaceId[tri] = oldFaceId[f];

		for (; it != end; ++it)
		{
			v0 = v1;
			v1 = tmp.add_vertex(mesh.position(*(it)));
			tri = tmp.add_triangle(centerVert, v0, v1);
			newFaceId[tri] = oldFaceId[f];
			facecount++;
			ENG_LOG_TRACE("INNER LOOP HIT" << std::endl);
		}

		// add last triangle using last and first vertex
		tri = tmp.add_triangle(centerVert, v1, firstVert);
		newFaceId[tri] = oldFaceId[f];
		facecount++;
	}

	ENG_LOG_DEBUG("FAce count " << facecount << std::endl);
	// deep copy of tmp mesh, including properties	
	mesh = tmp;
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
		if (suzanneNode != nullptr)
		{
			const auto* mesh = checked_cast<ENG::Component, ENG::Mesh<VertexPosNorTex>>(suzanneNode->mesh);
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

			auto& bbMesh = app.sceneState.posMeshes.emplace_back(app.device, app.physicalDevice, app.commands.get(), "SuzanneBB" , bbVertices, bbIndices, app.graphicsQueue);
			auto& bbNode = app.sceneState.graph.nodes.emplace_back();
			bbNode.name = "SuzanneBoundingBox";
			bbNode.nodeId = app.sceneState.graph.nodes.size() - 1;
			bbNode.parent = suzanneNode;
			bbNode.mesh = &bbMesh;
			bbNode.shaderId = ENG_SHADER::PosBB;
			suzanneNode->children.push_back(&bbNode);
		}

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

#include "scene/Mesh.hpp"
#include "scenes/ProceduralGeometry.hpp"
#include "renderer/vk/Renderer.hpp"
#include "pmp/surface_mesh.h"
#include "pmp/algorithms/triangulation.h"
#include "pmp/algorithms/shapes.h"
#include "pmp/algorithms/utilities.h"

#include "logger/Logging.hpp"
#include "renderer/vk/Utils.hpp"
#include "scene/Gltf.hpp"
#include "filesystem/FilesystemInterface.hpp"
#include "scene/Obj.hpp"

using namespace ENG;

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

ENG::Mesh<VertexPosNorCol>* load_pmp_mesh(
	const pmp::SurfaceMesh& mesh, const std::string& mesh_name, const std::string& node_name,
	VkRenderer& app, SceneState& sceneState)
{
		std::vector<VertexPosNorCol> vertices;
		std::vector<uint32_t> indices;
		const glm::vec3& color{ 0.5f, 0.6f, 0.6f };

		vertices.reserve(mesh.vertices_size());
		indices.resize(12); // unused

		const auto& points = mesh.get_vertex_property<pmp::Point>("v:point");

		for (const auto& face : mesh.faces())
		{
			// extract vertex positions into glm::vec3 positions (counter-clockwise order)
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

			const VertexPosNorCol& vert0 {v0, normal, color};
			const VertexPosNorCol& vert1 {v1, normal, color};
			const VertexPosNorCol& vert2 {v2, normal, color};

			vertices.emplace_back(vert0);
			vertices.emplace_back(vert1);
			vertices.emplace_back(vert2);
		}

		auto& pmpMesh = sceneState.posNorColMeshes.emplace_back(app.device, app.physicalDevice, app.commands.get(), mesh_name, vertices, indices, app.graphicsQueue);
		auto& pmpNode = sceneState.graph.nodes.emplace_back();
		pmpNode.name = node_name;
		pmpNode.nodeId = sceneState.graph.nodes.size() - 1;
		pmpNode.parent = sceneState.graph.root;
		pmpNode.mesh = &pmpMesh;
		pmpNode.shaderId = "Goldberg";
		sceneState.graph.root->children.push_back(&pmpNode);

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

#pragma once
#include "scene/Mesh.hpp"
#include "renderer/Renderer.hpp"
// Necessary definition for PMP header compilation
#ifndef M_PI
#define M_PI 3.1415926
#endif
#include "pmp/surface_mesh.h"
#include "pmp/algorithms/triangulation.h"
#include "pmp/algorithms/shapes.h"
#include "pmp/algorithms/utilities.h"

#include "logger/Logging.hpp"
#include "renderer/Utils.hpp"
#include "scene/Gltf.hpp"
#include "filesystem/FilesystemInterface.hpp"
#include "scene/Obj.hpp"

pmp::Point centroid(const pmp::SurfaceMesh& mesh, pmp::Face f);
void dual(pmp::SurfaceMesh& mesh);
void project_to_unit_sphere(pmp::SurfaceMesh& mesh);
pmp::SurfaceMesh create_tetrahedron();
pmp::SurfaceMesh create_hexahedron();
pmp::SurfaceMesh create_icosahedron();
pmp::SurfaceMesh create_dodecahedron();
ENG::Mesh<ENG::VertexPosNorCol>* load_pmp_mesh(
	const pmp::SurfaceMesh& mesh, const std::string& mesh_name, const std::string& node_name, VulkanTemplateApp& app);
void triangulate_as_triangle_fan_preserving_face_ids(pmp::SurfaceMesh& mesh);
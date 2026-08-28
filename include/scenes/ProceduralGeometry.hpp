#pragma once
#include "scene/Mesh.hpp"
#include "scene/Scene.hpp"
// Necessary definition for PMP header compilation
#ifndef M_PI
#define M_PI 3.1415926
#endif
#include "filesystem/FilesystemInterface.hpp"
#include "logger/Logging.hpp"
#include "pmp/algorithms/shapes.h"
#include "pmp/algorithms/triangulation.h"
#include "pmp/algorithms/utilities.h"
#include "pmp/surface_mesh.h"
#include "scene/Gltf.hpp"
#include "scene/Obj.hpp"

pmp::Point centroid(const pmp::SurfaceMesh& mesh, pmp::Face f);
void dual(pmp::SurfaceMesh& mesh);
void project_to_unit_sphere(pmp::SurfaceMesh& mesh);
pmp::SurfaceMesh create_tetrahedron();
pmp::SurfaceMesh create_hexahedron();
pmp::SurfaceMesh create_icosahedron();
pmp::SurfaceMesh create_dodecahedron();
void load_pmp_mesh(ENG::Node& parent, const pmp::SurfaceMesh& mesh, const std::string& mesh_name,
                   const std::string& node_name, const glm::vec4& color, ENG::SceneState& sceneState);
void triangulate_as_triangle_fan_preserving_face_ids(pmp::SurfaceMesh& mesh, const std::vector<glm::vec4>& faceColors,
                                                     ENG::SceneState& sceneState);

#pragma once
#include "scene/Scene.hpp"
// Necessary definition for PMP header compilation
#ifndef M_PI
#define M_PI 3.1415926
#endif
#include "pmp/surface_mesh.h"
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

// Builds a tetrahedron directly (bypassing PMP) with a distinct flat color per face, and
// adds it to the scene graph as a child of the root node. Scene-agnostic - any scene can
// use this without depending on another scene.
void create_tetrahedron_no_pmp(ENG::SceneState& sceneState, const std::string& nodeName);

#pragma once
#include "renderer/RenderAdapterI.hpp"
#include "scene/Scene.hpp"

// Builds a large, inward-facing solid-blue cube around the origin so it reads as a sky
// when viewed from inside. Reuses the same "PosNorCol" vertex format / shader as
// create_tetrahedron_no_pmp (see SceneWorld.hpp).
void create_blue_skybox(ENG::SceneState& sceneState, const std::string& nodeName, float halfExtent);

// A minimal scene: a blue skybox surrounding a single Tetrahedron at the origin. Exists
// alongside "WorldScene" as a second, independently loadable scene (see issue #8).
void initializeBlueSkyScene(ENG::SceneState& sceneState, RenderAdapterI& renderAdapter);

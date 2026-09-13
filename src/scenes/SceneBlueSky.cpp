#include "scenes/SceneBlueSky.hpp"

#include <vector>

#include "application/ConcurrentQueue.hpp"
#include "logger/Logging.hpp"
#include "scene/Mesh.hpp"
#include "scene/Scene.hpp"
#include "scenes/SceneWorld.hpp"
#include "scenes/SceneWorldInput.hpp"

static constexpr size_t SCENE_BLUE_SKY_MAX_NODES = 64;

namespace {

// Appends the two triangles (a, b, c) and (a, c, d) making up the quad a-b-c-d.
void append_quad(std::vector<glm::vec3>& positions, const glm::vec3& a, const glm::vec3& b, const glm::vec3& c,
                 const glm::vec3& d) {
    positions.push_back(a);
    positions.push_back(b);
    positions.push_back(c);
    positions.push_back(a);
    positions.push_back(c);
    positions.push_back(d);
}

}  // namespace

void create_blue_skybox(ENG::SceneState& sceneState, const std::string& nodeName, float halfExtent) {
    const float s = halfExtent;
    const glm::vec3 v0{-s, -s, -s};
    const glm::vec3 v1{s, -s, -s};
    const glm::vec3 v2{s, s, -s};
    const glm::vec3 v3{-s, s, -s};
    const glm::vec3 v4{-s, -s, s};
    const glm::vec3 v5{s, -s, s};
    const glm::vec3 v6{s, s, s};
    const glm::vec3 v7{-s, s, s};

    // Six walls, two triangles each. Each quad's vertex order is wound so it is
    // front-facing (CCW, matching the engine's VK_FRONT_FACE_COUNTER_CLOCKWISE /
    // back-face-cull convention) as seen from inside the box - the mirror image of an
    // ordinary outward-facing cube - so the cross-product normal computed below points
    // back toward the origin instead of away from it.
    std::vector<glm::vec3> positions;
    positions.reserve(36);
    append_quad(positions, v0, v1, v2, v3);  // -Z wall
    append_quad(positions, v1, v5, v6, v2);  // +X wall
    append_quad(positions, v4, v7, v6, v5);  // +Z wall
    append_quad(positions, v3, v7, v4, v0);  // -X wall
    append_quad(positions, v2, v6, v7, v3);  // +Y wall (ceiling)
    append_quad(positions, v0, v4, v5, v1);  // -Y wall (floor)

    const glm::vec4 skyBlue{0.25f, 0.45f, 0.85f, 1.0f};

    std::vector<ENG::VertexPosNorCol> vertices;
    vertices.reserve(positions.size());
    for (size_t i = 0; i + 2 < positions.size(); i += 3) {
        const auto& p0 = positions[i];
        const auto& p1 = positions[i + 1];
        const auto& p2 = positions[i + 2];
        const auto normal = glm::normalize(glm::cross(p1 - p0, p2 - p0));

        vertices.push_back(ENG::VertexPosNorCol{p0, normal, skyBlue});
        vertices.push_back(ENG::VertexPosNorCol{p1, normal, skyBlue});
        vertices.push_back(ENG::VertexPosNorCol{p2, normal, skyBlue});
    }

    auto& skyboxNode = sceneState.graph.create_node();
    skyboxNode.name = nodeName;
    skyboxNode.parent = sceneState.graph.root;
    sceneState.graph.root->children.push_back(&skyboxNode);

    // The index buffer is unused for the "PosNorCol" shader - vertices are drawn directly
    // and are already triangulated above (see create_tetrahedron_no_pmp / load_pmp_mesh).
    std::vector<uint32_t> unusedIndices(vertices.size());

    sceneState.hostMeshDataBindQueue.push(ENG::BindHostMeshDataEvent{
        ENG::HostMeshData{std::move(vertices), std::move(unusedIndices), "PosNorCol"}, skyboxNode.nodeId});
}

void initializeBlueSkyScene(ENG::SceneState& sceneState, RenderAdapterI& renderAdapter) {
    // Set callback handlers for inputs (shared with WorldScene - camera orbit / zoom
    // controls are generic, not tied to a particular scene's contents).
    SceneWorldInput::set_callbacks();

    sceneState.graph.nodes.reserve(SCENE_BLUE_SKY_MAX_NODES);
    sceneState.graph.cameras.reserve(4);

    sceneState.modelMatrices.resize(SCENE_BLUE_SKY_MAX_NODES);
    renderAdapter.init(SCENE_BLUE_SKY_MAX_NODES);
    sceneState.aabbs.resize(SCENE_BLUE_SKY_MAX_NODES);

    auto& root = sceneState.graph.nodes.emplace_back();
    sceneState.graph.root = &root;
    sceneState.graph.root->name = "Root";

    // This scene isn't backed by a glTF file, so build a minimal perspective camera by
    // hand instead of loading one (see load_gltf_node in Gltf.cpp for the glTF path).
    tinygltf::Camera gltfCamera;
    gltfCamera.type = "perspective";
    gltfCamera.perspective.yfov = 1.0;
    gltfCamera.perspective.znear = 0.1;
    gltfCamera.perspective.zfar = 100.0;
    auto* camera = &sceneState.graph.cameras.emplace_back(gltfCamera);

    auto& cameraNode = sceneState.graph.create_node();
    cameraNode.name = "MainCamera";
    cameraNode.parent = sceneState.graph.root;
    cameraNode.camera = camera;
    cameraNode.translation = glm::vec3(0.f, 0.f, 8.f);
    sceneState.graph.root->children.push_back(&cameraNode);
    sceneState.activeCameraNodeIdx = cameraNode.nodeId;

    create_blue_skybox(sceneState, "BlueSkybox", 50.0f);
    create_tetrahedron_no_pmp(sceneState, "Tetrahedron");

    // sends object data to graphics queue for rendering
    renderAdapter.draw(sceneState.hostMeshDataBindQueue);

    ENG_LOG_TRACE("Finished loading blue sky scene");

    auto* tetrahedronNode = find_node_by_name(sceneState.graph, "Tetrahedron");
    if (tetrahedronNode != nullptr) {
        tetrahedronNode->visible = true;
    }

    // Mouse-look rotates the camera node.
    sceneState.activeNodeIdx = static_cast<int>(cameraNode.nodeId);
}

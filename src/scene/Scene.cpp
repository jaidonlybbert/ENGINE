#include "scene/Scene.hpp"

namespace ENG {

Node& get_node_by_id(SceneGraph& sceneGraph, const size_t nodeId) {
    assert(nodeId < sceneGraph.nodes.size());
    return sceneGraph.nodes.at(nodeId);
}

glm::mat4 transformation_matrix(const Node& node) {
    glm::mat4 model_matrix = glm::mat4(1.0f);
    model_matrix = glm::scale(model_matrix, node.scale);
    model_matrix = model_matrix * glm::mat4_cast(node.rotation);
    model_matrix = glm::translate(model_matrix, node.translation);

    return model_matrix;
}

Node* find_node_by_name(const SceneGraph& graph, const std::string& name) {
    for (const auto& node : graph.nodes) {
        if (node.name == name) return const_cast<Node*>(&node);
    }

    return nullptr;
}

}  // namespace ENG

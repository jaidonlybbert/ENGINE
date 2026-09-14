#include "hid/InputI.hpp"
#include "logger/Logging.hpp"
#include "scene/Scene.hpp"

/*
 * Blender style camera - rotation around y axis is global, rotation around x axis is local to active object.
 * Reverses rotation around y-axis when camera is 'upside down'
 */
void node_rotation_follows_input_preserve_y_as_up(ENG::Node& activeNode, const double dx, const double dy) {
    ENG_LOG_TRACE("dx: " << dx << " dy: " << dy);
    constexpr auto sensitivity = 1.0f;

    const auto invert = (activeNode.rotation * glm::vec3(0.f, 1.f, 0.f)).y >= 0.f ? 1.f : -1.f;

    auto dx_quat = glm::angleAxis(glm::radians(static_cast<float>(dx) * sensitivity), glm::vec3(0.f, invert, 0.f));
    auto dy_quat = glm::angleAxis(glm::radians(static_cast<float>(dy) * sensitivity), glm::vec3(1.f, 0.f, 0.f));
    activeNode.rotation = dx_quat * activeNode.rotation * dy_quat;
}

void node_rotation_follows_input(ENG::Node& activeNode, const double dx, const double dy) {
    ENG_LOG_TRACE("dx: " << dx << " dy: " << dy);
    constexpr auto sensitivity = 1.0f;

    auto dx_quat = glm::angleAxis(glm::radians(static_cast<float>(dx) * sensitivity), glm::vec3(0.f, 1.f, 0.f));
    auto dy_quat = glm::angleAxis(glm::radians(static_cast<float>(dy) * sensitivity), glm::vec3(1.f, 0.f, 0.f));
    activeNode.rotation = activeNode.rotation * dx_quat * dy_quat;
}

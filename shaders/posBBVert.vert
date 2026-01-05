#version 450

layout(binding = 0) readonly uniform UniformBufferObject {
        mat4 model;
        mat4 view;
        mat4 proj;
} ubo;

layout(binding = 1) readonly buffer ModelMatrices {
        mat4 model[];
};

layout(location = 0) in vec3 inPosition;

layout(push_constant) uniform PushConstants {
        uint nodeIndex;
};

void main() {
        gl_Position = ubo.proj * ubo.view * model[nodeIndex] * vec4(inPosition, 1.0);
}

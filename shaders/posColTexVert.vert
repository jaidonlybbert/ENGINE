#version 450

layout(binding = 0) readonly uniform UniformBufferObject {
        mat4 model;
        mat4 view;
        mat4 proj;
} ubo;

layout(binding = 2) readonly buffer ModelMatrices {
        mat4 model[];
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(push_constant) uniform PushConstants {
        uint nodeIndex;
};

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() {
        gl_Position = ubo.proj * ubo.view * model[nodeIndex] * vec4(inPosition, 1.0);
        fragColor = inColor;
        fragTexCoord = inTexCoord;
}

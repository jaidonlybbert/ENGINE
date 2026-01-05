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
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec3 fragPos;
layout(location = 2) out vec3 fragColor;

layout(push_constant) uniform PushConstants {
        uint nodeIndex;
};

void main() {
        // world-space position
        fragPos = vec3(model[nodeIndex] * vec4(inPosition, 1.0));

        // transform normals
        fragNormal = mat3(transpose(inverse(model[nodeIndex]))) * inNormal;

        fragColor = inColor;

        gl_Position = ubo.proj * ubo.view * vec4(fragPos, 1.0);
}

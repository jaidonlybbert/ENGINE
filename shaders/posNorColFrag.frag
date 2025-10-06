#version 450

//layout(binding = 2) uniform UniformBufferObject {
//    vec3 lightPos;
//    vec3 lightCol;
//    vec3 objectCol;
//} light;

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec3 fragPos;
layout(location = 2) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

void main() {
    // Normal vector
    vec3 N = normalize(fragNormal);
    // Vector from vertex to light source
    // vec3 L = normalize(light.lightPos - fragPos);
    vec3 L = normalize(vec3(2.0, 2.0, 2.0) - fragPos);
    // Diffuse element of intensity
    float diff = max(dot(N, L), 0.0);

    // vec3 diffuse = diff * light.lightCol;
    vec3 diffuse = diff * vec3(1.0, 1.0, 1.0);
    vec4 result = vec4(diffuse, 1.0) * vec4(fragColor, 1.0);
    // Final color
    outColor = result;
    // outColor = vec4(1.0, 1.0, 1.0, 1.0);
}

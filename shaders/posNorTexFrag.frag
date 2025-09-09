#version 450

layout(binding = 1) uniform sampler2D texSampler;

//layout(binding = 2) uniform UniformBufferObject {
//    vec3 lightPos;
//    vec3 lightCol;
//    vec3 objectCol;
//} light;

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec3 fragPos;
layout(location = 2) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
//    vec4 texColor = texture(texSampler, fragTexCoord);

//    // Normal vector
//    vec3 N = normalize(fragNormal);
    // Vector from vertex to light source
//    vec3 L = normalize(light.lightPos - fragPos);
    // Diffuse element of intensity
//    float diff = max(dot(N, L), 0.0);

//    vec3 diffuse = diff * light.lightCol;
//    vec4 result = vec4(diffuse, 1.0) * texColor;
    // Final color
//    outColor = result;
    outColor = vec4(1.0, 1.0, 1.0, 1.0);
}

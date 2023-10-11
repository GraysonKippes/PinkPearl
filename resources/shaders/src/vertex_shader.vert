#version 450

layout(binding = 0) uniform MatrixUniformObject {
    mat4 model;
    mat4 view;
    mat4 projection;
} muo;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inColor;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec3 fragColor;

void main() {
    gl_Position = muo.projection * muo.view * muo.model * vec4(inPosition, 0.0, 1.0);
    fragTexCoord = inTexCoord;
    fragColor = inColor;
}
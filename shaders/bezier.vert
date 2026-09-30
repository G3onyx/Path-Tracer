#version 450 core

layout (location = 0) in vec3 vPos;

out vec3 worldPos;

uniform mat4 model;
uniform mat4 PV;

void main() {
    worldPos = vec3(model * vec4(vPos, 1.0));
    gl_Position = PV * vec4(worldPos, 1.0);
}
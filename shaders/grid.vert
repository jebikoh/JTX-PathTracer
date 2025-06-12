#version 450

layout(location = 0) out vec2 outUV;

const vec3 positions[3] = vec3[](
    vec3(-1.0, -1.0, 0.0),
    vec3( 3.0, -1.0, 0.0),
    vec3(-1.0,  3.0, 0.0)
);

void main() {
    vec3 pos = positions[gl_VertexIndex].xyz;
    gl_Position = vec4(pos, 1.0);
    outUV = (pos.xy + 1.0) * 0.5;
}

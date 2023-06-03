#version 400
in vec3 vp;

uniform float u_time;
uniform vec2  u_resolution;

void main()
{
    gl_Position = vec4(vp.x * abs(sin(u_time)), vp.y, vp.z, 1.0);
}

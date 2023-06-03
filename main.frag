#version 400
out vec4 frag_colour;
in vec4 gl_FragCoord;

uniform float u_time;
uniform vec2 u_resolution;

void main()
{
    /*vec3 rgb;
    rgb.x = 0.5 - abs(0.5 - gl_FragCoord.x / u_resolution.x);
    rgb.y = abs(0.5 - gl_FragCoord.x / u_resolution.x);
    rgb.z = 1.0 - abs(0.5 - gl_FragCoord.x / u_resolution.x);*/

    frag_colour = vec4(1.0, 1.0, 1.0, 1.0);
}

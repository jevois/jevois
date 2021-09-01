#version 300 es

layout(location = 0) in vec4 a_position;
layout(location = 1) in vec2 a_tex_coord;
out vec2 v_tex_coord;
uniform mat4 pvm;

void main()
{
  gl_Position = pvm * a_position;
  v_tex_coord = a_tex_coord;
}

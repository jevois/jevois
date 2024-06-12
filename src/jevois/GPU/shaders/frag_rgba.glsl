#version 300 es

precision highp float;
in vec2 v_tex_coord;
layout(location = 0) out vec4 out_color;
uniform sampler2D s_texture;

void main()
{
  out_color = texture(s_texture, v_tex_coord);
}

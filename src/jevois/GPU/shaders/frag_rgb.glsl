#version 300 es

precision mediump float;
in vec2 v_tex_coord;
layout(location = 0) out vec4 out_color;
uniform sampler2D s_texture;

uniform vec2 tdim;

void main()
{
  float tx = (floor(v_tex_coord.x * tdim.x) * 3.0 + 0.5);
  float r = texture(s_texture, vec2(tx / (tdim.x * 3.0), v_tex_coord.y)).r;
  float g = texture(s_texture, vec2((tx + 1.0) / (tdim.x * 3.0), v_tex_coord.y)).r;
  float b = texture(s_texture, vec2((tx + 2.0) / (tdim.x * 3.0), v_tex_coord.y)).r;

  out_color = vec4(r, g, b, 1.0);
}

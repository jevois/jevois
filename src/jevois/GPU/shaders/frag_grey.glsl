#version 300 es

precision mediump float;
in vec2 v_tex_coord;
layout(location = 0) out vec4 out_color;
uniform sampler2D s_texture;

uniform vec2 tdim;

void main()
{
  float lum = texture(s_texture, v_tex_coord).r;
  out_color = vec4(lum, lum, lum, 1.0);
}

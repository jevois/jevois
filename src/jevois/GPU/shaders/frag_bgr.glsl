#version 300 es

precision mediump float;
in vec2 v_tex_coord;
layout(location = 0) out vec4 out_color;
uniform sampler2D s_texture;
vec4 rgba;

void main()
{
  rgb = texture(s_texture, v_tex_coord).bgr; // swap components
  out_color = vec4(rgb, 1.0);
}

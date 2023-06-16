#version 300 es

precision mediump float;
in vec2 v_tex_coord;
layout(location = 0) out vec4 out_color;
uniform sampler2D s_texture;
uniform float twirlamount;

uniform vec2 tdim;

void main()
{
  // First the twirl transform:
  vec2 uv = v_tex_coord + vec2(-0.5, -0.5);
  float angle = atan(uv.y, uv.x);
  float radius = length(uv);
  angle += radius * twirlamount;
  uv = radius * vec2(cos(angle), sin(angle)) + 0.5;

  // Then get the grey value:
  float lum = texture(s_texture, uv).r;
  out_color = vec4(lum, lum, lum, 1.0);
}

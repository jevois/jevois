#version 300 es

precision highp float;
in vec2 v_tex_coord;
layout(location = 0) out vec4 out_color;
uniform sampler2D s_texture;

uniform float twirlamount;
uniform float alpha;
uniform vec2 tdim;

void main()
{
  // First the twirl transform:
  vec2 uv = v_tex_coord + vec2(-0.5, -0.5);
  float angle = atan(uv.y, uv.x);
  float radius = length(uv);
  angle += radius * twirlamount;
  uv = radius * vec2(cos(angle), sin(angle)) + 0.5;

  // Then get the R, G, B colors:
  float tx = (floor(uv.x * tdim.x) * 3.0 + 0.5);
  float r = texture(s_texture, vec2(tx / (tdim.x * 3.0), uv.y)).r;
  float g = texture(s_texture, vec2((tx + 1.0) / (tdim.x * 3.0), uv.y)).r;
  float b = texture(s_texture, vec2((tx + 2.0) / (tdim.x * 3.0), uv.y)).r;

  out_color = vec4(r, g, b, alpha);
}

#version 300 es

precision highp float;
in vec2 v_tex_coord;
layout(location = 0) out vec4 out_color;
uniform sampler2D s_texture;

uniform float twirlamount;
uniform float alpha;
uniform vec2 tdim;

const vec3 offset = vec3(-0.0625, -0.5, -0.5);
const mat3 converter = mat3(1.164, 1.164, 1.164,   0.000,-0.391, 2.018,   1.596,-0.813, 0.000);

void main()
{
  // First the twirl transform:
  vec2 uv = v_tex_coord + vec2(-0.5, -0.5);
  float angle = atan(uv.y, uv.x);
  float radius = length(uv);
  angle += radius * twirlamount;
  uv = radius * vec2(cos(angle), sin(angle)) + 0.5;

  // Then get YUYV and convert to RGB:
  vec4 yuyv = texture(s_texture, uv);
  float tx = uv.x * tdim.x;
  float odd = floor(mod(tx, 2.0));
  float y = odd * yuyv.z + (1.0 - odd) * yuyv.x;
  
  vec3 yuv = vec3(y, yuyv.y, yuyv.w);
  vec3 rgb = converter * (yuv + offset);
  
  out_color = vec4(rgb, alpha);
}

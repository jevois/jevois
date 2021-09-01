#version 300 es

precision mediump float;
in vec2 v_tex_coord;
layout(location = 0) out vec4 out_color;
uniform sampler2D s_texture;

uniform vec2 tdim;

const vec3 offset = vec3(-0.0625, -0.5, -0.5);
const mat3 converter = mat3(1.164, 1.164, 1.164,   0.000,-0.391, 2.018,   1.596,-0.813, 0.000);

void main()
{
  highp vec4 yuyv = texture(s_texture, v_tex_coord);
  highp float tx = v_tex_coord.x * tdim.x;
  float odd = floor(mod(tx, 2.0));
  float y = odd * yuyv.z + (1.0 - odd) * yuyv.x;
  
  vec3 yuv = vec3(y, yuyv.y, yuyv.w);
  vec3 rgb = converter * (yuv + offset);
  
  out_color = vec4(rgb, 1);
}

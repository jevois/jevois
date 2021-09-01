#version 300 es
#extension GL_EXT_YUV_target : require

precision mediump float;
in vec2 v_tex_coord;
layout(location = 0) out vec4 out_color;
uniform __samplerExternal2DY2YEXT u_sTexture;

uniform vec2 tdim;

void main()
{
  vec4 yuvTex = texture(u_sTexture, v_tex_coord);
  vec3 rgb = yuv_2_rgb(yuvTex.xyz, itu_601);
  out_color = vec4(rgb, 1);
}

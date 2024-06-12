#version 300 es
#extension GL_OES_EGL_image_external : require

precision highp float;
in vec2 v_tex_coord;
layout(location = 0) out vec4 out_color;
uniform samplerExternalOES s_texture;
uniform float twirlamount;
uniform float alpha;

void main()
{
  // First the twirl transform:
  vec2 uv = v_tex_coord + vec2(-0.5, -0.5);
  float angle = atan(uv.y, uv.x);
  float radius = length(uv);
  angle += radius * twirlamount;
  uv = radius * vec2(cos(angle), sin(angle)) + 0.5;

  // Then get the pixel value:
  out_color = texture(s_texture, uv);
  out_color.w *= alpha;
}

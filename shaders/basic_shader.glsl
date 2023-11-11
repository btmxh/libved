#type vertex

#version 320 es

layout(location = 0) out vec2 coords;

const vec2 positions[] = vec2[](
  vec2(-0.5, 0.5),
  vec2(-0.5, -0.5),
  vec2(0.5, 0.5),
  vec2(0.5, -0.5)
);

void main()
{
  vec2 aPos = positions[gl_VertexID];
  gl_Position = vec4(aPos.x * 2.0, aPos.y * 2.0, 0.0, 1.0);
  coords = vec2(aPos.x, -aPos.y) + 0.5;
}

#type fragment

#version 320 es
precision lowp float;

layout (location = 0) in vec2 coords;
layout (binding = 0) uniform sampler2D luma;
layout (binding = 1) uniform sampler2D chroma;

layout(location = 0)out vec4 FragColor;
const mat4 yuv2rgb = mat4(
    vec4(  1.1644,  1.1644,  1.1644,  0.0000 ),
    vec4(  0.0000, -0.2132,  2.1124,  0.0000 ),
    vec4(  1.7927, -0.5329,  0.0000,  0.0000 ),
    vec4( -0.9729,  0.3015, -1.1334,  1.0000 ));

void main()
{
  FragColor = yuv2rgb * vec4(texture(luma, coords).x, texture(chroma, coords).xy, 1.0);
}

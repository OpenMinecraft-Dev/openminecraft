#version 330 core
#extension GL_ARB_separate_shader_objects : enable

#include "basics/geometry.glsl"
#include "basics/vertexgen.glsl"

uniform ScreenData
{
    float width;
    float height;
}
ubo;

#vertex

layout(location = 0) out vec2 fragCoord;

void main()
{
    vec2 uv = vertexgen_quad_normal();
    fragCoord = uv;
    uv.y = 1 - uv.y;

    vec2 center = inShaderScenePos.xy + inShaderScenePos.zw * 0.5;
    vec2 size = abs(inShaderScenePos.zw) * inShaderSceneScale;
    vec2 screenPos = center - size * 0.5 + uv * size;

    gl_Position = vec4(geom_toNdc(screenPos, ubo.width, ubo.height), inShaderSceneDepth, 1.0);
}

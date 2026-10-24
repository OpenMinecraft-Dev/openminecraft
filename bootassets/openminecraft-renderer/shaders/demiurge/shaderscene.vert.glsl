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

    vec2 screenPos = inShaderScenePos.xy + uv * inShaderScenePos.zw;

    gl_Position = vec4(geom_toNdc(screenPos, ubo.width, ubo.height), 0.0, 1.0);
}

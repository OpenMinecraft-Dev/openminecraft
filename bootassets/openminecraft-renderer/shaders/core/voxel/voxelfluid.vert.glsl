#version 330 core
#extension GL_ARB_separate_shader_objects : enable

#vertex

#include "basics/vertexgen.glsl"

layout(location = 0) out vec2 voxTexCoord;
layout(location = 1) out vec3 voxNormal;
layout(location = 2) out float voxTexLayer;
layout(location = 3) out vec2 voxLight;
layout(location = 4) out float voxFactor;
layout(location = 5) flat out int voxSec;

#include "basics/structs/camera.glsl"
uniform sampler2DArray inTexture;
uniform sampler2DArray inTextureSec;
uniform samplerBuffer inChunkPos;

#define VOXEL_X (((voxelPos) >> 28) & 15)
#define VOXEL_Y (((voxelPos) >> 24) & 15)
#define VOXEL_Z (((voxelPos) >> 20) & 15)
#define VOXEL_ENABLED (((voxelPos) >> 19) & 1)
#define VOXEL_FACING_SIGN (((voxelPos) >> 18) & 1)
#define VOXEL_FACING_AXIS (((voxelPos) >> 16) & 3)
#define VOXEL_FLUIDFLOW (voxelPos & 1)
#define VOXEL_TEXTUREID ((voxelMetadata >> 16) & 0x3fff)
#define VOXEL_CHUNKID ((((voxelPos >> 1) & 7) << 16) | ((voxelMetadata) & 0xffff))
#define VOXEL_SL(n) ((voxelExtra >> (28 - 4 * (n))) & 15)
#define VOXEL_BL(n) ((voxelExtra >> (12 - 4 * (n))) & 15)
#define VOXEL_FLUIDH(n) float((voxelExtra2 >> (24 - 8 * n)) & 0xff)

void main()
{
    if (VOXEL_ENABLED == 0)
    {
        gl_Position = vec4(0.0, 0.0, 2.0, 1.0);
        return;
    }
    float unused = texture(inTexture, vec3(0.0)).x * texture(inTextureSec, vec3(0.0)).x;

    float bx = float(VOXEL_X);
    float by = float(VOXEL_Y);
    float bz = float(VOXEL_Z);
    vec2 orgin = VOXEL_FACING_SIGN == 1 ? vertexgen_quad_normal() : vertexgen_quad_normal_ccw();
    vec2 inv_or = vec2(1.0) - orgin;
    float sign = float(VOXEL_FACING_SIGN);
    vec3 worldPos;
    vec3 worldPosOffset;
    vec3 norm;
    vec2 uv;

    switch (VOXEL_FACING_AXIS)
    {
    case 0: {
        worldPos = vec3(0, orgin.x, orgin.y);
        worldPosOffset = vec3(sign, 0, 0);
        norm = vec3(1.0, 0.0, 0.0);
        uv = (sign == 0.0) ? vec2(orgin.y, inv_or.x) : inv_or.yx;
        voxFactor = 0.6;
        break;
    }
    case 1: {
        worldPos = vec3(orgin.x, orgin.y, 0);
        worldPosOffset = vec3(0, 0, sign);
        norm = vec3(0.0, 0.0, 1.0);
        uv = (sign == 0.0) ? inv_or.xy : vec2(orgin.x, inv_or.y);
        voxFactor = 0.8;
        break;
    }
    case 2: {
        worldPos = vec3(orgin.x, 0, inv_or.y);
        worldPosOffset = vec3(0, sign, 0);
        norm = vec3(0.0, 0.5, 0.0);
        uv = vec2(orgin.x, inv_or.y);
        voxFactor = sign == 1 ? 1.0 : 0.5;
        break;
    }
    default: {
        worldPos = vec3(0);
        break;
    }
    }

    worldPos += worldPosOffset;

    int idx = int(uv.x) << 1 | int(uv.y);
    float yoff = 0;
    if (VOXEL_FACING_AXIS == 2 && VOXEL_FACING_SIGN == 1)
    {
        yoff = VOXEL_FLUIDH(idx) / 255.0 - 1;
    }
    if (VOXEL_FACING_AXIS == 0 && VOXEL_FACING_SIGN == 0)
    {
        if (idx == 0)
        {
            yoff = VOXEL_FLUIDH(0) / 255.0 - 1;
            uv.y -= yoff;
        }
        else if (idx == 2)
        {
            yoff = VOXEL_FLUIDH(1) / 255.0 - 1;
            uv.y -= yoff;
        }
    }
    if (VOXEL_FACING_AXIS == 1 && VOXEL_FACING_SIGN == 0)
    {
        if (idx == 0)
        {
            yoff = VOXEL_FLUIDH(2) / 255.0 - 1;
            uv.y -= yoff;
        }
        else if (idx == 2)
        {
            yoff = VOXEL_FLUIDH(0) / 255.0 - 1;
            uv.y -= yoff;
        }
    }
    if (VOXEL_FACING_AXIS == 0 && VOXEL_FACING_SIGN == 1)
    {
        if (idx == 0)
        {
            yoff = VOXEL_FLUIDH(3) / 255.0 - 1;
            uv.y -= yoff;
        }
        else if (idx == 2)
        {
            yoff = VOXEL_FLUIDH(2) / 255.0 - 1;
            uv.y -= yoff;
        }
    }
    if (VOXEL_FACING_AXIS == 1 && VOXEL_FACING_SIGN == 1)
    {
        if (idx == 0)
        {
            yoff = VOXEL_FLUIDH(1) / 255.0 - 1;
            uv.y -= yoff;
        }
        else if (idx == 2)
        {
            yoff = VOXEL_FLUIDH(3) / 255.0 - 1;
            uv.y -= yoff;
        }
    }

    by += yoff;
    worldPos += vec3(bx, by, bz);

    vec3 coff = vec3(texelFetch(inChunkPos, VOXEL_CHUNKID * 3).r, texelFetch(inChunkPos, VOXEL_CHUNKID * 3 + 1).r,
                     texelFetch(inChunkPos, VOXEL_CHUNKID * 3 + 2).r);
    gl_Position = camera.viewProj * vec4(worldPos + coff, 1.0);

    if (VOXEL_FLUIDFLOW == 1)
    {
        uv = uv * 0.5 + 0.25;

        if (VOXEL_FACING_AXIS == 2 && VOXEL_FACING_SIGN == 1)
        {
            float h0 = VOXEL_FLUIDH(0);
            float h1 = VOXEL_FLUIDH(1);
            float h2 = VOXEL_FLUIDH(2);
            float h3 = VOXEL_FLUIDH(3);

            float gradX = (h2 + h3) - (h0 + h1);
            float gradZ = (h1 + h3) - (h0 + h2);
            vec2 flowDir = vec2(-gradX, -gradZ);

            float flowLen = length(flowDir);
            if (flowLen > 0.001)
            {
                vec2 f = flowDir / flowLen;

                float angle;
                if (abs(f.x) > abs(f.y) * 2.414)
                {
                    angle = f.x > 0.0 ? 0.0 : 3.14159265;
                }
                else if (abs(f.y) > abs(f.x) * 2.414)
                {
                    angle = f.y > 0.0 ? 1.57079633 : -1.57079633;
                }
                else if (f.x > 0.0 && f.y > 0.0)
                {
                    angle = 0.78539816;
                }
                else if (f.x > 0.0 && f.y < 0.0)
                {
                    angle = -0.78539816;
                }
                else if (f.x < 0.0 && f.y > 0.0)
                {
                    angle = 2.35619449;
                }
                else
                {
                    angle = -2.35619449;
                }

                float cosA = cos(angle);
                float sinA = sin(angle);
                vec2 centered = uv - 0.5;
                uv = vec2(centered.x * cosA - centered.y * sinA, centered.x * sinA + centered.y * cosA) + 0.5;
            }
        }
        voxTexCoord = uv;
    }
    else
    {
        voxTexCoord = uv;
    }
    voxNormal = normalize((vec4(norm * (VOXEL_FACING_SIGN == 0 ? -1 : 1), 0.0)).xyz);
    voxTexLayer = float(VOXEL_TEXTUREID);

    voxLight = vec2(float(VOXEL_BL(idx)) / 15, float(VOXEL_SL(idx)) / 15);
    voxSec = VOXEL_FLUIDFLOW;
}

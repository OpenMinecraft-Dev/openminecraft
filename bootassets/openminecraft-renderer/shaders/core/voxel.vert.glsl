#version 330 core
#extension GL_ARB_separate_shader_objects : enable

#vertex

#include "basics/vertexgen.glsl"

layout(location = 0) out vec2 voxTexCoord;
layout(location = 1) out vec3 voxNormal;
layout(location = 2) out float voxTexLayer;
layout(location = 3) out float voxAoLevel;

uniform ObjectInfo
{
    mat4 model;
}
ubo;
#include "basics/structs/camera.glsl"
#include "basics/structs/lighting.glsl"
uniform sampler2DArray inTexture;
uniform samplerBuffer inChunkPos;

#define VOXEL_X (((voxelPos) >> 28) & 15)
#define VOXEL_Y (((voxelPos) >> 24) & 15)
#define VOXEL_Z (((voxelPos) >> 20) & 15)
#define VOXEL_SKYLIGHT (((voxelPos) >> 16) & 15)
#define VOXEL_BLOCKLIGHT (((voxelPos) >> 12) & 15)
#define VOXEL_FACING_SIGN (((voxelPos) >> 10) & 1)
#define VOXEL_FACING_AXIS (((voxelPos) >> 8) & 3)
#define VOXEL_AO1 (((voxelPos) >> 6) & 3)
#define VOXEL_AO2 (((voxelPos) >> 4) & 3)
#define VOXEL_AO3 (((voxelPos) >> 2) & 3)
#define VOXEL_AO4 ((voxelPos) & 3)

void main()
{
    float unused = lighting.lightDirection.x + texture(inTexture, vec3(0.0)).x;

    float bx = float(VOXEL_X);
    float by = float(VOXEL_Y);
    float bz = float(VOXEL_Z);
    vec2 or = VOXEL_FACING_SIGN == 1 ? vertexgen_quad_normal() : vertexgen_quad_normal_ccw();
    vec2 inv_or = vec2(1.0) - or ;
    float sign = float(VOXEL_FACING_SIGN);
    vec3 worldPos;
    vec3 norm;
    vec2 uv;

    switch (VOXEL_FACING_AXIS)
    {
    case 0: {
        worldPos = vec3(bx + sign, by + or.x, bz + or.y);
        norm = vec3(1.0, 0.0, 0.0);
        uv = (sign == 0.0) ? vec2(or.y, inv_or.x) : inv_or.yx;
        break;
    }
    case 1: {
        worldPos = vec3(bx + or.x, by + or.y, bz + sign);
        norm = vec3(0.0, 0.0, 1.0);
        uv = (sign == 0.0) ? inv_or.xy : vec2(or.x, inv_or.y);
        break;
    }
    case 2: {
        worldPos = vec3(bx + or.x, by + sign, bz + inv_or.y);
        norm = vec3(0.0, 1.0, 0.0);
        uv = vec2(or.x, inv_or.y);
        break;
    }
    default: {
        worldPos = vec3(bx, by, bz);
        break;
    }
    }

    vec3 coff = vec3(texelFetch(inChunkPos, (voxelMetadata & 0xffff) * 3).r,
                     texelFetch(inChunkPos, (voxelMetadata & 0xffff) * 3 + 1).r,
                     texelFetch(inChunkPos, (voxelMetadata & 0xffff) * 3 + 2).r);
    gl_Position = camera.viewProj * ubo.model * vec4(worldPos + coff, 1.0);

    voxTexCoord = uv;
    voxNormal = normalize((ubo.model * vec4(norm * (VOXEL_FACING_SIGN == 0 ? -1 : 1), 0.0)).xyz);
    voxTexLayer = float(voxelMetadata >> 16);

    // INFO: forwarding
    // (0, 0) -> ao1
    // (0, 1) -> ao2
    // (1, 0) -> ao3
    // (1, 1) -> ao4
    int idx = int(uv.x) << 1 | int(uv.y);
    switch (idx)
    {
    case 0:
        voxAoLevel = VOXEL_AO1;
        break;
    case 1:
        voxAoLevel = VOXEL_AO2;
        break;
    case 2:
        voxAoLevel = VOXEL_AO3;
        break;
    case 3:
        voxAoLevel = VOXEL_AO4;
        break;
    }
}

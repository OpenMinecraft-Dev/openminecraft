#version 330 core
#extension GL_ARB_separate_shader_objects : enable

#vertex

#include "basics/vertexgen.glsl"

layout(location = 0) out vec2 voxTexCoord;
layout(location = 1) out vec3 voxNormal;
layout(location = 2) out float voxTexLayer;
layout(location = 3) out float voxAoLevel;
layout(location = 4) out vec2 voxLight;

uniform ObjectInfo
{
    mat4 model;
}
ubo;
#include "basics/structs/camera.glsl"
uniform sampler2DArray inTexture;
uniform samplerBuffer inChunkPos;

#define VOXEL_X (((voxelPos) >> 28) & 15)
#define VOXEL_Y (((voxelPos) >> 24) & 15)
#define VOXEL_Z (((voxelPos) >> 20) & 15)
#define VOXEL_ENABLED (((voxelPos) >> 19) & 1)
#define VOXEL_FACING_SIGN (((voxelPos) >> 18) & 1)
#define VOXEL_FACING_AXIS (((voxelPos) >> 16) & 3)
#define VOXEL_XD (((voxelPos) >> 12) & 15)
#define VOXEL_YD (((voxelPos) >> 8) & 15)
#define VOXEL_ZD (((voxelPos) >> 4) & 15)
#define VOXEL_ROTATION ((voxelMetadata >> 30) & 3)
#define VOXEL_TEXTUREID ((voxelMetadata >> 16) & 0x3fff)
#define VOXEL_CHUNKID ((voxelMetadata) & 0xffff)
#define VOXEL_SL(n) ((voxelExtra >> (28 - 4 * (n))) & 15)
#define VOXEL_BL(n) ((voxelExtra >> (12 - 4 * (n))) & 15)
#define VOXEL_SX (((voxelExtra2) >> 24) & 0xff)
#define VOXEL_SY (((voxelExtra2) >> 16) & 0xff)
#define VOXEL_SZ (((voxelExtra2) >> 8) & 0xff)
#define VOXEL_AO(n) ((voxelExtra2 >> (6 - 2 * (n))) & 3)

void main()
{
    if (VOXEL_ENABLED == 0)
    {
        gl_Position = vec4(0.0, 0.0, 2.0, 1.0);
        return;
    }
    float unused = texture(inTexture, vec3(0.0)).x;

    float bx = float(VOXEL_X) + float(VOXEL_XD) / 16;
    float by = float(VOXEL_Y) + float(VOXEL_YD) / 16;
    float bz = float(VOXEL_Z) + float(VOXEL_ZD) / 16;
    vec2 or = VOXEL_FACING_SIGN == 1 ? vertexgen_quad_normal() : vertexgen_quad_normal_ccw();
    vec2 inv_or = vec2(1.0) - or ;
    float sign = float(VOXEL_FACING_SIGN);
    vec3 worldPos;
    vec3 worldPosOffset;
    vec3 norm;
    vec2 uv, oruv;

    vec3 modelSize = vec3(float(VOXEL_SX) / 16, float(VOXEL_SY) / 16, float(VOXEL_SZ) / 16);

    switch (VOXEL_FACING_AXIS)
    {
    case 0: {
        worldPos = vec3(0, or.x, or.y);
        worldPosOffset = vec3(sign, 0, 0);
        norm = vec3(1.0, 0.0, 0.0);
        uv = (sign == 0.0) ? vec2(or.y, inv_or.x) : inv_or.yx;
        oruv = uv;
        uv *= modelSize.zy;
        break;
    }
    case 1: {
        worldPos = vec3(or.x, or.y, 0);
        worldPosOffset = vec3(0, 0, sign);
        norm = vec3(0.0, 0.0, 1.0);
        uv = (sign == 0.0) ? inv_or.xy : vec2(or.x, inv_or.y);
        oruv = uv;
        uv *= modelSize.xy;
        break;
    }
    case 2: {
        worldPos = vec3(or.x, 0, inv_or.y);
        worldPosOffset = vec3(0, sign, 0);
        norm = vec3(0.0, 0.5, 0.0);
        uv = vec2(or.x, inv_or.y);
        oruv = uv;
        uv *= modelSize.xz;
        break;
    }
    default: {
        worldPos = vec3(0);
        break;
    }
    }

    int rota = VOXEL_ROTATION;
    while (rota > 0)
    {
        uv = vec2(1 - uv.y, uv.x);
        --rota;
    }

    worldPos += worldPosOffset;
    worldPos *= modelSize;
    worldPos += vec3(bx, by, bz);

    vec3 coff = vec3(texelFetch(inChunkPos, VOXEL_CHUNKID * 3).r, texelFetch(inChunkPos, VOXEL_CHUNKID * 3 + 1).r,
                     texelFetch(inChunkPos, VOXEL_CHUNKID * 3 + 2).r);
    gl_Position = camera.viewProj * ubo.model * vec4(worldPos + coff, 1.0);

    voxTexCoord = uv;
    voxNormal = normalize((ubo.model * vec4(norm * (VOXEL_FACING_SIGN == 0 ? -1 : 1), 0.0)).xyz);
    voxTexLayer = float(VOXEL_TEXTUREID);

    // INFO: forwarding
    // (0, 0) -> ao1
    // (0, 1) -> ao2
    // (1, 0) -> ao3
    // (1, 1) -> ao4
    int idx = int(oruv.x) << 1 | int(oruv.y);
    voxAoLevel = VOXEL_AO(idx);
    voxLight = vec2(float(VOXEL_BL(idx)) / 15, float(VOXEL_SL(idx)) / 15);
}

#version 330 core
#extension GL_ARB_separate_shader_objects : enable

#vertex

#include "basics/vertexgen.glsl"

layout(location = 0) out vec2 voxTexCoord;
layout(location = 1) out vec3 voxNormal;
layout(location = 2) out float voxTexLayer;
layout(location = 3) out float voxAoLevel;
layout(location = 4) out vec2 voxLight;
layout(location = 5) out float voxFactor;
layout(location = 6) flat out int voxSecondaryTex;

#include "basics/structs/camera.glsl"
uniform sampler2DArray inTexture;
uniform sampler2DArray inTextureSec;
uniform samplerBuffer inChunkPos;

#define VOXEL_X (((voxelBasics) >> 28) & 15)
#define VOXEL_Y (((voxelBasics) >> 24) & 15)
#define VOXEL_Z (((voxelBasics) >> 20) & 15)
#define VOXEL_ENABLED (((voxelBasics) >> 19) & 1)
#define VOXEL_FACING_SIGN (((voxelBasics) >> 18) & 1)
#define VOXEL_FACING_AXIS (((voxelBasics) >> 16) & 3)
#define VOXEL_TEXSEC (((voxelBasics) >> 12) & 1)
#define VOXEL_RAXIS ((voxelBasics >> 10) & 3)
#define VOXEL_SHADE ((voxelBasics >> 9) & 1)
#define VOXEL_AO(n) ((voxelBasics >> (6 - 2 * (n))) & 3)
#define VOXEL_ROTATION ((voxelMetadata >> 30) & 3)
#define VOXEL_TEXTUREID ((voxelMetadata >> 16) & 0x3fff)
#define VOXEL_CHUNKID ((voxelMetadata) & 0xffff)
#define VOXEL_SL(n) ((voxelExtra >> (28 - 4 * (n))) & 15)
#define VOXEL_BL(n) ((voxelExtra >> (12 - 4 * (n))) & 15)

#define VOXEL_RCENTERXNEG ((voxelExtra3 >> 19) & 1)
#define VOXEL_RCENTERYNEG ((voxelExtra3 >> 18) & 1)
#define VOXEL_RCENTERZNEG ((voxelExtra3 >> 17) & 1)
#define VOXEL_RCENTERX ((voxelExtra3 >> 13) & 15)
#define VOXEL_RCENTERY ((voxelExtra3 >> 9) & 15)
#define VOXEL_RCENTERZ ((voxelExtra3 >> 5) & 15)

vec3 rotateVec(vec3 v, float angle, vec3 axis)
{
    float s = sin(angle);
    float c = cos(angle);
    float oc = 1.0 - c;
    return v * c + cross(axis, v) * s + axis * dot(axis, v) * oc;
}

vec2 doRotate(vec2 uv)
{
    int rota = VOXEL_ROTATION;
    while (rota > 0)
    {
        uv = vec2(uv.y, 1 - uv.x);
        --rota;
    }
    return uv;
}

void main()
{
    if (VOXEL_ENABLED == 0)
    {
        gl_Position = vec4(0.0, 0.0, 2.0, 1.0);
        return;
    }
    float unused = texture(inTexture, vec3(0.0)).x + texture(inTextureSec, vec3(0.0)).x;

    float rotationAngle = voxelRotationAngle;
    int rotationAxis = VOXEL_RAXIS;
    vec3 rotationCenter = voxelRotationCenter;

    vec3 modelOffset = voxelOffset / vec3(16.0f);

    float bx = float(VOXEL_X);
    float by = float(VOXEL_Y);
    float bz = float(VOXEL_Z);
    vec2 orgin = VOXEL_FACING_SIGN == 1 ? vertexgen_quad_normal() : vertexgen_quad_normal_ccw();
    vec2 inv_or = vec2(1.0) - orgin;
    float sign = float(VOXEL_FACING_SIGN);
    vec3 worldPos;
    vec3 worldPosOffset;
    vec3 norm;
    vec2 uv, oruv;

    vec2 uv0 = voxelUV0 / vec2(16.0);
    vec2 uv1 = voxelUV1 / vec2(16.0);

    vec3 modelSize = voxelSize / vec3(16.0);

    switch (VOXEL_FACING_AXIS)
    {
    case 0: {
        worldPos = vec3(0, orgin.x, orgin.y);
        worldPosOffset = vec3(modelSize.x == 0 ? 0 : sign, 0, 0);
        norm = vec3(1.0, 0.0, 0.0);
        uv = (sign == 0.0) ? vec2(orgin.y, inv_or.x) : inv_or.yx;
        oruv = uv;
        uv = doRotate(uv);
        voxFactor = 0.6;
        break;
    }
    case 1: {
        worldPos = vec3(orgin.x, orgin.y, 0);
        worldPosOffset = vec3(0, 0, modelSize.z == 0 ? 0 : sign);
        norm = vec3(0.0, 0.0, 1.0);
        uv = (sign == 0.0) ? inv_or.xy : vec2(orgin.x, inv_or.y);
        oruv = uv;
        uv = doRotate(uv);
        voxFactor = 0.8;
        break;
    }
    case 2: {
        worldPos = vec3(orgin.x, 0, inv_or.y);
        worldPosOffset = vec3(0, modelSize.y == 0 ? 0 : sign, 0);
        norm = vec3(0.0, 0.5, 0.0);
        uv = vec2(orgin.x, inv_or.y);
        oruv = uv;
        uv = doRotate(uv);
        voxFactor = sign == 1 ? 1.0 : 0.5;
        break;
    }
    default: {
        worldPos = vec3(0);
        break;
    }
    }

    uv = mix(uv0, uv1, uv);
    worldPos += worldPosOffset;
    worldPos *= modelSize;
    worldPos += modelOffset;

    if (rotationAxis != 3)
    {
        vec3 rotAxis = vec3(0.0);
        if (rotationAxis == 0)
            rotAxis = vec3(1.0, 0.0, 0.0);
        else if (rotationAxis == 1)
            rotAxis = vec3(0.0, 1.0, 0.0);
        else if (rotationAxis == 2)
            rotAxis = vec3(0.0, 0.0, 1.0);

        worldPos = rotateVec(worldPos - rotationCenter / vec3(16.0), radians(rotationAngle), rotAxis) +
                   rotationCenter / vec3(16.0);
    }
    else
    {
        worldPos = rotateVec(worldPos - rotationCenter / vec3(16.0), radians(rotationAngle), vec3(1.0, 0.0, 0.0));
        worldPos = rotateVec(worldPos, radians(voxelRotationAngleExt1), vec3(0.0, 1.0, 0.0));
        worldPos =
            rotateVec(worldPos, radians(voxelRotationAngleExt2), vec3(0.0, 0.0, 1.0)) + rotationCenter / vec3(16.0);
    }

    worldPos += vec3(bx, by, bz);

    vec3 coff = vec3(texelFetch(inChunkPos, VOXEL_CHUNKID * 3).r, texelFetch(inChunkPos, VOXEL_CHUNKID * 3 + 1).r,
                     texelFetch(inChunkPos, VOXEL_CHUNKID * 3 + 2).r);
    gl_Position = camera.viewProj * vec4(worldPos + coff, 1.0);

    voxTexCoord = uv;
    voxNormal = normalize((vec4(norm * (VOXEL_FACING_SIGN == 0 ? -1 : 1), 0.0)).xyz);
    voxTexLayer = float(VOXEL_TEXTUREID);

    // INFO: forwarding
    // (0, 0) -> ao1
    // (0, 1) -> ao2
    // (1, 0) -> ao3
    // (1, 1) -> ao4
    int idx = int(oruv.x) << 1 | int(oruv.y);
    voxAoLevel = VOXEL_AO(idx);
    voxLight = vec2(float(VOXEL_BL(idx)) / 15, float(VOXEL_SL(idx)) / 15);

    if (VOXEL_SHADE == 0)
    {
        voxFactor = 1.0;
    }

    voxSecondaryTex = VOXEL_TEXSEC;
}

#version 330 core
#extension GL_ARB_separate_shader_objects : enable

uniform ScreenData
{
    float width;
    float height;
}
ubo;
uniform Time
{
    float value;
}
timeVar;

layout(location = 0) in vec2 fragCoord;
layout(location = 0) out vec4 fragColor;

#define PI 3.14
#define TWO_PI 6.28

#define TIMESCALE 0.5

#define BLUR_STRENGTH 2.0
#define BLUR_RANGE 2.7

#define UI_COLOR vec4(0.5, 0.8, 1.0, 1.0)

float hash12(vec2 x)
{
    return fract(sin(dot(x, vec2(43.5287, 41.12871))) * 523.582);
}

vec2 hash21(float x)
{
    return fract(sin(x * vec2(24.0181, 52.1984)) * 5081.4972);
}

float hash11(float x)
{
    return fract(sin(x * 42.146291) * 4215.4827);
}

vec2 hash22(vec2 x)
{
    return fract(sin(x * mat2x2(24.4372, 12.47864, 32.3874, 29.4873)) * 4762.832);
}

mat2x2 rotationMatrix(in float angle)
{
    return mat2x2(-cos(angle), sin(angle), -sin(angle), -cos(angle));
}

float circle(in vec2 uv, in float radius, in float width)
{
    return smoothstep(width, width * 0.5, abs(radius - length(uv)));
}

float softCircle(in vec2 uv, in float radius, in float width)
{
    return smoothstep(width, 0.0, abs(radius - length(uv)));
}

float hardCircle(in vec2 uv, in float radius, in float width)
{
    return smoothstep(width, width * 0.99, abs(radius - length(uv)));
}

float dottedCircle(in vec2 uv, in float circleRadius, in float dotRadius, float dotsCount)
{
    float angle = atan(uv.y, uv.x);
    angle /= TWO_PI;
    angle += 0.5;
    angle = round(angle * dotsCount) / dotsCount;
    angle *= TWO_PI;
    // angle *= round(angle / TWO_PI * dotsCount) * TWO_PI;
    vec2 dotPoint = vec2(circleRadius, 0.0) * rotationMatrix(angle);
    return smoothstep(dotRadius, dotRadius * 0.5, distance(dotPoint, uv));

    return angle;
}

float circularSector(vec2 uv, in float radius, in float width, in float cutAngle)
{
    float angle = atan(uv.y, uv.x) + PI;
    float circ = circle(uv, radius, width);
    return circ * smoothstep(cutAngle, cutAngle - 0.001, abs(angle - cutAngle));
}

float cutSector(in vec2 uv, in float cutAngle, in float offset)
{
    float angle = atan(uv.y, uv.x) + PI + offset;
    angle = mod(angle, TWO_PI);
    return smoothstep(cutAngle, cutAngle - 0.0001, abs(angle - cutAngle));
}

float dashedCircle(vec2 uv, in float radius, in float width, in float density)
{
    float angle = atan(uv.y, uv.x) + PI;
    angle /= TWO_PI;
    angle = fract(angle * density);
    float circ = circle(uv, radius, width);
    return circ * smoothstep(0.1, 0.11, abs(angle - 0.5));
}

float dottedGrid(vec2 uv, in vec2 gridSpaces, float dotRadius)
{
    uv = mod(uv, gridSpaces) - gridSpaces * 0.5;
    return smoothstep(dotRadius, 0.0, abs(length(uv) - dotRadius));
}

float lineGrid(vec2 uv, in vec2 gridSpaces, float lineWidth)
{
    uv = mod(uv, gridSpaces) - gridSpaces * 0.5;
    float verticalLines = smoothstep(lineWidth, lineWidth * 0.5, abs(uv.x - lineWidth));
    float horizontalLines = smoothstep(lineWidth, lineWidth * 0.5, abs(uv.y - lineWidth));
    return verticalLines + horizontalLines;
}

float writings(vec2 uv, in float linesCount, in float maxLen, in float height, in float hash)
{
    float mul = smoothstep(0.0, 0.01, uv.x) * smoothstep(0.0, 0.01, uv.y) * smoothstep(height, height - 0.001, uv.y);

    uv.y = clamp(uv.y, 0.0, height);
    uv.y *= linesCount;
    float len = hash11(floor(uv.y - linesCount) + hash) * maxLen;
    return smoothstep(len, len * 0.5, uv.x) * hash12(uv) * mul * fract(uv.y);
}

float writingsGrid(vec2 uv, in vec2 gridSpace, in float scale)
{
    uv /= gridSpace;
    vec2 floorUV = floor(uv);
    vec2 fractUV = fract(uv);

    fractUV -= hash22(floorUV) * (1.0 - (1.0 / scale)) *
               (sin(timeVar.value * 0.6 * TIMESCALE + hash12(floorUV) * 12.0) * 0.5 + 0.5);
    fractUV *= scale;

    float writing = writings(fractUV, 10.0 * hash12(floorUV) + 5.0, 1.0, hash12(floorUV + 0.1) * 0.6, 1.0);

    return writing;
}

vec3 uvToCameraPlanePoint(in vec2 uv)
{
    return vec3(uv.x, uv.y, 1.5 + sin(timeVar.value * TIMESCALE * 0.3) * 0.2);
}

// xy - plane uv
// z - plane height
// w - distance to plane
vec4 raycastPlane(in vec3 rayOrigin, vec3 rayDirection, in float planeHeight)
{
    float distanceToPlane = abs(rayOrigin.y - planeHeight);
    rayDirection /= rayDirection.y;
    rayDirection *= distanceToPlane;
    vec3 hitPoint = rayOrigin + rayDirection;

    return vec4(hitPoint.x, hitPoint.z, hitPoint.y, length(rayDirection));
}

void main()
{
    vec2 uv = (fragCoord - 0.5) * vec2(ubo.width / ubo.height, 1.0);
    // vec2 uv = (2.0 * fragCoord - iResolution.xy) / iResolution.x;
    float time = timeVar.value * TIMESCALE;
    float angleTime = timeVar.value * 0.5 * TIMESCALE;

    float angleY = -time;
    float angleX = -0.8 + sin(angleTime) * 0.3;
    float angleZ = cos(time * 0.72) * 0.2;
    mat3 rotation = mat3(-cos(angleZ), sin(angleZ), 0, -sin(angleZ), -cos(angleZ), 0, 0, 0, 1) *
                    mat3(1, 0, 0, 0, -cos(angleX), sin(angleX), 0, -sin(angleX), -cos(angleX)) *
                    mat3(-cos(angleY), 0, sin(angleY), 0, 1, 0, -sin(angleY), 0, -cos(angleY));

    vec3 cameraPosition = vec3(sin(time), -3.1 - sin(angleTime) * 0.2, cos(time));
    cameraPosition.xz *= 0.9 + sin(angleTime) * 0.4;
    vec3 castPlanePoint = uvToCameraPlanePoint(uv);
    castPlanePoint = castPlanePoint * rotation;
    castPlanePoint += cameraPosition;
    vec3 rayOrigin = vec3(cameraPosition);
    vec3 rayDirection = normalize(castPlanePoint - cameraPosition);

    vec4 hitPoint = raycastPlane(rayOrigin, rayDirection, -4.0);
    float dist = hitPoint.w;
    float fog = smoothstep(5.0, 0.0, dist);
    float dots = dottedGrid(hitPoint.xy, vec2(0.03), 0.001);
    dots += hardCircle(hitPoint.xy, 0.3, 0.07 + 0.03 * sin(time * 0.6)) *
            cutSector(hitPoint.xy, 1.5 + cos(time * 0.4) * 2.0, time * 2.0) * 0.4;

    hitPoint = raycastPlane(rayOrigin, rayDirection, -4.02);
    float lines = lineGrid(hitPoint.xy, vec2(0.4), 0.001) * 0.5;
    lines += hardCircle(hitPoint.xy, 0.12, 0.02 + 0.01 * -sin(time * 0.6 + 2.0)) *
             cutSector(hitPoint.xy, 1.5 + cos(time * 1.4) * 2.0, -time * 0.8) * 0.4;

    hitPoint = raycastPlane(rayOrigin, rayDirection, -3.98);
    float lines2 = lineGrid(hitPoint.xy + 0.14, vec2(0.8), 0.002) * 0.4;

    hitPoint = raycastPlane(rayOrigin, rayDirection, -3.97);
    float dotCircle = dottedCircle(hitPoint.xy, 0.5, 0.003, 200.0) +
                      dottedCircle(hitPoint.xy, 0.5, 0.006, 20.0) * cutSector(hitPoint.xy, 2.3, time * 4.0);

    hitPoint = raycastPlane(rayOrigin, rayDirection, -3.9);
    float circle1 = circle(hitPoint.xy, 0.6, 0.003) * 0.4 +
                    0.4 *
                        (dottedCircle(hitPoint.xy, 0.61, 0.003, 300.0) + dottedCircle(hitPoint.xy, 0.61, 0.006, 30.0)) *
                        cutSector(hitPoint.xy, 1.0, -timeVar.value * 0.1);

    hitPoint = raycastPlane(rayOrigin, rayDirection, -3.94);
    float circle2 = circle(hitPoint.xy, 0.45, 0.003) * 0.3 + 0.3 * dottedCircle(hitPoint.xy, 0.46, 0.004, 250.0);

    hitPoint = raycastPlane(rayOrigin, rayDirection, -3.88);
    float circle3 = circle(hitPoint.xy, 0.42, 0.003) * 0.4 +
                    dottedCircle(hitPoint.xy, 0.42, 0.005, 20.0) * 0.8 * cutSector(hitPoint.xy, 1.0, time * 2.0);

    hitPoint = raycastPlane(rayOrigin, rayDirection, -3.8);
    float circle4 = dashedCircle(hitPoint.xy, 0.38, 0.006, 10.0) * 0.8 * cutSector(hitPoint.xy, 1.0, -time * 0.6);
    circle4 +=
        dashedCircle(hitPoint.xy, 0.3, 0.0045, 2.5 + sin(time * 2.0)) * cutSector(hitPoint.xy, 2.0, time * 0.28) * 0.7;

    hitPoint = raycastPlane(rayOrigin, rayDirection, -3.84);
    float circle5 = circle(hitPoint.xy, 0.15, 0.003) * 0.4 + dottedCircle(hitPoint.xy, 0.2, 0.003, 100.0) * 0.8 *
                                                                 cutSector(hitPoint.xy, 0.5, cos(time * 0.2) * 10.0);

    hitPoint = raycastPlane(rayOrigin, rayDirection, -3.8);
    circle5 += circle(hitPoint.xy, 0.1, 0.003) * 0.4 + dottedCircle(hitPoint.xy, 0.2, 0.003, 100.0) * 0.8 *
                                                           cutSector(hitPoint.xy, 1.0, sin(time * 0.2 + 2.0) * 10.0);

    hitPoint = raycastPlane(rayOrigin, rayDirection, -4.0);
    float writingGrid = writingsGrid(hitPoint.xy + time * 0.02, vec2(0.6, 0.6), 6.0);

    hitPoint = raycastPlane(rayOrigin, rayDirection, -3.8);
    writingGrid += writingsGrid(hitPoint.xy - time * 0.02 + 20.0, vec2(1.0, 1.0), 6.0);

    hitPoint = raycastPlane(rayOrigin, rayDirection, -3.85);
    float circle6 =
        circle(hitPoint.xy, 0.05, 0.0025) * 0.6 * cutSector(hitPoint.xy, 2.5 + sin(time * 0.67) * 0.5, time * 2.0);

    float UIMerge =
        writingGrid + dots + lines + lines2 + dotCircle + circle1 + circle2 + circle3 + circle4 + circle5 + circle6;
    UIMerge *= fog;

    // Output to screen
    fragColor = UI_COLOR * UIMerge;
}
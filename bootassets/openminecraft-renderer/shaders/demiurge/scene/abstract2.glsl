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
time;

layout(location = 0) in vec2 fragCoord;
layout(location = 0) out vec4 fragColor;
#define BackgroundColor vec3(0.91765, 0.88627, 0.82745)
#define Layer1Color vec3(0.95686, 0.27451, 0.16078)
#define Layer2Color vec3(0.10980, 0.12157, 0.27843)
#define Layer3Color vec3(0.18824, 0.20000, 0.52157)

#define degToRad 0.01745329252

float Rand(float i)
{
    return fract(sin(i * 23325.) * 35543.);
}

vec4 Rand4(float i)
{
    return fract(sin(i * vec4(23325., 53464., 76543., 12312)) * vec4(35543., 63454., 23454., 87651));
}

mat2 Rot(float a)
{
    float s = sin(a);
    float c = cos(a);
    return mat2(c, -s, s, c);
}

float DrawLine(in vec2 uv, in vec2 a, in vec2 b)
{
    vec2 ba = b - a;
    vec2 pa = uv - a;
    float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    return length(pa - h * ba);
}

float DrawLineSegment(in vec2 uv, float linesCount, float speed, float verticalAmplitude, float segmentSeed)
{
    float segmentMask = 0.;

    float iterationStep = 1. / linesCount;
    float t = time.value * speed * .1;

    float horizontalAmplitude = 3.5;
    vec2 lineWidthRange = vec2(0.2, 1.5);
    vec2 lineSizeRange = vec2(0.005, 0.035);

    float seedBase = Rand(segmentSeed);

    for (float i = 0.; i <= 1.; i += iterationStep)
    {
        float unitSpeed = mix(0.5, 2.0, Rand(i));
        float seed = t * unitSpeed + i + seedBase;
        float it = fract(seed);
        vec4 iHash = Rand4(i);

        float normit = it * 2. - 1.;

        float lineWidth = mix(lineWidthRange.x, lineWidthRange.y, pow(iHash.y, 2.));

        vec2 ap = vec2(-horizontalAmplitude * normit, iHash.x * verticalAmplitude);
        vec2 bp = ap + vec2(lineWidth, 0.);

        float lineSegmentDist = DrawLine(uv, ap, bp);

        float lineSize = mix(lineSizeRange.x, lineSizeRange.y, pow(iHash.z, 4.));

        segmentMask += smoothstep(lineSize + 0.002, lineSize - 0.002, lineSegmentDist);
    }

    return clamp(segmentMask, 0., 1.);
}

void main()
{
    vec2 uv = (2.0 * fragCoord - vec2(1.0)) * vec2(ubo.width / ubo.height, 1.0);

    uv *= Rot(degToRad * -35.);

    vec3 col = mix(Layer1Color, BackgroundColor, smoothstep(-0.055, -0.05, uv.y));
    col = mix(Layer2Color, col, smoothstep(-0.655, -0.65, uv.y));
    col = mix(Layer3Color, col, smoothstep(-1.305, -1.3, uv.y));

    col = mix(col, BackgroundColor, DrawLineSegment(uv - vec2(0., -0.3), 9., 0.5, 0.35, 0.2));
    col = mix(col, Layer1Color, DrawLineSegment(uv - vec2(0., 0.), 25., -.4, .5, 0.1));

    col = mix(col, Layer1Color, DrawLineSegment(uv - vec2(0., -1.0), 25., -0.4, .45, 0.4));
    col = mix(col, Layer2Color, DrawLineSegment(uv - vec2(0., -0.65), 25., 0.3, .3, 0.3));

    col = mix(col, Layer2Color, DrawLineSegment(uv - vec2(0., -1.8), 25., 0.3, .55, 0.6));
    col = mix(col, Layer3Color, DrawLineSegment(uv - vec2(0., -1.3), 25., -0.2, .3, 0.5));

    fragColor = vec4(col, 1.0);
}
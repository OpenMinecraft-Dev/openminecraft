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

vec3 sin_shape(in vec2 uv, in float offset_y)
{
    // Time varying pixel color
    float y = sin((uv.x + time.value * -0.06 + offset_y) * 5.5);

    float x = uv.x * 8.;
    float a = 1.;
    for (int i = 0; i < 5; i++)
    {
        x *= 0.53562;
        x += 6.56248;
        y += sin(x) * a;
        a *= .5;
    }

    float y0 = step(0.0, y * 0.08 - uv.y + offset_y);
    return vec3(y0, y0, y0);
}

vec2 rotate(vec2 coord, float alpha)
{
    float cosA = cos(alpha);
    float sinA = sin(alpha);
    return vec2(coord.x * cosA - coord.y * sinA, coord.x * sinA + coord.y * cosA);
}

vec3 scene(in vec2 uv)
{
    vec3 col = vec3(0.0, 0.0, 0.0);
    col += sin_shape(uv, 0.3) * 0.2;
    col += sin_shape(uv, 0.7) * 0.2;
    col += sin_shape(uv, 1.1) * 0.2;

    vec3 fragColor;

    if (col.x >= 0.6)
    {
        fragColor = vec3(0.27, 0.11, 0.64);
    }
    else if (col.x >= 0.4)
    {
        fragColor = vec3(0.55, 0.19, 0.69);
    }
    else if (col.x >= 0.2)
    {
        fragColor = vec3(0.68, 0.23, 0.65);
    }
    else
    {
        fragColor = vec3(0.86, 0.57, 0.68);
    }
    return fragColor;
}

void main()
{
    vec2 iResolution = vec2(ubo.width, ubo.height);
    vec2 r = rotate(fragCoord * iResolution + vec2(0.0, -300.0), 0.5);
    // Normalized pixel coordinates (from 0 to 1)
    vec3 col0 = scene((r * 2.0) / iResolution.xy);
    vec3 col1 = scene(((r * 2.0) + vec2(1.0, 0.0)) / iResolution.xy);
    vec3 col2 = scene(((r * 2.0) + vec2(1.0, 1.0)) / iResolution.xy);
    vec3 col3 = scene(((r * 2.0) + vec2(0.0, 1.0)) / iResolution.xy);

    // Output to screen
    fragColor = vec4((col0 + col1 + col2 + col2) / 4.0, 1.0);
}
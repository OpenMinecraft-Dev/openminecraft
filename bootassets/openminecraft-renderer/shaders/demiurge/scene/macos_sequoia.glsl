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
#define EPS 0.0001

float sdf(inout vec3 pos)
{
    vec2 mx = mod(pos.xz + 2.0, 4.0) - 2.0;
    mx += vec2(cos(pos.y + pos.z) * 0.1, cos(pos.y - pos.x) * 0.1);
    float angle = atan(mx.y, mx.x) + sin(pos.y) * 0.3;
    float dist = sqrt(mx.x * mx.x + mx.y * mx.y) - 1.2 + pos.y * 0.05 + sqrt(max(cos(angle * 10.), 0.1)) * 0.05 -
                 0.5 * 1. / sqrt(2. + pos.y * pos.y);
    if (dist < 0.1)
    {
        dist *= 0.6;
    }
    return clamp(dist * 0.9, -2.0, 1.0);
}

vec3 colFn(in float x)
{
    vec3 yellow = vec3(1.0, 0.9, 0.6);
    vec3 orange = vec3(0.9, 0.6, 0.6);
    vec3 lightBlue = vec3(0.4, 0.7, 1.0);
    vec3 blue = vec3(0.1, 0.1, 1.0);
    if (x < 0.333333)
    {
        x = x * 3.0;
        return mix(blue, lightBlue, clamp(x, 0., 1.));
    }
    else if (x < 0.666666)
    {
        x = x * 3.0 - 1.0;
        return mix(lightBlue, orange, clamp(x, 0., 1.));
    }
    else
    {
        x = x * 3.0 - 2.0;
        return mix(orange, yellow, clamp(x, 0., 1.));
    }
}

float shadeFn(in vec3 pos)
{
    vec2 mx = mod(pos.xz + 2.0, 4.0) - 2.0;
    mx += vec2(cos(pos.y + pos.z) * 0.1, cos(pos.y - pos.x) * 0.1);
    float angle = atan(mx.y, mx.x) + sin(pos.y) * 0.1;
    float old = abs(cos(angle * 40. + 0.3 * sin(angle * 23.))) * (0.1 + 0.1 * sin(pos.y + angle * 2.0)) +
                sqrt(clamp(cos(angle * 10.), 0.0, 1.0)) * 0.7 - 1.5;
    return old * (2. / sqrt(.5 + pos.y * pos.y) + 1.);
}

void main()
{
    float size = min(ubo.width, ubo.height);

    vec2 p = (fragCoord * vec2(ubo.width, ubo.height) - (vec2(ubo.width, ubo.height) / 2.0)) / size;

    vec3 cam = vec3(2.0, 0.0, time.value);

    float shade = 1.0;

    // march
    vec3 ray = cam;
    float ct, st;
    ct = cos(0.9);
    st = sin(0.9);

    vec3 dir = normalize(vec3(p, 1.0));
    dir.yz *= mat2(ct, st, -st, ct);
    dir.xz *= mat2(ct, st, -st, ct);

    float total_dist = 0.0;

    int i;
    for (i = 0; i < 40; i++)
    {
        float dist = sdf(ray);
        ray += dir * dist / sqrt(1.0 - dir.y * dir.y) * 0.99;
        total_dist += dist;
        if (dist < EPS)
        {
            // float num = 0.1 * total_dist + ray.y * 0.3 - 1.5;
            // shade = 1. / (1. + exp(-num));
            break;
        }
    }
    float num = 0.2 * total_dist + ray.y * 0.3 - 2.0;
    num += shadeFn(ray) * 0.1;

    shade = 1. / (1. + exp(-num));

    float l = length(p - vec2(0.0, 1.0));
    shade += pow(clamp((1. - l), 0., 1.), 4.0) * 0.01 * ray.y + dir.y * 0.05;

    vec3 col = colFn(shade);

    fragColor = vec4(col, 1.0);
}
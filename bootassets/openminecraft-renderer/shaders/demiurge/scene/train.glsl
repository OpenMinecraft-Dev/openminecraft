#version 330 core
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 fragCoord;
layout(location = 0) out vec4 fragColor;

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

float hash(vec2 p)
{
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

float noise(vec2 x)
{
    vec2 p = floor(x);
    vec2 f = fract(x);
    vec2 u = f * f * (3.0 - 2.0 * f);

    float a = hash(p);
    float b = hash(p + vec2(1.0, 0.0));
    float c = hash(p + vec2(0.0, 1.0));
    float d = hash(p + vec2(1.0, 1.0));

    return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

float fbm(vec2 x, int detail)
{
    float a = 0.0;
    float b = 1.0;
    float t = 0.0;
    for (int i = 0; i < detail; i++)
    {
        float n = noise(x);
        a += b * n;
        t += b;
        b *= 0.7;
        x *= 2.0;
    }
    return a / t;
}

float fbm2(vec2 x, int detail)
{
    float a = 0.0;
    float b = 1.0;
    float t = 0.0;
    for (int i = 0; i < detail; i++)
    {
        float n = noise(x);
        a += b * n;
        t += b;
        b *= 0.9;
        x *= 2.0;
    }
    return a / t;
}

float box(vec2 uv, float x1, float x2, float y1, float y2)
{
    return (uv.x > x1 && uv.x < x2 && uv.y > y1 && uv.y < y2) ? 1.0 : 0.0;
}

#define dot2(v) dot(v, v)
#define layer(dh, v) if (uv.y < h + midlevel - (dh)) { return vec4(v, 1.); }

vec4 foreground(vec2 uv, float t)
{
    float midlevel;
    float h;
    float disp;
    float dist;
    vec2 uv2;

    uv.y -= 0.2;
    // clouds foreground //////////////////////////////////////////////////////////////

    // c14
    midlevel = -0.1;
    disp = 1.7;
    dist = 1.0;
    uv2 = uv + vec2(t / dist + 40.0, 0.0);
    h = (fbm(uv2, 8) - 0.5) * disp;
    layer(0.12, vec3(0.43, 0.32, 0.31));
    layer(0.08, vec3(0.55, 0.42, 0.41));
    layer(0.04, vec3(0.66, 0.42, 0.40));
    layer(0., vec3(0.77, 0.48, 0.46));

    // c13

    midlevel = 0.05;
    disp = 1.7;
    dist = 2.0;
    uv2 = uv + vec2(t / dist + 38.0, 0.0);
    h = (fbm(uv2, 8) - 0.5) * disp;
    layer(0.1, vec3(0.95, 0.66, 0.48));
    layer(0.04, vec3(0.98, 0.76, 0.64));
    layer(0., vec3(0.95, 0.80, 0.77));

    return vec4(0.95, 0.80, 0.77, 0.);
}

vec4 background(vec2 uv, float t)
{
    float midlevel;
    float h;
    float disp;
    float dist;
    vec2 uv2;

    // clouds ///////////////////////////////////////////////////////

    // c12
    midlevel = 0.3;
    disp = 0.9;
    dist = 10.0;
    uv2 = uv + vec2(t / dist + 32.5, 0.0);
    h = (fbm(uv2, 8) - 0.5) * disp;
    layer(0.14, vec3(0.48, 0.19, 0.20));
    layer(0.1, vec3(0.68, 0.28, 0.19));
    layer(0.07, vec3(0.88, 0.38, 0.24));
    layer(0., vec3(0.95, 0.45, 0.30));

    // c11
    midlevel = 0.35;
    disp = 1.0;
    dist = 15.0;
    uv2 = uv + vec2(t / dist + 30.0, 0.0);
    h = (fbm(uv2, 8) - 0.5) * disp;
    layer(0.04, vec3(0.98, 0.76, 0.64));
    layer(0., vec3(0.95, 0.80, 0.77));

    // c10
    midlevel = 0.35;
    disp = 3.5;
    dist = 20.0;
    uv2 = uv + vec2(t / dist + 27.5, 0.0);
    h = (fbm(uv2, 8) - 0.5) * disp;
    layer(0.12, vec3(0.43, 0.32, 0.31));
    layer(0.08, vec3(0.55, 0.42, 0.41));
    layer(0.04, vec3(0.66, 0.42, 0.40));
    layer(0., vec3(0.77, 0.48, 0.46));

    // c9
    midlevel = 0.45;
    disp = 2.0;
    dist = 25.0;
    uv2 = uv + vec2(t / dist + 23.0, 0.0);
    h = (fbm(uv2, 8) - 0.5) * disp;
    layer(0.04, vec3(0.98, 0.57, 0.36));
    layer(0., vec3(1.0, 0.62, 0.44));

    // c8
    midlevel = 0.5;
    disp = 2.3;
    dist = 30.0;
    uv2 = uv + vec2(t / dist + 20.5, 0.0);
    h = (fbm(uv2, 8) - 0.5) * disp;
    layer(0.12, vec3(0.41, 0.27, 0.27));
    layer(0.08, vec3(0.53, 0.35, 0.32));
    layer(0.04, vec3(0.80, 0.24, 0.17));
    layer(0., vec3(0.99, 0.29, 0.20));

    // c7
    midlevel = 0.5;
    disp = 2.5;
    dist = 35.0;
    uv2 = uv + vec2(t / dist + 18.0, 0.0);
    h = (fbm(uv2, 8) - 0.5) * disp;
    layer(0.1, vec3(0.88, 0.38, 0.24));
    layer(0.05, vec3(0.98, 0.42, 0.28));
    layer(0., vec3(1.0, 0.48, 0.35));

    // c6
    midlevel = 0.6;
    disp = 2.0;
    dist = 40.0;
    uv2 = uv + vec2(t / dist + 18.0, 0.0);
    h = (fbm(uv2, 8) - 0.5) * disp;
    layer(0.1, vec3(0.95, 0.66, 0.48));
    layer(0., vec3(1.0, 0.76, 0.60));

    // c5
    midlevel = 0.75;
    disp = 3.5;
    dist = 45.0;
    uv2 = uv + vec2(t / dist + 15.5, 0.0);
    h = (fbm(uv2, 8) - 0.5) * disp;
    layer(0.2, vec3(1.0, 0.55, 0.33));
    layer(0.15, vec3(0.98, 0.50, 0.24));
    layer(0.1, vec3(0.90, 0.55, 0.40));
    layer(0., vec3(1.0, 0.62, 0.44));

    // c4
    midlevel = 0.7;
    disp = 2.7;
    dist = 50.0;
    uv2 = uv + vec2(t / dist + 12.0, 0.0);
    h = (fbm(uv2, 8) - 0.5) * disp;
    layer(0.04, vec3(0.73, 0.36, 0.30));
    layer(0., vec3(0.80, 0.40, 0.34));

    // c3
    midlevel = 0.8;
    disp = 2.7;
    dist = 60.0;
    uv2 = uv + vec2(t / dist + 9.5, 0.0);
    h = (fbm(uv2, 8) - 0.5) * disp;
    layer(0.1, vec3(0.93, 0.58, 0.35));
    layer(0., vec3(1.0, 0.76, 0.60));

    // c2
    midlevel = 0.9;
    disp = 3.0;
    dist = 70.0;
    uv2 = uv + vec2(t / dist + 7.0, 0.0);
    h = (fbm(uv2, 8) - 0.5) * disp;
    layer(0.1, vec3(0.56, 0.25, 0.22));
    layer(0.05, vec3(0.60, 0.30, 0.27));
    layer(0., vec3(0.74, 0.35, 0.30));

    // c1
    midlevel = 1.0;
    disp = 5.0;
    dist = 100.0;
    uv2 = uv + vec2(t / dist + 3.5, 0.0);
    h = (fbm(uv2, 8) - 0.5) * disp;
    layer(0.1, vec3(0.92, 0.85, 0.82));
    layer(0., vec3(1.0, 0.94, 0.91));

    return vec4(0.58, 0.7, 1.0, 1.);
}

void main()
{
    vec2 uv = fragCoord * vec2(ubo.width / ubo.height, 1.0);
    // uv.x += iTime;
    float t = time.value * 4.0;
    vec4 bg = background(uv, t);

    vec4 fg = vec4(0.);
    int n = 5;
    if (uv.y < 1.0)
        for (int i = 0; i < n; i++)
        {
            fg += foreground(uv, t + 4. * float(i) / float(n) / 60.) / (float(n));
        }

    vec3 col = bg.rgb;
    // train /////////////////////////////////////////////////////////////////////
    float k;
    float midlevel;
    float h;
    float disp;
    float dist;
    vec2 uv2;
    uv.y -= 0.2;
    // choo choo
    k = 1.0;
    uv2 = fract(uv * 9.0);
    float wagon = 1.0;
    wagon *= 1.0 - step(0.45, uv.x);
    wagon *= 1.0 - step(0.115, uv.y);
    wagon *= step(0.103, uv.y);
    wagon *= step(0.05, 1.0 - abs(uv2.x * 2.0 - 1.0));

    float join = 1.0;
    join *= 1.0 - step(0.45, uv.x);
    join *= 1.0 - step(0.11, uv.y);
    join *= step(0.107, uv.y);

    float roof = 1.0;
    roof *= 1.0 - step(0.45, uv.x);
    roof *= 1.0 - step(0.117, uv.y);
    roof *= step(0.11, uv.y);
    roof *= step(0.15, 1.0 - abs(uv2.x * 2.0 - 1.0));

    float loco = box(uv, 0.45, 0.5, 0.103, 0.112);
    float chem1 = box(uv, 0.49, 0.495, 0.103, 0.12);
    float chem2 = box(uv, 0.488, 0.496, 0.12, 0.123);
    float locoRoof = box(uv, 0.443, 0.47, 0.11, 0.117);

    float wheel = 1.0 - step(0.00004, dot2(uv - vec2(0.457, 0.106)));
    wheel += 1.0 - step(0.00002, dot2(uv - vec2(0.487, 0.105)));
    wheel += 1.0 - step(0.00002, dot2(uv - vec2(0.497, 0.105)));

    if (uv.x < 0.45 && uv.y > 0.025 && uv.y < 0.2)
    {
        wheel += 1.0 - step(0.002, dot2(uv2 - vec2(0.2, 0.95)));
        wheel += 1.0 - step(0.002, dot2(uv2 - vec2(0.8, 0.95)));
    }
    col = mix(col, vec3(0.18, 0.12, 0.15), join);
    col = mix(col, vec3(0.48, 0.19, 0.20), wagon);
    col = mix(col, vec3(0.18, 0.12, 0.15), roof);

    col = mix(col, vec3(0.38, 0.19, 0.20), loco);
    col = mix(col, vec3(0.38, 0.19, 0.20), chem1);
    col = mix(col, vec3(0.18, 0.12, 0.15), locoRoof);
    col = mix(col, vec3(0.18, 0.12, 0.15), chem2 + wheel);
    // loco smoke //////

    dist = 5.0;
    uv2 = uv + vec2(t / dist + 3.5, 0.0);
    uv2.x -= t / dist * 0.2;
    h = fbm2(uv2, 8) - 0.55;

    if (uv.x < 0.49)
    {
        float x = -uv.x + 0.49;
        float y = abs(uv.y + h * 0.4 - 0.16 * sqrt(x) - 0.12) - 0.8 * x * exp(-x * 10.0);
        if (y < 0.0)
            col = vec3(1.0, 0.94, 0.91);
        if (y < -0.02)
            col = vec3(0.92, 0.85, 0.82);
    }

    // bridge ///////
    dist = 5.0;
    uv2 = uv + vec2(t / dist + 32.5, 0.0);
    uv2.x = fract(uv2.x * 3.0);
    k = 1.0;
    k *= smoothstep(0.001, 0.003, abs(uv2.y - pow(uv2.x - 0.5, 2.0) * 0.15 - 0.12));
    k *= min(step(0.05, 1.0 - abs(uv2.x * 2.0 - 1.0)) + step(0.17, uv2.y), 1.0);
    k *= min(smoothstep(0.02, 0.05, 1.0 - abs(uv2.x * 2.0 - 1.0)) + step(0.177, uv2.y), 1.0);

    k *= min(step(0.1, uv2.y) + smoothstep(-0.09, -0.085, -uv2.y - 0.001 / (1.0 - abs(uv2.x * 2.0 - 1.0))), 1.0);

    k *= min(smoothstep(0.05, 0.2, 1.0 - abs(fract(uv2.x * 16.0) * 2.0 - 1.0)) +
                 step(0.12, uv2.y - pow(uv2.x - 0.5, 2.0) * 0.15) + step(-0.1, -uv2.y),
             1.0);
    col = mix(vec3(0.29, 0.09, 0.08) * smoothstep(-0.08, 0.08, uv.y), col, k);

    col = mix(col, fg.rgb, fg.a);

    // Output to screen
    uv = fragCoord;
    // col = mix(col, texture(iChannel1, uv).rgb, 0.3);
    // col *= 0.5 + 0.5 * pow(16.0 * uv.x * uv.y * (1.0 - uv.x) * (1.0 - uv.y), 0.2);
    fragColor = vec4(col, 1.0);
}

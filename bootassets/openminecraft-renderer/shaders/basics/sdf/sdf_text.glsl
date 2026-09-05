#ifndef SDF_TEXT_GLSL
#define SDF_TEXT_GLSL

const float INF = 1.0 / 0.0;
const float EPSILON = 1e-6;
const float DERIVATIVE_THRESHOLD = 1e-12;
const int ITERATION = 3;

float sdf_distanceToLineSegment(vec2 p, vec2 a, vec2 b)
{
    vec2 ab = b - a;
    vec2 ap = p - a;
    float len2 = dot(ab, ab);
    if (len2 < EPSILON)
        return length(ap);
    float t = clamp(dot(ap, ab) / len2, 0.0, 1.0);
    return length(p - (a + t * ab));
}

float sdf_distanceToQuadraticBezier(vec2 p, vec2 p0, vec2 p1, vec2 p2)
{
    vec2 a = p1 - p0;
    vec2 b = p2 - 2.0 * p1 + p0;
    vec2 c = p0 - p;

    float t = clamp(dot(p - p0, p2 - p0) / max(dot(p2 - p0, p2 - p0), EPSILON), 0.0, 1.0);

    for (int i = 0; i < ITERATION; ++i)
    {
        vec2 pt = mix(mix(p0, p1, t), mix(p1, p2, t), t);
        vec2 derivative = 2.0 * mix(p1 - p0, p2 - p1, t);
        float f = dot(pt - p, derivative);
        float fPrime = dot(derivative, derivative) + dot(pt - p, 2.0 * (p2 - 2.0 * p1 + p0));
        if (abs(fPrime) < EPSILON)
            break;
        t -= f / fPrime;
        t = clamp(t, 0.0, 1.0);
    }

    vec2 closest = mix(mix(p0, p1, t), mix(p1, p2, t), t);
    return length(p - closest);
}

int sdf_intersectLineY(float rayY, vec2 a, vec2 b, out float t)
{
    t = -1.0;
    float dy = b.y - a.y;
    if (abs(dy) < EPSILON)
        return 0;
    t = (rayY - a.y) / dy;
    return (t > 0.0 && t <= 1.0) ? 1 : 0;
}

int sdf_intersectQuadraticY(float rayY, vec2 p0, vec2 p1, vec2 p2, out float t1, out float t2)
{
    float a = p0.y - 2.0 * p1.y + p2.y;
    float b = 2.0 * (p1.y - p0.y);
    float c = p0.y - rayY;
    t1 = t2 = -1.0;

    if (abs(a) < EPSILON)
    {
        if (abs(b) < EPSILON)
            return 0;
        float t = -c / b;
        if (t >= 0.0 && t <= 1.0)
        {
            t1 = t;
            return 1;
        }
        return 0;
    }

    float disc = b * b - 4.0 * a * c;
    if (disc < 0.0)
        return 0;

    float sqrtD = sqrt(disc);
    float inv2a = 0.5 / a;
    float r1 = (-b - sqrtD) * inv2a;
    float r2 = (-b + sqrtD) * inv2a;

    if (r1 > r2)
    {
        float tmp = r1;
        r1 = r2;
        r2 = tmp;
    }

    int count = 0;
    if (r1 >= 0.0 && r1 <= 1.0)
    {
        t1 = r1;
        count = 1;
    }
    if (r2 >= 0.0 && r2 <= 1.0 && (count == 0 || r2 - r1 > EPSILON))
    {
        if (count == 0)
            t1 = r2;
        else
            t2 = r2;
        count++;
    }
    return count;
}

float sdf_evalQuadraticX(float t, vec2 p0, vec2 p1, vec2 p2)
{
    float mt = 1.0 - t;
    return mt * mt * p0.x + 2.0 * mt * t * p1.x + t * t * p2.x;
}

float sdf_derivativeQuadraticY(float t, vec2 p0, vec2 p1, vec2 p2)
{
    return 2.0 * mix(p1.y - p0.y, p2.y - p1.y, t);
}

vec2 sdf_cubicBezier(vec2 p0, vec2 p1, vec2 p2, vec2 p3, float t)
{
    float u = 1.0 - t;
    return u * u * u * p0 + 3.0 * u * u * t * p1 + 3.0 * u * t * t * p2 + t * t * t * p3;
}

#define SEGS 32
#define STEPS 32
float sdf_distanceToCubicBezier(vec2 p, vec2 p0, vec2 p1, vec2 p2, vec2 p3)
{
    float d = 1e10;
    vec2 prev = p0;
    for (int i = 1; i <= STEPS; ++i)
    {
        float t = float(i) / float(STEPS);
        vec2 current = sdf_cubicBezier(p0, p1, p2, p3, t);
        d = min(d, sdf_distanceToLineSegment(p, prev, current));
        prev = current;
    }
    return d;
}

#define ARC_SEGMENTS 32
float sdf_distanceToArc(vec2 p, vec2 start, vec2 end, float rx, float ry, float xrotDeg, bool largeArc, bool sweep)
{
    if (rx < 1e-6 || ry < 1e-6)
    {
        return sdf_distanceToLineSegment(p, start, end);
    }

    float phi = radians(xrotDeg);
    float cosPhi = cos(phi), sinPhi = sin(phi);

    vec2 mid = (start - end) * 0.5;
    float x1p = cosPhi * mid.x + sinPhi * mid.y;
    float y1p = -sinPhi * mid.x + cosPhi * mid.y;

    float lambda = (x1p * x1p) / (rx * rx) + (y1p * y1p) / (ry * ry);
    if (lambda > 1.0)
    {
        float s = sqrt(lambda);
        rx *= s;
        ry *= s;
    }

    float rx2 = rx * rx, ry2 = ry * ry;
    float den = rx2 * y1p * y1p + ry2 * x1p * x1p;
    float num = rx2 * ry2 - rx2 * y1p * y1p - ry2 * x1p * x1p;
    float coef = (largeArc != sweep) ? 1.0 : -1.0;
    coef *= sqrt(max(0.0, num / max(den, 1e-12)));

    float cxp = coef * (rx * y1p / ry);
    float cyp = -coef * (ry * x1p / rx);

    float cx = cosPhi * cxp - sinPhi * cyp + (start.x + end.x) * 0.5;
    float cy = sinPhi * cxp + cosPhi * cyp + (start.y + end.y) * 0.5;

    float theta1 = atan((y1p - cyp) / ry, (x1p - cxp) / rx);
    float theta2 = atan((-y1p - cyp) / ry, (-x1p - cxp) / rx);
    float delta = theta2 - theta1;

    if (!sweep && delta > 0.0)
        delta -= 2.0 * 3.14159265358979;
    else if (sweep && delta < 0.0)
        delta += 2.0 * 3.14159265358979;

    float d = 1e10;
    vec2 prev;
    vec2 first;
    for (int i = 0; i <= ARC_SEGMENTS; ++i)
    {
        float t = float(i) / float(ARC_SEGMENTS);
        float ang = theta1 + t * delta;

        float ex = cx + rx * cos(ang) * cosPhi - ry * sin(ang) * sinPhi;
        float ey = cy + rx * cos(ang) * sinPhi + ry * sin(ang) * cosPhi;
        vec2 cur = vec2(ex, ey);
        if (i == 0)
        {
            first = cur;
        }
        else
        {
            d = min(d, sdf_distanceToLineSegment(p, prev, cur));
        }
        prev = cur;
    }
    return d;
}

int sdf_windingCubic(vec2 p, vec2 p0, vec2 p1, vec2 p2, vec2 p3, int winding)
{
    vec2 prev = p0;
    for (int i = 1; i <= SEGS; ++i)
    {
        float t = float(i) / float(SEGS);
        vec2 cur = sdf_cubicBezier(p0, p1, p2, p3, t);
        float tInter;
        if (sdf_intersectLineY(p.y, prev, cur, tInter) > 0)
        {
            float xInter = prev.x + tInter * (cur.x - prev.x);
            if (xInter < p.x)
            {
                float dy = cur.y - prev.y;
                if (abs(dy) > 1e-5)
                {
                    winding += (dy > 0.0) ? 1 : -1;
                }
            }
        }
        prev = cur;
    }
    return winding;
}

int sdf_windingArc(vec2 p, vec2 start, vec2 end, float rx, float ry, float xrotDeg, bool largeArc, bool sweep,
                   int winding)
{
    if (rx < 1e-6 || ry < 1e-6)
    {
        float t;
        if (sdf_intersectLineY(p.y, start, end, t) > 0)
        {
            float xInter = start.x + t * (end.x - start.x);
            if (xInter < p.x)
            {
                float dy = end.y - start.y;
                if (abs(dy) > 1e-5)
                    winding += (dy > 0.0) ? 1 : -1;
            }
        }
        return winding;
    }

    float phi = radians(xrotDeg);
    float cosPhi = cos(phi), sinPhi = sin(phi);

    vec2 mid = (start - end) * 0.5;
    float x1p = cosPhi * mid.x + sinPhi * mid.y;
    float y1p = -sinPhi * mid.x + cosPhi * mid.y;

    float lambda = (x1p * x1p) / (rx * rx) + (y1p * y1p) / (ry * ry);
    if (lambda > 1.0)
    {
        float s = sqrt(lambda);
        rx *= s;
        ry *= s;
    }

    float rx2 = rx * rx, ry2 = ry * ry;
    float den = rx2 * y1p * y1p + ry2 * x1p * x1p;
    float num = rx2 * ry2 - rx2 * y1p * y1p - ry2 * x1p * x1p;
    float coef = (largeArc != sweep) ? 1.0 : -1.0;
    coef *= sqrt(max(0.0, num / max(den, 1e-12)));

    float cxp = coef * (rx * y1p / ry);
    float cyp = -coef * (ry * x1p / rx);

    float cx = cosPhi * cxp - sinPhi * cyp + (start.x + end.x) * 0.5;
    float cy = sinPhi * cxp + cosPhi * cyp + (start.y + end.y) * 0.5;

    float theta1 = atan((y1p - cyp) / ry, (x1p - cxp) / rx);
    float theta2 = atan((-y1p - cyp) / ry, (-x1p - cxp) / rx);
    float delta = theta2 - theta1;

    if (!sweep && delta > 0.0)
        delta -= 2.0 * 3.14159265358979;
    else if (sweep && delta < 0.0)
        delta += 2.0 * 3.14159265358979;

    vec2 prev = vec2(cx + rx * cos(theta1) * cosPhi - ry * sin(theta1) * sinPhi,
                     cy + rx * cos(theta1) * sinPhi + ry * sin(theta1) * cosPhi);

    for (int i = 1; i <= ARC_SEGMENTS; ++i)
    {
        float t = float(i) / float(ARC_SEGMENTS);
        float ang = theta1 + t * delta;
        vec2 cur = vec2(cx + rx * cos(ang) * cosPhi - ry * sin(ang) * sinPhi,
                        cy + rx * cos(ang) * sinPhi + ry * sin(ang) * cosPhi);

        float tInter;
        if (sdf_intersectLineY(p.y, prev, cur, tInter) > 0)
        {
            float xInter = prev.x + tInter * (cur.x - prev.x);
            if (xInter < p.x)
            {
                float dy = cur.y - prev.y;
                if (abs(dy) > 1e-5)
                    winding += (dy > 0.0) ? 1 : -1;
            }
        }
        prev = cur;
    }

    return winding;
}

#endif

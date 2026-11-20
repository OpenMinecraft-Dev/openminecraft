#ifndef TEXELBUF_GLSL
#define TEXELBUF_GLSL

float texelFetchF(samplerBuffer b, int i)
{
    return texelFetch(b, i).r;
}

#endif

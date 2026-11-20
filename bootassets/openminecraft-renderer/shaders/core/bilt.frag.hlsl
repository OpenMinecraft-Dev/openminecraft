Texture2D inTexture : register(t0);
SamplerState inTextureSampler : register(s0);

float4 main(float2 biltTexCoord : TEXCOORD0) : SV_Target {
  return inTexture.Sample(inTextureSampler, biltTexCoord);
}
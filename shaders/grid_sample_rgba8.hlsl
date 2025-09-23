Texture2D    uTex  : register(t0);
SamplerState uSamp : register(s0);

float4 main(float4 pos:SV_Position, float2 uv:TEXCOORD0) : SV_Target
{
    return uTex.Sample(uSamp, uv);
}

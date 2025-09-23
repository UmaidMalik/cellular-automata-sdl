struct VSOut
{ 
    float4 pos: SV_Position;
    float2 uv: TEXCOORD0;
};

VSOut main(uint vid : SV_VertexID)
{
    float2 pos[3] = { float2(-1,-1), float2(3,-1), float2(-1,3) };
    float2 uv [3] = { float2(0,0),   float2(2,0),  float2(0,2)  };
    VSOut o;
    o.pos = float4(pos[vid], 0,1);
    o.uv = uv[vid];
    return o;
}

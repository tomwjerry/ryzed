struct PS_INPUT
{
    float3 fNormal : NORMAL;
    float2 fTexUV : TEXCOORD0;
    int3 fTileIndexes : TILEINDEXES;
    float2 ftile0_uv : TEXCOORD1;
    float2 ftile1_uv : TEXCOORD2;
    float2 ftile2_uv : TEXCOORD3;
};

Texture2D tile0 : register(t0);
Texture2D tile1 : register(t1);
Texture2D tile2 : register(t2);
SamplerState tileSampler : register(s0);

float4 main(PS_INPUT input) : COLOR
{
    float3 uv0 = float3(input.tile0_uv, float(input.tileIndices.x));
    float4 finalColor = tile0.Sample(tileSampler, uv0);
    
    // --- LAYER 1 ---
    if (input.tileIndices.y != EMPTY_TILE) 
    {
        float3 uv1 = float3(input.tile1_uv, float(input.tileIndices.y));
        float4 color1 = tile1.Sample(tileSampler, uv1);
        // lerp maps seamlessly to GLSL's 'mix' function
        finalColor = lerp(finalColor, color1, color1.a);
    }
    
    // --- LAYER 2 ---
    if (input.tileIndices.z != EMPTY_TILE) 
    {
        float3 uv2 = float3(input.tile2_uv, float(input.tileIndices.z));
        float4 color2 = tile2.Sample(tileSampler, uv2);
        finalColor = lerp(finalColor, color2, color2.a);
    }
    
    // --- LIGHTING ---
    // Simple global directional light direction
    float3 lightDir = normalize(float3(0.5f, 1.0f, 0.3f));
    float ndotl = max(dot(normalize(input.normal), lightDir), 0.2f); // 0.2f ambient floor
    
    return float4(finalColor.rgb * ndotl, 1.0f);
}

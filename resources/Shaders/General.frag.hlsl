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
SamplerState tileSampler0 : register(s0);
SamplerState tileSampler1 : register(s1);
SamplerState tileSampler2 : register(s2);

static const uint EMPTY_TILE = 4095; // Matches NL_TILE_ELM_LAYER_EMPTY

float4 main(PS_INPUT input) : SV_TARGET
{
    float3 uv0 = float3(input.ftile0_uv, float(input.fTileIndexes.x));
    float4 finalColor = tile0.Sample(tileSampler0, uv0.xy);
    
    // --- LAYER 1 ---
    if (input.fTileIndexes.y != EMPTY_TILE) 
    {
        float3 uv1 = float3(input.ftile1_uv, float(input.fTileIndexes.y));
        float4 color1 = tile1.Sample(tileSampler1, uv1.xy);
        // lerp maps seamlessly to GLSL's 'mix' function
        finalColor = lerp(finalColor, color1, color1.a);
    }
    
    // --- LAYER 2 ---
    if (input.fTileIndexes.z != EMPTY_TILE) 
    {
        float3 uv2 = float3(input.ftile2_uv, float(input.fTileIndexes.z));
        float4 color2 = tile2.Sample(tileSampler2, uv2.xy);
        finalColor = lerp(finalColor, color2, color2.a);
    }
    
    // --- LIGHTING ---
    // Simple global directional light direction
    float3 lightDir = normalize(float3(0.5f, 1.0f, 0.3f));
    float ndotl = max(dot(normalize(input.fNormal), lightDir), 0.2f); // 0.2f ambient floor
    
    return float4(finalColor.rgb * ndotl, 1.0f);
}

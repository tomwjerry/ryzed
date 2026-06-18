struct PS_INPUT
{
    float3 fNormal : NORMAL;
    int fTileLayer : TILELAYER;
    float2 fTexUV : TEXCOORD0;
    float2 ftile0_uv : TEXCOORD1;
    float2 ftile1_uv : TEXCOORD2;
    float2 ftile2_uv : TEXCOORD3;
};

Texture2DArray<float4> tileImages0 : register(t0, space2);
SamplerState tileImagesSampler0 : register(s0, space2);

static const uint EMPTY_TILE = 4095; // Matches NL_TILE_ELM_LAYER_EMPTY

float4 main(PS_INPUT input) : SV_Target0
{
    float4 finalColor = tileImages0.Sample(tileImagesSampler0, float3(input.fTexUV + input.ftile0_uv, float(input.fTileLayer)));
    
    // --- LIGHTING ---
    // Simple global directional light direction
    float3 lightDir = normalize(float3(0.5f, 1.0f, 0.3f));
    float ndotl = max(dot(normalize(input.fNormal), lightDir), 0.6f); // 0.2f ambient floor
    
    return float4(finalColor.rgb * ndotl, 1.0f);
}

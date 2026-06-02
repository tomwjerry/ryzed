struct VS_INPUT
{
    float3 vPosition : POSITION;
    float3 vNormal : NORMAL;
    float2 vTexUV : TEXCOORD0;
    int3 vTileIndexes : TILEINDEXES;
    float2 vtile0_uv : TEXCOORD1;
    float2 vtile1_uv : TEXCOORD2;
    float2 vtile2_uv : TEXCOORD3;
};

struct VS_OUTPUT
{
    float4 fPosition : SV_POSITION;
    float3 fNormal : NORMAL;
    float2 fTexUV : TEXCOORD0;
    int3 fTileIndexes : TILEINDEXES;
    float2 ftile0_uv : TEXCOORD1;
    float2 ftile1_uv : TEXCOORD2;
    float2 ftile2_uv : TEXCOORD3;
};

cbuffer VPM : register(b0) 
{
    float4x4 view;
    float4x4 proj;
    float4x4 model;
};

static const uint EMPTY_TILE = 255; // Matches NL_TILE_ELM_LAYER_EMPTY

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;
    output.fNormal = input.vNormal;
    output.fTexUV = input.vTexUV;
    output.fTileIndexes = input.vTileIndexes;
    output.ftile0_uv = input.vtile0_uv;
    output.ftile1_uv = input.vtile1_uv;
    output.ftile2_uv = input.vtile2_uv;

    output.oPosition = mul(proj, view);
    output.oPosition = mul(output.oPosition, model);
    output.oPosition = mul(output.oPosition, float4(input.vPosition, 1.0));
    return output;
}

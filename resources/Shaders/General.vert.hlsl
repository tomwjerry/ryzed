struct VS_INPUT
{
    float3 vPosition : POSITION;
    float3 vNormal : NORMAL;
    float3 vTexUV : TEXCOORD0;
    int3 vTileIndexes : TILEINDEXES;
    float2 vtile0_uv : TEXCOORD1;
    float2 vtile1_uv : TEXCOORD2;
    float2 vtile2_uv : TEXCOORD3;
};

struct VS_OUTPUT
{
    float4 oPosition : SV_Position;
    float3 fNormal : NORMAL;
    float3 fTexUV : TEXCOORD0;
    int3 fTileIndexes : TILEINDEXES;
    float2 ftile0_uv : TEXCOORD1;
    float2 ftile1_uv : TEXCOORD2;
    float2 ftile2_uv : TEXCOORD3;
};

cbuffer CameraInfo : register(b0, space1) 
{
    float4x4 view;
    float4x4 proj;
};
cbuffer Model : register(b1, space1)
{
    float4x4 model;
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;
    output.fNormal = input.vNormal;
    output.fTexUV = input.vTexUV;
    output.fTileIndexes = input.vTileIndexes;
    output.ftile0_uv = input.vtile0_uv;
    output.ftile1_uv = input.vtile1_uv;
    output.ftile2_uv = input.vtile2_uv;

    output.oPosition = mul(proj, mul(view, mul(model, float4(input.vPosition, 1.0))));
    return output;
}

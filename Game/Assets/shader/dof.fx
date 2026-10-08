// スプライト共通（sprite.fx と同じ）
cbuffer cb : register(b0)
{
    float4x4 mvp;
    float4   mulColor;
};

// C++ の DofParam と同じ並び
cbuffer DofCb : register(b1)
{
    float focusDistance;   // ピントが合う距離
    float focusRange;      // ピントの合う幅（この外は全ボケ）
};

struct VSInput
{
    float4 pos : POSITION;
    float2 uv  : TEXCOORD0;
};

struct PSInput
{
    float4 pos : SV_POSITION;
    float2 uv  : TEXCOORD0;
};

Texture2D<float4> mainTexture : register(t0);   // くっきり版（w = カメラからの距離）
Texture2D<float4> bokeTexture : register(t1);   // ボケ版（DualBlur の結果）
sampler Sampler : register(s0);

PSInput VSMain(VSInput In)
{
    PSInput psIn;
    psIn.pos = mul(mvp, In.pos);
    psIn.uv  = In.uv;
    return psIn;
}

float4 PSMain(PSInput In) : SV_Target0
{
    float4 sharp = mainTexture.Sample(Sampler, In.uv);
    float3 boke  = bokeTexture.Sample(Sampler, In.uv).xyz;

    // ピントから離れるほど 1（全ボケ）に近づく
    float rate = saturate(abs(sharp.w - focusDistance) / focusRange);
    return float4(lerp(sharp.xyz, boke, rate), 1.0f);
}
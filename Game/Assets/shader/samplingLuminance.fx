// 輝度抽出用シェーダー
cbuffer cb : register(b0){
	float4x4 mvp;
	float4 mulColor;
};

struct VSInput{
	float4 pos : POSITION;
	float2 uv  : TEXCOORD0;
};

struct PSInput{
	float4 pos : SV_POSITION;
	float2 uv  : TEXCOORD0;
};

Texture2D<float4> mainTexture : register(t0);
sampler Sampler : register(s0);

PSInput VSMain(VSInput In)
{
	PSInput psIn;
	psIn.pos = mul(mvp, In.pos);
	psIn.uv = In.uv;
	return psIn;
}

float4 PSMain(PSInput In) : SV_Target0
{
	float4 color = mainTexture.Sample(Sampler, In.uv);
	float t = dot(color.xyz, float3(0.2125f, 0.7154f, 0.0721f));  // 輝度
	clip(t - 1.0f);   // 輝度 1.0 未満は捨てる（閾値は一旦固定）
	return color;
}
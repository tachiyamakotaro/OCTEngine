// デュアルブラー：ダウンサンプル用シェーダー
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

Texture2D<float4> mainTexture : register(t0);   // 入力（前の段の RT）
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
	// 入力テクスチャの半ピクセル幅
	float2 size;
	mainTexture.GetDimensions(size.x, size.y);
	float2 o = 0.5f / size;

	// 5タップ：中心を重く、四隅を混ぜながら縮小
	float4 sum = mainTexture.Sample(Sampler, In.uv) * 4.0f;
	sum += mainTexture.Sample(Sampler, In.uv + float2(-o.x, -o.y));
	sum += mainTexture.Sample(Sampler, In.uv + float2( o.x, -o.y));
	sum += mainTexture.Sample(Sampler, In.uv + float2(-o.x,  o.y));
	sum += mainTexture.Sample(Sampler, In.uv + float2( o.x,  o.y));
	return sum / 8.0f;
}
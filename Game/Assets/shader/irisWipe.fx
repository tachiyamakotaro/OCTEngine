/*!
 * @brief 丸く閉じるフェード（アイリスアウト）
 *
 * 画面いっぱいの黒い画像（Assets/sprite/fade.dds）を手前に置き、丸い穴をあける。
 * 穴の外は黒、穴の中は透明。C++ が穴の半径をだんだん小さくすると、黒い所が丸く閉じていく。
 * C++ 側は core::IrisWipe（b1 の IrisValues）。
 */

// スプライト共通（sprite.fx と同じ）
cbuffer cb : register(b0)
{
    float4x4 mvp;
    float4   mulColor;
};

// C++ の IrisWipe::IrisValues と同じ並び
cbuffer IrisCb : register(b1)
{
    float4 circle;   // xy = 穴の中心（ピクセル。画面の左上が 0）, z = 穴の半径（ピクセル）, w = 縁のぼかし（ピクセル）
    float4 screen;   // xy = 画面の大きさ（ピクセル）
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

Texture2D<float4> colorTexture : register(t0);
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
    float4 color = colorTexture.Sample(Sampler, In.uv) * mulColor;
    // 画面いっぱいの画像なので、UV に画面の大きさをかけるとピクセルの位置になる
    float2 pixel = In.uv * screen.xy;
    float distanceFromCenter = length(pixel - circle.xy);
    // 穴の外は 1（黒）、穴の中は 0（透明）。縁だけ少しぼかす
    float outside = smoothstep(circle.z - circle.w, circle.z, distanceFromCenter);
    color.a *= outside;
    return color;
}

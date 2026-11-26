/*!
 * @brief 縁付きの画像（メニューの文字など）
 *
 * 画像の透明でない所のまわりに、暗い縁を付ける。明るい所や込み入った 3D の背景の上でも文字を読みやすくするため。
 * ・C++ 側（SpriteRender::InitWithOutline）が、縁の太さの分だけ板を広げて描く。
 *   広げた板の UV を、画像の UV に直してから読む（画像の外は透明として扱う）。
 * ・そのピクセルのまわり（円の上の点、2重）で一番濃い透明度を「縁の濃さ」にする。
 * ・画像の色（乗算カラーをかけた物）を、縁の上に重ねる。乗算カラーの透明度は縁にもかける（点滅・フェードで一緒に消える）。
 */

// スプライト共通（sprite.fx と同じ）
cbuffer cb : register(b0)
{
    float4x4 mvp;
    float4   mulColor;
};

// C++ の SpriteRender::OutlineValues と同じ並び
cbuffer OutlineCb : register(b1)
{
    float4 outlineColor;    // rgb = 縁の色, a = 縁の濃さ
    float4 outlineParam;    // xy = 縁の太さ（画像の UV）, zw = 板を広げた倍率
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

/** 縁を探す向き（円を 16 等分） */
static const int DIRECTION_COUNT = 16;
static const float2 DIRECTIONS[DIRECTION_COUNT] =
{
    float2( 1.000f,  0.000f), float2( 0.924f,  0.383f), float2( 0.707f,  0.707f), float2( 0.383f,  0.924f),
    float2( 0.000f,  1.000f), float2(-0.383f,  0.924f), float2(-0.707f,  0.707f), float2(-0.924f,  0.383f),
    float2(-1.000f,  0.000f), float2(-0.924f, -0.383f), float2(-0.707f, -0.707f), float2(-0.383f, -0.924f),
    float2( 0.000f, -1.000f), float2( 0.383f, -0.924f), float2( 0.707f, -0.707f), float2( 0.924f, -0.383f),
};


/** 画像の中なら 1、外なら 0 */
float InsideImage(float2 uv)
{
    float2 inside = step(0.0f, uv) * step(uv, 1.0f);
    return inside.x * inside.y;
}

/** 画像の透明度（画像の外は 0） */
float SampleAlpha(float2 uv)
{
    return colorTexture.Sample(Sampler, saturate(uv)).a * InsideImage(uv);
}


PSInput VSMain(VSInput In)
{
    PSInput psIn;
    psIn.pos = mul(mvp, In.pos);
    psIn.uv = In.uv;
    return psIn;
}


float4 PSMain(PSInput In) : SV_Target0
{
    // 広げた板の UV → 画像の UV（板の真ん中を中心に、広げた倍率をかける）
    float2 uv = (In.uv - 0.5f) * outlineParam.zw + 0.5f;

    // 画像の色
    float4 fill = colorTexture.Sample(Sampler, saturate(uv)) * mulColor;
    fill.a *= InsideImage(uv);

    // 縁：まわりで一番濃い透明度（外側の円と、細い線のために半分の円の 2 重で見る）
    float around = 0.0f;
    [unroll]
    for (int i = 0; i < DIRECTION_COUNT; ++i)
    {
        float2 offset = DIRECTIONS[i] * outlineParam.xy;
        around = max(around, SampleAlpha(uv + offset));
        around = max(around, SampleAlpha(uv + offset * 0.5f));
    }
    float edge = saturate(around) * outlineColor.a * mulColor.a;

    // 画像の色を縁の上に重ねる（透明度は掛けていない色どうしで重ねる）
    float alpha = fill.a + edge * (1.0f - fill.a);
    float3 rgb = (fill.rgb * fill.a + outlineColor.rgb * edge * (1.0f - fill.a)) / max(alpha, 0.0001f);
    return float4(rgb, alpha);
}

/*!
 * @brief   アウトライン（黒い縁）
 *
 * mainRT のα には、model.fx が「カメラからの距離」を入れている（dof.fx と同じ使い方）。
 * まわりのピクセルに、自分より depthThreshold 以上「奥」のピクセルがあれば、
 * 自分は手前の物の縁なので線の色にする（線は手前の物の内側に引かれる）。
 *
 * C++ 側は RenderingEngine::OutlineParam（b1）。
 */

// スプライト共通（sprite.fx と同じ）
cbuffer cb : register(b0)
{
    float4x4 mvp;
    float4   mulColor;
};

// C++ の RenderingEngine::OutlineParam と同じ並び
cbuffer OutlineCb : register(b1)
{
    float4 outlineColor;     // rgb = 線の色, a = 線の濃さ
    float  depthThreshold;   // 奥との距離の差がこれ以上なら線（ワールド単位）
    float  thickness;        // 線の太さ（ピクセル）
    float2 screenSize;       // 画面の大きさ（ピクセル）
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

Texture2D<float4> sceneTexture : register(t0);   // mainRT（rgb = 色, a = カメラからの距離）
sampler Sampler : register(s0);

// 何も描いていない所の距離はとても大きい（クリアの値）。差を計算しても壊れないように上限を付ける
static const float MAX_DISTANCE = 60000.0f;

PSInput VSMain(VSInput In)
{
    PSInput psIn;
    psIn.pos = mul(mvp, In.pos);
    psIn.uv  = In.uv;
    return psIn;
}

float4 PSMain(PSInput In) : SV_Target0
{
    float4 center = sceneTexture.Sample(Sampler, In.uv);
    float  centerDistance = min(center.a, MAX_DISTANCE);

    // まわり8方向を、線の太さだけ離れた所で調べる
    static const float2 DIRECTIONS[8] =
    {
        float2( 1.0f,  0.0f), float2(-1.0f,  0.0f), float2( 0.0f,  1.0f), float2( 0.0f, -1.0f),
        float2( 0.7071f,  0.7071f), float2(-0.7071f,  0.7071f), float2( 0.7071f, -0.7071f), float2(-0.7071f, -0.7071f),
    };
    float2 pixel = thickness / screenSize;

    // まわりのうち、いちばん奥にある所との距離の差
    float maxDiff = 0.0f;
    [unroll]
    for (int i = 0; i < 8; i++)
    {
        float neighbor = min(sceneTexture.Sample(Sampler, In.uv + DIRECTIONS[i] * pixel).a, MAX_DISTANCE);
        maxDiff = max(maxDiff, neighbor - centerDistance);
    }

    // しきい値を越えたら線（少しだけ幅を持たせて、線の端のガタガタを減らす）
    float edge = smoothstep(depthThreshold, depthThreshold * 1.5f, maxDiff);
    float3 color = lerp(center.rgb, outlineColor.rgb, edge * outlineColor.a);

    // α（カメラからの距離）はそのまま残す（このあとのブルームや被写界深度で使う）
    return float4(color, center.a);
}

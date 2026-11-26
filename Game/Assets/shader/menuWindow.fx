/*!
 * @brief メニューのウィンドウ（角の丸い板）
 *
 * 画像を使わず、シェーダーで角の丸い四角を描く（どんな大きさにしても角や枠の太さが崩れない）。
 * 重なり（下から）
 *   影       … 少し下にずらして、ぼかした黒
 *   地       … 上から下へ少し暗くなる、暗い石の色（半透明）
 *   見出しの帯 … 上の端から帯の高さまで。下に金の細い線
 *   内側の線 … 枠の少し内側に、金の細い線（遺跡の飾り）
 *   枠       … 金の線
 * C++ 側は ui::MenuWindow（b1 の WindowValues）。板は影の分だけウィンドウより大きい。
 */

// スプライト共通（sprite.fx と同じ）
cbuffer cb : register(b0)
{
    float4x4 mvp;
    float4   mulColor;
};

// C++ の MenuWindow::WindowValues と同じ並び
cbuffer WindowCb : register(b1)
{
    float4 size;         // xy = ウィンドウの大きさ, zw = 板の大きさ（ピクセル）
    float4 shape;        // x = 角の丸み, y = 枠の太さ, z = 見出しの帯の高さ（0 なら帯なし）, w = 影の広がり
    float4 fillTop;      // 地の色（上の端）
    float4 fillBottom;   // 地の色（下の端）
    float4 frameColor;   // 枠と線の色
    float4 headerColor;  // 見出しの帯の色
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

/** 内側の線と枠の間（ピクセル） */
static const float INNER_LINE_GAP = 6.0f;
/** 内側の線の濃さ（枠の色に対して） */
static const float INNER_LINE_STRENGTH = 0.45f;
/** 影の濃さと、下へずらす割合（影の広がりに対して） */
static const float SHADOW_ALPHA = 0.55f;
static const float SHADOW_OFFSET_RATE = 0.35f;


/**
 * @brief 角の丸い四角までの距離（中が負、外が正。ピクセル）
 * @param p 四角の真ん中からの位置
 * @param halfSize 四角の大きさの半分
 * @param radius 角の丸み
 */
float RoundBoxDistance(float2 p, float2 halfSize, float radius)
{
    float2 q = abs(p) - halfSize + radius;
    return length(max(q, 0.0f)) + min(max(q.x, q.y), 0.0f) - radius;
}

/** 線の濃さ（距離 0 の所が 1。太さは約 width ピクセル） */
float LineMask(float distance, float width)
{
    return 1.0f - smoothstep(width * 0.5f - 0.5f, width * 0.5f + 0.5f, abs(distance));
}

/** 透明度を掛けていない色どうしで、top を bottom の上に重ねる */
float4 Over(float4 top, float4 bottom)
{
    float alpha = top.a + bottom.a * (1.0f - top.a);
    float3 rgb = (top.rgb * top.a + bottom.rgb * bottom.a * (1.0f - top.a)) / max(alpha, 0.0001f);
    return float4(rgb, alpha);
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
    // 板の真ん中からの位置（ピクセル。UV は下が +V なので、下が +Y）
    float2 p = (In.uv - 0.5f) * size.zw;
    float2 halfSize = size.xy * 0.5f;
    float radius = shape.x;
    float frameWidth = shape.y;
    float headerHeight = shape.z;
    float shadowSpread = max(shape.w, 1.0f);

    float d = RoundBoxDistance(p, halfSize, radius);

    // ---- 影（少し下にずらして、ぼかす） ----
    float shadowD = RoundBoxDistance(p - float2(0.0f, shadowSpread * SHADOW_OFFSET_RATE), halfSize, radius);
    float shadow = (1.0f - smoothstep(-shadowSpread * 0.5f, shadowSpread, shadowD)) * SHADOW_ALPHA;
    float4 color = float4(0.0f, 0.0f, 0.0f, shadow);

    // ---- 地（上から下へ） ----
    float t = saturate((p.y + halfSize.y) / max(size.y, 1.0f));
    float4 body = lerp(fillTop, fillBottom, t);

    // ---- 見出しの帯と、その下の金の線 ----
    if (headerHeight > 0.0f)
    {
        float headerBottom = -halfSize.y + headerHeight;
        float inHeader = 1.0f - smoothstep(headerBottom - 0.5f, headerBottom + 0.5f, p.y);
        body = lerp(body, headerColor, inHeader);
        // 線は内側の線の内側だけ
        float lineRange = 1.0f - smoothstep(-0.5f, 0.5f, d + frameWidth + INNER_LINE_GAP);
        float headerLine = LineMask(p.y - headerBottom, 2.0f) * lineRange;
        body.rgb = lerp(body.rgb, frameColor.rgb, headerLine);
        body.a = max(body.a, headerLine * frameColor.a);
    }

    // ---- 内側の細い線（枠から少し内側） ----
    float innerLine = LineMask(d + frameWidth + INNER_LINE_GAP, 1.5f) * INNER_LINE_STRENGTH;
    body.rgb = lerp(body.rgb, frameColor.rgb, innerLine);
    body.a = max(body.a, innerLine * frameColor.a);

    // ---- 枠（外の端から frameWidth の内側まで） ----
    float frame = smoothstep(-frameWidth - 0.5f, -frameWidth + 0.5f, d);
    body = lerp(body, frameColor, frame);

    // ---- 四角の外は消す（端は 1 ピクセルなめらかに） ----
    body.a *= 1.0f - smoothstep(-0.5f, 0.5f, d);

    color = Over(body, color);
    color.rgb *= mulColor.rgb;
    color.a *= mulColor.a;
    return color;
}

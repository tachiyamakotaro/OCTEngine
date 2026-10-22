/*!
 * @brief 暗がり（DarkZone）の黒い半透明の板
 *
 * model.fx を include して、頂点シェーダーとライトの値はそのまま使う。
 * 暗がりの値は、model.fx の DirectionLightCb（b1）の最後に足す（MODEL_CB_EXTRA）。
 * C++ 側は DarknessRenderer::Constants（ライトの値の後ろに暗がりの値をつなげた構造体）。
 *
 * 明るい所と暗い所の境目は、ぼかしの幅で少しずつ暗くなる（くっきり分かれない）。
 * いくつもの明るい所が重なる所は「1 −（1 − 明るさ）の積」で合わせる（max で合わせると、重なりの所に筋が出る）。
 * C++ の DarkZone::Glow / HiddenFloor::Update も同じ計算をしているので、画面で明るく見える所の足場が見える。
 *
 * エントリーポイント
 *   PSMainDarkness : 黒く塗る。光の線分の近く・プレイヤーのまわり・見えた足場の上は透明にする
 */

// 配列の大きさ（C++ の DarknessRenderer::MAX_SEGMENTS / MAX_RECTS と同じにする）
#define DARK_MAX_SEGMENTS 48
#define DARK_MAX_RECTS 16

// model.fx の DirectionLightCb の最後に足す値（C++ の DarknessRenderer::Constants と同じ並び）
//   darkParam          : x = 暗さ, y = 光の半径（境目の真ん中）, z = 光の境目のぼかしの幅, w = 範囲の端のぼかし
//   darkView           : xyz = カメラの前の向き, w = 平行投影なら 1
//   darkPlayer         : xy = プレイヤーの体の中心, z = 線分の数, w = 足場の数
//   darkZone           : xy = 暗がりの左下, zw = 右上
//   darkColor          : rgb = 暗がりの色, a = 見えた足場のまわりのぼかしの幅（C++ の Config::rectSoftWidth）
//   darkPlayerLight    : x = プレイヤーのまわりの丸の半径（境目の真ん中）, y = 丸の境目のぼかしの幅, z = ステージ全体なら 1
//   darkSegments[]     : xy = 光の線分の始点, zw = 終点
//   darkRects[]        : xy = 見えた足場の左下, zw = 右上
//   darkRectVisibility[] : x = 見え具合
// （fxc は複数行のマクロの中のコメントをうまく読めないことがあるので、マクロは1行で書く）
#define MODEL_CB_EXTRA float4 darkParam; float4 darkView; float4 darkPlayer; float4 darkZone; float4 darkColor; float4 darkPlayerLight; float4 darkSegments[DARK_MAX_SEGMENTS]; float4 darkRects[DARK_MAX_RECTS]; float4 darkRectVisibility[DARK_MAX_RECTS];

#include "model.fx"


// 点と線分の距離
float DarkDistanceToSegment(float2 p, float2 a, float2 b)
{
    float2 ab = b - a;
    float  lengthSq = dot(ab, ab);
    float  t = lengthSq > 0.0f ? saturate(dot(p - a, ab) / lengthSq) : 0.0f;
    return length(p - (a + ab * t));
}


// 明るさ（1 = 明るい 〜 0 = 暗い）：境目の真ん中 radius から、内側と外側に softWidth の半分ずつ、なめらかに変わる
// （C++ の DarkZone::Glow と同じ計算）
float DarkGlow(float distance, float radius, float softWidth)
{
    if (radius <= 0.0f && softWidth <= 0.0f)
    {
        return 0.0f;
    }
    float half = max(softWidth, 0.0f) * 0.5f;
    if (half <= 0.0f)
    {
        return distance <= radius ? 1.0f : 0.0f;
    }
    return 1.0f - smoothstep(radius - half, radius + half, distance);
}


float4 PSMainDarkness(SPSIn In) : SV_Target0
{
    // 板は地形より手前（カメラ側）にあるので、カメラから見て z=0 の面（ギミックやプレイヤーがいる面）の
    // どこに重なっているかに直してから、光やプレイヤーとの距離を測る
    float3 viewDir = darkView.w > 0.5f ? darkView.xyz : (In.worldPos - eyePos);
    float2 p = In.worldPos.xy;
    if (abs(viewDir.z) > 0.0001f)
    {
        p = In.worldPos.xy - viewDir.xy * (In.worldPos.z / viewDir.z);
    }

    // 暗がりの範囲の端をぼかす（内側ほど正の値）。ステージ全体の暗がりなら、どこも暗がりの中
    float edge = 1.0f;
    if (darkPlayerLight.z < 0.5f)
    {
        float2 fromMin = p - darkZone.xy;
        float2 fromMax = darkZone.zw - p;
        float  inside = min(min(fromMin.x, fromMin.y), min(fromMax.x, fromMax.y));
        float  feather = darkParam.w;
        edge = feather > 0.0f ? smoothstep(-feather, feather, inside) : (inside >= 0.0f ? 1.0f : 0.0f);
        clip(edge - 0.001f);
    }

    // 暗さの残り（1 = まったく明るくない）。明るい所ごとに（1 − 明るさ）をかけていく
    // プレイヤーのまわりは、マリオの暗闇ステージのように、いつも見える丸にする
    float dark = 1.0f - DarkGlow(length(p - darkPlayer.xy), darkPlayerLight.x, darkPlayerLight.y);

    int segmentCount = (int)darkPlayer.z;
    [loop]
    for (int i = 0; i < DARK_MAX_SEGMENTS; i++)
    {
        if (i >= segmentCount)
        {
            break;
        }
        float d = DarkDistanceToSegment(p, darkSegments[i].xy, darkSegments[i].zw);
        dark *= 1.0f - DarkGlow(d, darkParam.y, darkParam.z);
    }

    // 見えた暗がりの足場の上（足場のまわりを rectMargin だけ広げた四角。その外側を rectSoftWidth でぼかす）
    int rectCount = (int)darkPlayer.w;
    [loop]
    for (int r = 0; r < DARK_MAX_RECTS; r++)
    {
        if (r >= rectCount)
        {
            break;
        }
        float2 outside = max(max(darkRects[r].xy - p, p - darkRects[r].zw), 0.0f);
        float  d = length(outside);
        float  soft = max(darkColor.a, 1.0f);
        dark *= 1.0f - darkRectVisibility[r].x * (1.0f - smoothstep(0.0f, soft, d));
    }

    float alpha = darkParam.x * saturate(dark) * edge;
    return float4(darkColor.rgb, alpha);
}

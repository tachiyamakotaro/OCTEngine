/*!
 * @brief 暗がり（DarkZone）の黒い半透明の板
 *
 * model.fx を include して、頂点シェーダーとライトの値はそのまま使う。
 * 暗がりの値は、model.fx の DirectionLightCb（b1）の最後に足す（MODEL_CB_EXTRA）。
 * C++ 側は DarknessRenderer::Constants（ライトの値の後ろに暗がりの値をつなげた構造体）。
 *
 * エントリーポイント
 *   PSMainDarkness : 黒く塗る。光の線分の近く・プレイヤーのまわり・見えた足場の上は透明にする
 */

// 配列の大きさ（C++ の DarknessRenderer::MAX_SEGMENTS / MAX_RECTS と同じにする）
#define DARK_MAX_SEGMENTS 48
#define DARK_MAX_RECTS 16

// model.fx の DirectionLightCb の最後に足す値（C++ の DarknessRenderer::Constants と同じ並び）
//   darkParam          : x = 暗さ, y = 光の半径, z = プレイヤーの光の半径, w = 範囲の端のぼかし
//   darkView           : xyz = カメラの前の向き, w = 平行投影なら 1
//   darkPlayer         : xy = プレイヤーの体の中心, z = 線分の数, w = 足場の数
//   darkZone           : xy = 暗がりの左下, zw = 右上
//   darkColor          : rgb = 暗がりの色, a = 明るい所の端のぼかし（半径に対する割合）
//   darkPlayerLight    : x = プレイヤーのまわりの丸の半径, y = 丸の端のぼかし（半径に対する割合）
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


// 半径 radius の中なら 1、外へ向かってなめらかに 0 になる（softness = 端のぼかしの割合）
float DarkGlowSoft(float distance, float radius, float softness)
{
    if (radius <= 0.0f)
    {
        return 0.0f;
    }
    float inner = radius * (1.0f - softness);
    if (inner >= radius)
    {
        return distance <= radius ? 1.0f : 0.0f;
    }
    return 1.0f - smoothstep(inner, radius, distance);
}


// 光の線分のまわりの明るさ
float DarkGlow(float distance, float radius)
{
    return DarkGlowSoft(distance, radius, darkColor.a);
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

    // 暗がりの範囲の端をぼかす（内側ほど正の値）
    float2 fromMin = p - darkZone.xy;
    float2 fromMax = darkZone.zw - p;
    float  inside = min(min(fromMin.x, fromMin.y), min(fromMax.x, fromMax.y));
    float  feather = darkParam.w;
    float  edge = feather > 0.0f ? smoothstep(-feather, feather, inside) : (inside >= 0.0f ? 1.0f : 0.0f);
    clip(edge - 0.001f);

    // 明るさ（プレイヤーのまわり・光の線分の近く・見えた足場の上のうち、一番明るいもの）
    // プレイヤーのまわりは、マリオの暗闇ステージのように、いつもはっきり見える丸にする
    float light = DarkGlowSoft(length(p - darkPlayer.xy), darkPlayerLight.x, darkPlayerLight.y);

    int segmentCount = (int)darkPlayer.z;
    [loop]
    for (int i = 0; i < DARK_MAX_SEGMENTS; i++)
    {
        if (i >= segmentCount)
        {
            break;
        }
        float d = DarkDistanceToSegment(p, darkSegments[i].xy, darkSegments[i].zw);
        light = max(light, DarkGlow(d, darkParam.y));
    }

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
        light = max(light, darkRectVisibility[r].x * (1.0f - smoothstep(0.0f, 20.0f, d)));
    }

    float alpha = darkParam.x * (1.0f - saturate(light)) * edge;
    return float4(darkColor.rgb, alpha);
}

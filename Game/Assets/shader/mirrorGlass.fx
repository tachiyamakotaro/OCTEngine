/*!
 * @brief 鏡の境目の巨大な半透明ガラス（MirrorGlass.cpp から使う）
 *
 * 本物の世界（手前）と鏡の中の世界（奥）の境目＝奥の鏡の面に置く、ステージ全体ほどの大きさの半透明の板。
 * 鏡の中のプレイヤーやステージが、この板ごしに少し色が付いて見えるので「鏡の向こう」に見える。
 *
 *  ・地の色      … 薄い色を付ける（tintColor）
 *  ・パネルの継ぎ目 … 大きな鏡を何枚も並べたような細い線（panel）
 *  ・光の筋      … 斜めの光の帯がゆっくり流れる（sheenColor / param.zw）
 *  ・縁の光      … 板の端（ステージの端）ほど明るく光る（edgeColor）
 *  ・上ほど明るい  … 上から光が差しているようなグラデーション
 *  ・フレネル    … 斜めから見るほど不透明になる
 *
 * 模様はワールド座標で計算するので、板をどれだけ大きくしても模様は伸びない。
 * 出力の rgb は色、a は不透明度（AlphaBlendMode_Trans で後ろの絵と混ぜる）。
 * ※ model.fx は a にカメラからの距離を書いている（被写界深度用）。被写界深度を ON にする場合は
 *   ガラスの所だけ距離が消えるので、README の「鏡のガラス」の注意を見ること。
 */

////////////////////////////////////////////////
// ピクセルシェーダーへの入力（ModelVSCommon.h より先に定義する）
////////////////////////////////////////////////
struct SPSIn
{
    float4 pos      : SV_POSITION;
    float3 normal   : NORMAL;
    float3 tangent  : TANGENT;
    float3 biNormal : BINORMAL;
    float2 uv       : TEXCOORD0;
    float3 worldPos : TEXCOORD1;
};

#include "ModelVSCommon.h"

////////////////////////////////////////////////
// C++ の MirrorGlass::Constants と同じ並び（全部 float4）
////////////////////////////////////////////////
cbuffer MirrorGlassCb : register(b1)
{
    float4 tintColor;    // rgb = 地の色, a = 地の不透明度
    float4 edgeColor;    // rgb = 縁の光の色（1 より大きいとブルームで光る）, a = 縁の光の幅（ワールド単位）
    float4 sheenColor;   // rgb = 光の筋の色, a = 光の筋の不透明度
    float4 rect;         // 板の範囲 (minX, minY, maxX, maxY)
    float4 eyePosTime;   // xyz = カメラの位置, w = 時間（秒）
    float4 param;        // x = フレネルの強さ, y = 上ほど明るくする量, z = 光の筋の間隔, w = 光の筋の速さ
    float4 panel;        // x = パネルの大きさ, y = 継ぎ目の太さ, z = 継ぎ目の不透明度, w = 全体の不透明度の倍率
};

Texture2D<float4> albedoTexture : register(t0);
sampler Sampler : register(s0);

////////////////////////////////////////////////
// 頂点シェーダー（ModelVSCommon.h の VSMain から呼ばれる）
////////////////////////////////////////////////
SPSIn VSMainCore(SVSIn vsIn, float4x4 mWorldLocal, uniform bool isUsePreComputedVertexBuffer)
{
    SPSIn psIn;
    psIn.pos = CalcVertexPositionInWorldSpace(vsIn.pos, mWorldLocal, isUsePreComputedVertexBuffer);
    psIn.worldPos = psIn.pos.xyz;
    psIn.pos = mul(mView, psIn.pos);
    psIn.pos = mul(mProj, psIn.pos);
    CalcVertexNormalTangentBiNormalInWorldSpace(
        psIn.normal, psIn.tangent, psIn.biNormal,
        mWorldLocal, vsIn.normal, vsIn.tangent, vsIn.biNormal,
        isUsePreComputedVertexBuffer);
    psIn.uv = vsIn.uv;
    return psIn;
}

////////////////////////////////////////////////
// 光の帯（0〜1）。center を中心に、width の幅でやわらかく光る
////////////////////////////////////////////////
float Band(float t, float center, float width)
{
    float d = abs(t - center);
    return saturate(1.0f - d / max(width, 0.0001f));
}

////////////////////////////////////////////////
// ピクセルシェーダー
////////////////////////////////////////////////
float4 PSMain(SPSIn In) : SV_Target0
{
    float2 p = In.worldPos.xy;
    float time = eyePosTime.w;

    // ---- 地の色（テクスチャを描き込めば模様にもできる）----
    float3 albedo = albedoTexture.Sample(Sampler, In.uv).rgb;
    float3 baseColor = tintColor.rgb * albedo;
    float baseAlpha = tintColor.a;

    // 上ほど明るく（上から光が差しているように）
    float height01 = saturate((p.y - rect.y) / max(rect.w - rect.y, 1.0f));
    baseColor *= 1.0f + param.y * height01;

    // ---- フレネル：斜めから見るほど不透明に ----
    float3 normal = normalize(In.normal);
    float3 toEye = normalize(eyePosTime.xyz - In.worldPos);
    float facing = saturate(abs(dot(normal, toEye)));
    float fresnel = pow(1.0f - facing, 2.0f) * param.x;

    // ---- パネルの継ぎ目：大きな鏡を並べたような細い線 ----
    float seam = 0.0f;
    if (panel.x > 0.0f)
    {
        float2 cell = frac((p - rect.xy) / panel.x);
        float2 toLine = min(cell, 1.0f - cell) * panel.x;   // 一番近い継ぎ目までの距離（ワールド単位）
        float lineDist = min(toLine.x, toLine.y);
        seam = saturate(1.0f - lineDist / max(panel.y, 0.0001f));
    }

    // ---- 光の筋：斜めの帯がゆっくり流れる（太い帯 + その横の細い帯）----
    float sheen = 0.0f;
    if (param.z > 0.0f)
    {
        float t = frac(dot(p, float2(0.8f, 0.6f)) / param.z - time * param.w);
        sheen = Band(t, 0.50f, 0.10f) * 0.8f + Band(t, 0.64f, 0.025f);
    }

    // ---- 縁の光：板の端（ステージの端）ほど明るい ----
    float edgeDist = min(min(p.x - rect.x, rect.z - p.x), min(p.y - rect.y, rect.w - p.y));
    float edge = saturate(1.0f - edgeDist / max(edgeColor.a, 0.0001f));
    edge *= edge;

    // ---- 合成：色 × 不透明度 を足していき、最後に不透明度で割る（後ろとの混ぜ方は Trans）----
    float3 premul = baseColor * baseAlpha;
    float alpha = baseAlpha;

    premul += float3(1.0f, 1.0f, 1.0f) * fresnel;
    alpha += fresnel;

    float seamAlpha = seam * panel.z;
    premul += sheenColor.rgb * seamAlpha;
    alpha += seamAlpha;

    float sheenAlpha = sheen * sheenColor.a;
    premul += sheenColor.rgb * sheenAlpha;
    alpha += sheenAlpha;

    premul += edgeColor.rgb * edge;
    alpha += edge * 0.35f;

    alpha = saturate(alpha * panel.w);
    float3 color = premul * panel.w / max(alpha, 0.001f);
    return float4(color, alpha);
}

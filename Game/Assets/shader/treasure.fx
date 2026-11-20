/*!
 * @brief 宝物の間の宝箱・金貨・光の宝石・まわりの光
 *
 * model.fx を include して、頂点シェーダーとライトの計算（CalcLitColor。シーンのポイントライトも入る）を使う。
 *
 * 値は model.fx の DirectionLightCb（b1）の最後に足す（MODEL_CB_EXTRA）。
 * C++ 側は TreasureRenderer::Constants（ライトの値の後ろにつなげた構造体）。
 *
 * エントリーポイント
 *   PSMainTreasure     … 宝箱・金貨（ライトで照らす + テクスチャの α の分だけ自分で光る。宝石が光るほど強く光る）
 *   PSMainTreasureGem  … 光の宝石（自分で光る。ふちほど明るい）
 *   PSMainTreasureRays … 宝石のまわりの光の筋（半透明。根元ほど濃い。筋ごとにゆらめく）
 *   PSMainTreasureHalo … 宝石の後ろの丸い光（半透明。真ん中ほど濃い）
 */

// model.fx の DirectionLightCb の最後に足す値（C++ の TreasureRenderer::ShaderValues と同じ並び）
//   trGemColor  : rgb = 宝石の色, a = 光る強さ
//   trRayColor  : rgb = 光の筋の色, a = 光る強さ
//   trParam     : x = 時間（秒）, y = 光の出具合（0〜1。宝石が浮かぶほど 1 に近づく）
// （fxc は複数行のマクロの中のコメントをうまく読めないことがあるので、マクロは1行で書く）
#define MODEL_CB_EXTRA float4 trGemColor; float4 trRayColor; float4 trParam;

#include "model.fx"


// 金（テクスチャの α が大きい所）が自分で光る強さ（光が出る前・出きったとき）
static const float TR_SHINE_BASE = 0.25f;
static const float TR_SHINE_LIT = 1.4f;
// 光の筋の数（treasure_rays.tkm と同じ。筋ごとのゆらめきをずらすのに使う）
static const float TR_RAY_COUNT = 14.0f;


float4 PSMainTreasure(SPSIn In) : SV_Target0
{
    float4 color = CalcLitColor(In, true);
    float4 albedo = albedoTexture.Sample(Sampler, In.uv);
    // テクスチャの α が「自分で光る強さ」（金 0.35・箱の中の宝 1・木と鉄 0）
    float shine = lerp(TR_SHINE_BASE, TR_SHINE_LIT, trParam.y);
    color.rgb += albedo.rgb * albedo.a * shine * trRayColor.rgb;
    return color;
}


float4 PSMainTreasureGem(SPSIn In) : SV_Target0
{
    float3 normal = normalize(In.normal);
    float3 toEye = normalize(eyePos - In.worldPos);
    // ふちほど明るく（宝石の輪郭が光って見える）
    float rim = pow(1.0f - saturate(dot(normal, toEye)), 2.0f);
    // 面ごとに少しずつ明るさを変える（カットした面が見えるように）
    float facet = 0.75f + 0.25f * saturate(dot(normal, normalize(float3(0.3f, 0.8f, -0.5f))));
    float strength = trGemColor.a * (0.35f + 0.65f * trParam.y);
    float3 color = trGemColor.rgb * strength * facet + float3(1.0f, 1.0f, 1.0f) * rim * strength * 0.8f;
    return float4(color, length(In.worldPos - eyePos));
}


float4 PSMainTreasureRays(SPSIn In) : SV_Target0
{
    // uv.y：根元 0 → 先 1、uv.x：筋の番号 / 筋の数
    float fade = pow(saturate(1.0f - In.uv.y), 1.6f);
    float flicker = 0.7f + 0.3f * sin(trParam.x * 3.0f + In.uv.x * TR_RAY_COUNT * 2.4f);
    float alpha = fade * flicker * trParam.y;
    // 半透明は α（カメラからの距離）を書かない設定なので、α は混ぜる割合だけに使う
    return float4(trRayColor.rgb * trRayColor.a, alpha);
}


float4 PSMainTreasureHalo(SPSIn In) : SV_Target0
{
    float2 fromCenter = (In.uv - 0.5f) * 2.0f;
    float d = saturate(1.0f - length(fromCenter));
    float alpha = d * d * trParam.y * (0.85f + 0.15f * sin(trParam.x * 2.0f));
    return float4(trRayColor.rgb * trRayColor.a, alpha);
}

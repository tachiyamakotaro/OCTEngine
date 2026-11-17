/*!
 * @brief ゴールの門の光る物（扉の奥の光・松明の炎）
 *
 * model.fx を include して、頂点シェーダーはそのまま使う。ライトの計算はしない（自分で光る）。
 * 光る強さが 1 より大きいと、ブルームで光があふれる。
 * 値は model.fx の DirectionLightCb（b1）の最後に足す（MODEL_CB_EXTRA）。
 * C++ 側は GoalGateRenderer::GlowValues（ライトの値の後ろにつなげた構造体）。
 *
 * エントリーポイント
 *   PSMainGoalLight … 扉の奥の光（板の真ん中ほど明るい。UV は板の 0〜1）
 *   PSMainGoalFlame … 松明の炎（根元とカメラに向いた所ほど明るい。UV の v は 0 = 先・1 = 根元）
 */

// model.fx の DirectionLightCb の最後に足す値（C++ の GoalGateRenderer::GlowValues と同じ並び）
//   glowColor     : rgb = まわりの色, a = 光る強さ
//   glowCoreColor : rgb = 真ん中の色, a = 光る強さ
// （fxc は複数行のマクロの中のコメントをうまく読めないことがあるので、マクロは1行で書く）
#define MODEL_CB_EXTRA float4 glowColor; float4 glowCoreColor;

#include "model.fx"


// 扉の奥の光の、明るい所の中心（UV）と広がり
static const float2 LIGHT_CORE_CENTER = float2(0.5f, 0.55f);
static const float2 LIGHT_CORE_STRETCH = float2(1.0f, 0.7f);
static const float LIGHT_CORE_RADIUS = 0.6f;


float4 PSMainGoalLight(SPSIn In) : SV_Target0
{
    float2 d = (In.uv - LIGHT_CORE_CENTER) * LIGHT_CORE_STRETCH;
    float core = saturate(1.0f - length(d) / LIGHT_CORE_RADIUS);
    core = core * core * (3.0f - 2.0f * core);
    float3 color = lerp(glowColor.rgb * glowColor.a, glowCoreColor.rgb * glowCoreColor.a, core);
    // α はカメラからの距離（model.fx と同じ。アウトラインと被写界深度が使う）
    return float4(color, length(In.worldPos - eyePos));
}


float4 PSMainGoalFlame(SPSIn In) : SV_Target0
{
    float3 toEye = normalize(eyePos - In.worldPos);
    float facing = saturate(dot(normalize(In.normal), toEye));
    // 根元（v が 1 に近い）とカメラに向いた所ほど内側の色
    float core = saturate(In.uv.y * 0.9f + facing * facing * 0.6f - 0.35f);
    float3 color = lerp(glowColor.rgb * glowColor.a, glowCoreColor.rgb * glowCoreColor.a, core);
    return float4(color, length(In.worldPos - eyePos));
}

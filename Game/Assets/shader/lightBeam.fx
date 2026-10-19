/*!
 * @brief 光源から出る光（半透明。光の中にいるプレイヤーが透けて見える）
 *
 * model.fx を include して、頂点シェーダーとライトの値はそのまま使う。
 * 光の値は、model.fx の DirectionLightCb（b1）の最後に足す（MODEL_CB_EXTRA）。
 * C++ 側は LightBeamRenderer::Constants（ライトの値の後ろに光の値をつなげた構造体）。
 *
 * エントリーポイント
 *   PSMainLightBeam : こちらを向いた面は薄く、上下の縁は濃く少し光らせる。鏡で反射する端は鏡の面に沿って切る
 */

// model.fx の DirectionLightCb の最後に足す値（C++ の LightBeamRenderer::Constants と同じ並び）
//   beamColor    : rgb = 色, a = 不透明度
//   beamParam    : x = こちらを向いた面の不透明度の割合, y = 縁の明るさ
//   beamStartCap : 光の始まりの切り口　xy = 面の向き（こちら側を残す）, zw = 面の上の点。xy が 0 なら切らない
//   beamEndCap   : 光の終わりの切り口（同じ形）
// （fxc は複数行のマクロの中のコメントをうまく読めないことがあるので、マクロは1行で書く）
#define MODEL_CB_EXTRA float4 beamColor; float4 beamParam; float4 beamStartCap; float4 beamEndCap;

#include "model.fx"


float4 PSMainLightBeam(SPSIn In) : SV_Target0
{
    // 鏡で反射する端は、鏡の面より向こう側を捨てる（光の端が鏡の面に沿う）
    if (dot(beamStartCap.xy, beamStartCap.xy) > 0.0f)
    {
        clip(dot(In.worldPos.xy - beamStartCap.zw, beamStartCap.xy));
    }
    if (dot(beamEndCap.xy, beamEndCap.xy) > 0.0f)
    {
        clip(dot(In.worldPos.xy - beamEndCap.zw, beamEndCap.xy));
    }

    // 面がこちらを向いているほど薄く、斜めに見える面（光の上下の縁）ほど濃く明るくする
    // → 光の筋のように見え、真ん中は透けるので、光の中のプレイヤーが見える
    float3 toEye  = normalize(eyePos - In.worldPos);
    float  facing = abs(dot(normalize(In.normal), toEye));
    float  rim    = 1.0f - facing;

    float4 color = beamColor;
    color.a   *= lerp(beamParam.x, 1.0f, rim);
    color.rgb *= lerp(1.0f, beamParam.y, rim);
    return color;
}

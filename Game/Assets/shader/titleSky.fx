/*!
 * @brief タイトル画面の空（夕暮れの空・沈みかけの太陽・遠くの山・星）
 *
 * model.fx を include して、頂点シェーダーはそのまま使う。ライトの計算はしない。
 * 空は、いちばん奥に置いた大きな板（Box.tkm を薄く伸ばした物）に描く。
 * 色は「画面のどこか（SV_POSITION）」だけで決めるので、板の大きさや位置が変わっても見た目は変わらない。
 *   v = 0 が画面の上、1 が画面の下。u は 0〜画面の横長の比（16:9 なら 0〜1.78）
 *
 * 値は model.fx の DirectionLightCb（b1）の最後に足す（MODEL_CB_EXTRA）。
 * C++ 側は title::TitleSkyRenderer::ShaderValues（ライトの値の後ろにつなげた構造体）。
 *
 * エントリーポイント
 *   PSMainTitleSky … 空
 */

// model.fx の DirectionLightCb の最後に足す値（C++ の TitleSkyRenderer::ShaderValues と同じ並び）
//   skyTopColor      : rgb = いちばん上の色
//   skyMiddleColor   : rgb = 真ん中の色, a = 真ん中の色になる高さ（v）
//   skyHorizonColor  : rgb = 地平線の色, a = 地平線の高さ（v）
//   skyScreen        : x = 1 / 画面の幅, y = 1 / 画面の高さ, z = 時間（秒。星のまたたき）
//   skySun           : xy = 太陽の位置（u, v）, z = 太陽の大きさ, w = まわりの光の広がり
//   skySunColor      : rgb = 太陽の色, a = 光る強さ（1 より大きいとブルームで光があふれる）
//   skyFarMountain   : rgb = 遠くの山の色, a = 山のふもとの高さ（v）
//   skyNearMountain  : rgb = 近くの丘の色, a = 丘のふもとの高さ（v）
//   skyMountainShape : x = 遠くの山の高さ, y = 近くの丘の高さ, z = 山にかかるもやの濃さ, w = 星の明るさ
// （fxc は複数行のマクロの中のコメントをうまく読めないことがあるので、マクロは1行で書く）
#define MODEL_CB_EXTRA float4 skyTopColor; float4 skyMiddleColor; float4 skyHorizonColor; float4 skyScreen; float4 skySun; float4 skySunColor; float4 skyFarMountain; float4 skyNearMountain; float4 skyMountainShape;

#include "model.fx"


// 山の形（いくつかの波を足す。数を変えると山の形が変わる）
static const float3 FAR_MOUNTAIN_FREQ = float3(2.1f, 5.3f, 13.0f);
static const float3 FAR_MOUNTAIN_PHASE = float3(0.4f, 1.7f, 2.9f);
static const float3 NEAR_MOUNTAIN_FREQ = float3(3.4f, 8.1f, 19.0f);
static const float3 NEAR_MOUNTAIN_PHASE = float3(2.2f, 0.3f, 1.1f);
// 星を並べるマス目の数（縦。横は画面の比をかける）と、星が出る割合
static const float STAR_GRID = 70.0f;
static const float STAR_RATE = 0.12f;
// 星が出るのは画面の上のほうだけ（この v より下では消える）
static const float STAR_FADE_V = 0.45f;


/** 0〜1 の乱数（マス目ごとに決まる） */
float Hash(float2 p)
{
    p = frac(p * float2(123.34f, 456.21f));
    p += dot(p, p + 45.32f);
    return frac(p.x * p.y);
}


/** 山の稜線の高さ（0〜1。u ごと） */
float MountainProfile(float u, float3 freq, float3 phase)
{
    float h = sin(u * freq.x + phase.x) * 0.55f
            + sin(u * freq.y + phase.y) * 0.30f
            + sin(u * freq.z + phase.z) * 0.15f;
    return h * 0.5f + 0.5f;
}


/** 山の中か（稜線のふちは 1 ピクセルくらいでなめらかに） */
float MountainMask(float v, float ridgeV, float pixelV)
{
    return saturate((v - ridgeV) / (pixelV * 1.5f) + 0.5f);
}


float4 PSMainTitleSky(SPSIn In) : SV_Target0
{
    float2 screen = In.pos.xy;
    float aspect = skyScreen.y / skyScreen.x;
    float v = screen.y * skyScreen.y;
    float u = screen.x * skyScreen.y;  // 縦の長さで割る（太陽が丸く見えるように）

    // ---- 空のグラデーション（上 → 真ん中 → 地平線） ----
    float middleV = skyMiddleColor.a;
    float horizonV = skyHorizonColor.a;
    float3 color;
    if (v < middleV)
    {
        float t = saturate(v / middleV);
        color = lerp(skyTopColor.rgb, skyMiddleColor.rgb, t * t);
    }
    else
    {
        float t = saturate((v - middleV) / (horizonV - middleV));
        color = lerp(skyMiddleColor.rgb, skyHorizonColor.rgb, t * t * (3.0f - 2.0f * t));
    }

    // ---- 星（上のほうだけ。ゆっくりまたたく） ----
    float2 cell = floor(float2(u, v) * STAR_GRID);
    float2 inCell = frac(float2(u, v) * STAR_GRID) - 0.5f;
    float starSeed = Hash(cell);
    if (starSeed < STAR_RATE)
    {
        float2 offset = float2(Hash(cell + 7.1f), Hash(cell + 3.7f)) - 0.5f;
        float d = length(inCell - offset * 0.6f);
        float twinkle = 0.6f + 0.4f * sin(skyScreen.z * (1.5f + starSeed * 20.0f) + starSeed * 60.0f);
        float star = saturate(1.0f - d * 9.0f) * twinkle * saturate(1.0f - v / STAR_FADE_V);
        color += star * skyMountainShape.w;
    }

    // ---- 太陽（地平線の近く。まわりの光は地平線ぞいに横へ広げる） ----
    float2 toSun = float2(u, v) - skySun.xy;
    float sunDistance = length(toSun);
    float sunDisk = saturate((skySun.z - sunDistance) / (skyScreen.y * 2.0f) + 0.5f);
    float sunGlow = exp(-pow(length(toSun * float2(0.6f, 1.4f)) / skySun.w, 2.0f));
    color += skySunColor.rgb * skySunColor.a * (sunDisk + sunGlow * 0.55f);

    // ---- 遠くの山（もやで地平線の色に寄せる） ----
    float uNorm = u / aspect;
    float farRidge = skyFarMountain.a - MountainProfile(uNorm, FAR_MOUNTAIN_FREQ, FAR_MOUNTAIN_PHASE) * skyMountainShape.x;
    float farMask = MountainMask(v, farRidge, skyScreen.y);
    float farHaze = saturate((v - farRidge) / max(skyFarMountain.a - farRidge, 1e-4f)) * skyMountainShape.z;
    float3 farColor = lerp(skyFarMountain.rgb, skyHorizonColor.rgb, farHaze);
    color = lerp(color, farColor, farMask);

    // ---- 近くの丘 ----
    float nearRidge = skyNearMountain.a - MountainProfile(uNorm, NEAR_MOUNTAIN_FREQ, NEAR_MOUNTAIN_PHASE) * skyMountainShape.y;
    float nearMask = MountainMask(v, nearRidge, skyScreen.y);
    float nearHaze = saturate((v - nearRidge) / max(skyNearMountain.a - nearRidge, 1e-4f)) * skyMountainShape.z * 0.5f;
    float3 nearColor = lerp(skyNearMountain.rgb, skyHorizonColor.rgb, nearHaze);
    color = lerp(color, nearColor, nearMask);

    // α はカメラからの距離（model.fx と同じ。アウトラインと被写界深度が使う）
    return float4(color, length(In.worldPos - eyePos));
}

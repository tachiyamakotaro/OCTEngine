/*!
 * @brief ステージの奥の背景（遺跡の回廊・いちばん奥の壁・松明の金具）と、ステージの外の板
 *
 * model.fx を include して、頂点シェーダーはそのまま使う。ライトの計算は背景用に自分でする。
 *   ・明るさ = 環境光 × ambient + 平行光 × sunLight + 松明の光
 *   ・奥の層ほど霧の色に近づける（fog）。松明の光は霧より手前にあるので、霧をかけたあとに足す
 *   ・松明の光はエンジンのポイントライト（4つまで）は使わず、C++ が選んだ近くの BG_MAX_TORCH_LIGHT 個をここで足す
 *   ・ステージの範囲（地形を囲む矩形）の外は描かない（clip）。そこには奥の「ステージの外の板」が見える
 *     画面で見た位置で調べたいので、そのピクセルを見る線が地形の面（Z = 0）を通る所が、範囲の中か調べる
 *   ・石の模様は stage.fx と同じく、ワールド座標から UV を作る（石の大きさがそろう）。
 *     ただし層はカメラについてきて動くので、ついてきた分を引いてから作る（模様がモデルの上をすべらないように）
 *
 * 値は model.fx の DirectionLightCb（b1）の最後に足す（MODEL_CB_EXTRA）。
 * C++ 側は BackgroundRenderer::ShaderValues（ライトの値の後ろにつなげた構造体）。
 *
 * エントリーポイント
 *   PSMainBackground     … 柱・アーチ・壁（石の模様。UV はワールド座標から作る）
 *   PSMainBackgroundProp … 松明の金具（モデルの UV でテクスチャを貼る。皿の炭は自分で光る）
 *   PSMainBackdrop       … ステージの外の板（bgTint の色で塗るだけ）
 */

// 松明の光の数の上限（C++ の BackgroundRenderer::MAX_TORCH_LIGHT と同じにする）
#define BG_MAX_TORCH_LIGHT 16

// model.fx の DirectionLightCb の最後に足す値（C++ の BackgroundRenderer::ShaderValues と同じ並び）
//   bgLayer             : x = 環境光の明るさ, y = 平行光の明るさ, z = 霧の濃さ, w = 石のテクスチャ1枚ぶんのワールドの長さ
//   bgOffset            : xy = 層がカメラについてきた分
//   bgTint              : rgb = 石の色にかける色（ステージの外の板では、外の色）
//   bgFogColor          : rgb = 霧の色
//   bgStageBounds       : ステージの範囲（min.x, min.y, max.x, max.y）
//   bgView              : xyz = カメラの向き, w = 平行投影なら 1
//   bgTorchCount        : x = 松明の光の数
//   bgTorchPositionRange: xyz = 松明の光の位置, w = 届く距離
//   bgTorchColor        : rgb = 色 × 強さ
// （fxc は複数行のマクロの中のコメントをうまく読めないことがあるので、マクロは1行で書く）
#define MODEL_CB_EXTRA float4 bgLayer; float4 bgOffset; float4 bgTint; float4 bgFogColor; float4 bgStageBounds; float4 bgView; float4 bgTorchCount; float4 bgTorchPositionRange[BG_MAX_TORCH_LIGHT]; float4 bgTorchColor[BG_MAX_TORCH_LIGHT];

#include "model.fx"


// テクスチャ1枚ぶんの長さが 0 や負のときに割り算で壊れないようにする下限
static const float BG_MIN_TEXTURE_WORLD_SIZE = 1.0f;
// カメラの向きの Z が 0 に近いとき（真横から見るなど）に割り算で壊れないようにする下限
static const float BG_MIN_FORWARD_Z = 0.05f;
// 松明の光が面の裏側にも少し回り込む割合（柱の横の面もほんのり明るくする）
static const float BG_TORCH_WRAP = 0.25f;
// 松明の皿の炭が自分で光る強さ（テクスチャの赤が強く青が弱い所＝炭だけが光る）
static const float BG_EMBER_GLOW = 1.5f;


/*!
 * @brief ステージの範囲の外なら描かない
 * @details そのピクセルを見る線が、地形の面（Z = 0）を通る所で調べる（C++ の View::IsInsideStage と同じ）
 */
void ClipOutsideStage(float3 worldPos)
{
    float3 direction = bgView.w > 0.5f ? bgView.xyz : normalize(worldPos - eyePos);
    float t = worldPos.z / max(direction.z, BG_MIN_FORWARD_Z);
    float2 onStage = worldPos.xy - direction.xy * t;
    float inside = min(min(onStage.x - bgStageBounds.x, bgStageBounds.z - onStage.x),
                       min(onStage.y - bgStageBounds.y, bgStageBounds.w - onStage.y));
    clip(inside);
}


/*!
 * @brief ワールド座標から UV を作る（stage.fx の CalcStageWorldUV と同じ。面の向きで、どの2軸を使うか決める）
 */
float2 CalcBackgroundUV(float3 position, float3 normal)
{
    float3 coord = position / max(bgLayer.w, BG_MIN_TEXTURE_WORLD_SIZE);
    float3 n = abs(normal);
    // 手前・奥の面
    if (n.z >= n.x && n.z >= n.y)
    {
        return float2(coord.x, -coord.y);
    }
    // 上・下の面
    if (n.y >= n.x)
    {
        return float2(coord.x, coord.z);
    }
    // 左右の面
    return float2(coord.z, -coord.y);
}


/*!
 * @brief 松明の光（近いほど明るく、届く距離で 0）
 */
float3 CalcTorchLight(float3 worldPos, float3 normal)
{
    float3 total = 0.0f;
    int count = (int)bgTorchCount.x;
    for (int i = 0; i < count; i++)
    {
        float3 toLight = bgTorchPositionRange[i].xyz - worldPos;
        float lightDistance = length(toLight);
        float affect = saturate(1.0f - lightDistance / max(bgTorchPositionRange[i].w, 1.0f));
        affect *= affect;
        float facing = saturate(dot(normal, toLight / max(lightDistance, 0.001f)));
        facing = facing * (1.0f - BG_TORCH_WRAP) + BG_TORCH_WRAP;
        total += bgTorchColor[i].rgb * affect * facing;
    }
    return total;
}


/*!
 * @brief 色に光を当てて、霧をかける
 */
float4 CalcBackgroundColor(float3 albedo, float3 worldPos, float3 normal)
{
    float sun = max(0.0f, dot(normal, -ligDirection));
    float3 baseLight = ambientLight * bgLayer.x + ligColor * sun * bgLayer.y;
    float3 color = albedo * baseLight;
    // 奥の層ほど霧の色に近づける
    color = lerp(color, bgFogColor.rgb, bgLayer.z);
    // 松明の光は霧のあとに足す（近くの明かりは霧にかすまない）
    color += albedo * CalcTorchLight(worldPos, normal);
    // α はカメラからの距離（model.fx と同じ。アウトラインと被写界深度が使う）
    return float4(color, length(worldPos - eyePos));
}


float4 PSMainBackground(SPSIn In) : SV_Target0
{
    ClipOutsideStage(In.worldPos);
    float3 normal = normalize(In.normal);
    // 層がカメラについてきた分を引いて、模様をモデルに貼りつけたままにする
    float3 position = In.worldPos - float3(bgOffset.xy, 0.0f);
    float2 uv = CalcBackgroundUV(position, normal);
    float3 albedo = albedoTexture.Sample(Sampler, uv).rgb * bgTint.rgb;
    return CalcBackgroundColor(albedo, In.worldPos, normal);
}


float4 PSMainBackgroundProp(SPSIn In) : SV_Target0
{
    ClipOutsideStage(In.worldPos);
    float3 normal = normalize(In.normal);
    float3 albedo = albedoTexture.Sample(Sampler, In.uv).rgb;
    float4 color = CalcBackgroundColor(albedo, In.worldPos, normal);
    // 炭（だいだい色の所）は自分で光る。鉄（黒っぽい所）は r - b * 2 が負になるので光らない
    color.rgb += albedo * saturate(albedo.r - albedo.b * 2.0f) * BG_EMBER_GLOW;
    return color;
}


float4 PSMainBackdrop(SPSIn In) : SV_Target0
{
    return float4(bgTint.rgb, length(In.worldPos - eyePos));
}

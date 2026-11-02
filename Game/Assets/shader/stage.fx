/*!
 * @brief ステージの地形ブロック（石積みの遺跡）
 *
 * Box.tkm の UV は「面ごとに 0〜1」なので、そのまま貼るとブロックの大きさに合わせてテクスチャが伸びる
 * （横に長い床だと、石が横に何十倍にも伸びて板のように見える）。
 * そこで UV をモデルの UV ではなく「ワールド座標」から作り、どのブロックでも石が同じ大きさで並ぶようにする。
 * ワールド座標から作るので、となり合うブロックの石積みも切れ目なくつながる（1枚の壁に見える）。
 *
 * 面の向きで、どの2軸を使うか決める（solarPanel.fx の CalcPanelPlaneCoord と同じ考え方）。
 *   手前・奥の面 … x と y
 *   上・下の面   … x と z
 *   左右の面     … z と y
 * テクスチャは上が V=0 なので、y（上が＋）は符号を反転して、石の上下が逆にならないようにする。
 *
 * 地形ブロックは動かないので、ワールド座標そのままでよい
 * （動く床・動く壁で使うと、足場が動いても模様がその場に残るので、そちらは model.fx のまま）。
 *
 * model.fx を include して、頂点シェーダーとライトの計算はそのまま使う。
 * UV を書き換えてから CalcLitColor を呼ぶので、アルベド・法線マップ・スペキュラマップすべてがワールド座標の UV で読まれる。
 * 値は model.fx の DirectionLightCb（b1）の最後に足す（MODEL_CB_EXTRA）。
 * C++ 側は StageRenderer::StoneConstants（ライトの値の後ろに石積みの値をつなげた構造体）。
 *
 * エントリーポイント
 *   PSMainStage          影を受ける
 *   PSMainStageNoShadow  影を受けない
 */

// model.fx の DirectionLightCb の最後に足す値（C++ の StageRenderer::StoneConstants と同じ並び）
//   stoneTexture: x = テクスチャ1枚ぶんのワールドの長さ, yzw = 使わない
// （fxc は複数行のマクロの中のコメントをうまく読めないことがあるので、マクロは1行で書く）
#define MODEL_CB_EXTRA float4 stoneTexture;

#include "model.fx"


// テクスチャ1枚ぶんの長さが 0 や負のときに割り算で壊れないようにする下限
static const float MIN_TEXTURE_WORLD_SIZE = 1.0f;


/*!
 * @brief ワールド座標から UV を作る（面の向きで、どの2軸を使うか決める）
 */
float2 CalcStageWorldUV(SPSIn In)
{
    float3 coord = In.worldPos / max(stoneTexture.x, MIN_TEXTURE_WORLD_SIZE);
    float3 n = abs(normalize(In.normal));
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


float4 PSMainStage(SPSIn In) : SV_Target0
{
    In.uv = CalcStageWorldUV(In);
    return CalcLitColor(In, true);     // 影のサンプリングをする
}


float4 PSMainStageNoShadow(SPSIn In) : SV_Target0
{
    In.uv = CalcStageWorldUV(In);
    return CalcLitColor(In, false);    // 影のサンプリングをしない
}

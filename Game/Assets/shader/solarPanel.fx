/*!
 * @brief 動く床・動く壁に付いたソーラーパネル
 *
 * 見た目は「黒いセルが銀色の線で区切られて並び、何枚かのパネル（モジュール）が黒いすき間でつながった物」。
 * 模様はテクスチャではなく、「パネルの角からの距離」でシェーダーが作る（パネルの大きさが変わっても、セルの大きさが変わらないように）。
 * ワールド座標そのままで作ると、足場が動いても模様がその場に残り、模様だけ流れて見える（ベルトコンベアのようになる）ので、
 * パネルの角（panelOrigin）を引いて、模様がパネルと一緒に動くようにしている。
 * 光り具合で、セルの色が黒 → 灰色 → 白に変わり、白に近いほど光る（線と枠は銀色・黒のまま）。
 *
 * model.fx を include して、頂点シェーダーとライトの計算はそのまま使う。
 * モデルのテクスチャは真っ白（solar_panel.dds）なので、CalcLitColor の結果は「ライト × 影」の明るさになる。
 * パネルの値は、model.fx の DirectionLightCb（b1）の最後に足す（MODEL_CB_EXTRA）。
 * C++ 側は StageRenderer::PanelConstants（ライトの値の後ろにパネルの値をつなげた構造体）。
 *
 * エントリーポイント
 *   PSMainSolarPanel
 */

// model.fx の DirectionLightCb の最後に足す値（C++ の StageRenderer::PanelConstants と同じ並び）
//   panelFull  : rgb = 光っているときのセルの色（白）, a = 光り具合（0〜1）
//   panelEmpty : rgb = 光っていないときのセルの色（黒）, a = 白く光りきったとき自分で光る強さ
//   panelOrigin: xyz = パネルの角（ワールド座標。模様はここからの距離で作る）, w = 使わない
// （fxc は複数行のマクロの中のコメントをうまく読めないことがあるので、マクロは1行で書く）
#define MODEL_CB_EXTRA float4 panelFull; float4 panelEmpty; float4 panelOrigin;

#include "model.fx"


// セル1つの大きさ（ワールドの長さ。画面では 1 ≒ 0.75 ピクセルくらいなので、小さすぎると模様がつぶれる）
static const float CELL_SIZE = 10.0f;
// 1枚のパネル（モジュール）に並ぶセルの数（縦横とも）
static const float CELLS_PER_MODULE = 4.0f;
// セルを区切る線の太さの半分（セルの大きさに対する割合）
static const float CELL_LINE_HALF_WIDTH = 0.05f;
// パネルどうしのすき間の太さの半分（パネルの大きさに対する割合）
static const float MODULE_GAP_HALF_WIDTH = 0.04f;
// 外側の枠の太さ（面の UV に対する割合）
static const float FRAME_WIDTH = 0.03f;
// セルを区切る線の色（銀色）
static const float3 CELL_LINE_COLOR = float3(0.55f, 0.58f, 0.63f);
// 枠とパネルどうしのすき間の色（黒）
static const float3 FRAME_COLOR = float3(0.02f, 0.02f, 0.025f);


/*!
 * @brief 模様を描く平面の座標（パネルの角からの距離。面の向きで、どの2軸を使うか決める）
 * @details 前の面は xy、上下の面は xz、左右の面は zy。こうすると、どの面でもセルが正方形になる。
 *          パネルの角からの距離なので、足場が動くと模様も一緒に動く
 */
float2 CalcPanelPlaneCoord(SPSIn In)
{
    float3 local = In.worldPos - panelOrigin.xyz;
    float3 n = abs(normalize(In.normal));
    if (n.z >= n.x && n.z >= n.y)
    {
        return local.xy;
    }
    if (n.y >= n.x)
    {
        return local.xz;
    }
    return local.zy;
}


/*!
 * @brief 格子の線の上なら 1（coord が整数の所に線を引く。線の縁は1ピクセルぶんぼかして、ちらつかないようにする）
 * @param halfWidth 線の太さの半分（coord の単位）
 */
float GridLine(float2 coord, float halfWidth)
{
    float2 distanceToLine = abs(frac(coord + 0.5f) - 0.5f);
    float2 pixel = fwidth(coord);
    float2 onLine = 1.0f - smoothstep(halfWidth - pixel, halfWidth + pixel, distanceToLine);
    return max(onLine.x, onLine.y);
}


float4 PSMainSolarPanel(SPSIn In) : SV_Target0
{
    // ライト × 影の明るさ（テクスチャが真っ白なので、色は付いていない）
    float4 lit = CalcLitColor(In, true);

    // セルの線・パネル（モジュール）どうしのすき間・外側の枠
    float2 cellCoord = CalcPanelPlaneCoord(In) / CELL_SIZE;
    float cellLine = GridLine(cellCoord, CELL_LINE_HALF_WIDTH);
    float moduleGap = GridLine(cellCoord / CELLS_PER_MODULE, MODULE_GAP_HALF_WIDTH);
    float2 uvEdge = min(In.uv, 1.0f - In.uv);      // 面の端までの距離（UV）
    float2 uvPixel = fwidth(In.uv);
    float2 frameXY = 1.0f - smoothstep(FRAME_WIDTH - uvPixel, FRAME_WIDTH + uvPixel, uvEdge);
    float frame = max(moduleGap, max(frameXY.x, frameXY.y));

    // セルの色：光り具合で、黒 → 灰色 → 白
    float charge = saturate(panelFull.a);
    float3 cellColor = lerp(panelEmpty.rgb, panelFull.rgb, charge);

    // セル → 銀色の線 → 黒い枠 の順に重ねる
    float3 albedo = lerp(cellColor, CELL_LINE_COLOR, cellLine);
    albedo = lerp(albedo, FRAME_COLOR, frame);

    // セルの所だけ、白に近いほど光らせる（ライトが当たっていない面でも、光っているのがわかるように）
    float cellMask = (1.0f - cellLine) * (1.0f - frame);
    float3 glow = panelFull.rgb * panelEmpty.a * charge * cellMask;

    // a は model.fx と同じく「カメラからの距離」のまま返す
    return float4(albedo * lit.rgb + glow, lit.a);
}

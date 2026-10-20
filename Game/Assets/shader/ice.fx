/*!
 * @brief 氷（穴の縁がなめらかに丸く消えていく）
 *
 * model.fx を include して、頂点シェーダーとライトの計算はそのまま使う。
 * 氷の値は、model.fx の DirectionLightCb（b1）の最後に足す（MODEL_CB_EXTRA）。
 * C++ 側は IceRenderer::Constants（ライトの値の後ろに氷の値をつなげた構造体）。
 *
 * 氷はマスに分かれていて、マスごとの「残り具合」（1 = 冷たい氷 〜 0 = 溶けた）が t11 に入っている。
 * ピクセルの位置で、まわりの4マスの残り具合をなめらかにつなぎ（双線形補間）、
 * iceParam.x より小さい所を捨てる → マスの四角い形は見えず、穴の縁は丸い線になる。
 *
 * エントリーポイント
 *   PSMainIce : 穴を切り抜いて、穴の縁を明るくする。穴の向こうに見える内側（裏の面）は少し暗くする
 */

// model.fx の DirectionLightCb の最後に足す値（C++ の IceRenderer::Constants と同じ並び）
//   iceRect      : xy = 氷の左下, zw = 氷の右上（ワールド）
//   iceGrid      : x = 列の数, y = 行の数, z = マスの幅, w = マスの高さ
//   iceParam     : x = 描かない残り具合, y = 縁の幅, z = 内側の明るさ, w = 鏡に映った像なら 1（表裏が逆になる）
//   iceEdgeColor : rgb = 縁の色, a = どれだけ混ぜるか
// （fxc は複数行のマクロの中のコメントをうまく読めないことがあるので、マクロは1行で書く）
#define MODEL_CB_EXTRA float4 iceRect; float4 iceGrid; float4 iceParam; float4 iceEdgeColor;

#include "model.fx"

// マスごとの残り具合（並びは下の行から：行 × 列の数 + 列）
StructuredBuffer<float> g_iceField : register(t11);


// マス (column, row) の残り具合（氷の外は、いちばん近い端のマス）
float LoadIceField(int column, int row)
{
    int columns = (int)iceGrid.x;
    int rows    = (int)iceGrid.y;
    column = clamp(column, 0, columns - 1);
    row    = clamp(row, 0, rows - 1);
    return g_iceField[row * columns + column];
}


// ワールドの位置 (x, y) の残り具合（まわりの4マスの真ん中の値を、なめらかにつなぐ）
float SampleIceField(float2 worldXY)
{
    float2 grid = (worldXY - iceRect.xy) / iceGrid.zw - 0.5f;
    float2 cell = floor(grid);
    float2 t    = grid - cell;
    int column = (int)cell.x;
    int row    = (int)cell.y;
    float v00 = LoadIceField(column,     row);
    float v10 = LoadIceField(column + 1, row);
    float v01 = LoadIceField(column,     row + 1);
    float v11 = LoadIceField(column + 1, row + 1);
    return lerp(lerp(v00, v10, t.x), lerp(v01, v11, t.x), t.y);
}


float4 PSMainIce(SPSIn In, bool isFrontFace : SV_IsFrontFace) : SV_Target0
{
    // 溶けた所を捨てる（穴の形）
    float remain = SampleIceField(In.worldPos.xy);
    clip(remain - iceParam.x);

    // 両面を描いているので、裏の面（穴の向こうに見える箱の内側）は法線を裏返す
    // 鏡に映った像は拡大率が負なので、表裏が逆になる
    bool isOutside = (iceParam.w > 0.5f) ? !isFrontFace : isFrontFace;
    if (!isOutside)
    {
        In.normal   = -In.normal;
        In.tangent  = -In.tangent;
        In.biNormal = -In.biNormal;
    }

    float4 color = CalcLitColor(In, true);
    if (!isOutside)
    {
        // 内側は少し暗くして、氷の厚みに見せる
        color.rgb *= iceParam.z;
    }

    // 穴の縁（残り具合が描かない値に近い所）ほど縁の色に近づける。溶けている最中は少し光る
    float edge = 1.0f - saturate((remain - iceParam.x) / iceParam.y);
    color.rgb = lerp(color.rgb, iceEdgeColor.rgb, edge * iceEdgeColor.a);
    return color;
}

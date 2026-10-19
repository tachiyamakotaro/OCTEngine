/*!
 * @brief 偽物の壁（反射した光で開いた穴と、手鏡を当てた所が消える）と、手鏡の視界の扇形
 *
 * model.fx を include して、頂点シェーダー・ライティング・影はそのまま使う。
 * 手鏡の値は、model.fx の DirectionLightCb（b1）の最後に足す（MODEL_CB_EXTRA）。
 * C++ 側は FakeWallRenderer::Constants（ライトの値の後ろに手鏡の値をつなげた構造体）。
 *
 * エントリーポイント
 *   PSMainFakeWall  : 偽物の壁。消えている範囲（光で開いた穴、手鏡の今の視界と、少し前に当てていた所）のピクセルを捨てる（clip）。
 *                     消えている度合いが下がってくると、ざらざらと元に戻る（ディゾルブ）
 *   PSMainMirrorFan : 今の視界の扇形。扇形の外を捨てて、中を半透明で塗る（縁は点線）
 *   PSMainLightBeam : 光源から出る光。半透明で、こちらを向いた面は薄く、上下の縁は濃く少し光らせる
 */

// model.fx の DirectionLightCb の最後に足す値（C++ の FakeWallRenderer::Constants と同じ並び）
//   handMirrorCone  : 今の視界　xy = 扇形の頂点, zw = 向き（長さ1）
//   handMirrorParam : 今の視界　x = sin(開きの半分), y = cos(開きの半分), z = 長さ, w = 1 なら手鏡を構えている
//   edgeGlow        : rgb = 壁の縁の光の色, a = 縁の光の幅
//   fanColor        : rgb = 扇形の色, a = 不透明度
//   fanEdgeColor    : rgb = 扇形の縁の色, a = 不透明度
//   fanParam        : x = 縁の線の太さ, y = 点線の間隔
//   revealCone[i]   : 消えている範囲　xy = 扇形の頂点, zw = 向き（長さ1）
//   revealParam[i]  : 消えている範囲　x = sin(開きの半分), y = cos(開きの半分), z = 長さ, w = 消えている度合い（1 = 完全に消えている 〜 0 = 元通り）
//   revealInfo      : x = 消えている範囲の数, y = ざらざらの粒の大きさ, z = 戻りかけの所を光らせる幅
//   lightHoleSeg[i]   : 光で開いた穴　xy = 線分の始点, zw = 終点（同じなら丸）
//   lightHoleParam[i] : 光で開いた穴　x = 半径
//   lightHoleClip[i]  : 光で開いた穴　その穴を開けた壁の範囲（xy = 左下, zw = 右上）
//   lightHoleInfo     : x = 光で開いた穴の数
//   beamColor         : 光　rgb = 色, a = 不透明度
//   beamParam         : 光　x = こちらを向いた面の不透明度の割合, y = 縁の明るさ
//   beamStartCap      : 光の始まりの切り口　xy = 面の向き（こちら側を残す）, zw = 面の上の点。xy が 0 なら切らない
//   beamEndCap        : 光の終わりの切り口（同じ形）
// （fxc は複数行のマクロの中のコメントをうまく読めないことがあるので、マクロは1行で書く）
#define REVEAL_MAX 32
#define LIGHT_HOLE_MAX 16
#define MODEL_CB_EXTRA float4 handMirrorCone; float4 handMirrorParam; float4 edgeGlow; float4 fanColor; float4 fanEdgeColor; float4 fanParam; float4 revealCone[REVEAL_MAX]; float4 revealParam[REVEAL_MAX]; float4 revealInfo; float4 lightHoleSeg[LIGHT_HOLE_MAX]; float4 lightHoleParam[LIGHT_HOLE_MAX]; float4 lightHoleClip[LIGHT_HOLE_MAX]; float4 lightHoleInfo; float4 beamColor; float4 beamParam; float4 beamStartCap; float4 beamEndCap;

#include "model.fx"


////////////////////////////////////////////////
// 扇形までの距離（ワールド単位。どちらも扇形の内側なら正、外側なら負）
//   cone  : xy = 頂点, zw = 向き
//   param : x = sin(開きの半分), y = cos(開きの半分), z = 長さ
//   toEdge : 両側の縁の線まで
//   toArc  : 先の弧まで
////////////////////////////////////////////////
void CalcConeDistance(float2 worldXY, float4 cone, float4 param, out float toEdge, out float toArc)
{
    float2 d   = worldXY - cone.xy;
    float2 dir = cone.zw;

    float along = dot(d, dir);                      // 向きに沿って、どれだけ先か
    float side  = abs(dir.x * d.y - dir.y * d.x);   // 中心の線から、横にどれだけずれているか

    // 中心の線から「開きの半分」だけ傾いた縁の線までの距離
    // （頂点からの距離 r、中心からの角度 θ とすると r * sin(開きの半分 - θ) と同じ）
    toEdge = along * param.x - side * param.y;
    toArc  = param.z - length(d);
}


////////////////////////////////////////////////
// 点と線分の距離
////////////////////////////////////////////////
float DistanceToSegment(float2 p, float2 a, float2 b)
{
    float2 ab = b - a;
    float lengthSq = dot(ab, ab);
    float t = lengthSq > 0.0001f ? saturate(dot(p - a, ab) / lengthSq) : 0.0f;
    return length(p - (a + ab * t));
}


////////////////////////////////////////////////
// 0〜1 の乱数（同じ p なら同じ値。sin を使わない作り方なので、座標が大きくても模様が崩れにくい）
////////////////////////////////////////////////
float Hash12(float2 p)
{
    float3 p3 = frac(float3(p.xyx) * 0.1031f);
    p3 += dot(p3, p3.yzx + 33.33f);
    return frac((p3.x + p3.y) * p3.z);
}


////////////////////////////////////////////////
// 偽物の壁
////////////////////////////////////////////////
float4 PSMainFakeWall(SPSIn In) : SV_Target0
{
    // 地形と同じライティング（影を受ける）
    float4 color = CalcLitColor(In, true);

    int count = (int)revealInfo.x;
    int holeCount = (int)lightHoleInfo.x;
    if (count > 0 || holeCount > 0)
    {
        float2 p = In.worldPos.xy;
        float covered  = 0.0f;   // このピクセルがどれだけ消えているか（入っている範囲の、消えている度合いの一番大きいもの）
        float nearEdge = 0.0f;   // 範囲の縁のすぐ外側の光り具合

        // 手鏡の視界
        [loop]
        for (int i = 0; i < count; i++)
        {
            float toEdge, toArc;
            CalcConeDistance(p, revealCone[i], revealParam[i], toEdge, toArc);
            float s    = min(toEdge, toArc);
            float rate = revealParam[i].w;
            if (s > 0.0f)
            {
                covered = max(covered, rate);
            }
            else if (edgeGlow.a > 0.0f)
            {
                nearEdge = max(nearEdge, rate * saturate(1.0f + s / edgeGlow.a));   // 縁で 1、幅だけ離れると 0
            }
        }

        // 反射した光で開いた穴（その穴を開けた壁の上だけ。少しはみ出しても、壁の側面が消えるように 1 だけ広げる）
        [loop]
        for (int h = 0; h < holeCount; h++)
        {
            float4 area = lightHoleClip[h];
            if (p.x < area.x - 1.0f || p.y < area.y - 1.0f || p.x > area.z + 1.0f || p.y > area.w + 1.0f)
            {
                continue;
            }
            float s = lightHoleParam[h].x - DistanceToSegment(p, lightHoleSeg[h].xy, lightHoleSeg[h].zw);
            if (s > 0.0f)
            {
                covered = 1.0f;
            }
            else if (edgeGlow.a > 0.0f)
            {
                nearEdge = max(nearEdge, saturate(1.0f + s / edgeGlow.a));
            }
        }

        // 粒ごとに乱数を決めて、「消えている度合い」が乱数より大きい粒は描かない
        // → 度合いが 1 なら全部消え、下がってくるにつれて、粒がざらざらと戻ってくる
        float noise = Hash12(floor(p / revealInfo.y));
        clip(noise - covered);

        // 戻りかけの粒（もうすぐ消える粒）と、範囲の縁を光らせる
        float burn = 0.0f;
        if (covered > 0.0f && revealInfo.z > 0.0f)
        {
            burn = saturate(1.0f - (noise - covered) / revealInfo.z);
        }
        float glow = max(nearEdge, burn);
        color.rgb += edgeGlow.rgb * glow * glow;
    }
    return color;
}


////////////////////////////////////////////////
// 手鏡の視界の扇形（半透明。今の視界だけ）
////////////////////////////////////////////////
float4 PSMainMirrorFan(SPSIn In) : SV_Target0
{
    float toEdge, toArc;
    CalcConeDistance(In.worldPos.xy, handMirrorCone, handMirrorParam, toEdge, toArc);

    // 扇形の外は描かない
    clip(min(toEdge, toArc));

    float4 color = fanColor;
    float r = length(In.worldPos.xy - handMirrorCone.xy);

    // 先へ行くほど薄く
    color.a *= lerp(1.0f, 0.4f, saturate(r / max(handMirrorParam.z, 1.0f)));

    // 縁の線（両側は点線、弧は実線）
    if (toEdge < fanParam.x)
    {
        float dash = 1.0f;
        if (fanParam.y > 0.0f)
        {
            dash = step(0.5f, frac(r / fanParam.y));
        }
        color = lerp(color, fanEdgeColor, dash);
    }
    else if (toArc < fanParam.x)
    {
        color = fanEdgeColor;
    }
    return color;
}


////////////////////////////////////////////////
// 光源から出る光（半透明。光の中にいるプレイヤーが透けて見える）
////////////////////////////////////////////////
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
